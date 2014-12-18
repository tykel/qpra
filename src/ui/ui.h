
#ifndef QPRA_UI_H
#define QPRA_UI_H

#include <stdint.h>
#include <SDL2/SDL.h>
#include <GL/glx.h>
#ifdef _WIN32
#else
#include <gtk/gtk.h>
#endif


struct ui_window
{
    int width, height;
    const char *name;
    unsigned long id;

#ifdef _WIN32
#else
    GtkWidget *window;
    GtkWidget *area;
    GLXContext context;
#endif
};

/* HACK: we need to explicitly redefine SDL_Window so we can manually modify
 * the flags in the UI setup, so as to tell it GL is supported.
 */
struct SDL_Window
{
    const void *magic;
    Uint32 id;
    char *title;
    SDL_Surface *icon;
    int x, y;
    int w, h;
    int min_w, min_h;
    int max_w, max_h;
    Uint32 flags;
};

typedef struct SDL_Window SDL_Window;

#ifdef _WIN32

#define ui_init ui_init_windows
#define ui_window_new ui_window_new_windows
#define ui_run ui_run_windows

#else

#define ui_init ui_init_gtk
#define ui_window_new ui_window_new_gtk
#define ui_run ui_run_gtk

#endif

void ui_init(int, char **);
struct ui_window * ui_window_new(void);
void ui_run(struct ui_window*);

extern struct ui_window *window;

#endif
