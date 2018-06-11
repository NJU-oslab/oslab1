#include <am.h>
#include <amdev.h>

static _Device * find_device(int device_id) {
  for (int n = 1; ;n++) {
	_Device * dev = _device(n);
	if (!dev) break;
	if (dev->id == device_id) {
	  return dev;
	}
  }
  return NULL;
}

int uptime() {
  _UptimeReg uptime;
  _Device * dev = find_device(_DEV_TIMER);
  dev->read(_DEVREG_TIMER_UPTIME, &uptime, sizeof(uptime));
  return uptime.lo;
}

_KbdReg * readkey() {
  _KbdReg * kbd = NULL;
  _Device * dev = find_device(_DEV_INPUT);
  dev->read(_DEVREG_INPUT_KBD, kbd, sizeof(kbd));
  return kbd;
}

int screen_width() {
  _Device * dev = find_device(_DEV_VIDEO);
  _VideoInfoReg info;
  dev->read(_DEVREG_VIDEO_INFO, &info, sizeof(info));
  return info.width;
}

int screen_height() {
  _Device * dev = find_device(_DEV_VIDEO);
  _VideoInfoReg info;
  dev->read(_DEVREG_VIDEO_INFO, &info, sizeof(info));
  return info.height;
}

void draw_rect(uint32_t * pixels, int x, int y, int w, int h) {
  _Device * dev = find_device(_DEV_VIDEO);
  _FBCtlReg ctl;
  ctl.pixels = pixels;
  ctl.x = x;
  ctl.y = y;
  ctl.w = w;
  ctl.h = h;
  ctl.sync = 1;
  dev->write(_DEVREG_VIDEO_FBCTL, &ctl, sizeof(ctl));
}

void draw_sync() {
}
