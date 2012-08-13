/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
 * Copyright (c) 2012 Benjamin Venditti <benjamin.venditti@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 **/

#ifndef PSMOVE_PRIVATE_H
#define PSMOVE_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

    /**
     * PRIVATE DEFINITIONS FOR USE IN psmove.c AND psmove_*.c
     *
     * These constants are considered implementation details and should
     * not be used by external code (they are subject to change).
     *
     * All constants that need to be shared between psmove.c and other
     * implementation modules (psmove_*.c) should go here.
     **/

/* Macro: Print a critical message if an assertion fails */
#define psmove_CRITICAL(x) \
        {fprintf(stderr, "[PSMOVE] Assertion fail in %s: %s\n", __func__, x);}

/* Macro: Deprecated functions */
#define psmove_DEPRECATED(x) \
        {fprintf(stderr, "[PSMOVE] %s is deprecated: %s\n", __func__, x);}

/* Macros: Return immediately if an assertion fails + log */
#define psmove_return_if_fail(expr) \
        {if(!(expr)){psmove_CRITICAL(#expr);return;}}
#define psmove_return_val_if_fail(expr, val) \
        {if(!(expr)){psmove_CRITICAL(#expr);return(val);}}


/* Buffer size for calibration data */
#define PSMOVE_CALIBRATION_SIZE 49

/* Three blocks, minus 2x the header (2 bytes) for the 2nd and 3rd block */
#define PSMOVE_CALIBRATION_BLOB_SIZE (PSMOVE_CALIBRATION_SIZE*3 - 2*2)

#ifdef __cplusplus
}
#endif

#endif
