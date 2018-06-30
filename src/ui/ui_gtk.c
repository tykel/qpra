/*
 * ui/ui_gtk.c -- User interface code, Gtk-specific.
 *
 * Functionality of the Gtk UI functions: window initialization and callbacks,
 * UI loop, OpenGL rendering, SDL input.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include "ui/ui.h"
#include "ui/ui_gtk.h"
#include "ui/gtk_opengl.h"
#include "log.h"
#include "defs.h"

struct ui_window *window;
pthread_t t_render;
extern struct arg_pair s_pair;

int mark_done();
int done();
int core_running();
void *core_entry(void *);

int texname;


void *ui_render(void *data)
{
    GtkWidget *area = (GtkWidget *)data;
    while (!done()) {
        ui_draw_opengl();
        gtk_opengl_swap(area);
        usleep(16666);
    }
    LOGD("Render thread exiting.");
}


void ui_init_gtk(int argc, char **argv)
{
    int i, j;
    
    /* Initialize SDL. */
    SDL_Init(SDL_INIT_VIDEO);

    /* Initialize GTK. */
    gtk_init(&argc, &argv);

    /* XXX: Create an initial framebuffer texture. */
    pthread_mutex_lock(&fb_lock);
    for(j = 0; j < 224; ++j) {
        for(i = 0; i < 256; ++i) {
            framebuffer[(j*256 + i) * 4 + 0] = i;
            framebuffer[(j*256 + i) * 4 + 1] = i;
            framebuffer[(j*256 + i) * 4 + 2] = i;
            framebuffer[(j*256 + i) * 4 + 3] = 255;
        }
    }
    pthread_mutex_unlock(&fb_lock);
}

struct ui_window * ui_window_new_gtk(void)
{
    GtkWidget *menubar, *filemenu, *file, *open, *close, *quit;
    GtkWidget *optmenu, *options, *emusettings;
    GtkWidget *helpmenu, *help, *doc, *about;
    GtkWidget *box;
    int attributes[] = {
        GLX_RGBA,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        GLX_DOUBLEBUFFER, True,
        GLX_DEPTH_SIZE, 12,
        None
    };
    struct ui_window *window = malloc(sizeof(struct ui_window));
   
    /* Create the GTK+ window. */
    window->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window->window), box);
    
    menubar = gtk_menu_bar_new();
    /* Create File menu and items. */
    filemenu = gtk_menu_new();
    file = gtk_menu_item_new_with_label("File");
    open = gtk_menu_item_new_with_label("Open ROM");
    close = gtk_menu_item_new_with_label("Close ROM");
    quit = gtk_menu_item_new_with_label("Quit");
    /* Create Options menu and items. */
    optmenu = gtk_menu_new();
    options = gtk_menu_item_new_with_label("Options");
    emusettings = gtk_menu_item_new_with_label("Emulation settings");
    /* Create Help menu and items. */
    helpmenu = gtk_menu_new();
    help = gtk_menu_item_new_with_label("Help");
    doc = gtk_menu_item_new_with_label("Documentation");
    about = gtk_menu_item_new_with_label("About...");
   
    /* Add File menu to menu bar. */
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file), filemenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), open);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), close);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), quit);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file);
    /* Add Options menu to menu bar. */
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(options), optmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(optmenu), emusettings);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), options);
    /* Add Help menu to menu bar. */
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help), helpmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpmenu), doc);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpmenu), about);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), help);
    
    window->context = NULL;
    window->area = gtk_drawing_area_new();
    window->context = gtk_opengl_create(window->area, attributes,
            window->context, TRUE);
    
    g_object_set_data(G_OBJECT(window->window), "area", window->area);
    g_object_set_data(G_OBJECT(window->window), "context", window->context);
    
    gtk_widget_set_size_request(window->window, 768, 672); //512, 448);
    gtk_window_set_resizable (GTK_WINDOW(window->window), FALSE);

    gtk_box_pack_start(GTK_BOX(box), menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), window->area, TRUE, TRUE, 0);
    
    /* Connect events to their callbacks. */
    g_signal_connect(G_OBJECT(window->window), "delete-event",
            G_CALLBACK(ui_gtk_quit_delete), NULL);
    g_signal_connect(G_OBJECT(window->window), "destroy",
            G_CALLBACK(ui_gtk_quit_destroy), NULL);
    g_signal_connect(G_OBJECT(quit), "activate",
            G_CALLBACK(ui_gtk_quit), window->window);
    g_signal_connect(window->area, "configure_event",
            G_CALLBACK(gtk_area_configure), window->window);
    g_signal_connect(window->area, "realize",
            G_CALLBACK(gtk_area_start), window->window); 
   
    g_signal_connect(G_OBJECT(open), "activate",
            G_CALLBACK(ui_gtk_open_file), window->window); 
    g_signal_connect(G_OBJECT(window->window), "key_press_event",
            G_CALLBACK(ui_gtk_key_press), NULL);

    gtk_widget_show_all(window->window);

    return window;
}

