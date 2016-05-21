
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define PSMOVE_CALIBRATION_EXTENSION ".calibration"

enum _PSMoveCalibrationFlag {
    CalibrationFlag_None = 0,
    CalibrationFlag_HaveUSB,
};

struct _PSMoveCalibration {
    PSMove *move;

    char usb_calibration[PSMOVE_CALIBRATION_BLOB_SIZE];
    int flags;

    char *filename;

    /**
     * system_filename always points to calibration file stored in
     * system-wide data directory, while filename above points to
     * local calibration file for a non-root user or to system
     * calibration file if run by root.
     **/
    char *system_filename;

    /* Pre-calculated factors for accelerometer mapping */
    float ax, ay, az;

    /* Pre-calculated summands for accelerometer mapping */
    float bx, by, bz;

    /* Pre-calculated factors for gyroscope mapping */
    float gx, gy, gz;
};


/* PRIVATE FUNCTION DEFINITIONS - ONLY USED IN THIS MODULE DIRECTLY */

/**
 * Read calibration data from controller storage via USB
 *
 * calibration ... a valid PSMoveCalibration * instance.
 *
 * The calibration data blob will be read from the controller and will be
 * stored inside this calibration object for future use in value mapping.
 *
 * Returns nonzero on success, zero on error (e.g. not connected via USB)
 **/
int
psmove_calibration_read_from_usb(PSMoveCalibration *calibration);

/**
 * Load the calibration from persistent storage.
 *
 * Returns nonzero on success, zero on error.
 **/
int
psmove_calibration_load(PSMoveCalibration *calibration);

/**
 * Save the calibration to persistent storage.
 *
 * Returns nonzero on success, zero on error.
 **/
int
psmove_calibration_save(PSMoveCalibration *calibration);





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
    int x, y, z, t;

    printf("\n");

    /* https://github.com/nitsch/moveonpc/wiki/Calibration-data */

    for (orientation=0; orientation<6; orientation++) {
        x = psmove_calibration_decode(data, 0x04 + 6*orientation);
        y = psmove_calibration_decode(data, 0x04 + 6*orientation + 2);
        z = psmove_calibration_decode(data, 0x04 + 6*orientation + 4);
        printf("# Orientation #%d: (%5d | %5d | %5d)\n", orientation, x, y, z);
    }

    printf("\n");

    for (orientation=0; orientation<3; orientation++) {
        x = psmove_calibration_decode(data, 0x46 + 8*orientation);
        y = psmove_calibration_decode(data, 0x46 + 8*orientation + 2);
        z = psmove_calibration_decode(data, 0x46 + 8*orientation + 4);
        printf("# Gyro %c, 80 rpm: (%5d | %5d | %5d)\n", "XYZ"[orientation], x, y, z);
    }

    printf("\n");

    t = psmove_calibration_decode(data, 0x28);
    x = psmove_calibration_decode(data, 0x2a);
    y = psmove_calibration_decode(data, 0x2a + 2);
    z = psmove_calibration_decode(data, 0x2a + 4);
    printf("# Temperature at 0x28: (%5d)\n", t);
    printf("# Vector at 0x2a: (%5d | %5d | %5d)\n", x, y, z);

    printf("\n");

    t = psmove_calibration_decode(data, 0x30);
    x = psmove_calibration_decode(data, 0x32);
    y = psmove_calibration_decode(data, 0x32 + 2);
    z = psmove_calibration_decode(data, 0x32 + 4);
    printf("# Temperature at 0x30: (%5d)\n", t);
    printf("# Vector at 0x32: (%5d | %5d | %5d)\n", x, y, z);

    printf("\n");

    printf("# byte at 0x3F: %02x\n", (unsigned char) data[0x3F]);
}

void
psmove_calibration_get_usb_accel_values(PSMoveCalibration *calibration,
        int *x1, int *x2, int *y1, int *y2, int *z1, int *z2)
{
    assert(calibration != NULL);
    assert(psmove_calibration_supported(calibration));
    char *data = calibration->usb_calibration;

    int orientation;

    /* Minimum (negative) value of each axis */
    orientation = 1;
    *x1 = psmove_calibration_decode(data, 0x04 + 6*orientation);
    orientation = 5;
    *y1 = psmove_calibration_decode(data, 0x04 + 6*orientation + 2);
    orientation = 2;
    *z1 = psmove_calibration_decode(data, 0x04 + 6*orientation + 4);

    /* Maximum (positive) values of each axis */
    orientation = 3;
    *x2 = psmove_calibration_decode(data, 0x04 + 6*orientation);
    orientation = 4;
    *y2 = psmove_calibration_decode(data, 0x04 + 6*orientation + 2);
    orientation = 0;
    *z2 = psmove_calibration_decode(data, 0x04 + 6*orientation + 4);
}


