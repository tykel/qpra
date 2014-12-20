/*
 * main.c -- Program entry point.
 *
 * Spawns the emulation and audio worker threads. The main thread then does
 * window event listening and rendering.
 *
 */

#include "ui/ui.h"
#include "core/core.h"

pthread_t t_core;
pthread_t t_audio;

struct arg_pair
{
    int argc;
    char **argv;
};

int main(int argc, char **argv)
{
    struct arg_pair pair = { argc, argv };

    /* Setup the GUI window and components. */
    ui_init(argc, argv);
    window = ui_window_new();

    /* Spawn the main emulation core thread, if we have a file to load. */
    if(argc > 1)
        pthread_create(&t_core, NULL, core_entry, &pair);
    
    /* Start the GUI thread function. */
    ui_run(window);
    
    return 0;
}
