#ifndef _DEVICE_H
#define _DEVICE_H

#include <amdev.h>
int uptime();
_KbdReg * readkey();
void draw_rect(uint32_t * pixels, int x, int y, int w, int h);
void draw_sync();
int screen_width();
int screen_height();

#endif