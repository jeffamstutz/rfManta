/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005-2006
  Scientific Computing and Imaging Institute, University of Utah

  License for the specific language governing rights and limitations under
  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#ifndef Manta_Engine_PureOpenGLDisplay_h
#define Manta_Engine_PureOpenGLDisplay_h

#include <Core/Color/RGBColor.h>

#ifdef __APPLE__ 
#include <OpenGL/gl.h>
#else
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>
//#include <GL/glext.h>
#endif

// #ifdef __APPLE__
#  ifndef GL_ARB_vertex_buffer_object
typedef long GLintptrARB;
typedef long GLsizeiptrARB;
#  endif
// #endif

#include <string>

/*
  This will only make pure OpenGL calls and should not be dependent on
  any one architecture or libraries outside of OpenGL.
*/

namespace Manta {
  class Image;
  class SimpleImageBase;
  
  using namespace std;
  
  class PureOpenGLDisplay {
  public:
    PureOpenGLDisplay(const string& new_mode);
    virtual ~PureOpenGLDisplay();

    void setMode(const string& new_mode);

    // Called once after obtaining the OpenGL context
    void init();
    // Clears the screen with the given color.  You still need to swap
    // the buffer afterwards.
    static void clearScreen(float r, float g, float b, float a = 0);
    // Calls glViewport for you ( helper stuff for swig ;) For
    // whatever reason, you can't name this glViewport without having
    // bad things happen on the swig side.
    static void setGLViewport(int x, int y, int width, int height);
    // Renders the image
    void displayImage(const Image* image);

    bool verbose;
    bool single_buffered;
    bool use_buffersubdata;
  private:
    PureOpenGLDisplay(const PureOpenGLDisplay&);
    PureOpenGLDisplay& operator=(const PureOpenGLDisplay&);

  protected:
    string mode;

    bool need_texids;
    GLuint texids[2];

    struct PBO {
      PBO();
      ~PBO();

      bool need_ids;
      GLuint bufId, texId;
      GLsizeiptrARB texSize;
      GLenum format;
      GLenum type;
      bool have_alpha;

      GLint rowLength;
      int xres, yres;
      bool stereo;

      // Takes info from SimpleImageBase and fills in some values.
      // Returns true if resizing should occur.
      bool initialize(const SimpleImageBase* si);
    };
    PBO pbos[2];
    int current_pbo;

    bool have_texturerectangle;
    bool have_clientstorage;
    bool have_texturerange;
    bool have_pbo;
    bool big_endian;

    bool drawImage_texture(const Image* image);
    bool drawImage_pbo(const Image* image);
    bool drawImage_pixels(const Image* image);

    bool have_extension(const char* name);
    
    void gl_print_error(const char *file, int line);

    // returns false if there were problems
    bool glImageInfo(const Image* image,
                     GLenum& format, GLenum& type, bool& have_alpha);

    // Called at the beginning of the constructor
    void setDefaults();
    // Called at the end of the constructor
    void setup();
  };
}

#endif
