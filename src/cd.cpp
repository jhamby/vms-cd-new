/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set ts=8 sts=4 et sw=4 tw=80: */
/*
 * Ported from VAX MACRO to C++ by Jake Hamby <jake.hamby@gmail.com>.
 * Original: https://www.digiater.nl/openvms/freeware/v80/cd/
 *
 * Original license:
 *
 * ; Program:      CD.MAR V6.0
 * ; Author:       TECSys Development, Inc.
 * ; Date:         96.06.05
 * ; Updated:      98.08.07
 * ;
 * ; License:
 * ;    Ownership of and rights to these programs is retained by the author(s).
 * ;    Limited license to use and distribute the software in this library is
 * ;    hereby granted under the following conditions:
 * ;      1. Any and all authorship, ownership, copyright or licensing
 * ;         information is preserved within any source copies at all times.
 * ;      2. Under absolutely *NO* circumstances may any of this code be used
 * ;         in any form for commercial profit without a written licensing
 * ;         agreement from the author(s).  This does not imply that such
 * ;         a written agreement could not be obtained.
 * ;      3. Except by written agreement under condition 2, source shall
 * ;         be freely provided with all executables.
 * ;      4. Library contents may be transferred or copied in any form so
 * ;         long as conditions 1, 2, and 3 are met.  Nominal charges may
 * ;         be assessed for media and transferral labor without such charges
 * ;         being considered 'commercial profit' thereby violating condition 2.
 * ;
 * ; Warranty:
 * ;    These programs are distributed in the hopes that they will be useful, but
 * ;    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * ;    or FITNESS FOR A PARTICULAR PURPOSE.
 */

#define __NEW_STARLET 1

#include "cd.h"
#include "vms-descriptors.h"
#include "vms-file-ops.h"

// messages to print if we can't find the help text
// Note: the #pragma lines are required, or else the compiler will treat
// the external symbols as pointers to dereference, crashing the app.
// The C user's guide documents "globalvalue", but the examples showing
// how to print user messages aren't written in C and don't mention this
// critical requirement for declaring external references to constants.

#pragma extern_model save
#pragma extern_model globalvalue

extern int CD_VERSION;
extern int CD_HELP1;
extern int CD_HELP2;
extern int CD_HELP3;
extern int CD_NOCDHELP;

#pragma extern_model restore

// static descriptors for commands and logical names

static StaticStringDesc at_sign(        C_STR_INIT("@"));
static StaticStringDesc help_command(   C_STR_INIT("help @cdhelp cd"));
static StaticStringDesc help_lnm(       C_STR_INIT("CDHELP"));
static StaticStringDesc help_lnm_table( C_STR_INIT("LNM$FILE_DEV"));

static StaticStringDesc cd_command(
        C_STR_INIT("if f$search(\"cd.com\").nes.\"\" then @cd"));

static StaticStringDesc help_filename(      C_STR_INIT("cdhelp"));
static StaticStringDesc help_def_filename(  C_STR_INIT("sys$help:.hlb"));

// This version is a bit simpler than the VAX macro version, as we delegate
// calling $putmsg to cd_parse() and pass argc/argv vs. calling lib$foreign().
int main(int argc, char *argv[]) {
    struct cd_parse_results results = { DynamicStringDesc(), 0 };
    int status;

    // the actual work happens inside cd_parse().
    status = cd_parse(argc, argv, results);
    ERRCHK_RETURN(status);

    // optionally search for help file, then run help command if found.
    if (results.flags & RESULT_LAUNCH_HELP) {
        FileAccessBlock fab(help_filename, help_def_filename);

        status = sys$open(&fab);
        if ($VMS_STATUS_SUCCESS(status)) {
            status = sys$close(&fab);
            status = lib$do_command(&help_command);
        } else {
            int no_help_msgvec[] = {
                12,
                CD_NOCDHELP, 0,
                fab.fab$l_sts, fab.fab$l_stv,   // RMS error and status value
                CD_HELP1, 0,
                CD_HELP2, 0,
                CD_HELP3, 0,
                CD_VERSION, 0,
            };

            status = sys$putmsg(no_help_msgvec, 0, 0, 0);
        }
    }

    // optionally call custom DCL file or cd.com, depending on switches.
    if (results.flags & RESULT_DO_COMMAND) {
        // this will point to either "@" + a filespec, or cd_command
        StringDesc *do_command;

        if (results.flags & RESULT_DO_COMMAND_FILE) {
            do_command = &results.do_command_fspec;
            // insert an "@" before the filespec
            status = do_command->prefix_with(at_sign);
            ERRCHK_RETURN(status);
        } else {
            do_command = &cd_command;
        }

        status = lib$do_command(do_command);
    }

    // return the status of the last VMS call to DCL (success = 1, not 0).
    return status;
}
