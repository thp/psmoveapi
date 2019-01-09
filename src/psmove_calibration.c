
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

    char usb_calibration[PSMOVE_MAX_CALIBRATION_BLOB_SIZE];
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

    /* Pre-calculated raw drift values for gyroscope mapping */
    int dx, dy, dz;
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




// TODO: Combine these decoding functions with the ones in psmove.c

static inline int
psmove_calibration_decode_16bit_unsigned(char *data, int offset)
{
    unsigned char low = data[offset] & 0xFF;
    unsigned char high = (data[offset+1]) & 0xFF;
    return (low | (high << 8)) - 0x8000;
}

static inline short
psmove_calibration_decode_16bit_signed(char *data, int offset)
{
    unsigned short low = data[offset] & 0xFF;
    unsigned short high = (data[offset+1]) & 0xFF;
    return (short)(low | (high << 8));
}

static inline unsigned int
psmove_calibration_decode_12bits(char *data, int offset)
{
    unsigned char low = data[offset] & 0xFF;
    unsigned char high = (data[offset+1]) & 0xFF;
    return low | (high << 8);
}

static inline float
psmove_calibration_decode_float(char *data, int offset)
{
    uint32_t v = (data[offset] & 0xFF)
        | ((data[offset+1] & 0xFF) <<  8)
        | ((data[offset+2] & 0xFF) << 16)
        | ((data[offset+3] & 0xFF) << 24);
    return *((float *) &v);
}


static void
psmove_zcm1_calibration_parse_usb(PSMoveCalibration *calibration)
{
    assert(calibration != NULL);
    char *data = calibration->usb_calibration;
    int orientation;
    int x, y, z, t;
    float fx, fy, fz;

    printf("\n");

    /* https://github.com/nitsch/moveonpc/wiki/Calibration-data */

    t = psmove_calibration_decode_12bits(data, 0x02);
    printf("# Temperature: 0x%04X (%.0f °C)\n", t, _psmove_temperature_to_celsius(t));
    for (orientation=0; orientation<6; orientation++) {
        x = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6*orientation);
        y = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6*orientation + 2);
        z = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6*orientation + 4);
        printf("# Orientation #%d:      (%5d | %5d | %5d)\n", orientation, x, y, z);
    }

    printf("\n");

    t = psmove_calibration_decode_12bits(data, 0x42);
    printf("# Temperature: 0x%04X (%.0f °C)\n", t, _psmove_temperature_to_celsius(t));
    for (orientation=0; orientation<3; orientation++) {
        x = psmove_calibration_decode_16bit_unsigned(data, 0x46 + 8*orientation);
        y = psmove_calibration_decode_16bit_unsigned(data, 0x46 + 8*orientation + 2);
        z = psmove_calibration_decode_16bit_unsigned(data, 0x46 + 8*orientation + 4);
        printf("# Gyro %c, 80 rpm:      (%5d | %5d | %5d)\n", "XYZ"[orientation], x, y, z);
    }

    printf("\n");

    t = psmove_calibration_decode_12bits(data, 0x28);
    x = psmove_calibration_decode_16bit_unsigned(data, 0x2a);
    y = psmove_calibration_decode_16bit_unsigned(data, 0x2a + 2);
    z = psmove_calibration_decode_16bit_unsigned(data, 0x2a + 4);
    printf("# Temperature: 0x%04X (%.0f °C)\n", t, _psmove_temperature_to_celsius(t));
    printf("# Gyro, 0 rpm (@0x2a): (%5d | %5d | %5d)\n", x, y, z);

    printf("\n");

    t = psmove_calibration_decode_12bits(data, 0x30);
    x = psmove_calibration_decode_16bit_unsigned(data, 0x32);
    y = psmove_calibration_decode_16bit_unsigned(data, 0x32 + 2);
    z = psmove_calibration_decode_16bit_unsigned(data, 0x32 + 4);
    printf("# Temperature: 0x%04X (%.0f °C)\n", t, _psmove_temperature_to_celsius(t));
    printf("# Gyro, 0 rpm (@0x32): (%5d | %5d | %5d)\n", x, y, z);

    printf("\n");

    t = psmove_calibration_decode_12bits(data, 0x5c);
    fx = psmove_calibration_decode_float(data, 0x5e);
    fy = psmove_calibration_decode_float(data, 0x5e + 4);
    fz = psmove_calibration_decode_float(data, 0x5e + 8);
    printf("# Temperature: 0x%04X (%.0f °C)\n", t, _psmove_temperature_to_celsius(t));
    printf("# Vector @0x5e: (%f | %f | %f)\n", fx, fy, fz);

    fx = psmove_calibration_decode_float(data, 0x6a);
    fy = psmove_calibration_decode_float(data, 0x6a + 4);
    fz = psmove_calibration_decode_float(data, 0x6a + 8);
    printf("# Vector @0x6a: (%f | %f | %f)\n", fx, fy, fz);

    printf("\n");

    printf("# byte @0x3f:  0x%02x\n", (unsigned char) data[0x3f]);
    printf("# float @0x76: %f\n", psmove_calibration_decode_float(data, 0x76));
    printf("# float @0x7a: %f\n", psmove_calibration_decode_float(data, 0x7a));
}

