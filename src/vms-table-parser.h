/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set ts=8 sts=4 et sw=4 tw=80: */
/**
 * OpenVMS lib$table_parse wrapper.
 * Copyright 2021, Jake Hamby.
 * This code is licensed under an MIT license.
 */

#ifndef VMS_TABLE_PARSER_H
#define VMS_TABLE_PARSER_H

#define __NEW_STARLET 1

#include <inttypes.h>

#include <descrip.h>
#include <lib$routines.h>
#include <ssdef.h>
#include <tpadef.h>

#include "errchk.h"
#include "vms-descriptors.h"

/**
 * Wrapper for table parser state area.
 */
struct TableParser : TPADEF {
    /**
     * Initialize the parser to parse the specified string descriptor.
     */
    TableParser(StringDesc &input) {
        tpa$l_count = TPA$K_COUNT0;
        tpa$l_options = TPA$M_BLANKS;
        reset(input);
    }

    // Parse the buffer.
    int parse(uint32_t *state_table, uint32_t *key_table) {
        uint32_t *this_ptr = reinterpret_cast<uint32_t*>(this);
        return lib$table_parse(this_ptr, state_table, key_table);
    }

    // Reset the parser to the specified buffer.
    void reset(StringDesc &input) {
        tpa$l_stringcnt = input.length();
        tpa$l_stringptr = input.buf_ptr();
    }

    // Reset the parser to the specified buffer at `start_pos` (0-based).
    void reset(StringDesc &input, size_t start_pos) {
        tpa$l_stringcnt = input.length() - start_pos;
        tpa$l_stringptr = input.buf_ptr() + start_pos;
    }

    // Get a reference to the characters remaining to parse.
    uint32_t& remaining() {
        return tpa$l_stringcnt;
    }

    // Get the current position (0-based) given a reference to buffer start.
    size_t position(StringDesc &buffer) {
        return reinterpret_cast<char*>(tpa$l_stringptr) - buffer.buf_ptr();
    }

    // Get a pointer to the current character.
    // Because of the reinterpret_cast, we can't easily return a reference.
    char* next_char_ptr() {
        return reinterpret_cast<char*>(tpa$l_stringptr);
    }

    // Back up the parser by the specified number of bytes.
    void rewind_by(size_t length) {
        tpa$l_stringcnt += length;
        tpa$l_stringptr = next_char_ptr() - length;
    }

    // Set the next character to parse.
    void set_next_char_ptr(char *next_char_ptr) {
        tpa$l_stringptr = next_char_ptr;
    }

    // Get a reference to the byte count of the current token.
    uint32_t& token_length() {
        return tpa$l_tokencnt;
    }

    // Get a pointer to the start of the current token.
    char* token_ptr() {
        return reinterpret_cast<char*>(tpa$l_tokenptr);
    }

    // Get the user parameter passed to the action routine.
    uint32_t user_param() {
        return tpa$l_param;
    }
};

#endif
