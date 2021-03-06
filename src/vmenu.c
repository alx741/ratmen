#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
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
#include <stdbool.h>
#include <stddef.h>
#include "config.h"
#include "misc.h"
#include "args.h"

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


/* menu structure */
char **labels;                                 /* list of menu labels */
char **commands;                               /* list of menu commands */
int numitems;                                  /* total number of items */
int visible_items;                             /* number of items visible */


/* program determined settings */
int last_item     = -1;                        /* previously selected item */
int last_top      = -1;                        /* previous top item in menu */
bool full_redraw   =  true;                     /* redraw all menu items */
int cur_scroll_offset = 0;



/* function prototypes */
void ask_wm_for_delete(void);
void reap(int);
void redraw(int, int, int);
void redraw_mouse(int, int, int);

void run_menu(int);
void set_wm_hints(int, int, int);
void spawn(char *);


/*
 * Event loop monitor thingy (use option `--debug') stolen from:
 * http://www-h.eng.cam.ac.uk/help/tpl/graphics/X/X11R5/node41.html
 */
static char *event_names[] =
{
    "", "", "KeyPress", "KeyRelease", "ButtonPress", "ButtonRelease",
    "MotionNotify", "EnterNotify", "LeaveNotify", "FocusIn", "FocusOut",
    "KeymapNotify", "Expose", "GraphicsExpose", "NoExpose", "VisibilityNotify",
    "CreateNotify", "DestroyNotify", "UnmapNotify", "MapNotify", "MapRequest",
    "ReparentNotify", "ConfigureNotify", "ConfigureRequest", "GravityNotify",
    "ResizeRequest", "CirculateNotify", "CirculateRequest", "PropertyNotify",
    "SelectionClear", "SelectionRequest", "SelectionNotify", "ColormapNotify",
    "ClientMessage", "MappingNotify"
};




char *xresource_if(int test, Display *dpy, char *progname, char *resource)
{
    /* char *cut = ""; */
    char *tmp = "";
    if (test)
    {
        tmp = XGetDefault(dpy, PACKAGE, resource);
        if (tmp != NULL)                       /* found resource */
        {
            /*
            if ((cut = strchr(tmp, ' ')))      |* trunc at 1st space *|
                *cut++ = '\0';
            */
            if (opts.debug == true)
                fprintf(stderr, "  %s.%-12s: >%s<\n",
                        PACKAGE, resource, tmp);
            return tmp;
        }
        else if (opts.debug == true)                /* no resource found */
            fprintf(stderr, "  %s.%-12s: [not defined in X resources]\n",
                    PACKAGE, resource);
    }
    else if (opts.debug == true)                    /* given on command line */
        fprintf(stderr, "  %s.%-12s: [overriden by command line]\n",
                PACKAGE, resource);
    return NULL;
}


