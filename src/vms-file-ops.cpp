/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set ts=8 sts=4 et sw=4 tw=80: */
/**
 * OpenVMS file and directory operation wrappers.
 * Copyright 2021, Jake Hamby.
 * This code is licensed under an MIT license.
 *
 * Suggested reference material:
 *  - VSI OpenVMS Record Management Services Reference Manual
 *  - VSI OpenVMS System Services Reference Manuals
 */

#define __NEW_STARLET 1

#include "vms-file-ops.h"
#include "vms-quiet-setprv.h"

#include <stdio.h>

#include <iledef.h>
#include <lnmdef.h>

// Localized error messages.
// Note: the #pragma lines are required, or else the compiler will treat
// the external symbols as pointers to dereference, crashing the app.
// The C user's guide documents "globalvalue", but the examples showing
// how to print user messages aren't written in C and don't mention this
// critical requirement for declaring external references to constants.

#pragma extern_model save
#pragma extern_model globalvalue

extern int CD_UNAVAILABLE;

#pragma extern_model restore

/* Define static string descriptor for sys_disk. */
StaticStringDesc Filesystem::sys_disk(C_STR_INIT("SYS$DISK"));

/**
 * Translate a logical name using the algorithm from cdparse.mar.
 * Loops up to 32 times calling sys$trnlnm to translate the name
 * until it resolves to a real filespec, or else give up and
 * return SS$_TOOMANYLNAM. The destination StaticStringDesc must
 * be backed by a 256-byte buffer (or larger).
 *
 * If the third parameter is not NULL then it will be used as a pointer
 * to return whether or not the LNM is a searchlist, used by
 * get_current_directory() (maxidx is a global in the MACRO version).
 */
int Filesystem::get_lnm(StringDesc &lnm, StaticStringDesc &result,
                        bool *result_is_searchlist) {
    // read-only constants passed by reference (can be static)
    static uint32_t         trn_attr = LNM$M_CASE_BLIND;
    static StaticStringDesc trn_tabname(C_STR_INIT("LNM$FILE_DEV"));
    
    // output values passed by reference
    uint32_t attributes;    // bitfield
    int32_t max_index;      // signed (may be -1)

    ILE3 lnm_list[] = {
        256, LNM$_STRING,       result.buf_ptr(),   &result.length(),
        4,   LNM$_ATTRIBUTES,   &attributes,        NULL,
        4,   LNM$_MAX_INDEX,    &max_index,         NULL,
        0,   0,                 NULL,               NULL };

    StringDesc &current = lnm;          // this will change if we loop
    FIXED_STRING_DESC(tmp_buf, 256);    // set up on the stack if we loop
    int status;

    // Set result_is_searchlist to false if we need to return it.
    // There's only one place in the following loop that sets it to true.
    if (result_is_searchlist != NULL) {
        *result_is_searchlist = false;
    }

    // Try to translate the logical name up to 32 times.
    for (int iter = 0; iter < 32; ++iter) {
        // Create a temp descriptor so that we can remove any trailing ':'.
        // The MACRO version only created a temp descriptor if needed, but
        // the logic is easier to follow in C++ if we always make one here.
        StaticStringDesc query_lnm(current, 0, (current.last_char() == ':') ?
                current.length() - 1 : current.length());

        // Call sys$trnlnm to get the translated name, attribs, and max index.
        status = sys$trnlnm(&trn_attr, &trn_tabname, &query_lnm, NULL,
                            lnm_list);

        // If we fail here but have made at least one successful pass through
        // the loop, copy current to result, and return normal status.
        // This is a simplification of the logic in CD V6.0A, which makes an
        // extra call to sys$trnlnm in order to potentially avoid copying
        // the string back and forth. This shouldn't be a common code path,
        // since VMS should set LNM$M_TERMINAL for the final translation.
        if (!($VMS_STATUS_SUCCESS(status))) {
            if (iter > 0) {
                // Note: copy result from current in case it ends in ':'.
                result.copy_from(current);
                return SS$_NORMAL;
            } else {
                return status;      // return error if the first $trnlnm fails
            }
        }

        // If this is a concealed translation, then don't translate it.
        if (attributes & LNM$M_CONCEALED) {
            result.copy_from(current);
            return SS$_NORMAL;
        }

        // Return failure if this is a table type or has no equivalence.
        if ((attributes & LNM$M_TABLE) || (max_index == -1)) {
            return SS$_NOLOGNAM;
        }

        // Return success if this is the final (terminal) translation,
        // or if it's a concealed translation
        if (attributes & LNM$M_TERMINAL) {
            return SS$_NORMAL;      // we've reached the final translation
        }

        if (max_index > 0) {
            // LNM is a searchlist so we need to copy it to result.
            result.copy_from(current);

            // Add back the ":" if it's not there.
            if (result.last_char() != ':' && result.length() < 255) {
                result.append(':');     // add a ':' if it's not there
            }

            // We also want to tell get_current_directory() about it.
            if (result_is_searchlist != NULL) {
                *result_is_searchlist = true;
            }
            return SS$_NORMAL;
        }

        // Copy the result back to our temp buffer so we can try again.
        tmp_buf.copy_from(result);
        current = tmp_buf;
    }

    return SS$_TOOMANYLNAM;     // too many translations; give up.
}

/** Copy "Requested information unavailable" to result string. */
int Filesystem::copy_unavailable_error(StaticStringDesc &result) {
    // Set result to "Requested information unavailable" from messages.
    result.length() = 256;                  // reset max length
    int status = sys$getmsg(CD_UNAVAILABLE, // message id
                        &result.length(),   // pointer to length
                        &result,            // pointer to descriptor
                        0x01,               // flags: message text only
                        NULL);
    return status;
}

