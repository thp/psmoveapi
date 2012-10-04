
using io.thp.psmove;

public class LEDs {
    public static int Main(string [] args) {
        PSMove move = new PSMove();
        move.set_leds(0, 255, 0);
        move.update_leds();
        return 0;
    }
};

