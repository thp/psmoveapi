#include <vector>

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

#define main psmoveregister_main
extern "C" {
#include "psmoveregister.c"
}
#undef main

#define main moved_main
#include "moved.cpp"
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
extern "C" {
#include "battery_check.c"
}
#undef main

#define main dump_calibration_main
extern "C" {
#include "dump_calibration.c"
}
#undef main

static int
usage(const char *progname, std::vector<SubCommand> &subcommands)
{
    printf("Usage: %s <cmd>\n\nWhere <cmd> is one of:\n\n", progname);

    for (auto &cmd: subcommands) {
        printf("    %-20s ... %s\n", cmd.cmd, cmd.help);
    }
    printf("\n");

    return 0;
}

int
main(int argc, char *argv[])
{
    std::vector<SubCommand> subcommands;

    subcommands.emplace_back("pair", "Pair connected USB controllers to a host", psmovepair_main);
    subcommands.emplace_back("daemon", "Server locally connected controllers via UDP", moved_main);
    subcommands.emplace_back("register", "Register already paired controllers", psmoveregister_main);
    subcommands.emplace_back("calibrate", "Calibrate magnetometers of contollers", magnetometer_calibration_main);
    subcommands.emplace_back("dfu-mode", "Switch to DFU mode (potentially dangerous)", dfu_mode_main);
    subcommands.emplace_back("firmware-info", "Show information about the controller firmware", get_firmware_info_main);
    subcommands.emplace_back("auth-response", "Challenge-response authentication dev tool", auth_response_main);
    subcommands.emplace_back("battery-check", "Visualize the battery charge on connected controllers", battery_check_main);
    subcommands.emplace_back("dump-calibration", "Show the stored calibration information", dump_calibration_main);

    if (argc == 1 || strcmp(argv[1], "help") == 0) {
        return usage(argv[0], subcommands);
    }

    for (auto &cmd: subcommands) {
        if (strcmp(cmd.cmd, argv[1]) == 0) {
            return cmd.func(argc-1, argv+1);
        }
    }

    printf("Command not found: '%s', use 'help' to see a list of commands.\n", argv[1]);
    return 1;
}
