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

import io.thp.psmove.PSMove;
import io.thp.psmove.Frame;
import io.thp.psmove.psmoveapi;

public class SensorReading {
    public static void main(String [] args) {
        int connected = psmoveapi.count_connected();
        System.out.println("Connected controllers: " + connected);

        if (connected == 0) {
            System.out.println("Please connect a controller first.");
            System.exit(1);
        }

        PSMove move = new PSMove();

        System.out.println("PS Move Serial: " + move.get_serial());

        float [] ax = { 0.f }, ay = { 0.f }, az = { 0.f };
        float [] gx = { 0.f }, gy = { 0.f }, gz = { 0.f };
        float [] mx = { 0.f }, my = { 0.f }, mz = { 0.f };

        while (true) {
            while (move.poll() != 0) {
                move.get_accelerometer_frame(Frame.Frame_SecondHalf,
                        ax, ay, az);
                move.get_gyroscope_frame(Frame.Frame_SecondHalf,
                        gx, gy, gz);
                move.get_magnetometer_vector(mx, my, mz);
                System.out.format("ax: %.2f ay: %.2f az: %.2f ",
                        ax[0], ay[0], az[0]);
                System.out.format("gx: %.2f gy: %.2f gz: %.2f ",
                        gx[0], gy[0], gz[0]);
                System.out.format("mx: %.2f my: %.2f mz: %.2f\n",
                        mx[0], my[0], mz[0]);
            }
        }
    }
}
