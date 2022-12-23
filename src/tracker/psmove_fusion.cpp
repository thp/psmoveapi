
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


struct _PSMoveFusion {
    PSMoveTracker *tracker;

    float width;
    float height;

    glm::mat4 projection;
    glm::mat4 inverse_projection;
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
    fusion->inverse_projection = glm::inverse(fusion->projection);
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

    fusion->modelview = glm::translate(glm::mat4(1.f),
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

    float wx = 2.f * (camX - fusion->viewport[0]) / fusion->viewport[2] - 1.f;
    float wy = 2.f * (1.f - (camY - fusion->viewport[1]) / fusion->viewport[3]) - 1.f;

    float xx = (fusion->projection[0][0] * fusion->viewport[2]) / (4.f * camR);

    float zc = wx * fusion->inverse_projection[0][2] +
               wy * fusion->inverse_projection[1][2] +
                    fusion->inverse_projection[3][2];
    float zf = fusion->inverse_projection[2][2];
    float wc = wx * fusion->inverse_projection[0][3] +
               wy * fusion->inverse_projection[1][3] +
                    fusion->inverse_projection[3][3];
    float wf = fusion->inverse_projection[2][3];

    float wz = (xx * wc - zc) / (zf - xx * wf);

    glm::vec4 obj = fusion->inverse_projection * glm::vec4(wx, wy, wz, 1.f);

    if (x != NULL) {
        *x = obj.x / obj.w;
    }

    if (y != NULL) {
        *y = obj.y / obj.w;
    }

    if (z != NULL) {
        *z = obj.z / obj.w;
    }
}

void
psmove_fusion_free(PSMoveFusion *fusion)
{
    free(fusion);
}

