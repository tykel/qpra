/*
 * main.c -- Program entry point.
 *
 * Spawns the emulation and audio worker threads. The main thread then does
 * window event listening and rendering.
 *
 */

#include <time.h>
#include "ui/ui.h"
#include "core/core.h"

pthread_t t_core;
pthread_t t_render;
pthread_t t_audio;

static pthread_mutex_t s_done_mutex;
static volatile int s_done;

struct timespec ts_start;

int mark_done()
{
    pthread_mutex_lock(&s_done_mutex);
    s_done = 1;
    pthread_mutex_unlock(&s_done_mutex);
}

int done()
{
    unsigned int d;
    pthread_mutex_lock(&s_done_mutex);
    d = s_done;
    pthread_mutex_unlock(&s_done_mutex);
    return d;
}

struct arg_pair
{
    int argc;
    char **argv;
};

int main(int argc, char **argv)
{
    struct arg_pair pair = { argc, argv };

    pthread_mutex_init(&s_done_mutex, NULL);

    clock_gettime(CLOCK_REALTIME, &ts_start);

    /* Setup the GUI window and components. */
    ui_init(argc, argv);
    window = ui_window_new();

    /* Spawn the main emulation core thread, if we have a file to load. */
    if(argc > 1) {
        pthread_create(&t_core, NULL, core_entry, &pair);
    }
    
    /* Start the GUI thread function. */
    ui_run(window);
    if (t_core) {
        pthread_join(t_core, NULL);
    }
    
    pthread_mutex_destroy(&s_done_mutex);

    return 0;
}
