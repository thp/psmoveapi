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
#include "psmovepair.c"
#undef main

#define main psmoveregister_main
#include "psmoveregister.c"
#undef main

#define main moved_main
#include "moved.c"
#undef main

#define main magnetometer_calibration_main
#include "magnetometer_calibration.c"
#undef main

static int
usage(const char *progname, std::vector<SubCommand> &subcommands)
{
    printf("Usage: %s <cmd>\n\nWhere <cmd> is one of:\n", progname);

    for (auto &cmd: subcommands) {
        printf("    %-10s ..... %s\n", cmd.cmd, cmd.help);
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