static void
psmove_zcm2_calibration_parse_usb(PSMoveCalibration *calibration)
{
    assert(calibration != NULL);
    char *data = calibration->usb_calibration;
    int orientation;
    int x, y, z;

    printf("\n");

    /* https://github.com/nitsch/moveonpc/wiki/Calibration-data-CECH%E2%80%90ZCM2 */

    for (orientation=0; orientation<6; orientation++) {
        x = psmove_calibration_decode_16bit_signed(data, 0x04 + 6*orientation);
        y = psmove_calibration_decode_16bit_signed(data, 0x04 + 6*orientation + 2);
        z = psmove_calibration_decode_16bit_signed(data, 0x04 + 6*orientation + 4);
        printf("# Orientation #%d:      (%5d | %5d | %5d)\n", orientation, x, y, z);
    }

    printf("\n");

    x = psmove_calibration_decode_16bit_signed(data, 0x26);
    y = psmove_calibration_decode_16bit_signed(data, 0x26 + 2);
    z = psmove_calibration_decode_16bit_signed(data, 0x26 + 4);
    printf("# Gyro Bias?, 0 rpm (@0x26): (%5d | %5d | %5d)\n", x, y, z);

	printf("\n");

    for (orientation=0; orientation<6; orientation++) {
        x = psmove_calibration_decode_16bit_signed(data, 0x30 + 6*orientation);
        y = psmove_calibration_decode_16bit_signed(data, 0x30 + 6*orientation + 2);
        z = psmove_calibration_decode_16bit_signed(data, 0x30 + 6*orientation + 4);
        printf("# Gyro %c, 90 rpm:      (%5d | %5d | %5d)\n", "XYZXYZ"[orientation], x, y, z);
    }
}

void
psmove_calibration_get_usb_accel_values(PSMoveCalibration *calibration,
        int *x1, int *x2, int *y1, int *y2, int *z1, int *z2)
{
    assert(calibration != NULL);
    assert(psmove_calibration_supported(calibration));
    char *data = calibration->usb_calibration;

    int orientation;

    switch (psmove_get_model(calibration->move)) {
        case Model_ZCM1:
            /* Minimum (negative) value of each axis */
            orientation = 1;
            *x1 = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6*orientation);
            orientation = 5;
            *y1 = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6*orientation + 2);
            orientation = 2;
            *z1 = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6*orientation + 4);

            /* Maximum (positive) values of each axis */
            orientation = 3;
            *x2 = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6*orientation);
            orientation = 4;
            *y2 = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6*orientation + 2);
            orientation = 0;
            *z2 = psmove_calibration_decode_16bit_unsigned(data, 0x04 + 6*orientation + 4);
            break;
        case Model_ZCM2:
            /* Minimum (negative) value of each axis */
            orientation = 1;
            *x1 = psmove_calibration_decode_16bit_signed(data, 0x02 + 6*orientation);
            orientation = 3;
            *y1 = psmove_calibration_decode_16bit_signed(data, 0x02 + 6*orientation + 2);
            orientation = 5;
            *z1 = psmove_calibration_decode_16bit_signed(data, 0x02 + 6*orientation + 4);

            /* Maximum (positive) values of each axis */
            orientation = 0;
            *x2 = psmove_calibration_decode_16bit_signed(data, 0x02 + 6*orientation);
            orientation = 2;
            *y2 = psmove_calibration_decode_16bit_signed(data, 0x02 + 6*orientation + 2);
            orientation = 4;
            *z2 = psmove_calibration_decode_16bit_signed(data, 0x02 + 6*orientation + 4);
            break;
        default:
            psmove_CRITICAL("Unknown PS Move model");
            break;
    }
}


