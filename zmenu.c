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
// Background highlighted color
static const char *BGH = "#b8bb26";
// Foreground color
static const char *FG = "#ebdbb2";

// Window width
static const int WIDTH = 600;
// Window height
static const int HEIGHT = 40;

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
static bool running = true;

// Items list
typedef struct Item Item;
struct Item { char *text; };
static Item items[BUFSIZ];
static Item itemsMatch[BUFSIZ];

static char inputBuffer[BUFSIZ] = "";
static int selIndex = 0;

// Xft
static XftDraw *draw;
static XftFont *font;
static XftColor xftfg;


//// CODE STARTS HERE ////

// Prints customs errors to stderr
static void printError(const char errorName[], bool checkExit) 
{
    fprintf(stderr, "zmenu error: %s.\n", errorName);
    if (checkExit)
        exit(1);
}

// Allocates a color in unsigned long format
static unsigned long getColor(const char *hex) 
{
    XColor color;
    if (!XAllocNamedColor(dp, colormap, hex, &color, &color))
        printError("Cannot allocate color", true);
    return color.pixel;
}

// Frees all the resources
static void cleanup(void) 
{
    for (int i = 0; items[i].text != NULL; i++)
        free(items[i].text);

    XftDrawDestroy(draw);
    XftColorFree(dp, visual, colormap, &xftfg);
    XftFontClose(dp, font);

    XUngrabKeyboard(dp, CurrentTime);
    XFreeGC(dp, gc);
    XDestroyWindow(dp, win);
    XCloseDisplay(dp);
}

// Read the stdin text and allocates it
static void readStdin(void)
{
    char *line = NULL;
    size_t lineSize = 0;
    ssize_t len;

    for (int i = 0; (len = getline(&line, &lineSize, stdin)) != -1; i++)
    {
        if (i < BUFSIZ - 1)
        {
            if (line[len - 1] == '\n')
                line[len - 1] = '\0';
        
            if (!(items[i].text = strdup(line)))
                printError("strdup", true);
        }
        else
        {
            items[i].text = NULL;
            break;
        }
    }
    free(line);
}

// Filter items
static void match(void)
{
    int j = 0;
    char *searchText = inputBuffer + strlen(PROMPT);

    memset(itemsMatch, 0, sizeof(itemsMatch));

    for (int i = 0; items[i].text != NULL; i++)
        if (!*searchText || strcasestr(items[i].text, searchText))
            itemsMatch[j++] = items[i];
    selIndex = 0;
}

// Draw the menu items
static void drawMenu(void)
{
    XGlyphInfo ext;
    int textWidht, textx, texty, padding;

    texty = (HEIGHT + ((font->ascent + font->descent) / 2)) / 2;
    textx = (font->ascent + font->descent) / 4;
    padding = 5;
    
    XClearWindow(dp, win);

    XftTextExtentsUtf8(dp, font, (XftChar8 *)inputBuffer, strlen(inputBuffer), &ext);

    textWidht = ext.xOff;
    
    XftDrawStringUtf8(draw, &xftfg, font, textx, texty, (XftChar8 *)inputBuffer, strlen(inputBuffer));

    textx += textWidht + padding;

    for (int i = 0; itemsMatch[i].text != NULL; i++) 
    {
        if (textx >= WIDTH)
            break;

        XftTextExtentsUtf8(dp, font, (XftChar8 *)itemsMatch[i].text, strlen(itemsMatch[i].text), &ext);
        
        textWidht = ext.xOff + (padding * 2);

        if (i == selIndex)
        {
            XSetForeground(dp, gc, bgh);
            XFillRectangle(dp, win, gc, textx, 0, textWidht, HEIGHT);
        }
            XftDrawStringUtf8(draw, &xftfg, font, textx + padding, texty, 
                              (XftChar8 *)itemsMatch[i].text, strlen(itemsMatch[i].text));
        textx += textWidht;
    }
    XFlush(dp);
}

// Handles especial keys 
static void keysHandler(void)
{
    KeySym ksym = XKeycodeToKeysym(dp, (KeyCode)ev.xkey.keycode, 0);
    char buf[16] = "";
    unsigned int inputLen = strlen(inputBuffer);

    XLookupString(&ev.xkey, buf, sizeof(buf), &ksym, NULL);

    switch (ksym)
    {
        case XK_Escape:
            running = false;
            break;
        case XK_Return:
            if (itemsMatch[selIndex].text)
                printf("%s\n", itemsMatch[selIndex].text);
            running = false;
            break;
        case XK_BackSpace:
            if (inputLen > strlen(PROMPT))
            {
                inputBuffer[--inputLen] = '\0';
                match();
            }
            break;
        default:
            if (strlen(buf) == 1 && inputLen < sizeof(inputBuffer) - 1)
            {
                strcat(inputBuffer, buf);
                match();
            }
            break;
    }
    drawMenu();
}

// Manages window attributes
static void winSetup(int x, int y)
{
    XSetWindowAttributes wa;
    
    wa.override_redirect = True;
    wa.background_pixel = bg;
    wa.event_mask = ExposureMask | KeyPressMask;

    win = XCreateWindow(dp, rt, x, y, WIDTH, HEIGHT, 0, DefaultDepth(dp, sc), CopyFromParent, 
                        visual, CWOverrideRedirect | CWBackPixel | CWEventMask, &wa);

}

// Initialize variables
static void setup(void) 
{
    sc = DefaultScreen(dp);
    rt = RootWindow(dp, sc);
    visual = DefaultVisual(dp, sc);
    colormap = DefaultColormap(dp, sc);
    
    snprintf(inputBuffer, sizeof(inputBuffer), "%s", PROMPT);
    
    bg = getColor(BG);
    bgh = getColor(BGH);

    int wx = (DisplayWidth(dp, sc) - WIDTH) / 2;
    int wy = (DisplayHeight(dp, sc) - HEIGHT) / 2;
    
    winSetup(wx, wy);
 
    gc = XCreateGC(dp, win, 0, NULL);
    XSetForeground(dp, gc, bg);
    
    if (!(font = XftFontOpenName(dp, sc, FONT)))
        printError("Cannot load font", true);

    draw = XftDrawCreate(dp, win, visual, colormap);
    XftColorAllocName(dp, visual, colormap, FG, &xftfg);

    XMapRaised(dp, win);
    XGrabKeyboard(dp, rt, True, GrabModeAsync, GrabModeAsync, CurrentTime);
    XFlush(dp);
}

int main(void)
{
    if (!(dp = XOpenDisplay(NULL)))
        printError("Cannot open display", true);

    readStdin();
 
    setup();
    
    match();

    while (running)
    {
        XNextEvent(dp, &ev);
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