void ui_run_gtk(struct ui_window *window)
{
    LOGD("main UI thread - init draw then enter gtk_main");
    ui_draw_init();
    pthread_create(&t_render, NULL, ui_render, window->area);
    
    gtk_main(); 
}

gint ui_gtk_open_file(GtkWidget *widget, void *data)
{
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    dialog = gtk_file_chooser_dialog_new("Open ROM File",
                                         (GtkWindow *)data,
                                         action,
                                         ("_Cancel"),
                                         GTK_RESPONSE_CANCEL,
                                         ("_Open"),
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);
    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT && !core_running()) {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        char *filename = gtk_file_chooser_get_filename(chooser);
        int i;
        LOGD("ROM file to open: %s", filename);
        s_pair.argc++;
        s_pair.argv = realloc(s_pair.argv, sizeof(*s_pair.argv) * s_pair.argc);
        s_pair.argv[s_pair.argc - 1] = filename;
        pthread_create(&t_core, NULL, core_entry, &s_pair);
    }
    gtk_widget_destroy(dialog);

    return TRUE;
}

static gboolean ui_gtk_key_press(GtkWidget *widget, GdkEventKey *event, void *data)
{
    switch (event->keyval) {
        case GDK_KEY_o:
            if (event->state & GDK_CONTROL_MASK) {
                ui_gtk_open_file(widget, widget);
            }
            break;
        case GDK_KEY_Escape:
            gtk_widget_destroy(widget);
            break;
        default:
            break;
    }
    return FALSE;
}

static void ui_gtk_quit(GtkWidget *widget, void *data)
{
    gtk_widget_destroy(window->window);
}

static gint ui_gtk_quit_delete(GtkWidget *widget, void *data)
{
    return FALSE;
}

static void ui_gtk_quit_destroy(GtkWidget *widget, void *data)
{
    mark_done();
    pthread_join(t_render, NULL);
    SDL_Quit();
    //gtk_opengl_remove(window->area, window->context);
    //gtk_widget_destroy(window->window);
    //free(window);
    LOGD("UI thread exiting.");
    gtk_main_quit();
}

static void ui_draw_init(void)
{
    glGenTextures(1, &texname);
    glBindTexture(GL_TEXTURE_2D, texname);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    pthread_mutex_lock(&fb_lock);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 224, 0, GL_RGBA,
            GL_UNSIGNED_BYTE, framebuffer);
    pthread_mutex_unlock(&fb_lock);
    
    glClearColor(0.0, 0.0, 0.0, 1.0);
}

static void ui_draw_opengl(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 768, 0, 672, -1, 1); //512, 0, 448, -1, 1);
    glEnable(GL_TEXTURE_2D);
    pthread_mutex_lock(&fb_lock);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 224, GL_RGBA,
            GL_UNSIGNED_BYTE, framebuffer);
    pthread_mutex_unlock(&fb_lock);
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(1.0, 0.0);
        glVertex2i(768, 672); //448);
        glTexCoord2f(0.0, 0.0);
        glVertex2i(0, 672); //512, 448);
        glTexCoord2f(1.0, 1.0);
        glVertex2i(768, 0);
        glTexCoord2f(0.0, 1.0);
        glVertex2i(0, 0); //512, 0);
    glEnd();
}

static int gtk_area_configure(GtkWidget *widget, GdkEventConfigure *event, void *data)
{
    GtkWidget *window = (GtkWidget*)data;                                       
    GtkWidget *area = (GtkWidget*)g_object_get_data(G_OBJECT (window), "area"); 
    GLXContext context = (GLXContext)g_object_get_data(G_OBJECT (window), "context");
    GtkAllocation allocation;                                                   

    if (gtk_opengl_current (area, context) == TRUE) {                           
        gtk_widget_get_allocation (widget, &allocation);                        
        glViewport (0, 0, allocation.width, allocation.height);                 
    }                                                                           

    return TRUE;
}

static int gtk_area_start(GtkWidget *widget, void *data)
{
    GtkWidget *window = (GtkWidget *) data;                                     
    GtkWidget *area = (GtkWidget *) g_object_get_data (G_OBJECT (window), "area");
    GLXContext context = (GLXContext) g_object_get_data (G_OBJECT (window), "context");

    LOGD("gtk_area_start");
    if (gtk_opengl_current (area, context) == TRUE) {                           
        glDisable(GL_DEPTH_TEST);                                               
        glDisable(GL_CULL_FACE);                                                
        glDisable(GL_DITHER);                                                  
        glShadeModel(GL_SMOOTH);                                               
    }                                                                           

    return TRUE;
}
