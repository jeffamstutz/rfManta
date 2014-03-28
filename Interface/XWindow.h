
#ifndef Manta_Interface_XWindow_h
#define Manta_Interface_XWindow_h

#include <X11/Xlib.h>

namespace Manta {
  class XWindow {
  public:
    Window window;
    int xres, yres;
  };
}

#endif
