/*
 * ratmen.c
 * (Note: This program is fully documented using POD.
 * Try `pod2text ratmen.c', or use `pod2man' to generate a manpage.)
 *
 *
 * This program puts up a window that is just a menu, and executes
 * commands that correspond to the items selected.
 *
 * Initial idea: Arnold Robbins
 * Version using libXg: Matty Farrow (some ideas borrowed)
 * This code by: David Hogan and Arnold Robbins
 *
 * Copyright (c), Arnold Robbins and David Hogan
 *
 * Arnold Robbins
 * arnold@skeeve.atl.ga.us
 * October, 1994
 *
 * Code added to cause pop-up (unIconify) to move menu to mouse.
 * Christopher Platt
 * platt@coos.dartmouth.edu
 * May, 1995
 *
 * Said code moved to -teleport option, and -warp option added.
 * Arnold Robbins
 * June, 1995
 *
 * Code added to allow --foreground and --background colors.
 * John M. O'Donnell
 * odonnell@stpaul.lampf.lanl.gov
 * April, 1997
 *
 * Ratpoison windowmanager specific hacking; removed a lot of junk
 * and added keyboard functionality
 * Jonathan Walther
 * krooger@debian.org
 * September, 2001
 *
 * Derived from ratmenu and 9menu.
 * Zrajm C Akfohg <ratmen-mail@klingonska.org>
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/keysym.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <getopt.h>

#define FONT "9x15bold"
/*
#define MenuMask (ExposureMask|StructureNotifyMask|KeyPressMask|FocusChangeMask)
*/
#define MenuMask (ButtonPressMask|ButtonReleaseMask\
    |LeaveWindowMask|EnterWindowMask|PointerMotionMask|ButtonMotionMask\
    |ExposureMask|StructureNotifyMask|KeyPressMask|FocusChangeMask)

#define MenuMaskNoMouse (ExposureMask|StructureNotifyMask\
    |KeyPressMask|FocusChangeMask)



Display *dpy;                                  /* lovely X stuff */
int screen;
Window root;
int display_height;                            /* height of screen in pixels */
Window menuwin;
GC gc;
unsigned long fg;
unsigned long bg;

Colormap dcmap;
XColor color;
XFontStruct *font;
Atom wm_protocols;
Atom wm_delete_window;
int g_argc;         /* for XSetWMProperties to use */
char **g_argv;
int savex, savey;
Window savewindow;


#define Undef -1
char *progname;                                /* program name */

/* menu structure */
char **labels;                                 /* list of menu labels */
char **commands;                               /* list of menu commands */
int numitems;                                  /* total number of items */
int visible_items;                             /* number of items visible */


/* program determined settings */
int last_item     = -1;                        /* previously selected item */
int last_top      = -1;                        /* previous top item in menu */
int full_redraw   =  True;                     /* redraw all menu items */
int cur_scroll_offset = Undef;

/* command line setting variables */
enum  { left, center, right } align = Undef;   /* -l, -r, -c, --align WAY */
char *prevmenu     = NULL;                     /* -b, --back PREVMENU */
char *bgcname      = NULL;                     /*     --background BGCOLOR */
char *classname    = NULL;                     /* -C, --class CLASSNAME */
int   debug        = False;                    /*     --debug */
char *delimiter    = NULL;                     /* -d, --delimiter DELIM */
char *displayname;                             /* -D, --display DISPLAYNAME */
char *fgcname      = NULL;                     /*     --foreground FGCOLOR */
char *fontname     = NULL;                     /* -F, --font FNAME */
int   cur_item     = 0;                        /* -i, --item POSITION */
int   mouse_on     = Undef;                    /*     --mouse / --no-mouse */
enum  { print, execute } output = execute;     /* -p, --print */
int   scroll_offset = Undef;                   /* -o, --scroll-offset ITEMS */
char *shell        = "/bin/sh";                /* -S, --shell SHELL */
enum  { dreary, snazzy } style = Undef;        /* -s, --style STYLE */
char *titlename    = NULL;                     /* -t, --title NAME */
int   unfocus_exit = Undef;                    /* -u, --unfocus-exit */


/* function prototypes */
int  strcasecmp(char*, char*);                 /* string comparison */
void ask_wm_for_delete(void);
void reap(int);
void redraw_snazzy(int, int, int);
void redraw_dreary(int, int, int);
void redraw_mouse (int, int, int);
void (*redraw) (int, int, int) = redraw_dreary;

void run_menu(int);
void set_wm_hints(int, int, int);
void spawn(char*);
void help(void);
void version(void);


/*
 * Event loop monitor thingy (use option `--debug') stolen from:
 * http://www-h.eng.cam.ac.uk/help/tpl/graphics/X/X11R5/node41.html
 */
static char *event_names[] = {
    "", "", "KeyPress", "KeyRelease", "ButtonPress", "ButtonRelease",
    "MotionNotify", "EnterNotify", "LeaveNotify", "FocusIn", "FocusOut",
    "KeymapNotify", "Expose", "GraphicsExpose", "NoExpose", "VisibilityNotify",
    "CreateNotify", "DestroyNotify", "UnmapNotify", "MapNotify", "MapRequest",
    "ReparentNotify", "ConfigureNotify", "ConfigureRequest", "GravityNotify",
    "ResizeRequest", "CirculateNotify", "CirculateRequest", "PropertyNotify",
    "SelectionClear", "SelectionRequest", "SelectionNotify", "ColormapNotify",
    "ClientMessage", "MappingNotify"
};


/* produce error message */
void die(char *message) {
    fprintf(stderr, "%s: %s\n", progname, message);
    fprintf(stderr, "Try `%s --help' for more information.\n", progname);
    exit(1);
}


