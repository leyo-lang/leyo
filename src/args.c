#include <string.h>
#include <stdbool.h>

#include "../include/args.h" // argparser

ArgParser *ap = {0}; 

static bool starts_with_dash(const char *s) {
    return s[0] == '-';
}

void apInit(int argc, char *argv[]) {
    int i = 1;

    if (argc > 1 && !starts_with_dash(argv[1])) {
        ap->command = argv[1];
        ap->has_command = true;
        i = 2;
    }

    for (; i < argc; i++) {

        char *arg = argv[i];

        /* --output=file */
        if (strncmp(arg, "--", 2) == 0) {

            char *eq = strchr(arg, '=');

            if (eq) {

                *eq = '\0';

                ap->options[ap->option_count++] =
                    (AP_Option){
                        .name = arg,
                        .value = eq + 1
                    };

                continue;
            }

            /* --output file */
            if (i + 1 < argc && !starts_with_dash(argv[i + 1])) {

                ap->options[ap->option_count++] =
                    (AP_Option){
                        .name = arg,
                        .value = argv[++i]
                    };

                continue;
            }

            /* plain flag */
            ap->flags[ap->flag_count++] = arg;
            continue;
        }

        /* short flags */
        if (arg[0] == '-' && arg[1] != '\0') {

            /* -o file */
            if (strlen(arg) == 2 &&
                i + 1 < argc &&
                !starts_with_dash(argv[i + 1]))
            {
                ap->options[ap->option_count++] =
                    (AP_Option){
                        .name = arg,
                        .value = argv[++i]
                    };

                continue;
            }

            /* -abc */
            for (int j = 1; arg[j]; j++) {

                static char short_flag[AP_MAX_FLAGS][3];
                static int short_index = 0;

                short_flag[short_index][0] = '-';
                short_flag[short_index][1] = arg[j];
                short_flag[short_index][2] = '\0';

                ap->flags[ap->flag_count++] =
                    short_flag[short_index];

                short_index++;
            }

            continue;
        }

        ap->positionals[ap->positional_count++] = arg;
    }
}

bool apIsCommand(const char *cmd) {
    return ap->has_command &&
           strcmp(ap->command, cmd) == 0;
}

bool apHasFlag(const char *flag) {
    for (int i = 0; i < ap->flag_count; i++) {
        if (strcmp(ap->flags[i], flag) == 0)
            return true;
    }

    return false;
}

char *apGetOption(const char *name) {
    for (int i = 0; i < ap->option_count; i++) {
        if (strcmp(ap->options[i].name, name) == 0)
            return ap->options[i].value;
    }

    return NULL;
}

char *apGetPositional(int index) {
    if (index < 0 || index >= ap->positional_count) {
        return NULL;
    }

    return ap->positionals[index];
}