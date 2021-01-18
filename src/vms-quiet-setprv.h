/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set ts=8 sts=4 et sw=4 tw=80: */
/* 
 * OpenVMS system service wrappers.
 * Copyright 2021, Jake Hamby.
 * This code is licensed under an MIT license.
 */

#ifndef VMS_QUIET_SETPRV_H
#define VMS_QUIET_SETPRV_H

#define __NEW_STARLET 1

#include <gen64def.h>
#include <prvdef.h>

/*
 * This helper is used to acquire and drop READALL privilege, if available.
 * Comment out the definition of QUIET_ENABLE_READALL in descrip.mms or
 * bldcd.com if you'd like to remove this functionality for greater security.
 */

#ifdef QUIET_ENABLE_READALL

// enable/disable the READALL privilege
#define READALL(enable) quiet_setprv(enable, PRV$M_READALL)

int quiet_setprv(bool enable, unsigned long long privs);

#else
#define READALL(enable)

#endif  // QUIET_ENABLE_READALL

#endif  // VMS_QUIET_SETPRV_H
