#pragma once
#include <stdbool.h>

struct opts_t
{
    enum {a_undef, left, center, right} align; // -l, -r, -c, --align WAY
    enum {o_undef, print, execute} output;     // -p, --print
    enum {s_undef, dreary, snazzy} style;      // -s, --style STYLE
    char *prevmenu;                            // -b, --back PREVMENU
    char *bgcname;                             // --background BGCOLOR
    char *classname;                           // -C, --class CLASSNAME
    bool debug;                                // --debug
    char *delimiter;                           // -d, --delimiter DELIM
    char *displayname;                         // -D, --display DISPLAYNAME
    char *fgcname;                             // --foreground FGCOLOR
    char *fontname;                            // -F, --font FNAME
    int cur_item;                              // -i, --item POSITION
    bool mouse_on;                             // --mouse / --no-mouse
    int scroll_offset;                         // -o, --scroll-offset ITEMS
    char *shell;                               // -S, --shell SHELL
    char *titlename;                           // -t, --title NAME
    bool unfocus_exit;                         // -u, --unfocus-exit
};

extern struct opts_t opts;


/* Returns the ARGV index of the first element that is NOT an option */
int parse_args(int argc, char **argv);