void
psmove_calibration_get_usb_gyro_values(PSMoveCalibration *calibration,
        int *x, int *y, int *z)
{
    assert(calibration != NULL);
    assert(psmove_calibration_supported(calibration));
    char *data = calibration->usb_calibration;

    int bx, by, bz; /* Bias(?) values, need to sustract those */
    bx = psmove_calibration_decode(data, 0x2a);
    by = psmove_calibration_decode(data, 0x2a + 2);
    bz = psmove_calibration_decode(data, 0x2a + 4);

    int orientation;

    orientation = 0;
    *x = psmove_calibration_decode(data, 0x46 + 8*orientation) - bx;
    orientation = 1;
    *y = psmove_calibration_decode(data, 0x46 + 8*orientation + 2) - by;
    orientation = 2;
    *z = psmove_calibration_decode(data, 0x46 + 8*orientation + 4) - bz;
}

void
psmove_calibration_dump_usb(PSMoveCalibration *calibration)
{
    int j;

    assert(calibration != NULL);

    for (j=0; j<sizeof(calibration->usb_calibration); j++) {
        printf("%02x", (unsigned char) calibration->usb_calibration[j]);
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
    char *serial;
    int i;

    PSMoveCalibration *calibration =
        (PSMoveCalibration*)calloc(1, sizeof(PSMoveCalibration));

    calibration->move = move;

    if (psmove_connection_type(move) == Conn_USB) {
        _psmove_read_btaddrs(move, NULL, &addr);
        serial = _psmove_btaddr_to_string(addr);
    } else {
        serial = psmove_get_serial(move);
    }

    if (!serial) {
        psmove_CRITICAL("Could not determine serial from controller");
        free(calibration);
        return NULL;
    }

    for (i=0; i<strlen(serial); i++) {
        if (serial[i] == ':') {
            serial[i] = '_';
        }
    }

    char *template = malloc(strlen(serial) +
            strlen(PSMOVE_CALIBRATION_EXTENSION) + 1);
    strcpy(template, serial);
    strcat(template, PSMOVE_CALIBRATION_EXTENSION);

    calibration->filename = psmove_util_get_file_path(template);
    calibration->system_filename = psmove_util_get_system_file_path(template);

    free(template);
    free(serial);

    /* Try to load the calibration data from disk, or from USB */
    psmove_calibration_load(calibration);
    if (!psmove_calibration_supported(calibration)) {
        if (psmove_connection_type(move) == Conn_USB) {
            psmove_DEBUG("Storing calibration from USB\n");
            psmove_calibration_read_from_usb(calibration);
            psmove_calibration_save(calibration);
        }
    }

    /* Pre-calculate values used for mapping input */
    if (psmove_calibration_supported(calibration)) {
        /* Accelerometer reading (high/low) for each axis */
        int axlow, axhigh, aylow, ayhigh, azlow, azhigh;
        psmove_calibration_get_usb_accel_values(calibration,
                &axlow, &axhigh, &aylow, &ayhigh, &azlow, &azhigh);

        /**
         *
         * Calculation of accelermeter mapping (as factor of gravity, 1g):
         *
         *                2 * (raw - low)
         *  calibrated = ----------------  - 1
         *                 (high - low)
         *
         * with:
         *
         *  raw .... Raw sensor reading
         *  low .... Raw reading at -1g
         *  high ... Raw reading at +1g
         *
         * Now define:
         *
         *            2
         *  f = --------------
         *       (high - low)
         *
         * And combine constants:
         *
         *  c = - (f * low) - 1
         *
         * Then we get:
         *
         *  calibrated = f * raw + c
         *
         **/

        /* Accelerometer factors "f" */
        calibration->ax = 2.f / (float)(axhigh - axlow);
        calibration->ay = 2.f / (float)(ayhigh - aylow);
        calibration->az = 2.f / (float)(azhigh - azlow);

        /* Accelerometer constants "c" */
        calibration->bx = - (calibration->ax * (float)axlow) - 1.f;
        calibration->by = - (calibration->ay * (float)aylow) - 1.f;
        calibration->bz = - (calibration->az * (float)azlow) - 1.f;

        /**
         * Calculation of gyroscope mapping (in radiant per second):
         *
         *                 raw
         *  calibrated = -------- * 2 PI
         *                rpm60
         *
         *           60 * rpm80
         *  rpm60 = ------------
         *               80
         *
         * with:
         *
         *  raw ..... Raw sensor reading
         *  rpm80 ... Sensor reading at 80 RPM (from calibration blob)
         *  rpm60 ... Sensor reading at 60 RPM (1 rotation per second)
         *
         * Or combined:
         *
         *                80 * raw * 2 PI
         *  calibrated = -----------------
         *                  60 * rpm80
         *
         * Now define:
         *
         *       2 * PI * 80
         *  f = -------------
         *        60 * rpm80
         *
         * Then we get:
         *
         *  calibrated = f * raw
         *
         **/

        int gx80, gy80, gz80;
        psmove_calibration_get_usb_gyro_values(calibration,
                &gx80, &gy80, &gz80);

        float factor = (float)(2.f * M_PI * 80.f) / 60.f;
        calibration->gx = factor / (float)gx80;
        calibration->gy = factor / (float)gy80;
        calibration->gz = factor / (float)gz80;
    } else {
        /* No calibration data - pass-through input data */
        calibration->ax = 1.f;
        calibration->ay = 1.f;
        calibration->az = 1.f;

        calibration->bx = 0.f;
        calibration->by = 0.f;
        calibration->bz = 0.f;

        calibration->gx = 1.f;
        calibration->gy = 1.f;
        calibration->gz = 1.f;
    }

    return calibration;
}

int
psmove_calibration_read_from_usb(PSMoveCalibration *calibration)
{
    assert(calibration != NULL);

    char *data;
    size_t size;

    if (_psmove_get_calibration_blob(calibration->move, &data, &size)) {
        assert(size == PSMOVE_CALIBRATION_BLOB_SIZE);
        memcpy(calibration->usb_calibration, data, size);
        free(data);
        calibration->flags |= CalibrationFlag_HaveUSB;
        return 1;
    }

    return 0;
}

void
psmove_calibration_dump(PSMoveCalibration *calibration)
{
    psmove_return_if_fail(calibration != NULL);

    printf("File: %s\n", calibration->filename);
    printf("System file: %s\n", calibration->system_filename);
    printf("Flags: %x\n", calibration->flags);

    if (calibration->flags & CalibrationFlag_HaveUSB) {
        printf("Have USB calibration:\n");
        psmove_calibration_dump_usb(calibration);
    }
}

void
psmove_calibration_map_accelerometer(PSMoveCalibration *calibration,
        int *raw_input, float *ax, float *ay, float *az)
{
    psmove_return_if_fail(calibration != NULL);
    psmove_return_if_fail(raw_input != NULL);

    if (ax) {
        *ax = (float)raw_input[0] * calibration->ax + calibration->bx;
    }
    if (ay) {
        *ay = (float)raw_input[1] * calibration->ay + calibration->by;
    }
    if (az) {
        *az = (float)raw_input[2] * calibration->az + calibration->bz;
    }
}

void
psmove_calibration_map_gyroscope(PSMoveCalibration *calibration,
        int *raw_input, float *gx, float *gy, float *gz)
{
    psmove_return_if_fail(calibration != NULL);
    psmove_return_if_fail(raw_input != NULL);

    if (gx) {
        *gx = (float)raw_input[0] * calibration->gx;
    }

    if (gy) {
        *gy = (float)raw_input[1] * calibration->gy;
    }

    if (gz) {
        *gz = (float)raw_input[2] * calibration->gz;
    }
}

int
psmove_calibration_supported(PSMoveCalibration *calibration)
{
    psmove_return_val_if_fail(calibration != NULL, 0);

    return (calibration->flags & CalibrationFlag_HaveUSB) != 0;
}

int
psmove_calibration_load(PSMoveCalibration *calibration)
{
    psmove_return_val_if_fail(calibration != NULL, 0);
    FILE *fp;

    fp = fopen(calibration->filename, "rb");
    if (fp == NULL) {
        // use system file in case local is not available
        fp = fopen(calibration->system_filename, "rb");
        if (fp == NULL) {
            return 0;
        }
        return 0;
    }

    if (fread(calibration->usb_calibration,
              sizeof(calibration->usb_calibration), 1, fp) != 1) {
        psmove_CRITICAL("Unable to read USB calibration");
        fclose(fp);
        return 0;
    }
    if (fread(&(calibration->flags),
              sizeof(calibration->flags), 1, fp) != 1) {
        psmove_CRITICAL("Unable to read USB calibration");
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return 1;
}

int
psmove_calibration_save(PSMoveCalibration *calibration)
{
    psmove_return_val_if_fail(calibration != NULL, 0);

    FILE *fp;

    fp = fopen(calibration->filename, "wb");
    if (fp == NULL) {
        psmove_CRITICAL("Unable to write USB calibration");
        return 0;
    }

    if (fwrite(calibration->usb_calibration,
               sizeof(calibration->usb_calibration),
               1, fp) != 1) {
        psmove_CRITICAL("Unable to write USB calibration");
        fclose(fp);
        return 0;
    }

    if (fwrite(&(calibration->flags),
               sizeof(calibration->flags),
               1, fp) != 1) {
        psmove_CRITICAL("Unable to write USB calibration");
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return 1;
}

void
psmove_calibration_free(PSMoveCalibration *calibration)
{
    psmove_return_if_fail(calibration != NULL);

    free(calibration->filename);
    free(calibration->system_filename);
    free(calibration);
}

