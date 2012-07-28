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

#include "opencv2/core/core_c.h"

// uncomment this line to disable trace-output of the tracker
#define USE_TRACKER_TRACE

void psmove_trace_image(IplImage *image, char* name, int no_js_var);
void psmove_trace_image_at(IplImage *image, int index, char* target);
void psmove_trace_array_item_at(int index, char* target, char* value);
void psmove_trace_array_item(char* target, const char* value);
void psmove_trace_clear();
void psmove_trace_put_int_var(const char* var, int value);
void psmove_trace_put_color_var(const char* var, CvScalar color);
void psmove_trace_put_text_var(const char* var, const char* value);
void psmove_trace_put_log_entry(const char* type, const char* value);
void psmove_trace_put_text(const char* text);


#ifndef USE_TRACKER_TRACE
	#define psmove_html_trace_image(image, name,no_js_var)
	#define psmove_html_trace_image_at(image, index, target)
	#define psmove_html_trace_array_item(target,value)
	#define psmove_html_trace_array_item_at(index,target,value)
	#define psmove_html_trace_var_int(var,value)
	#define psmove_html_trace_var_text(var,value)
	#define psmove_html_trace_var_color(var,value)
	#define psmove_html_trace_log_entry(text)
	#define psmove_html_trace_text(text)
	#define psmove_html_trace_clear()
#else
	#define psmove_html_trace_image(image, name,no_js_var) psmove_trace_image((image),(name),(no_js_var))
	#define psmove_html_trace_image_at(image, index, target) psmove_trace_image_at((image),(index),(target))
	#define psmove_html_trace_array_item(target,value) psmove_trace_array_item((target), (value))
	#define psmove_html_trace_array_item_at(index,target,value) psmove_trace_array_item_at((index),(target), (value))
	#define psmove_html_trace_var_int(var,value) psmove_trace_put_int_var((var), (value))
	#define psmove_html_trace_var_text(var,value) psmove_trace_put_text_var((var), (value))
	#define psmove_html_trace_var_color(var,value) psmove_trace_put_color_var((var), (value))
	#define psmove_html_trace_log_entry(type,value) psmove_trace_put_log_entry((type), (value));
	#define psmove_html_trace_text(text) psmove_trace_put_text((text))
	#define psmove_html_trace_clear() psmove_trace_clear()
#endif

#endif /* TRACKER_TRACE_H_ */
