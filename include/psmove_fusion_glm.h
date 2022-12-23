#pragma once

/**
 * PS Move API - An interface for the PS Move Motion Controller
 * Copyright (c) 2022 Thomas Perl <m@thp.io>
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

#include <glm/glm.hpp>


static inline glm::vec3
psmove_fusion_get_position_vec3(PSMoveFusion *fusion, PSMove *move)
{
    glm::vec3 result { 0.f, 0.f, 0.f };
    psmove_fusion_get_position(fusion, move, &result.x, &result.y, &result.z);
    return result;
}

static inline glm::mat4
psmove_fusion_get_projection_matrix_mat4(PSMoveFusion *fusion)
{
    glm::mat4 result { 0.f };

    float *m = psmove_fusion_get_projection_matrix(fusion);
    for (int i=0; i<16; ++i) {
        result[i/4][i%4] = m[i];
    }

    return result;
}

static inline glm::mat4
psmove_fusion_get_modelview_matrix_mat4(PSMoveFusion *fusion, PSMove *move)
{
    glm::mat4 result { 0.f };

    float *m = psmove_fusion_get_modelview_matrix(fusion, move);
    for (int i=0; i<16; ++i) {
        result[i/4][i%4] = m[i];
    }

    return result;
}
