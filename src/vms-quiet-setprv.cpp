/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set ts=8 sts=4 et sw=4 tw=80: */
/* 
 * OpenVMS system service wrappers.
 * Copyright 2021, Jake Hamby.
 * This code is licensed under an MIT license.
 */

#include "vms-quiet-setprv.h"
#include "vms-get-info.h"

#include <inttypes.h>

/*
 * This helper is used to acquire and drop READALL privilege, if available.
 * Comment out the definition of QUIET_ENABLE_READALL in descrip.mms or
 * bldcd.com if you'd like to remove this functionality for greater security.
 */

#ifdef QUIET_ENABLE_READALL

int quiet_setprv(bool enable, unsigned long long privs) {
    // if we're disabling privileges, we can just call sys$setprv.
    if (!enable) {
        // slightly awkward casting for C++03.
        GENERIC_64 gen64_privs = { privs };
        return sys$setprv(enable, &gen64_privs, 0, 0);
    }

    GENERIC_64 auth_priv;           // authorized privs
    GENERIC_64 image_priv;          // image privs

    int status = SystemInfo::get_jpi_privs(auth_priv, image_priv);

    // just return if sys$getjpiw failed
    ERRCHK_RETURN(status);

    // start with bits we'd like to enable (READALL)
    GENERIC_64 masked_priv = { privs };

    // mask out unavailable privs, unless user has SETPRV auth
    if (!(auth_priv.gen64$q_quadword & PRV$M_SETPRV)) {
        masked_priv.gen64$q_quadword &= (auth_priv.gen64$q_quadword |
                                         image_priv.gen64$q_quadword);
    }

    // don't call setprv if we've masked out all the priv bits
    if (masked_priv.gen64$q_quadword != 0) {
        return sys$setprv(enable, &masked_priv, 0, 0);
    } else {
        return SS$_NOTALLPRIV;
    }
}

#endif  // QUIET_ENABLE_READALL
