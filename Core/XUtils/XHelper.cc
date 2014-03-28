#include <Core/XUtils/XHelper.h>
#include <Core/Color/RGBColor.h>

#include <GL/glx.h>

#include <string.h>
#include <iostream>

using namespace Manta;
using namespace std;
Mutex XHelper::Xlock("XHelper::Xlock");

XFontStruct* XHelper::getX11Font(Display* dpy,
                                 const char* fontstring) {
  // Should we lock X?
  XFontStruct* f = XLoadQueryFont(dpy, fontstring);
  if (!f)
    f = XLoadQueryFont(dpy, "-*-*-*-*-*-*-14-*-*-*-*-*-*-*");
  return f;
}

GLuint XHelper::getGLFont(XFontStruct* fontInfo) {
  Font id = fontInfo->fid;
  unsigned int first = fontInfo->min_char_or_byte2;
  unsigned int last = fontInfo->max_char_or_byte2;
  GLuint fontbase = glGenLists((GLuint) last+1);
  if (fontbase == 0) {
    cerr << "XHelper::getGLFont: ERROR!!!  Out of display lists\n";
    return 0;
  }
  // Should we lock X?
  glXUseXFont(id, first, last-first+1, fontbase+first);
  return fontbase;
}
    

// Print the string using the current OpenGL context.
void XHelper::printString(GLuint fontbase, double x, double y,
                          const char *s, const RGBColor& c) {
  // Set the font color
  glColor3f(c.r(), c.g(), c.b());
  
  glRasterPos2d(x,y);
  /*glBitmap(0, 0, x, y, 1, 1, 0);*/
  glPushAttrib (GL_LIST_BIT);
  glListBase(fontbase);
  glCallLists((int)strlen(s), GL_UNSIGNED_BYTE, (GLubyte *)s);
  glPopAttrib ();
}

// Calculates how many pixels wide a string will be using the
// specified font.
int XHelper::calc_width(XFontStruct* font_struct, const char* str) {
  XCharStruct overall;
  int ascent, descent;
  int dir;
  XTextExtents(font_struct, str, (int)strlen(str),
               &dir, &ascent, &descent, &overall);
  return overall.width;
}
