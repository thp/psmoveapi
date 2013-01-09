
using System;
using io.thp.psmove;

public class SensorReading {
    public static int Main(string [] args) {
        PSMove move = new PSMove();

        while (true) {
            if (move.poll() != 0) {
                int trigger = move.get_trigger();

                float ax, ay, az;
                move.get_accelerometer_frame(Frame.Frame_SecondHalf,
                        out ax, out ay, out az);

                Console.WriteLine(string.Format("Trigger: {0:0}, " +
                            "Accelerometer: {1:0.00} {2:0.00} {3:0.00}",
                            trigger, ax, ay, az));
            }
        }

        return 0;
    }
};

