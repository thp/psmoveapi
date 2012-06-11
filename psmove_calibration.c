
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



#include "psmove_private.h"
#include "psmove_calibration.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <libgen.h>
#include <sys/stat.h>

#define PSMOVE_CALIBRATION_EXTENSION ".calibration.txt"

enum _PSMoveCalibrationFlag {
    CalibrationFlag_None = 0,
    CalibrationFlag_HaveCustom,
    CalibrationFlag_HaveUSB,
};

struct _PSMoveCalibration {
    PSMove *move;

    float custom_calibration[PSMOVE_CALIBRATION_POSITIONS]
        [PSMOVE_CALIBRATION_FIELDS];
    char usb_calibration[PSMOVE_CALIBRATION_BLOB_SIZE];
    int flags;

    char *filename;
};



int
psmove_calibration_decode(char *data, int offset)
{
    unsigned char low = data[offset] & 0xFF;
    unsigned char high = (data[offset+1]) & 0xFF;
    return (low | (high << 8)) - 0x8000;
}

void
psmove_calibration_parse_usb(PSMoveCalibration *calibration)
{
    assert(calibration != NULL);
    char *data = calibration->usb_calibration;
    int orientation;
    int x, y, z;

    /* http://code.google.com/p/moveonpc/wiki/CalibrationData */

    for (orientation=0; orientation<6; orientation++) {
        x = psmove_calibration_decode(data, 0x04 + 6*orientation);
        y = psmove_calibration_decode(data, 0x04 + 6*orientation + 2);
        z = psmove_calibration_decode(data, 0x04 + 6*orientation + 4);
        printf("# Orientation #%d: (%5d | %5d | %5d)\n", orientation, x, y, z);
    }

    for (orientation=0; orientation<3; orientation++) {
        x = psmove_calibration_decode(data, 0x46 + 8*orientation);
        y = psmove_calibration_decode(data, 0x46 + 8*orientation + 2);
        z = psmove_calibration_decode(data, 0x46 + 8*orientation + 4);
        printf("# Gyro %c, 80 rpm: (%5d | %5d | %5d)\n", "XYZ"[orientation], x, y, z);
    }

    printf("# byte at 0x3F: %02hhx\n", data[0x3F]);
}

void
psmove_calibration_dump_usb(PSMoveCalibration *calibration)
{
    int j;

    assert(calibration != NULL);

    for (j=0; j<sizeof(calibration->usb_calibration); j++) {
        printf("%02hhx", calibration->usb_calibration[j]);
        if (j % 16 == 15) {
            printf("\n");
        } else if (j < sizeof(calibration->usb_calibration)-1) {
            printf(" ");
        }
    }
    printf("\n");

    psmove_calibration_parse_usb(calibration);
    printf("\n");
}



PSMoveCalibration *
psmove_calibration_new(PSMove *move)
{
    PSMove_Data_BTAddr addr;
    const char *data_dir;
    char *serial;
    int i;

    PSMoveCalibration *calibration =
        (PSMoveCalibration*)calloc(1, sizeof(PSMoveCalibration));

    calibration->move = move;

    if (psmove_connection_type(move) == Conn_USB) {
        psmove_read_btaddrs(move, NULL, &addr);
        serial = psmove_btaddr_to_string(addr);
    } else {
        serial = strdup(psmove_get_serial(move));
    }

    for (i=0; i<strlen(serial); i++) {
        if (serial[i] == ':') {
            serial[i] = '_';
        }
    }

    data_dir = psmove_util_get_data_dir();

    i = strlen(data_dir)+strlen(serial)+strlen(PSMOVE_CALIBRATION_EXTENSION)+1;
    calibration->filename = (char*)calloc(i, sizeof(char));
    strncpy(calibration->filename, data_dir, i);
    strncat(calibration->filename, serial, i);
    strncat(calibration->filename, PSMOVE_CALIBRATION_EXTENSION, i);

    free(serial);

    return calibration;
}

int
psmove_calibration_read_from_usb(PSMoveCalibration *calibration)
{
    assert(calibration != NULL);

    char *data;
    size_t size;

    if (psmove_get_calibration_blob(calibration->move, &data, &size)) {
        assert(size == PSMOVE_CALIBRATION_BLOB_SIZE);
        memcpy(calibration->usb_calibration, data, size);
        free(data);
        calibration->flags |= CalibrationFlag_HaveUSB;
        return 1;
    }

    return 0;
}