/* rewritten to use getopts by Zrajm */
int args(int argc, char **argv) {
    int c;
    while (1) {
        static struct option long_options[] = {
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
        if (c == -1) break;

        switch (c) {
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
                    align = left;
                else if (strcasecmp(optarg, "center") == 0)
                    align = center;
                else if (strcasecmp(optarg, "right") == 0)
                    align = right;
                else {
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
                debug = True;
                break;

            case 'd': /* -d, --delimiter DELIM */
                delimiter = optarg;
                if (strcmp(delimiter, "") == 0)
                    die("delimiter must be at least one character long");
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
                help();
                break;

            case 'i': /* -i, --item POSITION */
                cur_item = atoi(optarg) - 1;
                break;

            case 'm': /*     --mouse */
                mouse_on = True;
                break;

            case 'M': /*     --no-mouse */
                mouse_on = False;
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
                    style = dreary;
                else if (strcasecmp(optarg, "snazzy") == 0)
                    style = snazzy;
                else {
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
                unfocus_exit = True;
                break;

            case 'U': /* -U, --no-unfocus-exit */
                unfocus_exit = False;
                break;

            case 'V': /* -V, --version */
                version();
                break;

            case '?':
                /* getopt_long already printed an error message. */
                break;

            default:
                abort();
        }
    }
    return(optind);
}


char *xresource_if(int test, Display *dpy, char *progname, char *resource) {
    /* char *cut = ""; */
    char *tmp = "";
    if (test) {
        tmp = XGetDefault(dpy, progname, resource);
        if (tmp != NULL) {                     /* found resource */
            /*
            if ((cut = strchr(tmp, ' ')))      |* trunc at 1st space *|
                *cut++ = '\0';
            */
            if (debug == True)
                fprintf(stderr, "  %s.%-12s: >%s<\n",
                        progname, resource, tmp);
            return tmp;
        } else if (debug == True)              /* no resource found */
            fprintf(stderr, "  %s.%-12s: [not defined in X resources]\n",
                    progname, resource);
    } else if (debug == True)                  /* given on command line */
        fprintf(stderr, "  %s.%-12s: [overriden by command line]\n",
                progname, resource);
    return NULL;
}


/* completely written by Zrajm */
void xresources(Display *dpy) {
    char *tmp = "";
    if (debug == True) fprintf(stderr, "Reading X resources:\n");

    /* align: {left|center|right} */
    tmp = xresource_if((align == Undef), dpy, classname, "align");
    if (tmp != NULL) {
        if      (strcasecmp(tmp, "left"  ) == 0) align = left;
        else if (strcasecmp(tmp, "center") == 0) align = center;
        else if (strcasecmp(tmp, "right" ) == 0) align = right;
    }


    /* background: BGCOLOR */
    tmp = xresource_if((bgcname == NULL), dpy, classname, "background");
    if (tmp != NULL) bgcname = tmp;


    /* font: FNAME */
    tmp = xresource_if((fontname == NULL), dpy, classname, "font");
    if (tmp != NULL) fontname = tmp;


    /* foreground: FGCOLOR */
    tmp = xresource_if((fgcname == NULL), dpy, classname, "foreground");
    if (tmp != NULL) fgcname = tmp;


    /* mouse: {on|yes|true|off|no|false} */
    tmp = xresource_if((mouse_on == Undef), dpy, classname, "mouse");
    if (tmp != NULL) {
        if      (strcasecmp(tmp, "on"  )  == 0) mouse_on = True;
        else if (strcasecmp(tmp, "yes")   == 0) mouse_on = True;
        else if (strcasecmp(tmp, "true")  == 0) mouse_on = True;
        else if (strcasecmp(tmp, "off")   == 0) mouse_on = False;
        else if (strcasecmp(tmp, "no")    == 0) mouse_on = False;
        else if (strcasecmp(tmp, "false") == 0) mouse_on = False;
    }


    /* style: {dreary|snazzy} */
    tmp = xresource_if((style == Undef), dpy, classname, "style");
    if (tmp != NULL) {
        if      (strcasecmp(tmp, "dreary") == 0) style = dreary;
        else if (strcasecmp(tmp, "snazzy") == 0) style = snazzy;
    }


    /* scrollOffset: ITEMS */
    tmp = xresource_if((scroll_offset == Undef), dpy, classname, "scrollOffset");
    if (tmp != NULL) scroll_offset = atoi(tmp);


    /* unfocusExit: {on|yes|true|off|no|false} */
    tmp = xresource_if((unfocus_exit == Undef), dpy, classname, "unfocusExit");
    if (tmp != NULL) {
        if      (strcasecmp(tmp, "on"  )  == 0) unfocus_exit = True;
        else if (strcasecmp(tmp, "yes")   == 0) unfocus_exit = True;
        else if (strcasecmp(tmp, "true")  == 0) unfocus_exit = True;
        else if (strcasecmp(tmp, "off")   == 0) unfocus_exit = False;
        else if (strcasecmp(tmp, "no")    == 0) unfocus_exit = False;
        else if (strcasecmp(tmp, "false") == 0) unfocus_exit = False;
    }
}


/* completely written by Zrajm *|
void xresources(Display *dpy) {
    XrmDatabase resourceDb = 0;
    XrmValue    value;                     |* resource value container   *|
    char        *cp, *tmp;

    XrmInitialize();
    if (!(XResourceManagerString(dpy))) return;

    |* get Xresource database *|
    resourceDb = XrmGetStringDatabase(XResourceManagerString(dpy));

    |* align: {left|center|right} *|
    if (align == Undef &&
        XrmGetResource(resourceDb, "ratmen*align", "", &tmp, &value)
    ) {
        tmp = value.addr;
        if ((cp = strchr(tmp, ' '))) *cp++ = '\0';
        if      (strcasecmp(tmp, "left"  ) == 0) align = left;
        else if (strcasecmp(tmp, "center") == 0) align = center;
        else if (strcasecmp(tmp, "right" ) == 0) align = right;
    }

    |* background: BGCOLOR *|
    if (bgcname == NULL &&
        XrmGetResource(resourceDb, "ratmen*background", "", &tmp, &value)
    ) {
        tmp = value.addr;
        if ((cp = strchr(tmp, ' '))) *cp++ = '\0';
        if (tmp) bgcname = tmp;
    }

    |* font: FNAME *|
    if (fontname == NULL &&
        XrmGetResource(resourceDb, "ratmen*font", "", &tmp, &value)
    ) {
        tmp = value.addr;
        if ((cp = strchr(tmp, ' '))) *cp++ = '\0';
        if (tmp) fontname = tmp;
    }

    |* foreground: FGCOLOR *|
    if (fgcname == NULL &&
        XrmGetResource(resourceDb, "ratmen*foreground", "", &tmp, &value)
    ) {
        tmp = value.addr;
        if ((cp = strchr(tmp, ' '))) *cp++ = '\0';
        if (tmp) fgcname = tmp;
    }

    |* style: {dreary|snazzy} *|
    if (style == Undef &&
        XrmGetResource(resourceDb, "ratmen*style", "", &tmp, &value)
    ) {
        tmp = value.addr;
        if ((cp = strchr(tmp, ' '))) *cp++ = '\0';
        if      (strcasecmp(tmp, "dreary") == 0) style = dreary;
        else if (strcasecmp(tmp, "snazzy") == 0) style = snazzy;
    }

    |* scrollOffset: ITEMS *|
    if (scroll_offset == Undef &&
        XrmGetResource(resourceDb, "ratmen*scrollOffset", "", &tmp, &value)
    ) {
        tmp = value.addr;
        if ((cp = strchr(tmp, ' '))) *cp++ = '\0';
        scroll_offset = atoi(tmp);
    }

    |* unfocusExit: {on|yes|true|off|no|false} *|
    if (unfocus_exit == Undef &&
        XrmGetResource(resourceDb, "ratmen*unfocusExit", "", &tmp, &value)
    ) {
        tmp = value.addr;
        if ((cp = strchr(tmp, ' '))) *cp++ = '\0';
        if      (strcasecmp(tmp, "on"  )  == 0) unfocus_exit = True;
        else if (strcasecmp(tmp, "yes")   == 0) unfocus_exit = True;
        else if (strcasecmp(tmp, "true")  == 0) unfocus_exit = True;
        else if (strcasecmp(tmp, "off")   == 0) unfocus_exit = False;
        else if (strcasecmp(tmp, "no")    == 0) unfocus_exit = False;
        else if (strcasecmp(tmp, "false") == 0) unfocus_exit = False;
    }
    XrmDestroyDatabase(resourceDb);
}
*/


void items(int start, int count, char **arg) {
    int j;
    char *cut;

    if (delimiter) {                           /* using delimiter */
        /* do things similar to `9menu' */
        if (count-start < 1) die("not enough arguments");

        numitems = count-start;
        labels   = (char **) malloc(numitems*sizeof(char *));
        commands = (char **) malloc(numitems*sizeof(char *));
        if (commands == NULL || labels == NULL)
            die("cannot allocate memory for command and label arrays");

        for (j = 0; j < numitems; j ++) {
            labels[j] = arg[start+j];
            if ((cut  = strstr(labels[j], delimiter)) != NULL) {
                *cut  = '\0';
                 cut += strlen(delimiter);
                commands[j] = cut;
            } else
                commands[j] = labels[j];
        }
        return;
    } else {                                   /* without delimiter */
        /* do as `ratmenu' does */
        if ((count-start) % 2 != 0) die("not an even number of menu arguments");
        if ((count-start) * 2 <  1) die("not enough arguments");

        numitems = (count-start) / 2;
        labels   = (char **) malloc(numitems*sizeof(char *));
        commands = (char **) malloc(numitems*sizeof(char *));
        if (commands == NULL || labels == NULL)
            die("cannot allocate memory for command and label arrays");

        for (j=0; start < count; j ++) {
            labels[j]   = arg[start++];
            commands[j] = arg[start++];
            if (strcmp(commands[j], "") == 0)
                commands[j] = labels[j];
        }
        return;
    }
}


int HandleXError(Display *dpy, XErrorEvent *event) {
/*    gXErrorFlag = 1;*/
    return 0;
}




/* main --- crack arguments, set up X stuff, run the main menu loop */
int main(int argc, char **argv) {
    int i;
    char *cp;
    XGCValues gv;
    unsigned long mask;
    g_argc = argc;                             /* save command line args */
    g_argv = argv;


    /* get program name (=default X resource class & title) */
    if ((cp = strrchr(argv[0], '/')))          /* if argv[0] contains slash  */
        progname = ++cp;                       /*   use all after that       */
    else                                       /* otherwise                  */
        progname = argv[0];                    /*   use argv[0] as is        */

    /* set defaults for non-resource options */
    titlename = progname;                      /* default window title */
    classname = progname;                      /* default X resource class */
    i = args(argc, argv);                      /* process command line args */
    items(i, argc, argv);                      /* process menu items */

    dpy = XOpenDisplay(displayname);
    if (dpy == NULL) {
        fprintf(stderr, "%s: cannot open display", progname);
        if (displayname != NULL)
            fprintf(stderr, " %s", displayname);
        fprintf(stderr, "\n");
        exit(1);
    }

    /* make it ignore BadWindow errors */
    XSetErrorHandler(HandleXError);
    XFlush(dpy);

    /* use Xresource for undefined values */
    xresources(dpy);

    /* defaults (for undefined cases) */
    if (fontname == NULL) fontname = FONT;     /* default font */
    if (scroll_offset == Undef)                /* default scroll offset */
        scroll_offset = 3;
    if (mouse_on == Undef)                     /* mouse menu selection */
        mouse_on = True;
    if (style == snazzy) {                     /* default display style */
        redraw = redraw_snazzy;
    } else {
        redraw = redraw_dreary;
    }
    cur_scroll_offset = scroll_offset;

    screen         = DefaultScreen(dpy);
    root           = RootWindow(dpy, screen);
    display_height = DisplayHeight(dpy, screen);

    dcmap = DefaultColormap (dpy, screen);
    if (fgcname == NULL ||
        XParseColor(dpy, dcmap, fgcname, &color) == 0 ||
        XAllocColor(dpy, dcmap, &color) == 0
    ) {
        fg = WhitePixel(dpy, screen);
    } else { fg = color.pixel; }

    if (bgcname == NULL ||
        XParseColor(dpy, dcmap, bgcname, &color) == 0 ||
        XAllocColor(dpy, dcmap, &color) == 0
    ) {
        bg = BlackPixel(dpy, screen);
    } else { bg = color.pixel; }

    if ((font = XLoadQueryFont(dpy, fontname)) == NULL) {
        fprintf(stderr, "%s: fatal: cannot load font `%s'\n", progname, fontname);
        exit(1);
    }

    gv.foreground = fg^bg;
    gv.background = bg;
    gv.font = font->fid;
    gv.function = GXxor;
    gv.line_width = 0;
    mask = GCForeground | GCBackground | GCFunction | GCFont | GCLineWidth;
    gc = XCreateGC(dpy, root, mask, &gv);

    signal(SIGCHLD, reap);

    run_menu(cur_item);

    /* XCloseDisplay() cannot generate BadWindow */
    XCloseDisplay(dpy);
    exit(0);
}


/* spawn --- run a command */
void spawn(char *com) {
    int pid;
    static char *sh_base = NULL;

    if (sh_base == NULL) {
        sh_base = strrchr(shell, '/');
        if (sh_base != NULL)
            sh_base++;
        else
            sh_base = shell;
    }

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "%s: can't fork\n", progname);
        return;
    } else if (pid > 0)
        return;

    close(ConnectionNumber(dpy));
    execl(shell, sh_base, "-c", com, NULL);
    _exit(1);
}


/* reap --- collect dead children */
void reap(int s) {
    (void) wait((int *) NULL);
    signal(s, reap);
}


void version(void) {
    printf("%s (for Ratpoison) 2.2.3 (compiled 25 October 2007)\n"
        "Written by Arnold Robbins & David Hogan (1994-1995),\n"
        "John M. O'Donnell (1997), Jonathan Walther (2001) and\n"
        "Zrajm C Akfohg (2003, 2007).\n"
        "\n"
        "Copyright 1994-1995, 1997, 2001, 2003, 2007 by respective author.\n"
        "Distributed under the GNU Public License.\n", progname);
    exit(0);
}


/* usage --- print a usage message and die */
void help(void) {
    printf("Usage: %s [OPTION]... MENUITEM COMMAND ...\n"
        "   or: %s [OPTION]... {-d##|--delimiter ##} MENUITEM##COMMAND ...\n"
        "Create a simple menu in a separate window and run user selected command.\n"
        "\n", progname, progname);
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


/* run_menu --- put up the window, execute selected commands */
void run_menu(int cur) {
    KeySym key;
    XEvent event;
    XClientMessageEvent *cmsg;
    int i, wide, high, dx, dy, mousing = False, cur_store = cur;

    /* Currently selected menu item */
    if (cur  <  -1)       cur = 0;
    if (cur  >= numitems) cur = numitems-1;

    /* find widest menu item */
    dx = 0;
    for (i = 0; i < numitems; i++) {
        wide = XTextWidth(font, labels[i], strlen(labels[i])) + 4;
        if (wide > dx)
            dx = wide;
    }
    wide = dx;

    high = font->ascent + font->descent + 1;
    dy   = numitems * high;                    /* height of menu window */
    if (dy > display_height)                   /* shrink if outside screen */
        dy = display_height - (display_height % high);

    visible_items = dy / high;

    if (debug == True) {
        fprintf(stderr, "Facts about window to open:\n"
                "  Window height: %d\n"
                "  Font   height: %d\n"
                "  Window rows  : %d\n",
                dy, high, visible_items);
    }

    set_wm_hints(wide, dy, high);

    ask_wm_for_delete();

    /*
     * Note: When mouse is disabled, we simply do not allow for any
     * mouse-related X events to happen, here in this XSelectInput(),
     * rather than modifying the Event test loop below.
     */
    XSelectInput(dpy, menuwin,
        (mouse_on == True ? MenuMask : MenuMaskNoMouse));

    XMapWindow(dpy, menuwin);



    for (;;) {
        /* BadWindow not mentioned in doc */
        XNextEvent(dpy, &event);
        if (debug == True)
            fprintf(stderr, "X event: %s\n",
                event_names[event.type]); /* DEBUG thingy */
        switch (event.type) {

            case EnterNotify:
                if (mousing == True) break;
                cur_store = cur;
                mousing = True;
                break;

            case LeaveNotify:
                if (mousing == False) break;
                /* fprintf(stderr, "going out\n"); */
                cur = cur_store;
                redraw_mouse(cur, high, wide);
                mousing = False;
                break;

            case ButtonRelease:
                if (mousing == False) break;
                if (event.xbutton.button == Button1) { /* left button */
                    if (output == print) printf("%s\n", commands[cur]);
                    else spawn(commands[cur]);
                    return;
                } else if (event.xbutton.button == Button3) { /* right */
                    if (prevmenu) {
                        if (output == print) printf("%s\n", prevmenu);
                        else spawn(prevmenu);
                        return;
                    }
                } else return;                   /* middle button */
                break;


            case ButtonPress:
            case MotionNotify:
                if (mousing == False) break;
                cur = event.xbutton.y / high + last_top;
                redraw_mouse(cur, high, wide);
                break;


            case KeyPress:                     /* key is pressed in win */
                /* BadWindow not mentioned in doc */
                XLookupString(&event.xkey, NULL, 0, &key, NULL);
                switch (key) {
                    /* case ????: (Ctrl-L for refresh)
                        full_redraw = True;
                        redraw(cur, high, wide);
                        return 5
                    */
                    case XK_Escape:
                    case XK_q:
                        return;
                        break;

                    case XK_Left:
                    case XK_h:
                        if (prevmenu) {
                            if (output == print)
                                printf("%s\n", prevmenu);
                            else
                                spawn(prevmenu);
                            return;
                        }
                        break;

                    case XK_Right:
                    case XK_Return:
                    case XK_l:
                        if (output == print)
                            printf("%s\n", commands[cur]);
                        else
                            spawn(commands[cur]);
                        return;
                        break;

                    case XK_Tab:
                    case XK_space:
                    case XK_Down:
                    case XK_j:
                    case XK_plus:
                        ++cur == numitems ? cur  = 0           : 0 ;
                        cur_store = cur;
                        redraw(cur, high, wide);
                        break;

                    case XK_BackSpace:
                    case XK_Up:
                    case XK_k:
                    case XK_minus:
                        cur-- <=  0        ? cur = numitems - 1 : 0 ;
                        cur_store = cur;
                        redraw(cur, high, wide);
                        break;

                    case XK_Home:
                    case XK_Page_Up:
                        cur = 0;
                        cur_store = cur;
                        redraw(cur, high, wide);
                        break;

                    case XK_End:
                    case XK_Page_Down:
                        cur = numitems-1;
                        cur_store = cur;
                        redraw(cur, high, wide);
                        break;

                }
                break;

            case ConfigureNotify:              /* win is resized or moved */
                {
                    int new_height = 0;
                    XWindowAttributes gaga;
                    if (!XGetWindowAttributes(dpy, menuwin, &gaga))
                        break;
                    new_height = gaga.height - (gaga.height % high);
                    visible_items = new_height / high;
                    cur_scroll_offset = (visible_items-1)/2;
                    if (cur_scroll_offset > scroll_offset) {
                        cur_scroll_offset = scroll_offset;
                    }
                    if (debug == True) {
                        fprintf(stderr, "  Window resized/moved:\n"
                            "    Window height (old/new):%4d /%4d\n"
                            "    Visible rows  (old/new):%4d /%4d\n"
                            "    Scroll offset (cmd/cur):%4d /%4d\n",
                            gaga.height,      new_height,
                            gaga.height/high, new_height/high,
                            scroll_offset,    cur_scroll_offset);
                    }
                }
                break;

            case UnmapNotify:                  /* win becomes hidden */
                if (unfocus_exit == True) {
                    return;
                } else
                    /* can generate BadWindow */
                    XClearWindow(dpy, menuwin);
                break;

            /* FIXME: For some reason the code in `FocusIn' or `FocusOut' results in an
             * "Error: BadWindow (invalid Window parameter)" message in Ratpoison
             * (but it works, even if it look ugly).
             */
            case FocusIn: break;               /* win becomes focused again */
            case FocusOut:                     /* win becomes unfocused   */
                if (unfocus_exit == True) {
                    XDestroyWindow(dpy, menuwin);
                    return;
                }
                break;

            case MapNotify:                    /* win becomes visible again */

                {/* %%%% */
                    XWindowAttributes gaga;
                    int new_height = 0;
                    if (XGetWindowAttributes(dpy, menuwin, &gaga)) {
                        new_height = gaga.height - (gaga.height % high);
                        if (gaga.height != new_height) {
                            if (debug == True) {
                                fprintf(stderr, "RESIZING window to:\n"
                                        "  Old  height: %d\n"
                                        "  Font height: %d\n"
                                        "  New  height: %d\n",
                                        gaga.height, high, new_height);
                            }
                            /* FIXME window doesn't get resized, why not? */
                            XResizeWindow(dpy, menuwin, 100, new_height);
                            XSync(dpy, True);

                            if (XGetWindowAttributes(dpy, menuwin, &gaga)) {
                                if (debug == True) {
                                    fprintf(stderr, "Window height after resizing: %d\n",
                                            gaga.height);
                                }
                            }
                            visible_items = gaga.height / high;
                            cur_scroll_offset = (visible_items-1)/2;
                            if (cur_scroll_offset > scroll_offset) {
                                cur_scroll_offset = scroll_offset;
                            } else 
                                fprintf(stderr, "Shrinking scroll offset from %d to %d.\n",
                                        scroll_offset, cur_scroll_offset);
                        }
                    } else {
                        fprintf(stderr, "can't get window attributes\n");
                    }

                    /* make sure window isn't taller than screen */
                    /* FIXME: problems when used inside a ratpoison frame */

                }
            case Expose:                       /* win becomes partly covered */
                full_redraw = True;
                redraw(cur, high, wide);
                /* `while' skips immediately subsequent Expose events in queue,
                 * which avoids superflous window redraws; taken from:
                 * www-h.eng.cam.ac.uk/help/tpl/graphics/X/X11R5/node31.html)*/
                while (XCheckTypedWindowEvent(dpy,menuwin,Expose,&event))
                    ;
                break;

            case ClientMessage:
                cmsg = &event.xclient;
                if (cmsg->message_type == wm_protocols
                        && cmsg->data.l[0] == wm_delete_window)
                    return;
        }
    }
}


/* set_wm_hints --- set all the window manager hints */
void set_wm_hints(int wide, int high, int font_height) {
    XWMHints *wmhints;
    static XSizeHints sizehints = {
        USSize | PSize | PMinSize | PMaxSize | PBaseSize | PResizeInc,
        0, 0, 80, 80,                          /* x, y, width and height */
        1, 1,                                  /* min width and height */
        0, 0,                                  /* max width and height */
        1, 1,                                  /* width and height increments */
        {1, 1},                                /* x, y increments */
        {1, 1},                                /* aspect ratio - not used */
        0, 0,                                  /* base size */
        NorthWestGravity                       /* gravity */
    };
    XClassHint *classhints;
    XTextProperty wname;

    if ((wmhints = XAllocWMHints()) == NULL) {
        fprintf(stderr, "%s: cannot allocate window manager hints\n",
            progname);
        exit(1);
    }

    if ((classhints = XAllocClassHint()) == NULL) {
        fprintf(stderr, "%s: cannot allocate class hints\n",
            progname);
        exit(1);
    }

    /* window width */
    sizehints.width      = sizehints.base_width  =
    sizehints.min_width  = sizehints.max_width   = wide;

    /* window height */
    /* (always an even number of font heights) */
    sizehints.height     = sizehints.base_height = high;
    sizehints.min_height = sizehints.height_inc  = font_height;
    sizehints.max_height = numitems*font_height;

    if (XStringListToTextProperty(&titlename, 1, &wname) == 0) {
        fprintf(stderr, "%s: cannot allocate window name structure\n",
            progname);
        exit(1);
    }

    /* open menu window */
    menuwin = XCreateSimpleWindow(dpy, root, sizehints.x, sizehints.y,
                sizehints.width, sizehints.height, 1, fg, bg);

    wmhints->input         = True;
    wmhints->initial_state = NormalState;
    wmhints->flags         = StateHint | InputHint;

    classhints->res_name   = progname;
    classhints->res_class  = "ratmen";

    XSetWMProperties(dpy, menuwin, &wname, NULL,
        g_argv, g_argc, &sizehints, wmhints, classhints);
}

/* ask_wm_for_delete --- jump through hoops to ask WM to delete us */
void ask_wm_for_delete(void) {
    int status;

    wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    status = XSetWMProtocols(dpy, menuwin, &wm_delete_window, 1);

    if (status != True)
        fprintf(stderr, "%s: cannot ask for clean delete\n",
            progname);
}


/* redraw --- actually draw the menu */
void redraw_snazzy (int cur_item, int high, int wide) {
    int i, j, ty, tx;
    if (cur_item < 0) cur_item = 0;    /* negative item number */
    XClearWindow(dpy, menuwin);
    for (i = 0, j = cur_item; i < numitems; i++, j++) {
        j %= numitems;
        if (align == right) {
            tx = wide - XTextWidth(font, labels[j], strlen(labels[j]));
        } else if (align == center) {
            tx = (wide - XTextWidth(font, labels[j], strlen(labels[j]))) / 2;
        } else { /* align == left */
            tx = 0;
        }
        ty = i*high + font->ascent + 1;
        XDrawString(dpy, menuwin, gc, tx, ty, labels[j], strlen(labels[j]));
    }
    XFillRectangle(dpy, menuwin, gc, 0, 0, wide, high);
}

void redraw_mouse (int cur_item, int high, int wide) {
    if (cur_item <= last_top)
        cur_item = last_top; else
    if (cur_item > last_top+visible_items)
        cur_item = last_top+visible_items;
    if (cur_item == last_item) return;         /* no movement = no update */
    XFillRectangle(dpy, menuwin, gc, 0, (last_item - last_top) * high, wide, high);
    XFillRectangle(dpy, menuwin, gc, 0, (cur_item  - last_top) * high, wide, high);
    if (debug == True)
        fprintf(stderr, "current item: %d (of %d-%d)\n",
                cur_item, last_top, last_top+visible_items);
    last_item = cur_item;
}


void redraw_dreary (int cur_item, int high, int wide) {
    int i, j, ty, tx;
    int cur_top = last_top;                    /* local top var */

    if (cur_item == last_item && full_redraw != True)
        return;                                /* no movement = no update */

    /* change top item (i.e. scroll menu) */
    if (cur_item < last_top + cur_scroll_offset) { /* scroll upward */
        cur_top  = cur_item - cur_scroll_offset;
        if (cur_top < 0) cur_top = 0;          /* don't go above menu */
    } else if (                                /* scroll downward */
        cur_item > last_top - (cur_scroll_offset+1) + visible_items
    ) {
        cur_top  = cur_item + (cur_scroll_offset+1) - visible_items;
        if (cur_top > numitems - visible_items)/* don't go below menu */
            cur_top = numitems - visible_items;
    } else {
        if (cur_top < 0) cur_top = 0;          /* don't go above menu */
    }


    if (full_redraw == True || cur_top != last_top) {
        /* redraw all visible items */
        if (debug == True) fprintf(stderr, "  (full menu redraw)\n");
        XClearWindow(dpy, menuwin);
        for (i = 0; i < visible_items; i++) {
            j = cur_top + i;
            if (align == right) {
                tx = wide - XTextWidth(font, labels[j], strlen(labels[j]));
            } else if (align == center) {
                tx = (wide - XTextWidth(font, labels[j], strlen(labels[j]))) / 2;
            } else { /* align == left */
                tx = 0;
            }
            ty = i * high + font->ascent + 1;
            XDrawString(dpy, menuwin, gc, tx, ty, labels[j], strlen(labels[j]));
        }
        if (cur_item >= 0)
            XFillRectangle(dpy, menuwin, gc, 0, (cur_item - cur_top) * high, wide, high);
        last_top    = cur_top;
        full_redraw = False;
    } else {
        /* only invert last & current item */
        if (debug == True) fprintf(stderr, "  (partial menu redraw)\n");
        j =  cur_item -  cur_top;
        i = last_item - last_top;
        XFillRectangle(dpy, menuwin, gc, 0, (last_item - cur_top) * high, wide, high);
        if (cur_item >= 0)
            XFillRectangle(dpy, menuwin, gc, 0, (cur_item  - cur_top) * high, wide, high);
    }
    last_item = cur_item;
}

/*
void redraw_dreary (int cur, int high, int wide) {
    int i, ty, tx;
    XClearWindow(dpy, menuwin);
    for (i = 0; i < numitems; i++) {
        if (align == right) {
            tx = wide - XTextWidth(font, labels[i], strlen(labels[i]));
        } else if (align == center) {
            tx = (wide - XTextWidth(font, labels[i], strlen(labels[i]))) / 2;
        } else { |* align == left *|
            tx = 0;
        }
        ty = i*high + font->ascent + 1;
        XDrawString(dpy, menuwin, gc, tx, ty, labels[i], strlen(labels[i]));
    }
    XFillRectangle(dpy, menuwin, gc, 0, cur*high, wide, high);
}
*/

/*
#manpage: ratmen(1)

=head1 NAME

ratmen - create a menu to run commands


=head1 SYNOPSIS

B<ratmen> [I<OPTION>]... I<MENUITEM> I<COMMAND>...

B<ratmen> [I<OPTION>]... {B<-d>I<##>|B<--delimiter> I<##>} I<MENUITEM>I<##>I<COMMAND> ...



=head1 DESCRIPTION

B<Ratmen> is a simple program that accepts a list of MENUITEM and COMMAND pairs
on the command line. It creates a window that consists of nothing but a menu.
When a particular MENUITEM is selected by the user, the corresponding COMMAND
is executed or (using B<--print>) printed on standard output.

Menu items and commands may either be given separately (as in B<ratmenu>) using
two arguments for each menu option, or, optionally you may specify a delimiter
(using the B<--delimiter> option) and specify both menu item and command in the
same argument (similar to B<9menu>). The delimiter may be of any length.

If the command is omitted (or if an empty command is supplied when not using
B<--delimiter>) then the menu item text will be used as the command.


=head1 OPTIONS

Some options may be specified using X resouces (allowing you to define some
defaults you like in your in you F<~/.Xresources>, F<~/.Xdefaults> or similar).
To make this mechanism as useful as possible I would suggest that you refrain
from using such options on the command line, unless you really need to, and
instead enable/disable the corresponding X resource setting.

B<ratmen>'s default resource class is the name of the executed file, either
B<ratmen> or, if the executable was called through a link, the name of the link
in question. The command line option B<--class> may be used to override the
default X resource class name.


=over 8

=item B<--align> I<{left|center|right}>  (X resource: B<align>)

Aligns the text of the menu entries to the B<left>, B<center> or B<right>.
Defaults to B<left>. (Short options B<-l>, B<-c> and B<-r> may also be used for
B<left>, B<center> and B<right> respectivelly.)


=item B<-b>, B<--back> I<PREVMENU>

Run command PREVMENU when user goes back in the menu hierarchy. Useful when
using nested menus; it gives the user a way to back out and return to the
previous menu. Note that you can use this option for other things too. The
command specified by the B<--back> option is executed when the user hits one of
the "back" keys.


=item B<--background> I<BGCOLOR>  (X resource: B<background>)

Set the background color to BGCOLOR. By default, the background color is black.
BGCOLOR may be the name of any color accepted by your X server.


=item B<-C>, B<--class> I<CLASSNAME>

This option allows you to override B<ratmen>'s resource class. Normally it is
"ratmen", but it can be set to another class such as "ratmenu" to override
selected resources.


=item B<--debug>

Makes B<ratmen> talk quite a lot on standard error.


=item B<-d>, B<--delimiter> I<DELIM>

This changes the behaviour when parsing subsequent menuitem/command pairs.

Normally the command line argumens are taken to be alternately menu items and
their related commands, thus requiring an even number of arguments to be passed
to B<ratmen>. (If a command is given as "" it is taken to be the same as the
menu item.) This behaviour can make it somewhat difficult to distinguish
between menu item arguments and command arguments in cases where the menu grow
quite big (e.g. in a script). Therefore an alternative is provided...

If you specify a delimiter (using B<--delimiter>) this behaviour is changed and
the menu item and command are both expected to occur in the same argument,
separated by whatever delimiter you've specified. This makes the command line
easier to read (for a human) but is sometimes disadvantageous, especially in
autogenerated menus, since the delimiter in question cannot occur in the menu
item text. (See also L<"EXAMPLES"> below.)


=item B<-D>, B<--display> I<DISPLAYNAME>

Use the X display DISPLAYNAME, instead of the default display. Normally you
won't need to use this.


=item B<--foreground> I<FGCOLOR>  (X resource: B<foreground>)

Set the foreground color to FGCOLOR. By default, the foreground color is black.
FGCOLOR may be the name of any color accepted by your X server.


=item B<-F>, B<--font> I<FONTNAME>  (X resource: B<font>)

Use the font FONTNAME instead of the default font.


=item B<-h>, B<--help>

Output a brief help message.

=item B<--mouse>  (X resource: B<mouse>)

=item B<--no-mouse>

Enable/disable mouse support in B<ratmen>. The mouse support is quite limited
(you can't scroll a large menu using the mouse) and is only intended for those
moments when you instinctively want to click on something you see in order to
select it. See also L<MOUSE SUPPORT>.

Use C<ratmen.mouse: false> (or `no' or `off') in your X resource file to
disable mouse support as a default, and C<ratmen.mouse: true> (or `yes' or `on)
to enable it.


=item B<-i>, B<--item> I<POSITION>

Pre-select menu item number POSITION, instead of the first menu item, upon
opening the menu. Menu items are numbered from 1. This is sometimes useful in
scripts.

If POSITION is 0, then no item is selected initially. Going up one item will
make the menu jump to the last item, going down will jump to the first item.
Pressing enter while no item is selected is the same as aborting. There is no
way to zero entries, other than this, so if you move about you cannot return to
the state where no item is selected (but why should you ever want that?).

The described behavior is useful when the initially selected item denotes a
`current' value (useful when called from a script) and the absence of a
selected item may be used to indicate that there is no such `current' value.


=item B<-p>, B<--print>

Prints the COMMAND associated with the selected MENUITEM on standard output
instead of running it.

Using this option you can use a menu for all kinds of selections, and not only
for running a program. When using this option COMMAND no longer need to be a
valid command at all -- any string will work. See also L<"EXAMPLES"> below.


=item B<-o>, B<--scroll-offset> I<ITEMS>  (X resource: B<scrollOffset>)

If a menu is too large to fit in one window, it will become scrollable.
B<Ratmen> will try to keep at least ITEMS number of items between the current
position and the top or bottom of the menu. If you get closer than this, the
menu will scroll. As you get close to the top or bottom of the menu scrolling
will cease. (Default scroll offset is 3.)

Scroll offset may not be larger than half of the menu. If it is it will be cut
down to that value.


=item B<-S>, B<--shell> I<PROG>

Use I<PROG> as the shell to run commands, instead of B</bin/sh>. A popular
alternative shell is rc(1). If the shell cannot be executed, B<ratmen> will
silently fall back to using B</bin/sh>.


=item B<-s>, B<--style> I<{snazzy|dreary}>  (X resource: B<style>)

The default style is B<dreary>, where the highlight bar moves up and down the
menu as it does on all conventional keyboard controlled menus.  In B<dreary>
mode, the highlight bar, which shows the currently selected item, remains
stationary while all the menu items are rotated up or down when the cursor keys
are moved.


=item B<-t>, B<--title> I<NAME>

Change the title of the menu window to NAME. The default title is the last
component of the path used to run B<ratmen>, typically, "ratmen".


=item B<--unfocus-exit>  (X resource: B<unfocusExit>)

=item B<--no-unfocus-exit>

FIXME: Currently a `BadWindow' error message is generated by X when the current
instance of B<ratmen> dies of unfocus. Does anyone know how to fix this?

B<--unfocus-exit> causes B<ratmen> to die (without any option being selected)
if its window loses focus (it's probably not a good to use in combination with
a window manager that automatically focuses the window under your pointer).

B<--no-unfocus-exit> makes B<ratmen> survive unfocusing. This can be confusing
in some cases as it makes it possible to have start several menus at once
(normally the previous menu would die from unfocus).

Use C<ratmen.unfocusExit: false> (or `no' or `off') in your X resource file to
disable unfocus deaths as a default, and C<ratmen.unfocusExit: true> (or `yes'
or `on) to enable it.


=item B<-V>, B<--version>

This option prints the version of ratmen on the standard output, and then
exits with  an exit value of zero.

=back


=head1 KEYSTROKES

The B<Up> keystrokes move the selection to the next item up. The B<Down>
keystrokes move the selection to the next item down. When the selection reaches
the top or bottom, it scrolls around to the other side on pressing of the
appropriate keystroke. The B<Select> keystrokes execute the command
corresponding to the currently selected menu item, and exit ratmen. The
B<Back> keystrokes does nothing unless the B<--back> option was used, in which
case it will run the command specified by that option and exit ratmen. The
B<Exit> keystrokes quit ratmen without doing anything.

    Up      "k", Up_arrow, BackSpace, "-"
    Down    "j", Down_arrow, Space, Tab, "+"
    Select  "l", Right_arrow, Return
    Back    "h", Left_arrow
    Exit    "q", Escape


=head1 MOUSE SUPPORT

B<Ratmen> implements limited support for the rodent, you may select an item
(left), go to any previous menu, given by B<--back>, (right) or abort the menu
(any other; usually middle). The mouse cannot be used to scroll the menu.

Your rodent won't interfere with the normal operation. If you place the pointer
on the menu by mistake, simply move it outside of the menu to restore the
selection. You may, however, use the keys to manipulate an entry selected with
the mouse, in this case the key based selection sticks.

Actions are performed upon releasing a mouse butten. To cancel an action after
the button has been pressed, move it outside the menu window and release the
button (this goes for all the buttons).


=head1 EXAMPLES

How about creating a little remote shell menu? The B<ratmenu>ish approach would be

    ratmen --label Remotes xterm "" acme "rsh acme xterm" herman "rsh herman 9term" &

and to do it the B<9menu> way, type something like

    ratmen --label Remotes -d: xterm "acme:rsh acme xterm" "herman:rsh herman 9term" &

to do the trick. You could also make a menu containing some nice X programs to
run. Like this:

    ratmen --label "X progs" ghostview "" xdvi "" xeyes "" xneko "" &

Or like this:

    ratmen --label -d: "X progs" ghostview xdvi xeyes xneko &

That last one is a bit easier on the eyes, don't you think? If you want, you
can use the B<--back> to call an "earlier" menu, like this:

    ratmen --back ~/bin/mypreviousmenu "X Eyes" xeyes &

If you'd like to use a menu from within a shell script (a similar technique may
of course be employed from any other programming language, such as perl) you
could use the following:

    choice=`ratmen -pd: Abort Retry Ignore`

Now any of the options selected will be put into the environment variable
`$choice' (note, though, that this may also be empty if the user cancelled the

menu). Here B<-p> (or B<--print>) option is used to print the selected COMMAND
to standard out instead of running it, and B<-d> (or B<--delimiter>) is used
simply to avoid having to fill out the command line with a lot of ugly ""
arguments. (You could, of course, replace the colon in the command line with
any character that you don't use in the menu.)

And here are some lines from my F<~/.Xresources> file, for those interested:

! ratmen
ratmen*foreground:    yellow
ratmen*font:          -adobe-courier-medium-r-normal-*-18-*-*-*-m-*-iso8859-1
ratmen*unfocusExit:   true

This makes my menus easily distinguishable (since not much else is yellow in my
system configuration), easily readable (since I like courier) and doesn't
clutty my screen too much in case I happen to forget about them and go about
doing something else instead of choosing and item from the menu.


=head1 SEE ALSO

F</etc/X11/rgb.txt> where you may find the names of appropriate colours to use
with the B<--background> and B<--forground> options and X resources.


=head1 AUTHORS

The initial idea for this program was by Arnold Robbins, after having worked
with John Mackin's GWM Blit emulation. Matty Farrow wrote a version using
libXg, from which some ideas were borrowed. This code was written by David
Hogan and Arnold Robbins. Rich Salz motivated the B<-shell> option. Jonathan
Walther modified this code to play nicely with the ratpoison window manager by
removing handling of mouse events and iconification.

Zrajm C Akfogh <ratmen-mail@klingonska.org> changed command line syntax into
the more standard getopts, added scrolling capacity if menu is to large to fit
all at once, added B<--delimiter>, B<--item>, B<--print>, B<--scroll-offset>
and B<--unfocus-exit> options and X resource support (Yay! No need to specify
those longish font-thingies on the command line any more!).

The name `ratmen' is both an abbreviation of `ratmenu' (from which this program
is heavily derived) and a reference to the fact that *I* don't have any
religious reasons for not using the rodent (I like the keyboard, but I also
like freedom of choice).


=head1 FUTURE

I have not activelly made any changes to this program for several years, though
it was originally my intention to write a program that works both under X, and
in the console.

I later wrote B<termmen>, which much closer resembles my intentions for
B<ratmen>, but unfortunately only works in the console or terminal (i.e. does
not pop up a window of its own under X). Both B<termmen> and this program is
available from L<http://zrajm.klingonska.org/programs/>.

This program is written in C, which is not my native language, while B<Termmen>
is written as a zsh script. If anyone would like to continue development of
B<ratmen>, or a have a patch they'd like applied. Please feel free to send it
to me at ratmen-mail@klingonska.org.


=head1 HISTORY

[2003-02-21, 12:47-16:18] Implemented `--item' for choosing initially selected
item. Menu item and command is now given in the same string, separated with :,
which makes for nicer error detection and lesser errors since it is a bit
easier to keep track of what does which on the command line.

[2003-02-22, 15:38-16:43]

[2003-02-23, 02:19-02:36] Implemented `--delimiter' which now must be used to
get the "menuitem:command" (as opposed to "menuitem" "command") behaviour.
This, because I realised that when called from a script using an on-the-fly
generated menu it can be quite tedious to make sure that the delimiter does
not occur in the `menuitem' string (and thus fuck up the menu). Of course in
handwritten menus the delimiter approach is easier to handle, hence I allow
for both. Delimiter now also may be more than one character long.

[2003-02-23, 19:08-20:30] Implemented B<--unfocus-exit> which exits the menu if
it's window is unfocused.

[2003-02-23, 21:12-21:22]

[2003-02-24, 00:22-00:49]

[2003-02-24, 11:38-17:47] Now reads some options regulating appearance and
behaviour from X resources in addition to the command line.

[2003-02-25, 18:47-22:20] Wrote POD. At last found a really good name for the
product. The B<ratmen>.

[2003-02-26, 01:38-09:15] Now reads command line option using getopt; thusly
supports both --long-options and short ones (and bundles of short ones). Yay!
Still haven't been able to get the B<--unfocus-exit> to work properly.

[2003-02-26, 13:27-14:34] Fixxed B<--unfocus-exit> so that it now really makes
the menu die of unfocus. Unfortunately, however a `BadWindow' message is
generated by X when doing so, which looks quite ugly in B<ratpoison> (which
faithfully reports the error). I simply haven't been able to figure out what
command is causing this, but my best guess is that there's some unlucky X
function still queued which tries to manipulate the window after it has been
closed.

[2003-03-03, 00:13-01:54] Added the B<--print> option, which can be extremely
useful in scripts and other programs where you want to use a menu for some kind
of input, rather than for a platform from which to fire off some program. Added
L<"BUGS"> section below. Added the B<--debug> option which (as of now) dumps
some info on the events (key/mouse clicks focusing etc.) intercepted from X.

[2003-03-08, 11:21-11:29] Added a declaration of the subroutine `strcasecmp'
and thus eliminating a compiler warning. Program worked flawlessy even before
(and still does) but with less annoyance for me.. =|:-) The declaration thingy
was found on the 'net by googling for "strcasetmp introduction". Made arguments
to B<--align> and B<--style> case insensetive.

[2003-03-08, 20:27-22:58] Found and killed a bug which made X resources
override the command line options.

[2003-03-09, 07:04-10:15] Totally fixed the bug mentioned in previous comment.
Added short options B<-l>, B<-c> and B<-r> (for left, center and right aligning
of the menu text), and changed B<--label> to B<--title>. Also changed the size
of the menu window so that it will always be an even number of text lines
(looks pretty ogly when only half or a third of the last item can be seen).
Opening of window could be still more optimal though (the problem still arises
when X refuses to open the window with the requested height, e.g. when one uses
frames in ratpoison).

[2003-03-10, 00:53-04:07] v2.0 - Optimized menu redrawing routine for the
non-scrolling case (used to flicker quite nastily when moving down large menus
fast). Also finally got around to add scrolling capacity for menus too large to
fit all at once. Added the related B<--scroll-offset> option and X resource as
well.

[2003-03-10, 22:10-23:00] v2.0.1 - Begun fixxing window size bug (interrupted
by Buffy - The Vampire Slayer).

[2003-03-11, 00:31-04:25]

[2003-03-12, 00:13-01:14] v2.0.2 - Fixed weird update/redraw bug thingy. Used
to get a totally black empty menu sometimes. Bug probably introduced in last
session, and now deceased.

[2003-03-19, 10:48-12:38]

[2003-03-19, 14:17-20:18] v2.1 - Wrote better handling of X resources, comlete
with readable B<--debug> output and all.

[2003-03-19, 22:40-09:05] v2.2 - Implemented mouse support. Made B<--item=0>
special. Bugfix: Now opens a window of the correct size by telling X that it
wants a window whose vertical size should be any number of pixels evenly
dividable by the font height. (B<Rxvt> served as a source of inspiration for
this feature.) Also added keys Home/PgUp for going to the first item in the
menu and End/PgDn to go to the last. B<--class> now really works.

[2003-03-24, 15:41-16:32] v2.2.1 - Added options B<--mouse> and B<--no-mouse>
and the corresponding X resource B<ratmen.mouse> -- mouse is enabled by
default. Removed the short options B<-u> and B<-U> (synonyms for
B<--unfocus-exit> and B<--no-unfocus-exit>) as to not encourage the
useage of those options on the command line (should be set using X resources).

[2003-03-28, 11:29-11:32] v2.2.2 - Killed bug which didn't allow spaces to be
used in font names (this little bugfix also makes it quite necessary not to end
your X resources in any extraneous spaces).

[2007-10-25, 11:23-14:13] v2.2.3 -- Bugfix. Changed a couple of latin-1
characters 173 (soft hyphens) to the minus signs the should have been all the
time. Added the "FUTURE" heading above. Rewrote the Makefile.


=head1 BUGS

=over 4

=item o

When no item is selected (i.e. on startup with -i0) and you hover the menu with
the pointer, and then remove the pointer, the first item becomes selected.


=item o

The B<snazzy> mode. I'm probably going to remove it sometime in the future.
B<I> don't use it and I don't see any reason why anybody else should want to
either... (Especially not now that we got scrollable B<dreary> mode..)


=item o

Generates a `BadWindow' when dying of unfocus under the B<--unfocus-exit>
option.

This should be curable by means of XSetErrorHandler(), at least if it is not
B<ratpoison> causing these things.


=item o

Memory leaks? There are probably several (me being totally new to C) but since
the execution time should never really accumulate I haven't made it a priority
to kill 'em. Please tell me if you find any.


=item o

When using B<--item=0> and B<--style=snazzy> one must press arrow down *twice*
to select the first item on the menu. (I'll probably fix this by removing the
`snazzy' mode.)

=back


=head1 TODO

These are the things a want to do next in approximate order of priority:


=over 4

=item o

Parsing of standard input (so one may use B<ratmen> as a magic number) for
interpreting menu files. If this is to work, maybe some special command line
argument parsing is needed? (I strongly suspect that magic number arguments are
not passed in the same fashion, as if given on the command line..)


=item o

Option for outputting number of selected menu item (B<--print-number>/B<-P>
maybe?).


=item o

Change B<--print> behaviour to include B<-d>: or something automatically (so
that menu item text is default output).


=item o

User-configurable keys for up/down choose etc.

=item o

Menu shortcuts (defined in the menu) -- if autogenerated shortcuts are good
enough, this might not be needed.


=item o

Automatically generated key shortcuts (E.g. numbers 0-9 for items 1-10, or
letter 'a' letter to loop through all items begginning with an 'a', or "/" and
"?" to do incremental forward/backward search...)


=item o

Multiple selection mode (space=select; enter=finalize) (introduce colors for
marked entry and marked & selected entry).


=item o

Incremental search in menu (using bottom item space for input).


=item o

Replacing top/bottom item with arrow (or something) if scrollable in that
direction.

=back

=cut

*/
