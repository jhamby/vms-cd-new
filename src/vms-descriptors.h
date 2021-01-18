/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set ts=8 sts=4 et sw=4 tw=80: */
/**
 * OpenVMS string descriptor wrapper classes.
 * Copyright 2021, Jake Hamby.
 * This code is licensed under an MIT license.
 */

#ifndef VMS_DESCRIPTORS_H
#define VMS_DESCRIPTORS_H

#include <inttypes.h>
#include <string.h>

#include <descrip.h>
#include <str$routines.h>

#include "errchk.h"

/**
 * Parent class to wrap a static or dynamic string descriptor
 * and simplify access to STR$ routines.
 */
struct StringDesc : dsc$descriptor {
    // There's an easier way to do this in C++11, but for C++03,
    // it looks like we need to assign the fields in the constructor.
    StringDesc(unsigned short length, unsigned char type,
            unsigned char dsc_class, char *str) : dsc$descriptor() {
        dsc$w_length = length;
        dsc$b_dtype = type;
        dsc$b_class = dsc_class;
        dsc$a_pointer = str;
    }

    // Copy constructor
    StringDesc(const StringDesc &source) {
        // It should be safe to cast away const to match the prototype.
        int status = str$copy_dx(this, const_cast<StringDesc*>(&source));

        // signal any error because we can't return it
        ERRCHK_SIGNAL(status);
    }

    // Prefix a string with another descriptor (in place).
    int prefix_with(StringDesc &other) {
        return str$prefix(this, &other);
    }

    // Get a reference to the buffer start address.
    char*& buf_ptr() {
        return dsc$a_pointer;
    }

    // Get a reference to the character at index.
    char& operator[](size_t index) {
        return reinterpret_cast<char*>(dsc$a_pointer)[index];
    }

    // Get a reference to the last character in the buffer.
    char& last_char() {
        return reinterpret_cast<char*>(dsc$a_pointer)[dsc$w_length - 1];
    }

    // Get a reference to the buffer length;
    uint16_t& length() {
        return dsc$w_length;
    }

    // Find the position of the first char found in a set of characters.
    // Returns -1 if none of the chars is found (0-based to avoid confusion).
    int find_first_in(StringDesc &find_set) {
        int pos = str$find_first_in_set(this, &find_set);
        return pos - 1;
    }

    // Truncate the string.
    int truncate(int32_t length) {
        // In str$'s 1-based convention, end position is the length.
        return str$left(this, this, &length);
    }
};

// Macro hack to construct StaticStringDesc from string constants.
// We can't get the sizeof() from within a C++ constructor.
// Note: don't add any parentheses: this goes directly into the constructor!
#define C_STR_INIT(str) sizeof(str) - 1, str

// Another helper macro for defining fixed-length writable string buffers.
// This STATIC_ version is for declaring private global variables.
#define STATIC_FIXED_STRING_DESC(name, length)\
    static char name ## _buf[length];\
    static StaticStringDesc name(length, name ## _buf);

// Helper macro for defining local fixed-length writable string buffers.
#define FIXED_STRING_DESC(name, length)\
    char name ## _buf[length];\
    StaticStringDesc name(length, name ## _buf);

/**
 * A static string descriptor has a fixed length.
 */
struct StaticStringDesc : StringDesc {
    // Make a descriptor for the specified string.
    StaticStringDesc(size_t length, char *str) :
        StringDesc(length, DSC64$K_DTYPE_T, DSC64$K_CLASS_S, str) {}

    // Make a descriptor pointing to a substring of the source descriptor.
    // Please note: this constructor does not copy the source string data
    // and must not be used after the source buffer is freed.
    StaticStringDesc(StringDesc &source, size_t startPos, size_t length) :
        StringDesc(length, DSC64$K_DTYPE_T, DSC64$K_CLASS_S,
                reinterpret_cast<char*>(source.dsc$a_pointer) + startPos) {}

    // Append a character (buffer must have enough space!).
    void append(char ch) {
        dsc$a_pointer[dsc$w_length++] = ch;
    }

    // Copy from the source descriptor (buffer must have enough space!).
    void copy_from(StringDesc &source) {
        dsc$w_length = source.dsc$w_length;
        memmove(dsc$a_pointer, source.dsc$a_pointer, source.dsc$w_length);
    }

    // Concat from the source descriptor (up to max_length).
    void concat_from(StringDesc &source, int max_length) {
        int copy_length = source.dsc$w_length;
        int total_length = dsc$w_length + copy_length;
        if (total_length > max_length) {
            copy_length -= (total_length - max_length);
        }
                            
        memmove(dsc$a_pointer + dsc$w_length,
                source.dsc$a_pointer,
                copy_length);
        dsc$w_length += copy_length;
    }
};

/**
 * A dynamic string descriptor's memory is managed by the OS.
 * The class destructor will call str$free1_dx to clean up.
 */
struct DynamicStringDesc : StringDesc {
    // Make an empty dynamic descriptor.
    DynamicStringDesc() :
        StringDesc(0, DSC64$K_DTYPE_T, DSC64$K_CLASS_D, 0) {}

    // Deallocate the string on destruction.
    // Note: this is not a virtual destructor, so don't try to upcast
    // any heap-allocated strings and then delete them as a StringDesc&.
    ~DynamicStringDesc() {
        str$free1_dx(this);
    }

    // Clear the contents of the string (and deallocate).
    int clear() {
        return str$free1_dx(this);
    }

    // Replace contents with null-terminated string starting at `source`.
    int assign(const char *source) {
        uint16_t length = strlen(source);
        // It should be safe to cast away const here.
        return str$copy_r(this, &length, const_cast<char*>(source));
    }

    // Replace contents with `length` characters starting at `source`.
    int assign(const char *source, uint16_t length) {
        // It should be safe to cast away const here.
        return str$copy_r(this, &length, const_cast<char*>(source));
    }

    // Assign value from a static or dynamic string descriptor.
    int assign(StringDesc &source) {
        return str$copy_dx(this, &source);
    }

    // Append the specified string.
    int append(StringDesc &source) {
        return str$append(this, &source);
    }

    // Slide the string back by `count` bytes and shorten it.
    int slide_left(uint16_t count) {
        int start_pos = count + 1;     // VMS routines are 1-based
        return str$right(this, this, &start_pos);
    }

    // Slide the string back by `count` bytes starting at `start_pos`.
    // Unlike the VMS routine, start_pos is 0-based here.
    int slide_left_at(uint16_t count, size_t start_pos) {
        char *dest = buf_ptr() + start_pos;
        char *src = buf_ptr() + start_pos + count;
        memmove(dest, src, count);
        return truncate(length() - count);
    }
};

#endif
