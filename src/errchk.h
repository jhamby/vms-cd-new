/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set ts=8 sts=4 et sw=4 tw=80: */

// These error checking macros are based on errchk.h from:
// http://www.eight-cubed.com/examples/framework.php?file=errchk.h

#ifndef ERRCHK_H
#define ERRCHK_H

#include <lib$routines.h>
#include <stsdef.h>

// WARNING: "arg" is referenced twice, so don't use these macros
// to wrap function calls, or the function will be called twice.

#define ERRCHK_RETURN(arg)\
    if (!($VMS_STATUS_SUCCESS(arg))) return(arg)

#define ERRCHK_SIGNAL(arg)\
    if (!($VMS_STATUS_SUCCESS(arg))) (void) lib$signal(arg)

#define ERRCHK_STOP(arg)\
    if (!($VMS_STATUS_SUCCESS(arg))) (void) lib$stop(arg)

#define ERRCHK_GOTO_CLEANUP(arg)\
    if (!($VMS_STATUS_SUCCESS(arg))) goto cleanup

#endif
