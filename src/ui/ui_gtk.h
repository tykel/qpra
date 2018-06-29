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
static void ui_gtk_quit(GtkWidget *widget, void *data);
static gboolean ui_gtk_key_press(GtkWidget *widget, GdkEventKey *event, void *data);
static gint ui_gtk_quit_delete(GtkWidget *widget, void *data);
static void ui_gtk_quit_destroy(GtkWidget *widget, void *data);
static gint ui_gtk_open_file(GtkWidget *, void *data);
static int gtk_area_start(GtkWidget *, void *);
static int gtk_area_expose(GtkWidget *, cairo_t *cr, gpointer data); 
static int gtk_area_configure(GtkWidget *, GdkEventConfigure *, void *);

extern pthread_t t_core; 

#endif
