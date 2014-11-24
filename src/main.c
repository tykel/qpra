#include <SDL2/SDL.h>
#include "ui/ui.h"
#include "core/core.h"

pthread_t t_core;
pthread_t t_audio;

int main(int argc, char **argv)
{
    /* Setup the GUI window and components. */
    ui_init(argc, argv);
    window = ui_window_new();

    /* Spawn the main emulation core thread. */
    pthread_create(&t_core, NULL, core_entry, NULL);
    
    /* Start the GUI thread function. */
    ui_run(window);

    
    return 0;
}
