/*
 * ui/ui_gtk.h -- User interface code, Gtk-specific (header).
 *
 * Declares the Gtk UI functions.
 *
 */

#ifndef QPRA_UI_GTK_H
#define QPRA_UI_GTK_H

#include <pthread.h>

void ui_init_gtk(int, char **);
struct ui_window * ui_window_new_gtk(void);

static void ui_draw_init(void);
static void ui_draw_opengl(void);
static void ui_gtk_quit(void);
static void ui_gtk_quit_destroy(void);
static int gtk_area_start(GtkWidget *, void *);
static int gtk_area_configure(GtkWidget *, GdkEventConfigure *, void *);

extern uint8_t *framebuffer;
extern pthread_t t_core; 

#endif
