#include <stdio.h>
#include <config.h>

void print_help(void)
{
    printf("Usage: %s [OPTION]... MENUITEM COMMAND ...\n"
           "   or: %s [OPTION]... {-d::|--delimiter ::} MENUITEM::COMMAND ...\n"
           "Create a simple X menu in a separate window and run user selected command.\n"
           "\n", PROGNAME, PROGNAME);
    printf(
        "  -l, -c, -r,                      set window text alignment (Xresource)\n"
        "        --align {left|center|right}\n"
        "  -b, --back PREVMENU              command to run on `back'\n"
        "      --background BGCOLOR         set background color (Xresource)\n"
        "  -C, --class CLASSNAME            CLASSNAME of Xresources to look for\n"
        "      --debug                      enable debug output on stderr\n"
        "  -d, --delimiter DELIM            pair up each MENUITEM and COMMAND\n");
    printf(
        "                                   in one argument separated by DELIM\n"
        "  -D, --display DISPLAYNAME        open menu on named X display\n"
        "      --foreground FGCOLOR         set foreground color (Xresource)\n"
        "  -F, --font FNAME                 Use X font FNAME (Xresource)\n"
        "  -h, --help                       display this help and exit\n"
        "  -i, --item POSITION              set MENUITEM selected on startup\n"
        "      --mouse / --no-mouse         enable/disable mouse support (Xresource)\n");
    printf(
        "  -p, --print                      print COMMAND on stdout, don't execute\n"
        "  -o, --scroll-offset ITEMS        border distance for scrolling (Xresource)\n"
        "  -S, --shell SHELL                SHELL in which to run commands\n"
        "  -s, --style {snazzy|dreary}      select menu style (Xresource)\n"
        "  -t, --title NAME                 set window title to NAME\n"
        "      --unfocus-exit /             enable/disable exit on loosing window\n"
        "        --no-unfocus-exit            focus (Xresource)\n");
    printf(
        "  -V, --version                    output version information and exit\n"
        "\n");

    exit(0);
}


void print_version(void)
{
    printf("%s 2.2.3\n"
           "Written by Arnold Robbins & David Hogan (1994-1995),\n"
           "John M. O'Donnell (1997), Jonathan Walther (2001),\n"
           "Zrajm C Akfohg (2003, 2007) and Daniel Campoverde (2016)\n"
           "\n"
           "Copyright 1994-1995, 1997, 2001, 2003, 2007 by respective author.\n"
           "Distributed under the GNU Public License.\n", PROGNAME);
    exit(0);
}
