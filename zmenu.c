// zmenu - a small, simpler dmenu clone
#include <X11/X.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/Xrender.h>

////////// USER CONFIG //////////

// Frosty is a fork of terminus
static const char *FONT = "Frosty:pixelsize=14";
static const char *PROMPT = ">> ";

// Background color
static const char *BG = "#1d2021";
// Foreground color
static const char *FG = "#ebdbb2";
// Background highlighted color
static const char *BGH = "#ebdbb2";
// Foreground highlighted color
static const char *FGH = "#1d2021";
//static const char *BGH = "#282828";
//static const char *FGH = "#b8bb26";

// Window width
static const int WIDTH = 400;
// Window height
static const int HEIGHT = 40;
// -1 = Left | 0 = Center | 1 = Right
static  const int PX = 0;
// -1 = Top | 0 = Center | 1 = Bottom
static const int PY = 0;
////////// USER CONFIG //////////

// Globals //

// X11
static Display *dp;
static int sc;
static Window rt, win;
static GC gc;
static Visual *visual;
static Colormap colormap;
static XEvent ev;
static unsigned long bg, bgh;
static int wx, wy;
static bool running = true;

// Items list
typedef struct Item Item;
struct Item { char *text; };
static Item items[BUFSIZ];

static char inputBuffer[BUFSIZ] = "";
static int inputLen = 0;
static int numItems = 0;
static int selindex = 0;

// Xft
static XftDraw *draw;
static XftFont *font;
static XftColor fg, fgh;


//// CODE STARTS HERE ////

// Prints an error to stderr, given the arguments:
// errorName, the name of the error and checkExit
// if want to call exit
static void printError(const char errorName[], bool checkExit) 
{
    fprintf(stderr, "zmenu error: %s.\n", errorName);
    if (checkExit)
        exit(1);
}

// Allocates a color in unsigned long format
// given the argument hex, and hex color code
static unsigned long getColor(const char *hex) 
{
    XColor color;
    if (!XAllocNamedColor(dp, colormap, hex, &color, &color))
        printError("Cannot allocate color", false);
    return color.pixel;
}

// Return the position, given the arguments:
// int user, the user wanted positon, and
// int wh, the size of the window (width or height)
static int position(int user, int wh)
{
    int axis;
    switch (user)
    {
        case 1:
            if (wh != WIDTH)
                axis = DisplayHeight(dp, sc) - wh;
            else
                axis = DisplayWidth(dp, sc) - wh;
            break;
        case 0:
            if (wh != WIDTH)
                axis = (DisplayHeight(dp, sc) - wh) / 2;
            else
                axis = (DisplayWidth(dp, sc) - wh) / 2;
            break;
        case -1:
            axis = 0;
            break;
        default:
            break;
    }
    return axis;
}

// Frees all the resources
static void cleanup(void) 
{
    for (int i = 0; i < numItems; i++) 
        free(items[i].text);

    XftDrawDestroy(draw);
    XftColorFree(dp, visual, colormap, &fg);
    XftColorFree(dp, visual, colormap, &fgh);
    XftFontClose(dp, font);

    XUngrabKeyboard(dp, CurrentTime);
    XFreeGC(dp, gc);
    XDestroyWindow(dp, win);
    XCloseDisplay(dp);
}

// Read the stdin separes it into lines
// and adds it to the array of items
static void readStdin(void)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, stdin)) != -1)
    {
        if (line[read - 1] == '\n')
            line[read - 1] = '\0';

        items[numItems].text = strdup(line);
        numItems++;
    }
    free(line);
}

static void drawMenu(void)
{
    int textx = 10;
    int texty = 10;

    // Draw the inputBuffer
    XftDrawStringUtf8(draw, &fg, font, textx, texty, (XftChar8 *)inputBuffer, sizeof(inputBuffer));

}

static void keysHandler(void)
{

}

// Manages window attributes
static void winSetup(void)
{
    XSetWindowAttributes wa;
    wx = position(PX, WIDTH);
    wy = position(PY, HEIGHT);

    wa.override_redirect = True;
    wa.background_pixel = bg;
    wa.event_mask = ExposureMask | KeyPressMask;

    win = XCreateWindow(dp, rt, wx, wy, WIDTH, HEIGHT, 0, DefaultDepth(dp, sc), CopyFromParent, 
                        visual, CWOverrideRedirect | CWBackPixel | CWEventMask, &wa);
}

// Initialize variables
static void setup(void) 
{
    sc = DefaultScreen(dp);
    rt = RootWindow(dp, sc);
    visual = DefaultVisual(dp, sc);
    colormap = DefaultColormap(dp, sc);
    
    bg = getColor(BG);
    bgh = getColor(BGH);

    winSetup();

    gc = XCreateGC(dp, win, 0, NULL);
    XSetForeground(dp, gc, bgh);
    
    if (!(font = XftFontOpenName(dp, sc, FONT)))
        printError("Cannot load font", true);

    draw = XftDrawCreate(dp, win, visual, colormap);
    XftColorAllocName(dp, visual, colormap, FG, &fg);
    XftColorAllocName(dp, visual, colormap, FGH, &fgh);

    XMapRaised(dp, win);
    //XGrabKeyboard(dp, rt, True, GrabModeAsync, GrabModeAsync, CurrentTime);
    XFlush(dp);
}

int main(void)
{
    if (!(dp = XOpenDisplay(NULL)))
        printError("Cannot open display", true);

    setup();

    while (running)
    {
        switch (ev.type) 
        {
            case Expose:
                drawMenu();
                break;
            case KeyPress:
                keysHandler();
                break;
        }
    }
    
    cleanup();

    return 0;
}
