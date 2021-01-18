/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set ts=8 sts=4 et sw=4 tw=80: */
/**
 * OpenVMS sys$command I/O wrapper.
 * Copyright 2021, Jake Hamby.
 * This code is licensed under an MIT license.
 */

#ifndef VMS_SYS_COMMAND_IO_H
#define VMS_SYS_COMMAND_IO_H

#define __NEW_STARLET 1

#include <inttypes.h>

#include "vms-descriptors.h"

/**
 * Utility class for reading and writing to sys$command.
 * The channel is assigned and deassigned with the object lifetime.
 */
class SysCommandChannel {
public:
    /**
     * Note: non-virtual destructor.
     */
    ~SysCommandChannel() {
        deassign();         // close channel if caller forgot to
    }

    /**
     * Assign the command channel or return a failure status.
     */
    int assign();

    /**
     * Deassign the channel if allocated.
     */
    int deassign();

    /**
     * Print the descriptor to SYS$COMMAND.
     */
    int print(StringDesc &message);

private:
    // Command channel, or 0 if not open yet.
    uint16_t sys_command_channel;

    // String descriptor for opening SYS$COMMAND.
    static StaticStringDesc sys_command;
};

#endif
