
 /**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2012 Thomas Perl <m@thp.io>
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

#ifndef XDO_OSX_H
#define XDO_OSX_H

/**
 * A partial, barebones implementation of libxdo targetting Mac OS X
 *
 * This library implement just enough of the libxdo API for the PS Move
 * mouse emulator example to compile and control the mouse in OS X.
 *
 * Thomas Perl <m@thp.io>; 2012-10-05
 **/


// The parameter is ignored, so the value doesn't really matter
#define CURRENTWINDOW 0

// Opaque structure for implementation-specific state data
typedef struct _xdo_t xdo_t;

xdo_t *
xdo_new(const char *display);

void
xdo_keysequence_down(xdo_t *xdo, int window, const char *key, int delay);

void
xdo_keysequence_up(xdo_t *xdo, int window, const char *key, int delay);

void
xdo_mousedown(xdo_t *xdo, int window, int button);

void
xdo_mouseup(xdo_t *xdo, int window, int button);

void
xdo_mousemove_relative(xdo_t *xdo, int x, int y);

void
xdo_free(xdo_t *xdo);

#endif /* XDO_OSX_H */
