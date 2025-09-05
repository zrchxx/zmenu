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
const char *FONT = "Frosty:pixelsize=14";
const char *PROMPT = ">>> ";

// Background color
static const char *BG  = "#1d2021";
// Foreground color
static const char *FG  = "#ebdbb2";
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

// Structs
typedef struct Item Item;
struct Item 
{
    char *text;
    Item *prev, *next;
};

// X11
static Display *dp;
static int sc;
static Window rt, win;
static GC gc;
static XEvent ev;
static bool running = true;

// Items list
static Item *headList = NULL; // General head
static Item *tailList = NULL; // General tail
//static Item *headMatch = NULL; // Filter head
//static Item *tailMatch = NULL; // Filter tail
static char buffer[BUFSIZ] = "";
static char userInput[BUFSIZ] = "";
static int selindex = 0;

// Xft
static XftDraw *draw;
static XftFont *font;
static XftColor fg, fgh;
static Visual *visual;
static Colormap colormap;


//// CODE STARTS HERE ////

// Just for debug
void debug(void)
{
    Item *x;
    for (x = headList; x != NULL; x = x->next)
        printf("x->text = %s\n", x->text);
}

// Frees all the resources
void cleanup(void) 
{
    Item *tmp;
    while (headList != NULL) 
    {
        tmp = headList;
        free(tmp->text);
        headList = headList->next;
        free(tmp);
    }
    tailList = NULL;

    XftDrawDestroy(draw);
    XftColorFree(dp, visual, colormap, &fg);
    XftColorFree(dp, visual, colormap, &fgh);
    XftFontClose(dp, font);

    XUngrabKeyboard(dp, CurrentTime);
    XFreeGC(dp, gc);
    XDestroyWindow(dp, win);
    XCloseDisplay(dp);
}

// Prints an error to stderr, given the arguments:
// errorName, the name of the error and checkExit
// if want to call exit
void printError(const char errorName[], bool checkExit) 
{
    fprintf(stderr, "zmenu error: %s.\n", errorName);
    if (checkExit)
        exit(1);
}

// Allocates a color in unsigned long format
// given the argument hex, and hex color code
unsigned long getColor(const char *hex) 
{
    XColor color;
    if (!XAllocNamedColor(dp, colormap, hex, &color, &color))
        printError("Cannot allocate color", false);
    return color.pixel;
}

// Return the position, given the arguments:
// int user, the user wanted positon, and
// int wh, the size of the window (width or height)
int position(int user, int wh)
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

// Search and item in a linked list, given
// the arguments: *list (general or filter)
// *match works as the index to filter
Item *searchItem(Item *list, char *match)
{
    if (!list || !match)
        return NULL;

    Item *tmp;
    for (tmp = list; tmp != NULL; tmp = tmp->next)
        if (strcmp(tmp->text, match) == 0)
            return tmp;

    return NULL;
}

// Malloc an item and link it to the general list
void addItem(char *text)
{
    if (searchItem(headList, text))
        printError("item already exist", false);

    Item *new = malloc(sizeof(Item));

    if (!new)
        printError("Malloc failed", true);

    new->text = text;
    
    if (headList)
    {
        tailList->next = new;
        new->next = NULL;
        new->prev = tailList;
        tailList = new;
    }
    else 
    {
        new->next = NULL;
        new->prev = NULL;
        headList = new;
        tailList = new;
    }
}

// Read the stdin separes it into lines
// and insert then in a item of the general list
void readStdin(void)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, stdin)) != -1)
    {
        if (line[read - 1] == '\n')
            line[read - 1] = '\0';
        addItem(strdup(line));
    }
    free(line);
}

// find a match for a char type
void findMatch(void)
{
    Item *tmp;
    for (tmp = headList; tmp != NULL; tmp = tmp->next) 
    {
        if (strstr(text, userInput) != NULL)
        {

        }
    }
}

// Draw the menu
void drawMenu(void)
{
    char buffer;
    int xtext = 10;
    int ytext = 10;
    int len = 10;

    

    XftDrawStringUtf8(draw, &fg, font, xtext, ytext, (FcChar8 *)buffer, len);
}

// manages key press events
void keyEvent(XKeyEvent *ekv)
{
    KeySym ksym = NoSymbol;
    char buf[64];
    int len = XLookupString(ekv, buf, sizeof(buf), &ksym, NULL);
    
    if (ekv->state & ShiftMask) 
        switch (ksym) 
            case XK_Tab: 
                selindex--;

    switch (ksym) 
    {
        case XK_Escape:
            cleanup();
            running = false;
            break;
        case XK_Return:
            // print to stdout the selected index
            debug();
            break;
        case XK_Tab:
            selindex++;
    }
    //drawMenu();
}

// Set things up
void setup(void) 
{
    sc = DefaultScreen(dp);
    rt = RootWindow(dp, sc);
    visual = DefaultVisual(dp, sc);
    colormap = DefaultColormap(dp, sc);
    XSetWindowAttributes wa;
    
    int x = position(PX, WIDTH);
    int y = position(PY, HEIGHT);
    unsigned long bg = getColor(BG);
    unsigned long bgh = getColor(BGH);
    
    if (!(font = XftFontOpenName(dp, sc, FONT)))
        printError("Cannot load font", true);
    
    wa.override_redirect = True;
    wa.background_pixel = bg;
    wa.event_mask = ExposureMask | KeyPressMask;

    win = XCreateWindow(dp, rt, x, y, WIDTH, HEIGHT, 0, DefaultDepth(dp, sc), CopyFromParent, visual,
                        CWOverrideRedirect | CWBackPixel | CWEventMask, &wa);
    
    gc = XCreateGC(dp, win, 0, NULL);
    XSetForeground(dp, gc, bgh);

    draw = XftDrawCreate(dp, win, visual, colormap);
    XftColorAllocName(dp, visual, colormap, FG, &fg);
    XftColorAllocName(dp, visual, colormap, FGH, &fgh);

    XMapRaised(dp, win);
    XGrabKeyboard(dp, rt, True, GrabModeAsync, GrabModeAsync, CurrentTime);
    XFlush(dp);
}

int main(void)
{
    if (!(dp = XOpenDisplay(NULL)))
        printError("Cannot open display", true);
    
    readStdin();
    
    while(running)
    {
        switch (ev.type) 
        {
            //case Expose:
            //    if (ev.xexpose.count == 0) 
            //        drawMenu();
            //    break;
            case KeyPress:
                keyEvent(&ev.xkey);
                break;
        }
    }
    
    //debug();
    
    cleanup();
    return EXIT_SUCCESS;
}
