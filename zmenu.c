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
static const int WIDTH = 500;
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
        printError("Cannot allocate color", false);
    return color.pixel;
}

// Handle the position of the menu
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
        if (line[len - 1] == '\n')
            line[len - 1] = '\0';
        
        if (!(items[i].text = strdup(line)))
            printError("strdup", true);
    }
    free(line);
}

// Filter items

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

    for (int i = 0; items[i].text != NULL; i++) 
    {
        if (items[i].text == NULL)
            break;
        
        if (textx >= WIDTH)
            break;

        XftTextExtentsUtf8(dp, font, (XftChar8 *)items[i].text, strlen(items[i].text), &ext);
        
        textWidht = ext.xOff + (padding * 2);

        if (strcmp(items[i].text, items[selIndex].text) == 0)
        {
            XSetForeground(dp, gc, bgh);
            XFillRectangle(dp, win, gc, textx, 0, textWidht, HEIGHT);
        }
            XftDrawStringUtf8(draw, &xftfg, font, textx + padding, texty, (XftChar8 *)items[i].text,
                              strlen(items[i].text));
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
            printf("%s\n", items[selIndex].text);
            running = false;
            break;
        case XK_BackSpace:
            if (inputLen > strlen(PROMPT))
                inputBuffer[--inputLen] = '\0';
            break;
        case XK_Tab:
        {
            int index;
            for (index = 0; items[index].text != NULL; index++);
            
            if (index == 0)
                break;

            if (ev.xkey.state & ShiftMask)
                if (selIndex != 0)
                    selIndex--;
                else
                    selIndex = 0;
            else
                if (selIndex != --index)
                    selIndex++;
                else
                    selIndex = index;
            printf("item[%d].text = %s\n\n", selIndex, items[selIndex].text);
            break;
        }
        default:
            if (strlen(buf) == 1 && inputLen < sizeof(inputBuffer) - 1) 
                strcat(inputBuffer, buf);
            break;
    }
    drawMenu();
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
    
    snprintf(inputBuffer, sizeof(inputBuffer), "%s", PROMPT);
    
    bg = getColor(BG);
    bgh = getColor(BGH);

    winSetup();
 
    gc = XCreateGC(dp, win, 0, NULL);
    XSetForeground(dp, gc, bg);
    XFillRectangle(dp, win, gc, wx, wy, WIDTH, HEIGHT);
    
    if (!(font = XftFontOpenName(dp, sc, FONT)))
        printError("Cannot load font", true);

    draw = XftDrawCreate(dp, win, visual, colormap);
    XftColorAllocName(dp, visual, colormap, FG, &xftfg);

    XMapRaised(dp, win);
    XGrabKeyboard(dp, rt, True, GrabModeAsync, GrabModeAsync, CurrentTime);
    XFlush(dp);
}

int main(int argc, char *argv[])
{
    if (!(dp = XOpenDisplay(NULL)))
        printError("Cannot open display", true);

    readStdin();
 
    setup();

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
