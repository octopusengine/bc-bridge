#include "application.h"
#include <argp.h>

static error_t parse_opt(int key, char *arg, struct argp_state *state);

int main(int argc, char **argv)
{
    bool quit;

    struct argp argp;

    char *doc;

    doc = "Software interface between Clown.Hub and Bridge Module";

    static struct argp_option options[] =
    {
        { "furious",  'f', 0, 0,  "Do not wait for the initial start string" },
        { "log",  'l', "level",  0, "dump|debug|info|warning|error|fatal" },
//        { "exit",  'e', 0,  0, "exit on remove bridge, no wait on reconnect device" }, //TODO jo nebo ne
        { "version",  'v', 0,  0, "Show version" },
        { 0 }
    };

    memset(&argp, 0, sizeof(argp));

    argp.options = options;
    argp.parser = parse_opt;
    argp.args_doc = 0;
    argp.doc = doc;

    application_parameters_t application_parameters =
    {
        .furious_mode = false,
//        .exit = false,
        .log_level = BC_LOG_LEVEL_WARNING
    };

    argp_parse(&argp, argc, argv, 0, 0, &application_parameters);

    application_init(&application_parameters);

    quit = false;

    while (!quit)
    {
        application_loop(&quit);
//        if (application_parameters.exit)
//        {
//            break;
//        }
        sleep(5);
    }

    exit(EXIT_SUCCESS);
}

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    application_parameters_t *application_parameters = state->input;

    switch (key)
    {
        case 'f':
        {
            application_parameters->furious_mode = true;
            break;
        }
        case 'v':
        {
            printf("version bc-bridge " FIRMWARE_RELEASE " " FIRMWARE_DATETIME " \n");
            printf("support <support@bigclown.com> \n");
            exit(EXIT_SUCCESS);
        }
        case 'e':
        {
            application_parameters->exit = true;
            break;
        }
        case 'l':
        {
            if (strcmp(arg, "dump") == 0)
            {
                application_parameters->log_level = BC_LOG_LEVEL_DUMP;
            }
            else if (strcmp(arg, "debug") == 0)
            {
                application_parameters->log_level = BC_LOG_LEVEL_DEBUG;
            }
            else if (strcmp(arg, "info") == 0)
            {
                application_parameters->log_level = BC_LOG_LEVEL_INFO;
            }
            else if (strcmp(arg, "warning") == 0)
            {
                application_parameters->log_level = BC_LOG_LEVEL_WARNING;
            }
            else if (strcmp(arg, "error") == 0)
            {
                application_parameters->log_level = BC_LOG_LEVEL_ERROR;
            }
            else if (strcmp(arg, "fatal") == 0)
            {
                application_parameters->log_level = BC_LOG_LEVEL_FATAL;
            }
            else
            {
                return ARGP_ERR_UNKNOWN;
            }

            break;
        }
        default:
        {
            return ARGP_ERR_UNKNOWN;
        }
    }

    return 0;
}
