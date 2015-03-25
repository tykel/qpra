/*
 * ui/ui.c -- User interface code.
 *
 * Functionality common to the Windows and Gtk UI versions.
 *
 */

#include "ui/ui.h"

pthread_mutex_t fb_lock;
uint8_t framebuffer[256 * 224 * 4];

void ui_lock_fb(void)
{
    pthread_mutex_lock(&fb_lock);
}

void ui_unlock_fb(void)
{
    pthread_mutex_unlock(&fb_lock);
}

void *ui_get_fb(void)
{
    return framebuffer;
}
