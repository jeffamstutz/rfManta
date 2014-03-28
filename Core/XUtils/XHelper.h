/************************************
 *
 * XHelper
 *
 * For now this is a class with static function helpers.  Do not
 * inherit from it.
 *
 * Author: James Bigler
 *
 *************************************/

#include <GL/gl.h>
#include <X11/Xlib.h>

#include <Core/Thread/Mutex.h>

#ifndef Manta_Engine_XHelper_h
#define Manta_Engine_XHelper_h

#ifdef __sgi
#define __FONTSTRING__ "-adobe-helvetica-bold-r-normal--12-120-*-*-p-*-iso8859-1"
#else
#define __FONTSTRING__ "-adobe-*-*-*-*-*-14-*-*-*-*-*-*-*"
#endif

namespace Manta {

  class RGBColor;
  
  class XHelper {
  public:

    // Use this to make sure that only one X program is opening
    // windows at a time.
    static Mutex Xlock;
    
    // If there was an error NULL is returned.
    static XFontStruct* getX11Font(Display* dpy,
                                   const char* fontstring=__FONTSTRING__);
    // If there was an error 0 is returned.
    static GLuint getGLFont(XFontStruct* fontInfo);
    
    // Print the string using the current OpenGL context.
    static void printString(GLuint fontbase, double x, double y,
                            const char *s, const RGBColor& c);
    
    // Calculates how many pixels wide a string will be using the
    // specified font.
    static int calc_width(XFontStruct* font_struct, const char* str);

    
  };
} // namespace Manta

#endif // Manta_Engine_XHelper_h
