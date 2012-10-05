
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


/**
 * A partial, barebones implementation of libxdo targetting Mac OS X
 *
 * This library implement just enough of the libxdo API for the PS Move
 * mouse emulator example to compile and control the mouse in OS X.
 *
 * Thomas Perl <m@thp.io>; 2012-10-05
 **/

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>

#include "xdo_osx.h"

#define UNIMPLEMENTED fprintf(stderr, "[XDO OSX] FIXME: %s unimplemented.\n", __func__)

struct _xdo_t {
    CGPoint point; // Point for CGEventCreateMouseEvent
    NSPoint mouse; // Current mouse coordinate (from NSEvent mouseLocation)

    NSRect screen_rect; // Rectangle of the main screen (FIXME: Multi-monitor setups?)

    int pressed; // The currently-pressed button ID (if any)
};

void
_xdo_update_point(xdo_t *xdo, int x, int y) {
    xdo->mouse = [NSEvent mouseLocation];

    x += xdo->mouse.x;
    // Y coordinate is inverted in NSEvent (0=bottom of the screen)
    y += xdo->screen_rect.size.height - xdo->mouse.y;

    // Coordinates need clipping to avoid strange "jumping" behavior
    xdo->point.x = MAX(1, MIN(xdo->screen_rect.size.width - 1, x));
    xdo->point.y = MAX(1, MIN(xdo->screen_rect.size.height - 1, y));
}

void
_xdo_post_event(xdo_t *xdo, CGEventType type, CGMouseButton button)
{
    CGEventRef event = CGEventCreateMouseEvent(NULL, type, xdo->point, button);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
}



xdo_t *
xdo_new(const char *display)
{
    xdo_t *xdo = (xdo_t*)calloc(1, sizeof(xdo_t));
    xdo->screen_rect = [[NSScreen mainScreen] frame];
    return xdo;
}

void
xdo_keysequence_down(xdo_t *xdo, int window, const char *key, int delay)
{
    UNIMPLEMENTED;
}

void
xdo_keysequence_up(xdo_t *xdo, int window, const char *key, int delay)
{
    UNIMPLEMENTED;
}

void
xdo_mousedown(xdo_t *xdo, int window, int button)
{
    CGEventType event_type;
    CGMouseButton event_button;

    switch (button) {
        case 1:
            event_type = kCGEventLeftMouseDown;
            event_button = kCGMouseButtonLeft;
            break;
        case 2:
            event_type = kCGEventOtherMouseDown;
            event_button = kCGMouseButtonCenter;
            break;
        default:
            event_type = kCGEventRightMouseDown;
            event_button = kCGMouseButtonRight;
            break;
    }

    _xdo_update_point(xdo, 0, 0);
    _xdo_post_event(xdo, event_type, event_button);

    xdo->pressed = button;
}

void
xdo_mouseup(xdo_t *xdo, int window, int button)
{
    CGEventType event_type;
    CGMouseButton event_button;

    switch (button) {
        case 1:
            event_type = kCGEventLeftMouseUp;
            event_button = kCGMouseButtonLeft;
            break;
        case 2:
            event_type = kCGEventOtherMouseUp;
            event_button = kCGMouseButtonCenter;
            break;
        default:
            event_type = kCGEventRightMouseUp;
            event_button = kCGMouseButtonRight;
            break;
    }

    _xdo_update_point(xdo, 0, 0);
    _xdo_post_event(xdo, event_type, event_button);

    xdo->pressed = 0;
}

void
xdo_mousemove_relative(xdo_t *xdo, int x, int y)
{
    CGEventType event_type;
    CGMouseButton event_button;

    switch (xdo->pressed) {
        case 0:
            event_type = kCGEventMouseMoved;
            event_button = kCGMouseButtonLeft; // will be ignored
            break;
        case 1:
            event_type = kCGEventLeftMouseDragged;
            event_button = kCGMouseButtonLeft;
            break;
        case 2:
            event_type = kCGEventOtherMouseDragged;
            event_button = kCGMouseButtonCenter;
            break;
        default:
            event_type = kCGEventRightMouseDragged;
            event_button = kCGMouseButtonRight;
            break;
    }

    _xdo_update_point(xdo, x, y);
    _xdo_post_event(xdo, event_type, event_button);
}

void
xdo_free(xdo_t *xdo)
{
    free(xdo);
}

