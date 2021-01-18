/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set ts=8 sts=4 et sw=4 tw=80: */
/*
 * Main C++ header file for CD.
 * Copyright 2021, Jake Hamby.
 * This code is licensed under an MIT license.
 */

#ifndef VMS_CD_H_
#define VMS_CD_H_

#include "vms-descriptors.h"

// Status flags passed in return block to caller. The original
// version returned status in the condition code, and also as
// a "switches" field in the return block. Now the condition
// code is just checked for success and returned to VMS if not.

#define RESULT_DO_COMMAND           0x01
#define RESULT_DO_COMMAND_FILE      0x02
#define RESULT_LAUNCH_HELP          0x04

// Replacement for "retblk" in the original VAX version. The refactored
// cd_parse() now calls sys$putmsg() as needed, so main() only needs
// to handle optionally calling lib$do_command or opening the program help.
struct cd_parse_results {
    StringDesc do_command_fspec;    // Use different DO if /COM= used
    unsigned int flags;
};

// Function prototypes.

/**
 * This version of cd_parse() takes argc/argv from main and returns a
 * VMS condition code, which should be SS$_NORMAL. If not, return it.
 * Otherwise, main() will check the flags to call the cd.com command file,
 * or a custom command file (in do_command_name). The caller should
 * initialize do_command_name to an empty dynamic string descriptor.
 */
int cd_parse(int argc, char *argv[], struct cd_parse_results &results);

#endif  // VMS_CD_H_
