#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <GL/glx.h>
#include "ui/ui.h"
#include "ui/ui_gtk.h"

SDL_Window *swindow;
SDL_Renderer *renderer;
SDL_Texture *texture;
struct ui_window *window;

void ui_init_gtk(int argc, char **argv)
{
    gtk_init(&argc, &argv);
}

struct ui_window * ui_window_new_gtk(void)
{
    struct ui_window *window = malloc(sizeof(struct ui_window));
   
    /* Create the GTK+ window. */
    window->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_app_paintable(window->window, TRUE);
    gtk_widget_set_size_request(window->window, 512, 448);
    gtk_window_set_resizable (GTK_WINDOW(window->window), FALSE);
    g_signal_connect(window->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_timeout_add(16, (GSourceFunc)ui_draw_gtk, window);
    gtk_widget_show(window->window);
    window->id = GDK_WINDOW_XID(gtk_widget_get_window(window->window));
    
    /* Attach SDL to that window. */
    SDL_Init(SDL_INIT_VIDEO);
    swindow = SDL_CreateWindowFrom((void*)window->id);
    swindow->flags |= SDL_WINDOW_OPENGL;
    SDL_GL_LoadLibrary(NULL);

    renderer = SDL_CreateRenderer(swindow, -1,
            SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STATIC, 512, 448);
    
    SDL_SetWindowTitle(swindow, "QPRA [SDL]");
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    
    return window;
}

void ui_run_gtk(struct ui_window *window)
{
    gtk_main();
}

static gboolean ui_draw_gtk(GtkWidget *widget, cairo_t *cr, gpointer ptr)
{
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}
