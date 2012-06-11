
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


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "psmove.h"
#include "psmove_calibration.h"

#define POSITIONS 6
#define READINGS_PER_POSITION 200

/**
 * Maximum value of the accelerometer deviation vector,
 * if this limit is reached, let the user retry the position
 **/
#define MAX_DEVIATION_VECTOR 100

struct _SensorReading {
    double ax, ay, az;
    double gx, gy, gz;
    double mx, my, mz;
};

typedef struct _SensorReading SensorReading;

struct _Position {
    char *name;
    int readings;
    SensorReading data[READINGS_PER_POSITION];
    SensorReading min;
    SensorReading max;
    SensorReading avg;
    SensorReading dev;
};

typedef struct _Position Position;

void
wait_for_button(PSMove *move, enum PSMove_Button button)
{
    /* Wait until the button is depressed */
    while (1) {
        if (psmove_poll(move) && (psmove_get_buttons(move) & button) == 0)
            break;

        usleep(10000);
    }

    /* Wait until the button is pressed again */
    while (1) {
        if (psmove_poll(move) && (psmove_get_buttons(move) & button) != 0)
            break;

        usleep(10000);
    }
}

void
get_min_reading(SensorReading *min, const SensorReading *cur)
{
    if (min->ax > cur->ax) min->ax = cur->ax;
    if (min->ay > cur->ay) min->ay = cur->ay;
    if (min->az > cur->az) min->az = cur->az;

    /* XXX: Gyroscope */

    if (min->mx > cur->mx) min->mx = cur->mx;
    if (min->my > cur->my) min->my = cur->my;
    if (min->mz > cur->mz) min->mz = cur->mz;
}

void
get_max_reading(SensorReading *max, const SensorReading *cur)
{
    if (max->ax < cur->ax) max->ax = cur->ax;
    if (max->ay < cur->ay) max->ay = cur->ay;
    if (max->az < cur->az) max->az = cur->az;

    /* XXX: Gyroscope */

    if (max->mx < cur->mx) max->mx = cur->mx;
    if (max->my < cur->my) max->my = cur->my;
    if (max->mz < cur->mz) max->mz = cur->mz;
}

void
add_reading(SensorReading *avg, const SensorReading cur)
{
    avg->ax += cur.ax;
    avg->ay += cur.ay;
    avg->az += cur.az;

    avg->gx += cur.gx;
    avg->gy += cur.gy;
    avg->gz += cur.gz;

    avg->mx += cur.mx;
    avg->my += cur.my;
    avg->mz += cur.mz;
}

void
divide_reading(SensorReading *avg, int divisor)
{
    avg->ax /= divisor;
    avg->ay /= divisor;
    avg->az /= divisor;

    avg->gx /= divisor;
    avg->gy /= divisor;
    avg->gz /= divisor;

    avg->mx /= divisor;
    avg->my /= divisor;
    avg->mz /= divisor;
}

#define POW2(x) ((x)*(x))

SensorReading
square_diff(const SensorReading *cur, const SensorReading *avg)
{
    SensorReading tmp;

    tmp.ax = POW2(cur->ax - avg->ax);
    tmp.ay = POW2(cur->ay - avg->ay);
    tmp.az = POW2(cur->az - avg->az);

    tmp.gx = POW2(cur->gx - avg->gx);
    tmp.gy = POW2(cur->gy - avg->gy);
    tmp.gz = POW2(cur->gz - avg->gz);

    tmp.mx = POW2(cur->mx - avg->mx);
    tmp.my = POW2(cur->my - avg->my);
    tmp.mz = POW2(cur->mz - avg->mz);

    return tmp;
}

void
sqrt_reading(SensorReading *dev)
{
    dev->ax = sqrt(dev->ax);
    dev->ay = sqrt(dev->ay);
    dev->az = sqrt(dev->az);

    dev->gx = sqrt(dev->gx);
    dev->gy = sqrt(dev->gy);
    dev->gz = sqrt(dev->gz);

    dev->mx = sqrt(dev->mx);
    dev->my = sqrt(dev->my);
    dev->mz = sqrt(dev->mz);
}


double
calculate_calibration(Position *position)
{
    int reading;

    memset(&position->avg, 0, sizeof(SensorReading));
    memset(&position->dev, 0, sizeof(SensorReading));

    position->min = position->max = position->data[0];
    for (reading=0; reading<READINGS_PER_POSITION; reading++) {
        SensorReading cur = position->data[reading];

        get_min_reading(&position->min, &cur);
        get_max_reading(&position->max, &cur);
        add_reading(&position->avg, cur);
    }
    divide_reading(&position->avg, READINGS_PER_POSITION);

    for (reading=0; reading<READINGS_PER_POSITION; reading++) {
        SensorReading cur = position->data[reading];
        add_reading(&position->dev, square_diff(&cur, &position->avg));
    }
    divide_reading(&position->dev, READINGS_PER_POSITION);
    sqrt_reading(&position->dev);

    printf("%s:\n", position->name);

    //printf("a (min: %5.0f | %5.0f | %5.0f)\n", position->min.ax, position->min.ay, position->min.az);
    //printf("a (max: %5.0f | %5.0f | %5.0f)\n", position->max.ax, position->max.ay, position->max.az);
    printf("a (avg: %5.0f | %5.0f | %5.0f)\n", position->avg.ax, position->avg.ay, position->avg.az);
    printf("a (dev: %5.0f | %5.0f | %5.0f)\n", position->dev.ax, position->dev.ay, position->dev.az);

    //printf("m (min: %5.0f | %5.0f | %5.0f)\n", position->min.mx, position->min.my, position->min.mz);
    //printf("m (max: %5.0f | %5.0f | %5.0f)\n", position->max.mx, position->max.my, position->max.mz);
    printf("m (avg: %5.0f | %5.0f | %5.0f)\n", position->avg.mx, position->avg.my, position->avg.mz);
    printf("m (dev: %5.0f | %5.0f | %5.0f)\n", position->dev.mx, position->dev.my, position->dev.mz);

    printf("\n");

    return sqrt(POW2(position->dev.ax) +
                POW2(position->dev.ay) +
                POW2(position->dev.az));
}

