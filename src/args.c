#include "args.h"
#include "misc.h"
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

#define Undef -1

enum align_t align   = Undef;
enum output_t output = execute;
enum style_t style   = Undef;
char *prevmenu       = NULL;
char *bgcname        = NULL;
char *classname      = NULL;
bool   debug         = false;
char *delimiter      = NULL;
char *displayname;
char *fgcname        = NULL;
char *fontname       = NULL;
int   cur_item       = 0;
bool   mouse_on      = Undef;
int   scroll_offset  = Undef;
char *shell          = "/bin/sh";
char *titlename      = NULL;
bool   unfocus_exit  = Undef;

int args(int argc, char **argv)
{
    int c;
    while (1)
    {
        static struct option long_options[] =
        {
            {"align",           required_argument, 0, 'A'}, /* no shortopt */
            {"back",            required_argument, 0, 'b'},
            {"background",      required_argument, 0, 'B'}, /* no shortopt */
            {"class",           required_argument, 0, 'c'},
            {"debug",           no_argument,       0, 'X'}, /* no shortopt */
            {"delimiter",       required_argument, 0, 'd'},
            {"display",         required_argument, 0, 'D'},
            {"foreground",      required_argument, 0, 'E'}, /* no shortopt */
            {"font",            required_argument, 0, 'F'},
            {"help",            no_argument,       0, 'h'},
            {"item",            required_argument, 0, 'i'},
            {"mouse",           no_argument,       0, 'm'}, /* no shortopt */
            {"no-mouse",        no_argument,       0, 'M'}, /* no shortopt */
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

        /* getopt_long stores the option index here. */
        int option_index = 0;
        c = getopt_long(argc, argv, "b:cC:d:D:F:hi:lo:prS:s:t:V",
                        long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
        {
            break;
        }

        switch (c)
        {
            /*case 0:
                printf("gaga\n");*/
            /* If this option set a flag, do nothing else now. */
            /*if (long_options[option_index].flag != 0)
                break;
            printf ("option %s", long_options[option_index].name);
            if (optarg)
                printf (" with arg %s", optarg);
            printf ("\n");
            break;*/

            case 'A': /* --align {left|center|right} */
                if (strcasecmp(optarg, "left") == 0)
                {
                    align = left;
                }
                else if (strcasecmp(optarg, "center") == 0)
                {
                    align = center;
                }
                else if (strcasecmp(optarg, "right") == 0)
                {
                    align = right;
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
                prevmenu = optarg;
                break;

            case 'B': /* --background BGCOLOR */
                bgcname = optarg;
                break;

            case 'c': /* -c, --align=center */
                align = center;
                break;

            case 'C': /* -C, --class CLASSNAME */
                classname = optarg;
                break;

            case 'X': /* --debug */
                debug = true;
                break;

            case 'd': /* -d, --delimiter DELIM */
                delimiter = optarg;
                if (strcmp(delimiter, "") == 0)
                {
                    die("delimiter must be at least one character long");
                }
                break;

            case 'D': /* -D, --display DISPLAYNAME */
                displayname = optarg;
                break;

            case 'F': /* -F, --font FNAME */
                fontname = optarg;
                break;

            case 'E': /* --foreground FGCOLOR */
                fgcname = optarg;
                break;

            case 'h': /* -h, --help */
                print_help();
                break;

            case 'i': /* -i, --item POSITION */
                cur_item = atoi(optarg) - 1;
                break;

            case 'm': /*     --mouse */
                mouse_on = true;
                break;

            case 'M': /*     --no-mouse */
                mouse_on = false;
                break;

            case 'l': /* -l, --align=left */
                align = left;
                break;

            case 'p': /* -p, --print */
                output = print;
                break;

            case 'o': /* -o, --scroll-offset ITEMS */
                scroll_offset = atoi(optarg);
                break;

            case 'r': /* -r, --align=right */
                align = right;
                break;

            case 'S': /* -S, --shell SHELL */
                shell = optarg;
                break;

            case 's': /* -s, --style {snazzy|dreary} */
                if (strcasecmp(optarg, "dreary") == 0)
                {
                    style = dreary;
                }
                else if (strcasecmp(optarg, "snazzy") == 0)
                {
                    style = snazzy;
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
                titlename = optarg;
                break;

            case 'u': /* -u, --unfocus-exit */
                unfocus_exit = true;
                break;

            case 'U': /* -U, --no-unfocus-exit */
                unfocus_exit = false;
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
    return (optind);
}