void
psmove_calibration_get_zcm1_usb_gyro_values(PSMoveCalibration *calibration,
        int *x, int *y, int *z)
{
    assert(calibration != NULL);
    assert(psmove_calibration_supported(calibration));
    char *data = calibration->usb_calibration;

    int bx, by, bz; /* Bias(?) values, need to subtract those */
    bx = psmove_calibration_decode_16bit_unsigned(data, 0x2a);
    by = psmove_calibration_decode_16bit_unsigned(data, 0x2a + 2);
    bz = psmove_calibration_decode_16bit_unsigned(data, 0x2a + 4);

    int orientation;

    orientation = 0;
    *x = psmove_calibration_decode_16bit_unsigned(data, 0x46 + 8*orientation) - bx;
    orientation = 1;
    *y = psmove_calibration_decode_16bit_unsigned(data, 0x46 + 8*orientation + 2) - by;
    orientation = 2;
    *z = psmove_calibration_decode_16bit_unsigned(data, 0x46 + 8*orientation + 4) - bz;
}

void
psmove_calibration_get_zcm2_usb_gyro_values(PSMoveCalibration *calibration,
        int *x1, int *x2, int *y1, int *y2, int *z1, int *z2,
		int *dx, int *dy, int *dz)
{
    assert(calibration != NULL);
    assert(psmove_calibration_supported(calibration));
    char *data = calibration->usb_calibration;

    int orientation;

    /* Minimum (negative) value of each axis */
    orientation = 3;
    *x1 = psmove_calibration_decode_16bit_signed(data, 0x30 + 6*orientation);
    orientation = 4;
    *y1 = psmove_calibration_decode_16bit_signed(data, 0x30 + 6*orientation + 2);
    orientation = 5;
    *z1 = psmove_calibration_decode_16bit_signed(data, 0x30 + 6*orientation + 4);

    /* Maximum (positive) values of each axis */
    orientation = 0;
    *x2 = psmove_calibration_decode_16bit_signed(data, 0x30 + 6*orientation);
    orientation = 1;
    *y2 = psmove_calibration_decode_16bit_signed(data, 0x30 + 6*orientation + 2);
    orientation = 2;
    *z2 = psmove_calibration_decode_16bit_signed(data, 0x30 + 6*orientation + 4);

    *dx = psmove_calibration_decode_16bit_signed(data, 0x26);
    *dy = psmove_calibration_decode_16bit_signed(data, 0x26 + 2);
    *dz = psmove_calibration_decode_16bit_signed(data, 0x26 + 4);
}

void
psmove_calibration_dump_usb(PSMoveCalibration *calibration)
{
    int j;

    assert(calibration != NULL);

    enum PSMove_Model_Type model = psmove_get_model(calibration->move);

    size_t calibration_blob_size = (model == Model_ZCM1)
        ? PSMOVE_ZCM1_CALIBRATION_BLOB_SIZE
        : PSMOVE_ZCM2_CALIBRATION_BLOB_SIZE;

    for (j=0; j<calibration_blob_size; j++) {
        printf("%02x", (unsigned char) calibration->usb_calibration[j]);
        if (j % 16 == 15) {
            printf("\n");
        } else if (j < sizeof(calibration->usb_calibration)-1) {
            printf(" ");
        }
    }
    printf("\n");

    switch (model) {
        case Model_ZCM1:
            psmove_zcm1_calibration_parse_usb(calibration);
            break;
        case Model_ZCM2:
            psmove_zcm2_calibration_parse_usb(calibration);
            break;
        default:
            psmove_CRITICAL("Unknown PS Move model");
            break;
    }

    printf("\n");
}



