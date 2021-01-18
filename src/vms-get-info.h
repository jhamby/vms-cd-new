/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set ts=8 sts=4 et sw=4 tw=80: */
/**
 * OpenVMS system info wrappers.
 * Copyright 2021, Jake Hamby.
 * This code is licensed under an MIT license.
 */

#ifndef VMS_GET_INFO_H
#define VMS_GET_INFO_H

#define __NEW_STARLET 1

#include <inttypes.h>
#include <string.h>

#include <dcdef.h>
#include <devdef.h>
#include <descrip.h>
#include <dvidef.h>
#include <efndef.h>
#include <gen64def.h>
#include <iledef.h>
#include <iosbdef.h>
#include <jpidef.h>
#include <ssdef.h>
#include <starlet.h>
#include <uaidef.h>

#include "errchk.h"
#include "vms-descriptors.h"
#include "vms-quiet-setprv.h"

/**
 * OpenVMS system info wrapper utility class.
 */
struct SystemInfo {
    /**
     * Returns the authorized and image privileges for this process.
     */
    static int get_jpi_privs(GENERIC_64 &auth_priv, GENERIC_64 &image_priv) {
        ILE3 get_jpi_privs_items[] = {
            8, JPI$_AUTHPRIV, &auth_priv, NULL,
            8, JPI$_IMAGPRIV, &image_priv, NULL,
            0, 0, NULL, NULL
        };

        IOSB iosb;
        int status = sys$getjpiw( EFN$C_ENF,
                                  0, 0,
                                  get_jpi_privs_items,
                                  &iosb,
                                  0, 0 );

        ERRCHK_RETURN(status);
        return iosb.iosb$l_getxxi_status;
    }

    /**
     * Copies the process's username to the specified 12-byte string descriptor.
     * The original CD used this for "~", but this version uses SYS$LOGIN for the
     * user's own home directory (so we don't need READALL) so we don't need it.
     * It might come in handy later, or for other projects, so here's the code.
     */
    static int get_jpi_username(StaticStringDesc &username) {
        ILE3 get_jpi_username_items[] = {
            12, JPI$_USERNAME, username.dsc$a_pointer, NULL,
            0, 0, NULL, NULL
        };

        IOSB iosb;
        int status = sys$getjpiw( EFN$C_ENF,
                                  0, 0,
                                  get_jpi_username_items,
                                  &iosb,
                                  0, 0 );

        ERRCHK_RETURN(status);
        return iosb.iosb$l_getxxi_status;
     }

    /**
     * Check if the specified assigned channel is a terminal (for user I/O).
     * If successful, the result is passed by reference to the caller.
     */
    static int is_channel_terminal_type(uint16_t channel, bool &result) {
        uint32_t dev_chars;     // device characteristics
        uint32_t dev_class;     // device class

        ILE3 get_dvi_chars_class[] = {
            4, DVI$_DEVCHAR, &dev_chars, NULL,
            4, DVI$_DEVCLASS, &dev_class, NULL,
            0, 0, NULL, NULL
        };

        IOSB iosb;
        int status = sys$getdviw( EFN$C_ENF,
                                  channel,
                                  0,
                                  get_dvi_chars_class,
                                  &iosb,
                                  0, 0, 0);

        ERRCHK_RETURN(status);
        ERRCHK_RETURN(iosb.iosb$l_getxxi_status);

        // test if the device class is terminal, or if the device
        // characteristics include DEV$M_TRM, but not net device or mailbox.
        if (dev_class == DC$_TERM ||
            (dev_chars & (DEV$M_TRM | DEV$M_NET | DEV$M_MBX)) == DEV$M_TRM) {
            result = true;
        } else {
            result = false;
        }
        return SS$_NORMAL;
    }

    /* Get home directory for another user. */
    static int get_uai_dir(StringDesc &username, DynamicStringDesc &result) {
        FIXED_STRING_DESC(def_device, 32);
        FIXED_STRING_DESC(def_dir, 64);

        ILE3 uai_itemlist[] = {
            32, UAI$_DEFDEV, def_device.buf_ptr(), &def_device.length(),
            64, UAI$_DEFDIR, def_dir.buf_ptr(), &def_dir.length(),
            0, 0, NULL, NULL };

        READALL(1);     // toggle on READALL just for this call
        int status = sys$getuai( 0, 0, &username, uai_itemlist, 0, 0, 0);
        READALL(0);
        ERRCHK_RETURN(status);

        // copy device and dir into buffer and return status
        return str$concat(&result, &def_device, &def_dir);
    }
};

#endif
