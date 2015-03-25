/*
 * ui/ui.h -- User interface code (header).
 *
 * Declares the UI window structure, and redefines the ui_* functions to use
 * those found on the host platform.
 *
 */

#ifndef QPRA_UI_H
#define QPRA_UI_H

#include <stdint.h>
#include <pthread.h>
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

#ifdef _WIN32

#define ui_init ui_init_windows
#define ui_window_new ui_window_new_windows
#define ui_run ui_run_windows

#else

#define ui_init ui_init_gtk
#define ui_window_new ui_window_new_gtk
#define ui_run ui_run_gtk

#endif

extern pthread_mutex_t fb_lock;
extern uint8_t framebuffer[256 * 224 * 4];

void ui_init(int, char **);
struct ui_window * ui_window_new(void);
void ui_run(struct ui_window*);

void ui_lock_fb(void);
void ui_unlock_fb(void);
void *ui_get_fb(void);

extern struct ui_window *window;

#endif
