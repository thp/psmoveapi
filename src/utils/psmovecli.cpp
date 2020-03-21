#include <vector>

#include "psmoveapi.h"

typedef int (*subcommand_func_t)(int argc, char *argv[]);

struct SubCommand {
    SubCommand(const char *cmd=nullptr, const char *help=nullptr, subcommand_func_t func=nullptr)
        : cmd(cmd)
        , help(help)
        , func(func)
    {
    }

    const char *cmd;
    const char *help;
    subcommand_func_t func;
};

#define main psmovepair_main
extern "C" {
#include "psmovepair.c"
}
#undef main

#define main psmoveremotepair_main
#include "psmoveremotepair.cpp"
#undef main

#define main psmoveregister_main
extern "C" {
#include "psmoveregister.c"
}
#undef main

#define main moved_main
#include "daemon/moved.cpp"
#undef main

#define main magnetometer_calibration_main
extern "C" {
#include "magnetometer_calibration.c"
}
#undef main

#define main dfu_mode_main
extern "C" {
#include "psmove_dfu_mode.c"
}
#undef main

#define main get_firmware_info_main
extern "C" {
#include "psmove_get_firmware_info.c"
}
#undef main

#define main auth_response_main
extern "C" {
#include "psmove_auth_response.c"
}
#undef main

#define main battery_check_main
#include "battery_check.cpp"
#undef main

#define main dump_calibration_main
extern "C" {
#include "dump_calibration.c"
}
#undef main

#define main test_responsiveness_main
extern "C" {
#include "test_responsiveness.c"
}
#undef main

#define main test_extension_main
extern "C" {
#include "test_extension.c"
}
#undef main

#define main test_led_pwm_frequency_main
extern "C" {
#include "test_led_pwm_frequency.c"
}
#undef main

#if defined(PSMOVE_USE_SIXPAIR)
#define main sixpair_main
extern "C" {
#include "sixpair.c"
}
#undef main
#endif /* PSMOVE_USE_SIXPAIR */

static int
usage(const char *progname, std::vector<SubCommand> &subcommands)
{
    printf("Usage: %s <cmd>\n\nWhere <cmd> is one of:\n\n", progname);

    for (auto &cmd: subcommands) {
        if (cmd.cmd) {
            printf("    %-20s ... %s\n", cmd.cmd, cmd.help);
        } else {
            printf("\n  %s:\n\n", cmd.help);
        }
    }
    printf("\n");

    return 0;
}

namespace {

struct AnsiMapping {
    AnsiMapping(int index, float r, float g, float b) : index{index}, color{r, g, b} {}

    int index;
    RGB color;
};

std::vector<AnsiMapping>
ansi_color_table = {
    AnsiMapping(1, 1.f, 0.f, 0.f), // red
    AnsiMapping(2, 0.f, 1.f, 0.f), // green
    AnsiMapping(3, 1.f, 1.f, 0.f), // yellow
    AnsiMapping(4, 0.f, 0.f, 1.f), // blue
    AnsiMapping(5, 1.f, 0.f, 1.f), // magenta
    AnsiMapping(6, 0.f, 1.f, 1.f), // cyan
    AnsiMapping(7, 1.f, 1.f, 1.f), // white
};

class ListHandler : public psmoveapi::Handler {
public:
    ListHandler()
        : waiting(0)
        , done(0)
    {
        char *host_address = psmove_port_get_host_bluetooth_address();
        printf("Host address: %s\n", host_address ? host_address : "unknown");
    }

    virtual void connect(Controller *controller) {
        waiting++;
    }

    virtual void update(Controller *controller) {
        if (controller->user_data == this) {
            // Already handled this controller
            return;
        }

        // Can now report this controller, as battery data is available
        const char *connection_type_str = "unknown";
        if (controller->usb && controller->bluetooth) {
            connection_type_str = "USB+Bluetooth";
        } else if (controller->usb) {
            connection_type_str = "USB";
        } else if (controller->bluetooth) {
            connection_type_str = "Bluetooth";
        }

        const char *battery_str = "unknown";
        switch (controller->battery) {
            case Batt_20Percent: battery_str = "20%"; break;
            case Batt_40Percent: battery_str = "40%"; break;
            case Batt_60Percent: battery_str = "60%"; break;
            case Batt_80Percent: battery_str = "80%"; break;
            case Batt_MAX: battery_str = "100%"; break;
            case Batt_CHARGING: battery_str = "charging"; break;
            case Batt_CHARGING_DONE: battery_str = "charged"; break;
        }

        auto &ansi = ansi_color_table[controller->index % ansi_color_table.size()];

        printf("Controller %d: \033[9%dm%s\033[0m (%s, battery: %s)\n",
                controller->index, ansi.index, controller->serial,
                connection_type_str, battery_str);
        controller->color = ansi.color;

        // Mark this controller as reported
        controller->user_data = this;
        done++;
    }

    virtual void disconnect(Controller *controller) {
        if (controller->user_data == this) {
            done--;
        }

        waiting--;
    }

    int waiting;
    int done;
};

};

int
list_main(int argc, char *argv[])
{
    ListHandler handler;
    psmoveapi::PSMoveAPI api(&handler);

    do {
        api.update();
    } while (handler.done < handler.waiting);

    return 0;
}

int
main(int argc, char *argv[])
{
    std::vector<SubCommand> subcommands;

    subcommands.emplace_back("pair", "Pair connected USB controllers to a host", psmovepair_main);
    subcommands.emplace_back("remote-pair", "Pair connected USB controller to a moved host", psmoveremotepair_main);
    subcommands.emplace_back("daemon", "Serve locally-connected controllers via UDP", moved_main);
    subcommands.emplace_back("register", "Register already-paired controllers", psmoveregister_main);
    subcommands.emplace_back("calibrate", "Calibrate magnetometers of controllers", magnetometer_calibration_main);
    subcommands.emplace_back("battery", "Visualize the battery charge on connected controllers", battery_check_main);
    subcommands.emplace_back("extensions", "Show sharp shooter and racing wheel extension data", test_extension_main);
    subcommands.emplace_back("list", "List connected controllers", list_main);

#if defined(PSMOVE_USE_SIXPAIR)
    subcommands.emplace_back("pair-nav", "Pair navigation controller via USB (using sixpair)", sixpair_main);
#endif /* PSMOVE_USE_SIXPAIR */

    subcommands.emplace_back(nullptr, "Debugging Tools (for developers)", nullptr);

    subcommands.emplace_back("dump-calibration", "Show the stored calibration information", dump_calibration_main);
    subcommands.emplace_back("auth-response", "Challenge-response authentication dev tool", auth_response_main);
    subcommands.emplace_back("dfu-mode", "Switch to DFU mode (potentially dangerous)", dfu_mode_main);
    subcommands.emplace_back("firmware-info", "Show information about the controller firmware", get_firmware_info_main);

    subcommands.emplace_back(nullptr, "Test Tools", nullptr);

    subcommands.emplace_back("responsiveness", "Test how quickly the controllers react", test_responsiveness_main);
    subcommands.emplace_back("led-pwm-frequency", "Test LED PWM frequency modulation", test_led_pwm_frequency_main);

    if (argc == 1 || strcmp(argv[1], "help") == 0) {
        return usage(argv[0], subcommands);
    }

    for (auto &cmd: subcommands) {
        if (cmd.cmd != nullptr && strcmp(cmd.cmd, argv[1]) == 0) {
            return cmd.func(argc-1, argv+1);
        }
    }

    printf("Command not found: '%s', use 'help' to see a list of commands.\n", argv[1]);
    return 1;
}
