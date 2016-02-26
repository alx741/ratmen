#include "args.h"
#include "misc.h"
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

struct opts_t opts = { a_undef, execute, s_undef, NULL, NULL, NULL, false, NULL,
    NULL, NULL, NULL, 0, false, 0, "/bin/sh", NULL, false };

static void set_args(char option);


int parse_args(int argc, char **argv)
{
    static struct option long_options[] =
    {
        {"align",           required_argument, 0, 'A'},
        {"back",            required_argument, 0, 'b'},
        {"background",      required_argument, 0, 'B'},
        {"class",           required_argument, 0, 'c'},
        {"debug",           no_argument,       0, 'X'},
        {"delimiter",       required_argument, 0, 'd'},
        {"display",         required_argument, 0, 'D'},
        {"foreground",      required_argument, 0, 'E'},
        {"font",            required_argument, 0, 'F'},
        {"help",            no_argument,       0, 'h'},
        {"item",            required_argument, 0, 'i'},
        {"mouse",           no_argument,       0, 'm'},
        {"no-mouse",        no_argument,       0, 'M'},
        {"print",           no_argument,       0, 'p'},
        {"scroll-offset",   required_argument, 0, 'o'},
        {"shell",           required_argument, 0, 'S'},
        {"style",           required_argument, 0, 's'},
        {"title",           required_argument, 0, 't'},
        {"unfocus-exit",    no_argument,       0, 'u'},
        {"no-unfocus-exit", no_argument,       0, 'U'},
        {"version",         no_argument,       0, 'V'},
        {0, 0, 0, 0}
    };

    while (1)
    {
        int current_longoption_index = 0;
        int option = getopt_long(argc, argv, "b:cC:d:D:F:hi:lo:prS:s:t:V",
                                 long_options, &current_longoption_index);

        /* Detect the end of the options. */
        if (option == -1)
        {
            break;
        }

        set_args(option);
    }

    return (optind);
}


static void set_args(char option)
{
    switch (option)
    {
        case 'A': /* --align {left|center|right} */
            if (strcasecmp(optarg, "left") == 0)
            {
                opts.align = left;
            }
            else if (strcasecmp(optarg, "center") == 0)
            {
                opts.align = center;
            }
            else if (strcasecmp(optarg, "right") == 0)
            {
                opts.align = right;
            }
            else
            {
                char buffer[200] = "";
                sprintf(buffer, "unknown align argument `%s' "
                        "(should be `left', `center' or `right')", optarg);
                die(buffer);
            }
            break;

        case 'b': /* -b, --back PREVMENU */
            opts.prevmenu = optarg;
            break;

        case 'B': /* --background BGCOLOR */
            opts.bgcname = optarg;
            break;

        case 'c': /* -c, --align=center */
            opts.align = center;
            break;

        case 'C': /* -C, --class CLASSNAME */
            opts.classname = optarg;
            break;

        case 'X': /* --debug */
            opts.debug = true;
            break;

        case 'd': /* -d, --delimiter DELIM */
            opts.delimiter = optarg;
            if (strcmp(opts.delimiter, "") == 0)
            {
                die("delimiter must be at least one character long");
            }
            break;

        case 'D': /* -D, --display DISPLAYNAME */
            opts.displayname = optarg;
            break;

        case 'F': /* -F, --font FNAME */
            opts.fontname = optarg;
            break;

        case 'E': /* --foreground FGCOLOR */
            opts.fgcname = optarg;
            break;

        case 'h': /* -h, --help */
            print_help();
            break;

        case 'i': /* -i, --item POSITION */
            opts.cur_item = atoi(optarg) - 1;
            break;

        case 'm': /*     --mouse */
            opts.mouse_on = true;
            break;

        case 'M': /*     --no-mouse */
            opts.mouse_on = false;
            break;

        case 'l': /* -l, --align=left */
            opts.align = left;
            break;

        case 'p': /* -p, --print */
            opts.output = print;
            break;

        case 'o': /* -o, --scroll-offset ITEMS */
            opts.scroll_offset = atoi(optarg);
            break;

        case 'r': /* -r, --align=right */
            opts.align = right;
            break;

        case 'S': /* -S, --shell SHELL */
            opts.shell = optarg;
            break;

        case 's': /* -s, --style {snazzy|dreary} */
            if (strcasecmp(optarg, "dreary") == 0)
            {
                opts.style = dreary;
            }
            else if (strcasecmp(optarg, "snazzy") == 0)
            {
                opts.style = snazzy;
            }
            else
            {
                char buffer[200] = "";
                sprintf(buffer, "unknown style argument `%s' "
                        "(should be `snazzy' or `dreary')", optarg);
                die(buffer);
            }
            break;

        case 't': /* -t, --title NAME */
            opts.titlename = optarg;
            break;

        case 'u': /* -u, --unfocus-exit */
            opts.unfocus_exit = true;
            break;

        case 'U': /* -U, --no-unfocus-exit */
            opts.unfocus_exit = false;
            break;

        case 'V': /* -V, --version */
            print_version();
            break;

        case '?':
            /* getopt_long already printed an error message. */
            break;

        default:
            abort();
    }
}
