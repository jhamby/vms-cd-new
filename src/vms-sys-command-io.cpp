/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set ts=8 sts=4 et sw=4 tw=80: */
/**
 * OpenVMS sys$command I/O wrapper.
 * Copyright 2021, Jake Hamby.
 * This code is licensed under an MIT license.
 */

#define __NEW_STARLET 1

#include "vms-get-info.h"
#include "vms-sys-command-io.h"

#include <efndef.h>
#include <iodef.h>
#include <iosbdef.h>
#include <psldef.h>
#include <ssdef.h>
#include <starlet.h>

/* Define static string descriptor for SYS$COMMAND. */
StaticStringDesc SysCommandChannel::sys_command(C_STR_INIT("SYS$COMMAND"));

/**
 * Assign the command channel or return a failure status.
 */
int SysCommandChannel::assign() {
    // first, try to assign the channel
    int status = sys$assign(&sys_command,
                            &sys_command_channel,
                            PSL$C_USER, 0, 0);
    ERRCHK_RETURN(status);

    // now, make sure it's a terminal
    bool is_terminal;
    status = SystemInfo::is_channel_terminal_type(
                            sys_command_channel, is_terminal);

    if (!($VMS_STATUS_SUCCESS(status))) {
        deassign();
        return status;
    }

    // return success if the dev class is terminal, or if the device
    // characteristics include DEV$M_TRM, but not net device or mailbox.
    if (is_terminal) {
        return SS$_NORMAL;
    } else {
        // wrong type: deassign and return error
        deassign();
        return SS$_IVDEVNAM;
    }
}

/**
 * Deassign the channel if allocated.
 */
int SysCommandChannel::deassign() {
    int status = sys$dassgn(sys_command_channel);
    sys_command_channel = 0;    // indicate channel is now gone
    return status;              // unlikely to fail
}

/**
 * Print the descriptor to SYS$COMMAND.
 */
int SysCommandChannel::print(StringDesc &message) {
    IOSB iosb;
    int status = sys$qiow(
            EFN$C_ENF,
            sys_command_channel,
            IO$_WRITEVBLK,
            &iosb,
            NULL, 0,    // astadr, astprm
            message.buf_ptr(),
            message.length(),
            0, 0, 0, 0);

    return status;
}
