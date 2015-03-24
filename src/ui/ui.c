/*
 * ui/ui.c -- User interface code.
 *
 * Functionality common to the Windows and Gtk UI versions.
 *
 */

#include "ui/ui.h"

uint8_t framebuffer[256 * 224 * 4];

void *ui_get_fb(void)
{
    return framebuffer;
}