/* completely written by Zrajm */
void xresources(Display *dpy)
{
    char *tmp = "";
    if (opts.debug == true)
    {
        fprintf(stderr, "Reading X resources:\n");
    }

    /* align: {left|center|right} */
    tmp = xresource_if((opts.align == a_undef), dpy, opts.classname, "align");
    if (tmp != NULL)
    {
        if (strcasecmp(tmp, "left") == 0)
        {
            opts.align = left;
        }
        else if (strcasecmp(tmp, "center") == 0)
        {
            opts.align = center;
        }
        else if (strcasecmp(tmp, "right") == 0)
        {
            opts.align = right;
        }
    }


    /* background: BGCOLOR */
    tmp = xresource_if((opts.bgcname == NULL), dpy, opts.classname, "background");
    if (tmp != NULL)
    {
        opts.bgcname = tmp;
    }


    /* font: FNAME */
    tmp = xresource_if((opts.fontname == NULL), dpy, opts.classname, "font");
    if (tmp != NULL)
    {
        opts.fontname = tmp;
    }


    /* foreground: FGCOLOR */
    tmp = xresource_if((opts.fgcname == NULL), dpy, opts.classname, "foreground");
    if (tmp != NULL)
    {
        opts.fgcname = tmp;
    }


    /* mouse: {on|yes|true|off|no|false} */
    tmp = xresource_if((opts.mouse_on == false), dpy, opts.classname, "mouse");
    if (tmp != NULL)
    {
        if (strcasecmp(tmp, "on")  == 0)
        {
            opts.mouse_on = true;
        }
        else if (strcasecmp(tmp, "yes")   == 0)
        {
            opts.mouse_on = true;
        }
        else if (strcasecmp(tmp, "true")  == 0)
        {
            opts.mouse_on = true;
        }
        else if (strcasecmp(tmp, "off")   == 0)
        {
            opts.mouse_on = false;
        }
        else if (strcasecmp(tmp, "no")    == 0)
        {
            opts.mouse_on = false;
        }
        else if (strcasecmp(tmp, "false") == 0)
        {
            opts.mouse_on = false;
        }
    }


    /* /1* style: {dreary|snazzy} *1/ */
    /* tmp = xresource_if((opts.style == s_undef), dpy, opts.classname, "style"); */
    /* if (tmp != NULL) */
    /* { */
    /*     if (strcasecmp(tmp, "dreary") == 0) */
    /*     { */
    /*         opts.style = dreary; */
    /*     } */
    /*     else if (strcasecmp(tmp, "snazzy") == 0) */
    /*     { */
    /*         opts.style = snazzy; */
    /*     } */
    /* } */


    /* scrollOffset: ITEMS */
    tmp = xresource_if((opts.scroll_offset == 0), dpy, opts.classname, "scrollOffset");
    if (tmp != NULL)
    {
        opts.scroll_offset = atoi(tmp);
    }


    /* unfocusExit: {on|yes|true|off|no|false} */
    tmp = xresource_if((opts.unfocus_exit == false), dpy, opts.classname, "unfocusExit");
    if (tmp != NULL)
    {
        if (strcasecmp(tmp, "on")  == 0)
        {
            opts.unfocus_exit = true;
        }
        else if (strcasecmp(tmp, "yes")   == 0)
        {
            opts.unfocus_exit = true;
        }
        else if (strcasecmp(tmp, "true")  == 0)
        {
            opts.unfocus_exit = true;
        }
        else if (strcasecmp(tmp, "off")   == 0)
        {
            opts.unfocus_exit = false;
        }
        else if (strcasecmp(tmp, "no")    == 0)
        {
            opts.unfocus_exit = false;
        }
        else if (strcasecmp(tmp, "false") == 0)
        {
            opts.unfocus_exit = false;
        }
    }
}


void items(int start, int count, char **arg)
{
    int j;
    char *cut;

    if (opts.delimiter)                             /* using delimiter */
    {
        /* do things similar to `9menu' */
        if (count - start < 1)
        {
            die("not enough arguments");
        }

        numitems = count - start;
        labels   = (char **) malloc(numitems * sizeof(char *));
        commands = (char **) malloc(numitems * sizeof(char *));
        if (commands == NULL || labels == NULL)
        {
            die("cannot allocate memory for command and label arrays");
        }

        for (j = 0; j < numitems; j ++)
        {
            labels[j] = arg[start + j];
            if ((cut  = strstr(labels[j], opts.delimiter)) != NULL)
            {
                *cut  = '\0';
                cut += strlen(opts.delimiter);
                commands[j] = cut;
            }
            else
            {
                commands[j] = labels[j];
            }
        }
        return;
    }
    else                                       /* without delimiter */
    {
        if ((count - start) % 2 != 0)
        {
            die("not an even number of menu arguments");
        }
        if ((count - start) * 2 <  1)
        {
            die("not enough arguments");
        }

        numitems = (count - start) / 2;
        labels   = (char **) malloc(numitems * sizeof(char *));
        commands = (char **) malloc(numitems * sizeof(char *));
        if (commands == NULL || labels == NULL)
        {
            die("cannot allocate memory for command and label arrays");
        }

        for (j = 0; start < count; j ++)
        {
            labels[j]   = arg[start++];
            commands[j] = arg[start++];
            if (strcmp(commands[j], "") == 0)
            {
                commands[j] = labels[j];
            }
        }
        return;
    }
}


int HandleXError(Display *dpy, XErrorEvent *event)
{
    /*    gXErrorFlag = 1;*/
    return 0;
}