PSMoveCalibration *
psmove_calibration_new(PSMove *move)
{
    PSMove_Data_BTAddr addr;
    char *serial;
    int i;

    enum PSMove_Model_Type model = psmove_get_model(move);

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
         * Calculation of accelerometer mapping (as factor of gravity, 1g):
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
         *  rpmXX ... Sensor reading at XX RPM (from calibration blob)
         *  rpm60 ... Sensor reading at 60 RPM (1 rotation per second)
         *
         * Or combined:
         *
         *                XX * raw * 2 PI
         *  calibrated = -----------------
         *                  60 * rpmXX
         *
         * Now define:
         *
         *       2 * PI * XX
         *  f = -------------
         *        60 * rpmXX
         *
         * Then we get:
         *
         *  calibrated = f * raw
         *
         **/

        const float k_rpm_to_rad_per_sec = (2.0f * (float)M_PI) / 60.0f;

        switch (model) {
            case Model_ZCM1:
                {
                    /* ZCM1 gyro calibrations are at +80RPM */
                    int gx80, gy80, gz80;
                    psmove_calibration_get_zcm1_usb_gyro_values(calibration,
                            &gx80, &gy80, &gz80);

                    const float k_calibration_rpm= 80.f;
                    const float factor = k_calibration_rpm * k_rpm_to_rad_per_sec;

                    calibration->gx = factor / (float)gx80;
                    calibration->gy = factor / (float)gy80;
                    calibration->gz = factor / (float)gz80;

                    // Per frame drift taken into account using adjusted gain values
                    calibration->dx = 0;
                    calibration->dy = 0;
                    calibration->dz = 0;
                }
                break;
            case Model_ZCM2:
                /* ZCM2 gyro calibrations are at +90RPM and -90RPM

                   Note on the bias values (dx, dy, and dz).
                   Given the slightly asymmetrical min and max 90RPM readings you might think
                   that there is a bias in the gyros that you should compute by finding
                   the y-intercept value (the y-intercept of the gyro reading/angular speed line)
                   using the formula b= y_hi - m*x_hi, but this results in pretty bad
                   controller drift. We get much better results ignoring the y-intercept
                   and instead use the presumed "drift" values stored at 0x26
                   */
                {
                    int gx90_low, gx90_high, gy90_low, gy90_high, gz90_low, gz90_high;
                    psmove_calibration_get_zcm2_usb_gyro_values(calibration,
                            &gx90_low, &gx90_high, &gy90_low, &gy90_high, &gz90_low, &gz90_high,
                            &calibration->dx, &calibration->dy, &calibration->dz);

                    const float k_calibration_rpm= 90.f;
                    const float calibration_hi= k_calibration_rpm * k_rpm_to_rad_per_sec;
                    const float calibration_low= -k_calibration_rpm * k_rpm_to_rad_per_sec;
                    const float factor = calibration_hi - calibration_low;

                    // Compute the gain value (the slope of the gyro reading/angular speed line)
                    calibration->gx = factor / (float)(gx90_high - gx90_low);
                    calibration->gy = factor / (float)(gy90_high - gy90_low);
                    calibration->gz = factor / (float)(gz90_high - gz90_low);
                }
                break;
            default:
                psmove_CRITICAL("Unknown PS Move model");
                break;
        }
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

        calibration->dx = 0;
        calibration->dy = 0;
        calibration->dz = 0;
    }

    return calibration;
}

int
psmove_calibration_read_from_usb(PSMoveCalibration *calibration)
{
    assert(calibration != NULL);

    char *data;
    size_t size;

    switch (psmove_get_model(calibration->move)) {
        case Model_ZCM1:
            if (_psmove_get_zcm1_calibration_blob(calibration->move, &data, &size) == 1) {
                assert(size == PSMOVE_ZCM1_CALIBRATION_BLOB_SIZE);
                memcpy(calibration->usb_calibration, data, size);
                free(data);
                calibration->flags |= CalibrationFlag_HaveUSB;
                return 1;
            }
            break;
        case Model_ZCM2:
            if (_psmove_get_zcm2_calibration_blob(calibration->move, &data, &size) == 1) {
                assert(size == PSMOVE_ZCM2_CALIBRATION_BLOB_SIZE);
                memcpy(calibration->usb_calibration, data, size);
                free(data);
                calibration->flags |= CalibrationFlag_HaveUSB;
                return 1;
            }
            break;
        default:
            psmove_CRITICAL("Unknown PS Move model");
            break;
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
        *gx = (float)(raw_input[0] - calibration->dx) * calibration->gx;
    }

    if (gy) {
        *gy = (float)(raw_input[1] - calibration->dy) * calibration->gy;
    }

    if (gz) {
        *gz = (float)(raw_input[2] - calibration->dz) * calibration->gz;
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
            psmove_WARNING("No calibration file found (%s or %s)\n",
                    calibration->filename, calibration->system_filename);
            return 0;
        }
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

