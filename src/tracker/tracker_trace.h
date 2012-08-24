/**
 * PS Move API - An interface for the PS Move Motion Controller
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

#ifndef TRACKER_TRACE_H_
#define TRACKER_TRACE_H_

#include "psmove_config.h"

#include "opencv2/core/core_c.h"

#ifndef PSMOVE_USE_TRACKER_TRACE
#    define psmove_html_trace_image(image, name, no_js_var)
#    define psmove_html_trace_image_at(image, index, target)
#    define psmove_html_trace_array_item(target, value)
#    define psmove_html_trace_array_item_at(index, target, value)
#    define psmove_html_trace_put_int_var(var, value)
#    define psmove_html_trace_put_color_var(var, value)
#    define psmove_html_trace_put_text_var(var, value)
#    define psmove_html_trace_put_log_entry(type, value)
#    define psmove_html_trace_put_text(text)
#    define psmove_html_trace_clear()
#else
void psmove_html_trace_image(IplImage *image, char* name, int no_js_var);
void psmove_html_trace_image_at(IplImage *image, int index, char* target);
void psmove_html_trace_array_item(char* target, const char* value);
void psmove_html_trace_array_item_at(int index, char* target, char* value);
void psmove_html_trace_put_int_var(const char* var, int value);
void psmove_html_trace_put_color_var(const char* var, CvScalar color);
void psmove_html_trace_put_text_var(const char* var, const char* value);
void psmove_html_trace_put_log_entry(const char* type, const char* value);
void psmove_html_trace_put_text(const char* text);
void psmove_html_trace_clear();
#endif

#endif /* TRACKER_TRACE_H_ */
