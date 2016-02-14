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
        XrmGetResource(resourceDb, "vmenu*align", "", &tmp, &value)
    ) {
        tmp = value.addr;
        if ((cp = strchr(tmp, ' '))) *cp++ = '\0';
        if      (strcasecmp(tmp, "left"  ) == 0) align = left;
        else if (strcasecmp(tmp, "center") == 0) align = center;
        else if (strcasecmp(tmp, "right" ) == 0) align = right;
    }

    |* background: BGCOLOR *|
    if (bgcname == NULL &&
        XrmGetResource(resourceDb, "vmenu*background", "", &tmp, &value)
    ) {
        tmp = value.addr;
        if ((cp = strchr(tmp, ' '))) *cp++ = '\0';
        if (tmp) bgcname = tmp;
    }

    |* font: FNAME *|
    if (fontname == NULL &&
        XrmGetResource(resourceDb, "vmenu*font", "", &tmp, &value)
    ) {
        tmp = value.addr;
        if ((cp = strchr(tmp, ' '))) *cp++ = '\0';
        if (tmp) fontname = tmp;
    }

    |* foreground: FGCOLOR *|
    if (fgcname == NULL &&
        XrmGetResource(resourceDb, "vmenu*foreground", "", &tmp, &value)
    ) {
        tmp = value.addr;
        if ((cp = strchr(tmp, ' '))) *cp++ = '\0';
        if (tmp) fgcname = tmp;
    }

    |* style: {dreary|snazzy} *|
    if (style == Undef &&
        XrmGetResource(resourceDb, "vmenu*style", "", &tmp, &value)
    ) {
        tmp = value.addr;
        if ((cp = strchr(tmp, ' '))) *cp++ = '\0';
        if      (strcasecmp(tmp, "dreary") == 0) style = dreary;
        else if (strcasecmp(tmp, "snazzy") == 0) style = snazzy;
    }

    |* scrollOffset: ITEMS *|
    if (scroll_offset == Undef &&
        XrmGetResource(resourceDb, "vmenu*scrollOffset", "", &tmp, &value)
    ) {
        tmp = value.addr;
        if ((cp = strchr(tmp, ' '))) *cp++ = '\0';
        scroll_offset = atoi(tmp);
    }

    |* unfocusExit: {on|yes|true|off|no|false} *|
    if (unfocus_exit == Undef &&
        XrmGetResource(resourceDb, "vmenu*unfocusExit", "", &tmp, &value)
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

                    case XK_d:
                        if (event.xkey.state == 4)
                        {
                            (cur+=visible_items/4) >= numitems? cur=numitems-1 : 0;
                            cur_store = cur;
                            redraw(cur, high, wide);
                        }
                        break;

                    case XK_u:
                        if (event.xkey.state == 4)
                        {
                            (cur-=visible_items/4) <= 0? cur=0 : 0;
                            cur_store = cur;
                            redraw(cur, high, wide);
                        }
                        break;

                    case XK_f:
                        if (event.xkey.state == 4)
                        {
                            (cur+=visible_items) >= numitems? cur=numitems-1 : 0;
                            cur_store = cur;
                            redraw(cur, high, wide);
                        }
                        break;

                    case XK_b:
                        if (event.xkey.state == 4)
                        {
                            (cur-=visible_items) <= 0? cur=0 : 0;
                            cur_store = cur;
                            redraw(cur, high, wide);
                        }
                        break;

                    case XK_g:
                        cur = 0;
                        cur_store = cur;
                        redraw(cur, high, wide);
                        break;

                    case XK_G:
                        cur = numitems-1;
                        cur_store = cur;
                        redraw(cur, high, wide);
                        break;

                    case XK_H:
                        if (last_top < 0)
                        {
                            cur = 0;
                        }
                        else
                        {
                            cur = last_top;
                        }
                        cur_store = cur;
                        redraw(cur, high, wide);
                        break;

                    case XK_M:
                        cur = last_top+(visible_items/2);
                        cur_store = cur;
                        redraw(cur, high, wide);
                        break;

                    case XK_L:
                        cur = last_top+(visible_items-1);
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
    classhints->res_class  = "vmenu";

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