void
psmove_calibration_set_custom(PSMoveCalibration *calibration,
        float *positions, size_t n_positions, size_t n_fields)
{
    assert(calibration != NULL);
    assert(positions != NULL);
    assert(n_positions == 6);
    assert(n_fields == 9);

    int i, j;

    for (i=0; i<n_positions; i++) {
        for (j=0; j<n_fields; j++) {
            calibration->custom_calibration[i][j] = positions[i*n_fields+j];
        }
    }

    calibration->flags |= CalibrationFlag_HaveCustom;
}

void
psmove_calibration_dump(PSMoveCalibration *calibration)
{
    int i, j;

    assert(calibration != NULL);

    printf("File: %s\n", calibration->filename);
    printf("Flags: %x\n", calibration->flags);

    if (calibration->flags & CalibrationFlag_HaveUSB) {
        printf("Have USB calibration:\n");
        psmove_calibration_dump_usb(calibration);
    }

    if (calibration->flags & CalibrationFlag_HaveCustom) {
        printf("Have custom calibration:\n");
        printf("         ax         ay         az         mx         my         mz\n");
        for (i=0; i<PSMOVE_CALIBRATION_POSITIONS; i++) {
            printf("#%d: ", i);
            for (j=0; j<PSMOVE_CALIBRATION_FIELDS; j++) {
                if (j < 3 || j > 5 /* Don't print the gyro column */) {
                    printf("%10.2f ", calibration->custom_calibration[i][j]);
                }
            }
            printf("\n");
        }
    }
}

enum PSMove_Calibration_Method
psmove_calibration_map(PSMoveCalibration *calibration,
        enum PSMove_Calibration_Method preferred,
        int *input, float *output, size_t n)
{
    assert(calibration != NULL);
    assert(preferred == Calibration_Any ||
            preferred == Calibration_USB ||
            preferred == Calibration_Custom);
    assert(input != NULL);
    assert(output != NULL);
    assert(n == 3); // TODO: support n==6 and n==9 too!

    int i;

    if (!psmove_calibration_supports_method(calibration, Calibration_Custom)) {
        for (i=0; i<n; i++) {
            output[i] = (float)input[i];
        }
        return Calibration_None;
    }

    /* ax, ay, az */
    output[0] = ((float)input[0])/(calibration->custom_calibration[4][0] - calibration->custom_calibration[5][0])*2.;
    output[1] = ((float)input[1])/(calibration->custom_calibration[0][1] - calibration->custom_calibration[1][1])*2.;
    output[2] = ((float)input[2])/(calibration->custom_calibration[2][2] - calibration->custom_calibration[3][2])*2.;

    return Calibration_Custom;
}

int
psmove_calibration_supports_method(PSMoveCalibration *calibration,
        enum PSMove_Calibration_Method method)
{
    assert(calibration != NULL);
    assert(method == Calibration_USB || method == Calibration_Custom);

    switch (method) {
        case Calibration_USB:
            return (calibration->flags & CalibrationFlag_HaveUSB) != 0;
            break;
        case Calibration_Custom:
            return (calibration->flags & CalibrationFlag_HaveCustom) != 0;
            break;
        default:
            return 0;
    }
}

int
psmove_calibration_load(PSMoveCalibration *calibration)
{
    assert(calibration != NULL);
    FILE *fp;

    fp = fopen(calibration->filename, "rb");
    if (fp == NULL) {
        return 0;
    }

    assert(fread(calibration->custom_calibration,
                sizeof(calibration->custom_calibration),
                1, fp) == 1);
    assert(fread(calibration->usb_calibration,
                sizeof(calibration->usb_calibration),
                1, fp) == 1);
    assert(fread(&(calibration->flags),
                sizeof(calibration->flags),
                1, fp) == 1);
    fclose(fp);

    return 1;
}

int
psmove_calibration_save(PSMoveCalibration *calibration)
{
    assert(calibration != NULL);

    char *parent = dirname(calibration->filename);
    struct stat st;
    FILE *fp;

    if (stat(parent, &st) != 0) {
        if (mkdir(parent, 0777) != 0) {
            return 0;
        }
    }

    fp = fopen(calibration->filename, "wb");
    assert(fp != NULL);
    assert(fwrite(calibration->custom_calibration,
                sizeof(calibration->custom_calibration),
                1, fp) == 1);
    assert(fwrite(calibration->usb_calibration,
                sizeof(calibration->usb_calibration),
                1, fp) == 1);
    assert(fwrite(&(calibration->flags),
                sizeof(calibration->flags),
                1, fp) == 1);
    fclose(fp);

    return 1;
}

void
psmove_calibration_destroy(PSMoveCalibration *calibration)
{
    assert(calibration != NULL);

    free(calibration->filename);
    free(calibration);
}

