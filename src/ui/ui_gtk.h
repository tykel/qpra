
#ifndef QPRA_UI_GTK_H
#define QPRA_UI_GTK_H

void ui_init_gtk(int, char **);
struct ui_window * ui_window_new_gtk(void);

static gboolean ui_draw_gtk(GtkWidget *, cairo_t *, gpointer);

#endif