/**
* Get the current directory, copied to the specified string descriptor,
 * which must be static type, 4096 bytes (maximum ODS-5 dirspec size).
 */
int Filesystem::get_current_directory(StaticStringDesc &result) {
    // use our get_lnm() function to translate "SYS$DISK"
    bool is_searchlist;
    int status = get_lnm(sys_disk, result, &is_searchlist);

    if (!($VMS_STATUS_SUCCESS(status))) {
        copy_unavailable_error(result);
        return status;      // return the failure from get_lnm()
    }

    // append dir text if not a search list
    if (!is_searchlist) {
        FIXED_STRING_DESC(def_dir, 4096);   // buffer to get default dir
        status = sys$setddir(NULL, &def_dir.length(), &def_dir);
        
        if (!($VMS_STATUS_SUCCESS(status))) {
            copy_unavailable_error(result);
            return status;      // return the failure from sys$setddir
        }

        result.concat_from(def_dir, 4096);    // append the default dir
    }

    return SS$_NORMAL;  // success
}

// Note: NAML$C_MAXRSS is 4095: round up to 4096.
static char expand_str[4096];
static char result_str[4096];

// Note: NAM$C_MAXRSS is 255: round up to 256.
static char expand_str_short[256];
static char result_str_short[256];

/**
 * Utility routine: get_dirinfo - retrieve info on the designated directory
 */
int Filesystem::get_dirinfo(StaticStringDesc &dirspec,
                            StaticStringDesc &altdirspec) {
    LongNameBlock naml;
    naml.naml$b_rss                 = NAM$C_MAXRSS;
    naml.naml$l_rsa                 = result_str_short;
    naml.naml$b_nop                 = NAML$M_NO_SHORT_UPCASE;
    naml.naml$b_ess                 = NAM$C_MAXRSS;
    naml.naml$l_esa                 = expand_str_short;
    naml.naml$l_long_expand         = expand_str;
    naml.naml$l_long_expand_alloc   = NAML$C_MAXRSS;
    naml.naml$l_long_result         = result_str;
    naml.naml$l_long_result_alloc   = NAML$C_MAXRSS;
    naml.naml$l_long_filename_size  = dirspec.length();
    naml.naml$l_long_filename       = dirspec.buf_ptr();

    FileAccessBlock fab;
    fab.fab$l_fop                   = FAB$M_NAM;
    fab.fab$l_naml                  = &naml;
    fab.fab$l_fna                   = (char *)(-1);   // use NAML for filename

    int status = sys$parse(&fab);
    ERRCHK_RETURN(status);

    // FINISH ME
    return SS$_BUGCHECK;
}

/**
 * Utility routine: short_dirspec
 */
int Filesystem::short_dirspec(StaticStringDesc &dirspec,
                              StaticStringDesc &out_dirspec) {
    LongNameBlock naml;
    naml.naml$b_rss                 = NAM$C_MAXRSS;
    naml.naml$l_rsa                 = result_str_short;
    naml.naml$b_nop                 = NAML$M_NO_SHORT_UPCASE;
    naml.naml$b_ess                 = NAM$C_MAXRSS;
    naml.naml$l_esa                 = expand_str_short;
    naml.naml$l_long_expand         = expand_str;
    naml.naml$l_long_expand_alloc   = NAML$C_MAXRSS;
    naml.naml$l_long_result         = result_str;
    naml.naml$l_long_result_alloc   = NAML$C_MAXRSS;
    naml.naml$l_long_filename_size  = dirspec.length();
    naml.naml$l_long_filename       = dirspec.buf_ptr();

    FileAccessBlock fab;
    fab.fab$l_fop                   = FAB$M_NAM;
    fab.fab$l_naml                  = &naml;
    fab.fab$l_fna                   = (char *)(-1);   // use NAML for filename

    int status;

    READALL(1);
    status = sys$parse(&fab);
    if (!($VMS_STATUS_SUCCESS(status))) {
        // try again with just syntax checking
        naml.naml$b_nop |= NAML$M_SYNCHK;
        status = sys$parse(&fab);
    }
    READALL(0);
    ERRCHK_RETURN(status);

    // length of (nodespec + device, if any) + directory
    size_t esa_short_len = (naml.naml$l_dir - naml.naml$l_esa) + naml.naml$b_dir;
    out_dirspec.length() = esa_short_len;
    memcpy(out_dirspec.buf_ptr(), naml.naml$l_esa, esa_short_len);

    return SS$_NORMAL;
}

/**
 * Utility routine: special_dir_compare
 */
int Filesystem::special_dir_compare(StaticStringDesc &dirspec1,
                                    StaticStringDesc &dirspec2) {
    int result = str$case_blind_compare(&dirspec1, &dirspec2);
    if (result == 0) {
        return 0;           // strings are equal
    }

    if (dirspec1.length() == dirspec2.length()) {
        return result;      // not an underscore issue
    }

    // make a writable copy of the descriptors
    StaticStringDesc ds1_copy = dirspec1;
    StaticStringDesc ds2_copy = dirspec2;

    // If one of them has a leading '_', remove it and try again.
    if (ds1_copy.length() != 0 && ds1_copy[0] == '_') {
        ds1_copy.length()--;
        ds1_copy.buf_ptr()++;
    } else if (ds2_copy.length() != 0 && ds2_copy[0] == '_') {
        ds2_copy.length()--;
        ds2_copy.buf_ptr()++;
    }
    
    return str$case_blind_compare(&ds1_copy, &ds2_copy);
}