int
main(int argc, char* argv[])
{
    PSMove *move;
    const char *serial;
    int i, j;
    int x, y, z;
    double deviation_vector;
    FILE *fp;
    PSMoveCalibration *calibration;
    float custom[6*9];

    Position positions[POSITIONS] = {
        { .name = "bulb up" },
        { .name = "bulb down" },
        { .name = "buttons up" },
        { .name = "buttons down" },
        { .name = "buttons left" },
        { .name = "buttons right" },
    };

    for (i=0; i<POSITIONS; i++) {
        positions[i].readings = 0;
        memset(positions[i].data, 0, sizeof(positions[i].data));
    }

    if (psmove_count_connected() < 1) {
        fprintf(stderr, "No controllers connected.\n");
        return EXIT_FAILURE;
    }

    move = psmove_connect();

    if (move == NULL) {
        fprintf(stderr, "Error connecting to the first controller.\n");
        return EXIT_FAILURE;
    }

    if (psmove_connection_type(move) != Conn_Bluetooth) {
        fprintf(stderr, "Please connect the controller via Bluetooth.\n");
        return EXIT_FAILURE;
    }

    serial = psmove_get_serial(move);
    printf("Serial number: %s\n", serial);

    for (i=0; i<POSITIONS; i++) {
        do {
            printf("Put the controller in the position '%s' and press the Move button\n",
                    positions[i].name);
            wait_for_button(move, Btn_MOVE);
            positions[i].readings = 0;

            while (positions[i].readings < READINGS_PER_POSITION) {
                if (psmove_poll(move)) {
                    printf("\rTaking reading %d...", positions[i].readings + 1);
                    fflush(stdout);

                    SensorReading *reading = &(positions[i].data[positions[i].readings]);

                    psmove_get_accelerometer(move, &x, &y, &z);
                    reading->ax = (double)x;
                    reading->ay = (double)y;
                    reading->az = (double)z;

                    psmove_get_gyroscope(move, &x, &y, &z);
                    reading->gx = (double)x;
                    reading->gy = (double)y;
                    reading->gz = (double)z;

                    psmove_get_magnetometer(move, &x, &y, &z);
                    reading->mx = (double)x;
                    reading->my = (double)y;
                    reading->mz = (double)z;

                    positions[i].readings++;
                }
            }
            printf("\rAll readings done for %s.\n", positions[i].name);
            deviation_vector = calculate_calibration(&positions[i]);

            if (deviation_vector > MAX_DEVIATION_VECTOR) {
                printf("\n\n  DEVIATION TOO HIGH - PLEASE RETRY\n\n");
            }
        } while (deviation_vector > MAX_DEVIATION_VECTOR);
    }

    calibration = psmove_calibration_new(move);
    psmove_calibration_load(calibration);

    for (i=0; i<POSITIONS; i++) {
        custom[i*9 + 0] = positions[i].avg.ax;
        custom[i*9 + 1] = positions[i].avg.ay;
        custom[i*9 + 2] = positions[i].avg.az;

        custom[i*9 + 3] = 0;//positions[i].avg.gx;
        custom[i*9 + 4] = 0;//positions[i].avg.gy;
        custom[i*9 + 5] = 0;//positions[i].avg.gz;

        custom[i*9 + 6] = positions[i].avg.mx;
        custom[i*9 + 7] = positions[i].avg.my;
        custom[i*9 + 8] = positions[i].avg.mz;
    }

    psmove_calibration_set_custom(calibration, custom, POSITIONS, 9);

    psmove_calibration_save(calibration);
    psmove_calibration_destroy(calibration);

    /*
    snprintf(tmp, sizeof(tmp), "calibration.%s.txt", serial);
    for (i=0; i<strlen(tmp); i++) {
        if (tmp[i] == ':') {
            tmp[i] = '-';
        }
    }

    fp = fopen(tmp, "w");
    assert(fp != NULL);
    for (i=0; i<POSITIONS; i++) {
        fprintf(fp, "%f %f %f\n", positions[i].avg.ax, positions[i].avg.ay, positions[i].avg.az);
    }
    fclose(fp);
    */

    fp = fopen("orientation.csv", "w");
    assert(fp != NULL);
    fprintf(fp, "ax,ay,az\n");
    for (i=0; i<POSITIONS; i++) {
        for (j=0; j<READINGS_PER_POSITION; j++) {
            fprintf(fp, "%f,%f,%f\n", positions[i].data[j].ax, positions[i].data[j].ay, positions[i].data[j].az);
        }
    }
    fclose(fp);

    psmove_disconnect(move);

    return EXIT_SUCCESS;
}

