
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



#include "psmove_filter.h"

#include <string.h>
#include <assert.h>

struct _PSMove_Reading {
    int ax, ay, az;
    int gx, gy, gz;
    int mx, my, mz;
};

typedef struct _PSMove_Reading PSMove_Reading;

struct _PSMoveFilter {
    PSMove *move;
    float alpha;
    int first;

    PSMove_Reading last;
    PSMove_Reading now;
};

PSMoveFilter *
psmove_filter_new(PSMove *move)
{
    PSMoveFilter *filter = (PSMoveFilter*)calloc(1, sizeof(PSMoveFilter));

    filter->move = move;
    filter->alpha = 1;
    filter->first = 1;

    return filter;
}

#define MIX_VALUE(now, last, r, att, alpha) \
    (now.att = (last.att) + (((r.att) - (last.att)) * alpha))

void
psmove_filter_update(PSMoveFilter *filter)
{
    PSMove_Reading r;

    assert(filter != NULL);

    memcpy(&filter->last, &filter->now, sizeof(PSMove_Reading));
    psmove_get_accelerometer(filter->move, &r.ax, &r.ay, &r.az);
    psmove_get_gyroscope(filter->move, &r.gx, &r.gy, &r.gz);
    psmove_get_magnetometer(filter->move, &r.mx, &r.my, &r.mz);

    if (filter->first) {
        memcpy(&filter->now, &r, sizeof(PSMove_Reading));
        filter->first = 0;
        return;
    }

    MIX_VALUE(filter->now, filter->last, r, ax, filter->alpha);
    MIX_VALUE(filter->now, filter->last, r, ay, filter->alpha);
    MIX_VALUE(filter->now, filter->last, r, az, filter->alpha);

    MIX_VALUE(filter->now, filter->last, r, gx, filter->alpha);
    MIX_VALUE(filter->now, filter->last, r, gy, filter->alpha);
    MIX_VALUE(filter->now, filter->last, r, gz, filter->alpha);
}

void
psmove_filter_set_alpha(PSMoveFilter *filter, float alpha)
{
    assert(filter != NULL);

    filter->alpha = alpha;
}

float
psmove_filter_get_alpha(PSMoveFilter *filter)
{
    assert(filter != NULL);

    return filter->alpha;
}


void
psmove_filter_get_accelerometer(PSMoveFilter *filter, int *x, int *y, int *z)
{
    assert(filter != NULL);

    if (x != NULL) {
        *x = filter->now.ax;
    }

    if (y != NULL) {
        *y = filter->now.ay;
    }

    if (z != NULL) {
        *z = filter->now.az;
    }
}

void
psmove_filter_destroy(PSMoveFilter *filter)
{
    free(filter);
}

