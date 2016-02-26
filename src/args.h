#pragma once
#include <stdbool.h>

enum align_t {left, center, right};
enum output_t {print, execute};
enum style_t {dreary, snazzy};

extern enum align_t align;   // -l, -r, -c, --align WAY
extern enum output_t output; // -p, --print
extern enum style_t style;   // -s, --style STYLE
extern char *prevmenu;       // -b, --back PREVMENU
extern char *bgcname;        // --background BGCOLOR
extern char *classname;      // -C, --class CLASSNAME
extern bool debug;           // --debug
extern char *delimiter;      // -d, --delimiter DELIM
extern char *displayname;    // -D, --display DISPLAYNAME
extern char *fgcname;        // --foreground FGCOLOR
extern char *fontname;       // -F, --font FNAME
extern int cur_item;         // -i, --item POSITION
extern bool mouse_on;        // --mouse / --no-mouse
extern int scroll_offset;    // -o, --scroll-offset ITEMS
extern char *shell;          // -S, --shell SHELL
extern char *titlename;      // -t, --title NAME
extern bool unfocus_exit;    // -u, --unfocus-exit


/* Returns the ARGV index of the first element that is NOT an option */
int parse_args(int argc, char **argv);
