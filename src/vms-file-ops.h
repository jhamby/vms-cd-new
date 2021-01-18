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

#ifndef VMS_FILE_OPS_H
#define VMS_FILE_OPS_H

#define __NEW_STARLET 1

#include <inttypes.h>

#include <descrip.h>
#include <efndef.h>
#include <gen64def.h>
#include <rms.h>
#include <ssdef.h>
#include <starlet.h>

#include "errchk.h"
#include "vms-descriptors.h"

/**
 * The NAM block is the original VMS filespec data structure.
 */
struct NameBlock : NAM {
    // Construct a default NameBlock.
    NameBlock() : NAM(cc$rms_nam) { }
};

/**
 * The NAML block was added to support filenames >255 characters.
 * RMS operations may update NAM, NAML, or both.
 */
struct LongNameBlock : NAML {
    // Construct a default LongNameBlock.
    LongNameBlock() : NAML(cc$rms_naml) { }
};

/**
 * The FAB (file access block) defines file characteristics.
 */
struct FileAccessBlock : FAB {
    // Construct a default FileAccessBlock.
    FileAccessBlock() : FAB(cc$rms_fab) { }

    // Construct a FAB with a filename and default filename.
    // We can add more parameters if we need to override fac or shr.
    FileAccessBlock(StringDesc &filename, StringDesc &def_filename) :
            FAB(cc$rms_fab) {
        // try to initialize fields in order
        fab$l_fna           = filename.buf_ptr();
        fab$l_dna           = def_filename.buf_ptr();
        fab$b_fns           = filename.length();
        fab$b_dns           = def_filename.length();
        fab$b_fac           = FAB$M_GET;    // file access
        fab$b_shr           = FAB$M_GET;    // file sharing
    }
};

/**
 * Utility class for filespec and directory operations.
 */
struct Filesystem {

    // String descriptor for looking up the current disk (SYS$DISK).
    static StaticStringDesc sys_disk;

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
    static int get_lnm(StringDesc &lnm, StaticStringDesc &result,
                       bool *result_is_searchlist = NULL);

    /**
     * Get the current directory.
     */
    static int get_current_directory(StaticStringDesc &result);

    /**
     * Get info on the current directory.
     */
    static int get_dirinfo(StaticStringDesc &dirspec,
                           StaticStringDesc &altdirspec);

    /**
     * Convert the dirspec into its short (DID) form.
     * We sometimes must do this to fit ODS-5 paths into DCL symbols.
     */
    static int short_dirspec(StaticStringDesc &dirspec,
                             StaticStringDesc &out_dirspec);

    /**
     * Perform a case-insensitive directory comparison.
     */
    static int special_dir_compare(StaticStringDesc &dirspec1,
                                   StaticStringDesc &dirspec2);

private:

    /** Copy "Requested information unavailable" to result string. */
    static int copy_unavailable_error(StaticStringDesc &result);
};

#endif
