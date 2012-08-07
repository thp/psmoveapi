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

#include <stdio.h>

#ifdef WIN32
#	include <direct.h>
#else
#	include  <sys/stat.h>
#	include  <sys/types.h>
#endif

#include "psmove.h"
#include "tracker_trace.h"
#include "../tracker/tracker_helpers.h"

typedef struct {
    FILE *fp;
    int img_count;
} TrackerTrace;

TrackerTrace tracker_trace = {
    .fp = NULL,
    .img_count = 0,
};


const char *
tracker_trace_filename(const char *template)
{
    static char filename[1024];

    strncpy(filename, psmove_util_get_data_dir(), sizeof(filename));
    strncat(filename, template, sizeof(filename));

    return filename;
}


FILE *
tracker_trace_file()
{
    if (!tracker_trace.fp) {
        tracker_trace.fp = fopen(tracker_trace_filename("debug.js"), "w");
    }

    return tracker_trace.fp;
}

#define tracker_trace_printf(...) { \
    fprintf(tracker_trace_file(), __VA_ARGS__); \
    fflush(tracker_trace_file()); \
}


void
psmove_html_trace_clear()
{
    tracker_trace.img_count = 0;
    if (tracker_trace.fp) {
        fclose(tracker_trace.fp);
    }

    time_t rawtime;
    struct tm* timeinfo;
    char texttime[256];
    time(&rawtime);
    timeinfo=localtime(&rawtime);
    strftime(texttime,256,"%Y-%m-%d / %H:%M:%S",timeinfo);

    FILE *pFile = tracker_trace_file();
    if (pFile) {
        fputs("originals = new Array();\n", pFile);
        fputs("rawdiffs = new Array();\n", pFile);
        fputs("threshdiffs = new Array();\n", pFile);
        fputs("erodediffs = new Array();\n", pFile);
        fputs("finaldiff = new Array();\n", pFile);
        fputs("filtered = new Array();\n", pFile);
        fputs("contours = new Array();\n", pFile);
        fputs("log_table = new Array();\n\n", pFile);
    }

    psmove_html_trace_put_text_var("time",texttime);
}

void
psmove_html_trace_image_at(IplImage *image, int index, char* target)
{
    char img_name[256];

    // write image to file sysxtem
    sprintf(img_name, "image_%d.jpg", tracker_trace.img_count);
    th_save_jpg(tracker_trace_filename(img_name), image, 100);

    tracker_trace.img_count++;

    // write image-name to java script array
    psmove_html_trace_array_item_at(index, target, img_name);
}

void
psmove_html_trace_image(IplImage *image, char* var, int no_js_var)
{
    char img_name[256];
    // write image to file sysxtem
    sprintf(img_name, "image_%s.jpg", var);
    th_save_jpg(tracker_trace_filename(img_name), image, 100);

    // write image-name to java variable (if desired)
    if (!no_js_var) {
        psmove_html_trace_put_text_var(var,img_name);
    }
}

void
psmove_html_trace_array_item_at(int index, char* target, char* value)
{
    tracker_trace_printf("%s[%d]='%s';\n", target, index, value);
}

void
psmove_html_trace_array_item(char* target, const char* value)
{
    tracker_trace_printf("%s.push('%s');\n", target, value);
}

void
psmove_html_trace_put_log_entry(const char* type, const char* value)
{
    tracker_trace_printf("log_table.push({type:'%s', value:'%s'});\n", type, value);
}

void
psmove_html_trace_put_text(const char* text)
{
    tracker_trace_printf("%s\n", text);
}

void
psmove_html_trace_put_int_var(const char* var, int value)
{
    tracker_trace_printf("%s=%d;\n", var, value);
}

void
psmove_html_trace_put_text_var(const char* var, const char* value)
{
    tracker_trace_printf("%s='%s';\n", var, value);
}

void
psmove_html_trace_put_color_var(const char* var, CvScalar color)
{
    char text[32];
    unsigned int r = (unsigned int) round(color.val[2]);
    unsigned int g = (unsigned int) round(color.val[1]);
    unsigned int b = (unsigned int) round(color.val[0]);
    sprintf(text, "%02X%02X%02X", r, g, b);
    psmove_html_trace_put_text_var(var, text);
}