/* main --- crack arguments, set up X stuff, run the main menu loop */
int main(int argc, char **argv)
{
    int i;
    XGCValues gv;
    unsigned long mask;
    g_argc = argc;                             /* save command line args */
    g_argv = argv;


    /* set defaults for non-resource options */
    opts.titlename = PACKAGE;                      /* default window title */
    opts.classname = PACKAGE;                      /* default X resource class */
    i = parse_args(argc, argv);                      /* process command line args */
    items(i, argc, argv);                      /* process menu items */

    dpy = XOpenDisplay(opts.displayname);
    if (dpy == NULL)
    {
        fprintf(stderr, "%s: cannot open display", PACKAGE);
        if (opts.displayname != NULL)
        {
            fprintf(stderr, " %s", opts.displayname);
        }
        fprintf(stderr, "\n");
        exit(1);
    }

    /* make it ignore BadWindow errors */
    XSetErrorHandler(HandleXError);
    XFlush(dpy);

    /* use Xresource for undefined values */
    xresources(dpy);

    /* defaults (for undefined cases) */
    if (opts.fontname == NULL)
    {
        opts.fontname = FONT;    /* default font */
    }
    if (opts.scroll_offset == 0)                /* default scroll offset */
    {
        opts.scroll_offset = 3;
    }
    if (opts.mouse_on == false)                     /* mouse menu selection */
    {
        opts.mouse_on = true;
    }

    cur_scroll_offset = opts.scroll_offset;

    screen         = DefaultScreen(dpy);
    root           = RootWindow(dpy, screen);
    display_height = DisplayHeight(dpy, screen);

    dcmap = DefaultColormap(dpy, screen);
    if (opts.fgcname == NULL ||
            XParseColor(dpy, dcmap, opts.fgcname, &color) == 0 ||
            XAllocColor(dpy, dcmap, &color) == 0
       )
    {
        fg = WhitePixel(dpy, screen);
    }
    else
    {
        fg = color.pixel;
    }

    if (opts.bgcname == NULL ||
            XParseColor(dpy, dcmap, opts.bgcname, &color) == 0 ||
            XAllocColor(dpy, dcmap, &color) == 0
       )
    {
        bg = BlackPixel(dpy, screen);
    }
    else
    {
        bg = color.pixel;
    }

    if ((font = XLoadQueryFont(dpy, opts.fontname)) == NULL)
    {
        fprintf(stderr, "%s: fatal: cannot load font `%s'\n", PACKAGE, opts.fontname);
        exit(1);
    }

    gv.foreground = fg ^ bg;
    gv.background = bg;
    gv.font = font->fid;
    gv.function = GXxor;
    gv.line_width = 0;
    mask = GCForeground | GCBackground | GCFunction | GCFont | GCLineWidth;
    gc = XCreateGC(dpy, root, mask, &gv);

    signal(SIGCHLD, reap);

    run_menu(opts.cur_item);

    /* XCloseDisplay() cannot generate BadWindow */
    XCloseDisplay(dpy);
    exit(0);
}


/* spawn --- run a command */
void spawn(char *com)
{
    int pid;
    static char *sh_base = NULL;

    if (sh_base == NULL)
    {
        sh_base = strrchr(opts.shell, '/');
        if (sh_base != NULL)
        {
            sh_base++;
        }
        else
        {
            sh_base = opts.shell;
        }
    }

    pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "%s: can't fork\n", PACKAGE);
        return;
    }
    else if (pid > 0)
    {
        return;
    }

    close(ConnectionNumber(dpy));
    execl(opts.shell, sh_base, "-c", com, NULL);
    _exit(1);
}


/* reap --- collect dead children */
void reap(int s)
{
    (void) wait((int *) NULL);
    signal(s, reap);
}






