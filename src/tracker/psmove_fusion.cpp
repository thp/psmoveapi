
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


#include "psmove_fusion.h"
#include "../psmove_private.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#define PSMOVE_FUSION_STEP_EPSILON (.0001)

struct _PSMoveFusion {
    PSMoveTracker *tracker;

    float width;
    float height;

    glm::mat4 projection;
    glm::mat4 modelview;
    glm::vec4 viewport;
};


PSMoveFusion *
psmove_fusion_new(PSMoveTracker *tracker, float z_near, float z_far)
{
    PSMoveFusion *fusion = (PSMoveFusion*)calloc(1, sizeof(PSMoveFusion));

    fusion->tracker = tracker;

    int width, height;
    psmove_tracker_get_size(tracker, &width, &height);

    fusion->width = (float)width;
    fusion->height = (float)height;

    fusion->projection = glm::perspectiveFov<float>(PSEYE_FOV_BLUE_DOT,
            fusion->width, fusion->height, z_near, z_far);
    fusion->viewport = glm::vec4(0., 0., fusion->width, fusion->height);

    return fusion;
}

float *
psmove_fusion_get_projection_matrix(PSMoveFusion *fusion)
{
    psmove_return_val_if_fail(fusion != NULL, NULL);

    return glm::value_ptr(fusion->projection);
}

float *
psmove_fusion_get_modelview_matrix(PSMoveFusion *fusion, PSMove *move)
{
    psmove_return_val_if_fail(fusion != NULL, NULL);
    psmove_return_val_if_fail(move != NULL, NULL);

    float w, x, y, z;
    psmove_get_orientation(move, &w, &x, &y, &z);
    glm::quat quaternion(w, x, y, z);

    psmove_fusion_get_position(fusion, move, &x, &y, &z);

    fusion->modelview = glm::translate(glm::mat4(),
            glm::vec3(x, y, z)) * glm::mat4_cast(quaternion);

    return glm::value_ptr(fusion->modelview);
}

void
psmove_fusion_get_position(PSMoveFusion *fusion, PSMove *move,
        float *x, float *y, float *z)
{
    psmove_return_if_fail(fusion != NULL);
    psmove_return_if_fail(move != NULL);

    float camX, camY, camR;
    psmove_tracker_get_position(fusion->tracker, move, &camX, &camY, &camR);

    float winX = (float)camX;
    float winY = fusion->height - (float)camY;
    float winZ = .5; /* start value for binary search */

    float targetWidth = 2.0f*camR;

    glm::vec3 obj;

    /* Binary search for the best distance based on the current projection */
    float step = .25;
    while (step > PSMOVE_FUSION_STEP_EPSILON) {
        /* Calculate center position of sphere */
        obj = glm::unProject(glm::vec3(winX, winY, winZ),
                glm::mat4(), fusion->projection, fusion->viewport);

        /* Project left edge center of sphere */
        glm::vec3 left = glm::project(glm::vec3(obj.x - .5, obj.y, obj.z),
                glm::mat4(), fusion->projection, fusion->viewport);

        /* Project right edge center of sphere */
        glm::vec3 right = glm::project(glm::vec3(obj.x + .5, obj.y, obj.z),
                glm::mat4(), fusion->projection, fusion->viewport);

        float width = std::abs(right.x - left.x);
        if (width > targetWidth) {
            /* Too near */
            winZ += step;
        } else if (width < targetWidth) {
            /* Too far away */
            winZ -= step;
        } else {
            /* Perfect fit */
            break;
        }
        step *= .5;
    }

    if (x != NULL) {
        *x = obj.x;
    }

    if (y != NULL) {
        *y = obj.y;
    }

    if (z != NULL) {
        *z = obj.z;
    }
}

void
psmove_fusion_free(PSMoveFusion *fusion)
{
    free(fusion);
}