/* run_menu --- put up the window, execute selected commands */
void run_menu(int cur)
{
    KeySym key;
    XEvent event;
    XClientMessageEvent *cmsg;
    bool mousing = false;
    int i, wide, high, dx, dy, cur_store = cur;

    /* Currently selected menu item */
    if (cur  <  -1)
    {
        cur = 0;
    }
    if (cur  >= numitems)
    {
        cur = numitems - 1;
    }

    /* find widest menu item */
    dx = 0;
    for (i = 0; i < numitems; i++)
    {
        wide = XTextWidth(font, labels[i], strlen(labels[i])) + 4;
        if (wide > dx)
        {
            dx = wide;
        }
    }
    wide = dx;

    high = font->ascent + font->descent + 1;
    dy   = numitems * high;                    /* height of menu window */
    if (dy > display_height)                   /* shrink if outside screen */
    {
        dy = display_height - (display_height % high);
    }

    visible_items = dy / high;

    if (opts.debug == true)
    {
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
                 (opts.mouse_on == true ? MenuMask : MenuMaskNoMouse));

    XMapWindow(dpy, menuwin);



    for (;;)
    {
        /* BadWindow not mentioned in doc */
        XNextEvent(dpy, &event);
        if (opts.debug == true)
            fprintf(stderr, "X event: %s\n",
                    event_names[event.type]); /* DEBUG thingy */
        switch (event.type)
        {

            case EnterNotify:
                if (mousing == true)
                {
                    break;
                }
                cur_store = cur;
                mousing = true;
                break;

            case LeaveNotify:
                if (mousing == false)
                {
                    break;
                }
                /* fprintf(stderr, "going out\n"); */
                cur = cur_store;
                redraw_mouse(cur, high, wide);
                mousing = false;
                break;

            case ButtonRelease:
                if (mousing == false)
                {
                    break;
                }
                if (event.xbutton.button == Button1)   /* left button */
                {
                    if (opts.output == print)
                    {
                        printf("%s\n", commands[cur]);
                    }
                    else
                    {
                        spawn(commands[cur]);
                    }
                    return;
                }
                else if (event.xbutton.button == Button3)     /* right */
                {
                    if (opts.prevmenu)
                    {
                        if (opts.output == print)
                        {
                            printf("%s\n", opts.prevmenu);
                        }
                        else
                        {
                            spawn(opts.prevmenu);
                        }
                        return;
                    }
                }
                else
                {
                    return;    /* middle button */
                }
                break;


            case ButtonPress:
            case MotionNotify:
                if (mousing == false)
                {
                    break;
                }
                cur = event.xbutton.y / high + last_top;
                redraw_mouse(cur, high, wide);
                break;


            case KeyPress:                     /* key is pressed in win */
                /* BadWindow not mentioned in doc */
                XLookupString(&event.xkey, NULL, 0, &key, NULL);
                switch (key)
                {
                    case XK_Escape:
                    case XK_q:
                        return;
                        break;

                    case XK_Left:
                    case XK_h:
                        if (opts.prevmenu)
                        {
                            if (opts.output == print)
                            {
                                printf("%s\n", opts.prevmenu);
                            }
                            else
                            {
                                spawn(opts.prevmenu);
                            }
                            return;
                        }
                        break;

                    case XK_Right:
                    case XK_Return:
                    case XK_l:
                        if (opts.output == print)
                        {
                            printf("%s\n", commands[cur]);
                        }
                        else
                        {
                            spawn(commands[cur]);
                        }
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
                            (cur += visible_items / 4) >= numitems ? cur = numitems - 1 : 0;
                            cur_store = cur;
                            redraw(cur, high, wide);
                        }
                        break;

                    case XK_u:
                        if (event.xkey.state == 4)
                        {
                            (cur -= visible_items / 4) <= 0 ? cur = 0 : 0;
                            cur_store = cur;
                            redraw(cur, high, wide);
                        }
                        break;

                    case XK_f:
                        if (event.xkey.state == 4)
                        {
                            (cur += visible_items) >= numitems ? cur = numitems - 1 : 0;
                            cur_store = cur;
                            redraw(cur, high, wide);
                        }
                        break;

                    case XK_b:
                        if (event.xkey.state == 4)
                        {
                            (cur -= visible_items) <= 0 ? cur = 0 : 0;
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
                        cur = numitems - 1;
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
                        cur = last_top + (visible_items / 2);
                        cur_store = cur;
                        redraw(cur, high, wide);
                        break;

                    case XK_L:
                        cur = last_top + (visible_items - 1);
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
                        cur = numitems - 1;
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
                {
                    break;
                }
                new_height = gaga.height - (gaga.height % high);
                visible_items = new_height / high;
                cur_scroll_offset = (visible_items - 1) / 2;
                if (cur_scroll_offset > opts.scroll_offset)
                {
                    cur_scroll_offset = opts.scroll_offset;
                }
                if (opts.debug == true)
                {
                    fprintf(stderr, "  Window resized/moved:\n"
                            "    Window height (old/new):%4d /%4d\n"
                            "    Visible rows  (old/new):%4d /%4d\n"
                            "    Scroll offset (cmd/cur):%4d /%4d\n",
                            gaga.height,      new_height,
                            gaga.height / high, new_height / high,
                            opts.scroll_offset,    cur_scroll_offset);
                }
            }
            break;

            case UnmapNotify:                  /* win becomes hidden */
                if (opts.unfocus_exit == true)
                {
                    return;
                }
                else
                    /* can generate BadWindow */
                {
                    XClearWindow(dpy, menuwin);
                }
                break;

            /* FIXME: For some reason the code in `FocusIn' or `FocusOut' results in an
             * "Error: BadWindow (invalid Window parameter)" message in Ratpoison
             * (but it works, even if it look ugly).
             */
            case FocusIn:
                break;               /* win becomes focused again */
            case FocusOut:                     /* win becomes unfocused   */
                if (opts.unfocus_exit == true)
                {
                    XDestroyWindow(dpy, menuwin);
                    return;
                }
                break;

            case MapNotify:                    /* win becomes visible again */

            {
                /* %%%% */
                XWindowAttributes gaga;
                int new_height = 0;
                if (XGetWindowAttributes(dpy, menuwin, &gaga))
                {
                    new_height = gaga.height - (gaga.height % high);
                    if (gaga.height != new_height)
                    {
                        if (opts.debug == true)
                        {
                            fprintf(stderr, "RESIZING window to:\n"
                                    "  Old  height: %d\n"
                                    "  Font height: %d\n"
                                    "  New  height: %d\n",
                                    gaga.height, high, new_height);
                        }
                        /* FIXME window doesn't get resized, why not? */
                        XResizeWindow(dpy, menuwin, 100, new_height);
                        XSync(dpy, true);

                        if (XGetWindowAttributes(dpy, menuwin, &gaga))
                        {
                            if (opts.debug == true)
                            {
                                fprintf(stderr, "Window height after resizing: %d\n",
                                        gaga.height);
                            }
                        }
                        visible_items = gaga.height / high;
                        cur_scroll_offset = (visible_items - 1) / 2;
                        if (cur_scroll_offset > opts.scroll_offset)
                        {
                            cur_scroll_offset = opts.scroll_offset;
                        }
                        else
                            fprintf(stderr, "Shrinking scroll offset from %d to %d.\n",
                                    opts.scroll_offset, cur_scroll_offset);
                    }
                }
                else
                {
                    fprintf(stderr, "can't get window attributes\n");
                }

                /* make sure window isn't taller than screen */
                /* FIXME: problems when used inside a ratpoison frame */

            }
            case Expose:                       /* win becomes partly covered */
                full_redraw = true;
                redraw(cur, high, wide);
                /* `while' skips immediately subsequent Expose events in queue,
                 * which avoids superflous window redraws; taken from:
                 * www-h.eng.cam.ac.uk/help/tpl/graphics/X/X11R5/node31.html)*/
                while (XCheckTypedWindowEvent(dpy, menuwin, Expose, &event))
                    ;
                break;

            case ClientMessage:
                cmsg = &event.xclient;
                if (cmsg->message_type == wm_protocols
                        && cmsg->data.l[0] == wm_delete_window)
                {
                    return;
                }
        }
    }
}


/* set_wm_hints --- set all the window manager hints */
void set_wm_hints(int wide, int high, int font_height)
{
    XWMHints *wmhints;
    static XSizeHints sizehints =
    {
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

    if ((wmhints = XAllocWMHints()) == NULL)
    {
        fprintf(stderr, "%s: cannot allocate window manager hints\n",
                PACKAGE);
        exit(1);
    }

    if ((classhints = XAllocClassHint()) == NULL)
    {
        fprintf(stderr, "%s: cannot allocate class hints\n",
                PACKAGE);
        exit(1);
    }

    /* window width */
    sizehints.width      = sizehints.base_width  =
                               sizehints.min_width  = sizehints.max_width   = wide;

    /* window height */
    /* (always an even number of font heights) */
    sizehints.height     = sizehints.base_height = high;
    sizehints.min_height = sizehints.height_inc  = font_height;
    sizehints.max_height = numitems * font_height;

    if (XStringListToTextProperty(&opts.titlename, 1, &wname) == 0)
    {
        fprintf(stderr, "%s: cannot allocate window name structure\n",
                PACKAGE);
        exit(1);
    }

    /* open menu window */
    menuwin = XCreateSimpleWindow(dpy, root, sizehints.x, sizehints.y,
                                  sizehints.width, sizehints.height, 1, fg, bg);

    wmhints->input         = true;
    wmhints->initial_state = NormalState;
    wmhints->flags         = StateHint | InputHint;

    classhints->res_name   = PACKAGE;
    classhints->res_class  = "vmenu";

    XSetWMProperties(dpy, menuwin, &wname, NULL,
                     g_argv, g_argc, &sizehints, wmhints, classhints);
}

/* ask_wm_for_delete --- jump through hoops to ask WM to delete us */
void ask_wm_for_delete(void)
{
    int status;

    wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", false);
    wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", false);
    status = XSetWMProtocols(dpy, menuwin, &wm_delete_window, 1);

    if (status != true)
        fprintf(stderr, "%s: cannot ask for clean delete\n",
                PACKAGE);
}


void redraw_mouse(int cur_item, int high, int wide)
{
    if (cur_item <= last_top)
    {
        cur_item = last_top;
    }
    else if (cur_item > last_top + visible_items)
    {
        cur_item = last_top + visible_items;
    }
    if (cur_item == last_item)
    {
        return;    /* no movement = no update */
    }
    XFillRectangle(dpy, menuwin, gc, 0, (last_item - last_top) * high, wide, high);
    XFillRectangle(dpy, menuwin, gc, 0, (cur_item  - last_top) * high, wide, high);
    if (opts.debug == true)
        fprintf(stderr, "current item: %d (of %d-%d)\n",
                cur_item, last_top, last_top + visible_items);
    last_item = cur_item;
}


void redraw(int cur_item, int high, int wide)
{
    int i, j, ty, tx;
    int cur_top = last_top;                    /* local top var */

    if (cur_item == last_item && full_redraw != true)
    {
        return;    /* no movement = no update */
    }

    /* change top item (i.e. scroll menu) */
    if (cur_item < last_top + cur_scroll_offset)   /* scroll upward */
    {
        cur_top  = cur_item - cur_scroll_offset;
        if (cur_top < 0)
        {
            cur_top = 0;    /* don't go above menu */
        }
    }
    else if (                                  /* scroll downward */
        cur_item > last_top - (cur_scroll_offset + 1) + visible_items
    )
    {
        cur_top  = cur_item + (cur_scroll_offset + 1) - visible_items;
        if (cur_top > numitems - visible_items)/* don't go below menu */
        {
            cur_top = numitems - visible_items;
        }
    }
    else
    {
        if (cur_top < 0)
        {
            cur_top = 0;    /* don't go above menu */
        }
    }


    if (full_redraw == true || cur_top != last_top)
    {
        /* redraw all visible items */
        if (opts.debug == true)
        {
            fprintf(stderr, "  (full menu redraw)\n");
        }
        XClearWindow(dpy, menuwin);
        for (i = 0; i < visible_items; i++)
        {
            j = cur_top + i;
            if (opts.align == right)
            {
                tx = wide - XTextWidth(font, labels[j], strlen(labels[j]));
            }
            else if (opts.align == center)
            {
                tx = (wide - XTextWidth(font, labels[j], strlen(labels[j]))) / 2;
            }
            else     /* align == left */
            {
                tx = 0;
            }
            ty = i * high + font->ascent + 1;
            XDrawString(dpy, menuwin, gc, tx, ty, labels[j], strlen(labels[j]));
        }
        if (cur_item >= 0)
        {
            XFillRectangle(dpy, menuwin, gc, 0, (cur_item - cur_top) * high, wide, high);
        }
        last_top    = cur_top;
        full_redraw = false;
    }
    else
    {
        /* only invert last & current item */
        if (opts.debug == true)
        {
            fprintf(stderr, "  (partial menu redraw)\n");
        }
        j =  cur_item -  cur_top;
        i = last_item - last_top;
        XFillRectangle(dpy, menuwin, gc, 0, (last_item - cur_top) * high, wide, high);
        if (cur_item >= 0)
        {
            XFillRectangle(dpy, menuwin, gc, 0, (cur_item  - cur_top) * high, wide, high);
        }
    }
    last_item = cur_item;
}
