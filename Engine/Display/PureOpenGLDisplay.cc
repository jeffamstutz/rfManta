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

#define GL_GLEXT_PROTOTYPES

#include <Engine/Display/PureOpenGLDisplay.h>
#include <Image/NullImage.h>
#include <Image/Pixel.h>
#include <Image/SimpleImage.h>
#include <Core/Util/Args.h>
#include <Core/Exceptions/IllegalValue.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Exceptions/IllegalArgument.h>

#if  !defined(__APPLE__) || MANTA_ENABLE_X11
#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>

#ifndef _WIN32
#include <GL/glext.h>
#endif

#else
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>
#endif

#include <iostream>
#include <typeinfo>
#include <cstring>

#ifdef __APPLE__
// For some reason, apple has these extensions but doesn't have the proper
// definitions when it is being used under X11.  Define them here.
#ifndef GL_EXT_texture_rectangle
#  define GL_EXT_texture_rectangle 1
#  define GL_TEXTURE_RECTANGLE_EXT          0x84F5
#  define GL_TEXTURE_BINDING_RECTANGLE_EXT  0x84F6
#  define GL_PROXY_TEXTURE_RECTANGLE_EXT    0x84F7
#  define GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT 0x84F8
#endif
#ifndef GL_APPLE_client_storage
#  define GL_APPLE_client_storage 1
#  define GL_UNPACK_CLIENT_STORAGE_APPLE    0x85B2
#endif
#ifndef GL_APPLE_texture_range
#  define GL_APPLE_texture_range 1
#  define GL_TEXTURE_RANGE_LENGTH_APPLE      0x85B7
#  define GL_TEXTURE_RANGE_POINTER_APPLE     0x85B8
#  define GL_TEXTURE_STORAGE_HINT_APPLE      0x85BC
#  define GL_TEXTURE_MINIMIZE_STORAGE_APPLE  0x85B6
#  define GL_STORAGE_PRIVATE_APPLE           0x85BD
#  define GL_STORAGE_CACHED_APPLE            0x85BE
#  define GL_STORAGE_SHARED_APPLE            0x85BF

#  ifdef GL_GLEXT_FUNCTION_POINTERS
     typedef void (* glTextureRangeAPPLEProcPtr) (GLenum target, GLsizei length, const GLvoid *pointer);
     typedef void (* glGetTexParameterPointervAPPLEProcPtr) (GLenum target, GLenum pname, GLvoid **params);
#  else
     extern "C" {
       extern void glTextureRangeAPPLE(GLenum target, GLsizei length, const GLvoid *pointer);
       extern void glGetTexParameterPointervAPPLE(GLenum target, GLenum pname, GLvoid **params);
     }
#  endif /* GL_GLEXT_FUNCTION_POINTERS */
#endif

#ifndef GL_ARB_pixel_buffer_object
#  define GL_ARB_pixel_buffer_object 1
#  define GL_PIXEL_PACK_BUFFER_ARB                        0x88EB
#  define GL_PIXEL_UNPACK_BUFFER_ARB                      0x88EC
#  define GL_PIXEL_PACK_BUFFER_BINDING_ARB                0x88ED
#  define GL_PIXEL_UNPACK_BUFFER_BINDING_ARB              0x88EF
#endif

#ifndef GL_ARB_vertex_buffer_object
#  ifdef GL_GLEXT_FUNCTION_POINTERS
typedef void (* glBindBufferARBProcPtr) (GLenum target, GLuint buffer);
typedef void (* glDeleteBuffersARBProcPtr) (GLsizei n, const GLuint *buffers);
typedef void (* glGenBuffersARBProcPtr) (GLsizei n, GLuint *buffers);
typedef void (* glBufferDataARBProcPtr) (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
typedef void (* glBufferSubDataARBProcPtr) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
typedef GLvoid *(* glMapBufferARBProcPtr) (GLenum target, GLenum access);
typedef GLboolean (* glUnmapBufferARBProcPtr) (GLenum target);
#else
extern "C" {
extern void glBindBufferARB(GLenum target, GLuint buffer);
extern void glDeleteBuffersARB(GLsizei n, const GLuint *buffers);
extern void glGenBuffersARB(GLsizei n, GLuint *buffers);
extern void glBufferDataARB(GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
extern void glBufferSubDataARB(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
extern void glGetBufferSubDataARB(GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid *data);
extern GLvoid *glMapBufferARB(GLenum target, GLenum access);
extern GLboolean glUnmapBufferARB(GLenum target);
}
#endif /* GL_GLEXT_FUNCTION_POINTERS */
#endif

#ifndef GL_ARB_vertex_buffer_object
#define GL_ARB_vertex_buffer_object 1
#define GL_ARRAY_BUFFER_ARB                                0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB                        0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB                        0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB                0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB                 0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB                 0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB                  0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB                  0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB          0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB              0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB        0x889C
#define GL_FOG_COORD_ARRAY_BUFFER_BINDING_ARB              0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB                 0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB          0x889F
#define GL_STREAM_DRAW_ARB                                 0x88E0
#define GL_STREAM_READ_ARB                                 0x88E1
#define GL_STREAM_COPY_ARB                                 0x88E2
#define GL_STATIC_DRAW_ARB                                 0x88E4
#define GL_STATIC_READ_ARB                                 0x88E5
#define GL_STATIC_COPY_ARB                                 0x88E6
#define GL_DYNAMIC_DRAW_ARB                                0x88E8
#define GL_DYNAMIC_READ_ARB                                0x88E9
#define GL_DYNAMIC_COPY_ARB                                0x88EA
#define GL_READ_ONLY_ARB                                   0x88B8
#define GL_WRITE_ONLY_ARB                                  0x88B9
#define GL_READ_WRITE_ARB                                  0x88BA
#define GL_BUFFER_SIZE_ARB                                 0x8764
#define GL_BUFFER_USAGE_ARB                                0x8765
#define GL_BUFFER_ACCESS_ARB                               0x88BB
#define GL_BUFFER_MAPPED_ARB                               0x88BC
#define GL_BUFFER_MAP_POINTER_ARB                          0x88BD
/* Obsolete */
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB         0x889D
#endif


#endif /* __APPLE__ */

// If these extensions are not availble, define these constants to something so
// that the code will still compile.  It won't get executed due to logic in
// the setup code
#ifndef GL_EXT_texture_rectangle
// Check for ARB version
#  ifdef GL_ARB_texture_rectangle
#    define GL_EXT_texture_rectangle GL_ARB_texture_rectangle
#    define GL_TEXTURE_RECTANGLE_EXT GL_TEXTURE_RECTANGLE_ARB
#  else
#    define GL_EXT_texture_rectangle 0
#    define GL_TEXTURE_RECTANGLE_EXT 0
#  endif
#endif

// Pixel/Vertex Buffers are unavailable on some platforms, notably Prism.
#ifndef GL_ARB_pixel_buffer_object
#  define glGenBuffersARB( a, b )
#  define glBindBufferARB( a, b )
#  define glBufferDataARB( a, b, c, d )
#  define glUnmapBufferARB( a ) 0
#  define glBufferSubDataARB( a, b, c, d )
#  define glMapBufferARB( a, b ) 0
#  define GL_PIXEL_UNPACK_BUFFER_ARB 0
#  define GL_STREAM_DRAW_ARB 0
#  define GL_WRITE_ONLY_ARB 0
// This define is used in conditionals.
#  define GL_ARB_pixel_buffer_object 0
#endif

#ifndef GL_APPLE_client_storage
#  define GL_APPLE_client_storage 0
#endif
#ifndef GL_APPLE_texture_range
#  define GL_APPLE_texture_range 0
#  define GL_TEXTURE_STORAGE_HINT_APPLE 0
#  ifndef GL_STORAGE_SHARED_APPLE
#    define GL_STORAGE_SHARED_APPLE 0
#  endif
#  define glTextureRangeAPPLE(x, y, z)
#endif

#ifndef GL_ABGR_EXT
#define GL_ABGR_EXT 0
#endif

#ifndef GL_BGRA
#define GL_BGRA 0
#endif

#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV 0
#endif

#ifndef GL_UNSIGNED_INT_8_8_8_8
#define GL_UNSIGNED_INT_8_8_8_8 0
#endif

#ifndef GL_UNPACK_CLIENT_STORAGE_APPLE
#define GL_UNPACK_CLIENT_STORAGE_APPLE 0
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0
#endif


using namespace Manta;
using namespace std;

PureOpenGLDisplay::PBO::PBO()
  :
  need_ids(true),
  texSize(0),
  format(0),
  type(0)
{
}

PureOpenGLDisplay::PBO::~PBO()
{
}

bool
PureOpenGLDisplay::PBO::initialize(const SimpleImageBase* si)
{
  si->getResolution(stereo, xres, yres);
  rowLength = si->getRowLength();

  // We need the whole buffer size including the padding
  GLsizeiptrARB new_texSize =
    si->getRowLength()*yres*si->pixelSize()*(stereo? 2: 1);
  if (new_texSize > texSize) {
    texSize = new_texSize;
    return true;
  } else {
    texSize = new_texSize;
    return false;
  }
}

PureOpenGLDisplay::PureOpenGLDisplay(const string& new_mode)
{
  setDefaults();
  setMode(new_mode);
  setup();
}

PureOpenGLDisplay::~PureOpenGLDisplay()
{
}

void
PureOpenGLDisplay::setDefaults()
{
  mode = "texture";
  verbose = false;
  single_buffered = true;
  use_buffersubdata = true;
}

void
PureOpenGLDisplay::setup()
{
  need_texids = true;
  current_pbo = 0;
  int x = 0x12345678;
  char* p = (char*)&x;
  if(p[0] == 0x12)
    big_endian = true;
  else
    big_endian = false;
}

void
PureOpenGLDisplay::setMode(const string& new_mode)
{
  if (new_mode  != "") {
    if(mode != "image" && mode != "texture" && mode != "pbo")
      throw IllegalValue<string>("Illegal dispay mode", mode);
    mode = new_mode;
  }
}

void
PureOpenGLDisplay::init()
{
  // Check for texturing extensions
  have_texturerectangle = false;
  if(GL_EXT_texture_rectangle &&
     (have_extension("GL_EXT_texture_rectangle") ||
      have_extension("GL_ARB_texture_rectangle")))
    {
      have_texturerectangle = true;
      if (verbose) cout << "Have GL texture rectangle\n";
    }
  have_clientstorage = false;
  if(GL_APPLE_client_storage && have_extension("GL_APPLE_client_storage")){
    have_clientstorage = true;
    if (verbose) cout << "Have GL client storage\n";
  }
  have_texturerange = false;
  if(GL_APPLE_texture_range && have_extension("GL_APPLE_texture_range")){
    have_texturerange = true;
    if (verbose) cout << "Have GL texture range\n";
  }
  have_pbo = false;
  if(GL_ARB_pixel_buffer_object &&
     (have_extension("GL_ARB_pixel_buffer_object") ||
      have_extension("GL_EXT_pixel_buffer_object")))
    {
      have_pbo = true;
      if (verbose) cout << "Have GL pixel buffer object\n";
    }


  // We don't want depth to be taken into consideration
  glDisable(GL_DEPTH_TEST);
}

void
PureOpenGLDisplay::clearScreen(float r, float g, float b, float a)
{
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT);
}

void
PureOpenGLDisplay::setGLViewport(int x, int y, int width, int height)
{
  glViewport(x, y, width, height);
}

void
PureOpenGLDisplay::displayImage(const Image* image)
{
  // We will look at the typeid of the image to determine what kind it
  // is and avoid extra dynamic_casts.  We do that instead of using a
  // virtual function in image to avoid having a zillion virtual functions
  // in the Image interface.  This assumes that there will be more display
  // types that image types.
  if(typeid(*image) == typeid(NullImage))
    return;

  bool stereo;
  int xres, yres;
  image->getResolution(stereo, xres, yres);
  glViewport(0, 0, xres, yres);

  if(mode == "pbo" && drawImage_pbo(image)){
    // Drawn by texture...
    if (verbose) cerr << "pbo mode drew it\n";
  } else if (mode == "texture" && drawImage_texture(image)) {
    // Drawn by pbo...
    if (verbose) cerr << "texture mode drew it\n";
  } else if(drawImage_pixels(image)){
    // Drawn by pixels...
    if (verbose) cerr << "image mode drew it\n";
  } else {
    throw InternalError("Unknown image type in OpenGLDisplay");
  }
  gl_print_error(__FILE__,__LINE__);
}

bool
PureOpenGLDisplay::drawImage_pixels(const Image* image)
{
  const SimpleImageBase* si = dynamic_cast<const SimpleImageBase*>(image);
  if(!si)
    return false;
  bool stereo;
  int xres, yres;
  image->getResolution(stereo, xres, yres);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, xres, 0, yres);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.375, 0.375, 0.0);
  GLenum format;
  GLenum type;
  if(typeid(*image) == typeid(SimpleImage<RGBA8Pixel>)){
    format = GL_RGBA;
    type = GL_UNSIGNED_BYTE;
  } else if(typeid(*image) == typeid(SimpleImage<RGB8Pixel>)){
    format = GL_RGB;
    type = GL_UNSIGNED_BYTE;
  } else if(typeid(*image) == typeid(SimpleImage<ABGR8Pixel>)){
    format = GL_ABGR_EXT;
    type = GL_UNSIGNED_BYTE;
  } else if(typeid(*image) == typeid(SimpleImage<ARGB8Pixel>)){
    format = GL_BGRA;
    type = big_endian?GL_UNSIGNED_INT_8_8_8_8_REV : GL_UNSIGNED_INT_8_8_8_8;
  } else if(typeid(*image) == typeid(SimpleImage<BGRA8Pixel>)){
    format = GL_BGRA;
    type = GL_UNSIGNED_BYTE;
  } else if(typeid(*image) == typeid(SimpleImage<RGBAfloatPixel>)){
    format = GL_RGBA;
    type = GL_FLOAT;
  } else if(typeid(*image) == typeid(SimpleImage<RGBZfloatPixel>)){
      format = GL_RGBA;
      type = GL_FLOAT;
  } else if(typeid(*image) == typeid(SimpleImage<RGBfloatPixel>)){
    format = GL_RGB;
    type = GL_FLOAT;
  } else {
    return false;
  }
  glPixelStorei(GL_UNPACK_ROW_LENGTH, si->getRowLength());
  if(stereo){
    glDrawBuffer(GL_BACK_LEFT);
    glRasterPos2i(0,0);
    glDrawPixels(xres, yres, format, type, si->getRawData(0));
    glDrawBuffer(GL_BACK_RIGHT);
    glRasterPos2i(0,0);
    glDrawPixels(xres, yres, format, type, si->getRawData(1));
  } else {
    glDrawBuffer(GL_BACK);
    glRasterPos2i(0,0);
    glDrawPixels(xres, yres, format, type, si->getRawData(0));
  }
  return true;
}

bool
PureOpenGLDisplay::drawImage_texture(const Image* image)
{
  if(!have_texturerectangle)
    return false;
  const SimpleImageBase* si = dynamic_cast<const SimpleImageBase*>(image);
  if(!si)
    return false;

  bool stereo;
  int xres, yres;
  image->getResolution(stereo, xres, yres);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, 1, 0, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glDisable(GL_TEXTURE_2D);
  glEnable(GL_TEXTURE_RECTANGLE_EXT);

  if(need_texids){
    // Free old textures if necessary...
    glGenTextures(2, texids);
    need_texids = false;
  }

  glBindTexture(GL_TEXTURE_RECTANGLE_EXT, texids[0]);
  //  glTextureRangeAPPLE(GL_TEXTURE_RECTANGLE_EXT, xres*yres*4, si->getRawData(0));
  if(have_clientstorage)
    glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
  glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, si->getRowLength());

  if(have_texturerange)
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_STORAGE_HINT_APPLE,
                    GL_STORAGE_SHARED_APPLE);
  glTexParameteri (GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri (GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  GLenum format;
  GLenum type;
  bool have_alpha = true;

  if (!glImageInfo(image, format, type, have_alpha)) {
    return false;
  }

  // Load textures
  glTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, have_alpha? GL_RGBA : GL_RGB,
               xres, yres, 0, format, type,
               si->getRawData(0));
  if(stereo){
    glBindTexture(GL_TEXTURE_RECTANGLE_EXT, texids[1]);
    glTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, have_alpha? GL_RGBA : GL_RGB,
                 xres, yres, 0, format, type,
                 si->getRawData(1));
    // Bind the left image again
    glBindTexture(GL_TEXTURE_RECTANGLE_EXT, texids[0]);
  }

  glDrawBuffer(stereo? GL_BACK_LEFT : GL_BACK);
  glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(0, 0);
    glTexCoord2f(xres, 0);
    glVertex2f(1, 0);
    glTexCoord2f(xres, yres);
    glVertex2f(1, 1);
    glTexCoord2f(0, yres);
    glVertex2f(0, 1);
  glEnd();
  if(stereo){
    glDrawBuffer(GL_BACK_RIGHT);
    glBindTexture(GL_TEXTURE_RECTANGLE_EXT, texids[1]);
    glBegin(GL_QUADS);
      glTexCoord2f(0, 0);
      glVertex2f(0, 0);
      glTexCoord2f(xres, 0);
      glVertex2f(1, 0);
      glTexCoord2f(xres, yres);
      glVertex2f(1, 1);
      glTexCoord2f(0, yres);
      glVertex2f(0, 1);
    glEnd();
  }
  glDisable(GL_TEXTURE_RECTANGLE_EXT);
  return true;
}

bool
PureOpenGLDisplay::drawImage_pbo(const Image* image)
{
  if(!have_texturerectangle || !have_pbo)
    return false;
  const SimpleImageBase* si = dynamic_cast<const SimpleImageBase*>(image);
  if(!si)
    return false;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, 1, 0, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glDisable(GL_TEXTURE_2D);
  glEnable(GL_TEXTURE_RECTANGLE_EXT);

  GLenum format;
  GLenum type;
  bool have_alpha;

  if (!glImageInfo(image, format, type, have_alpha)) {
    return false;
  }

  PBO* pbo = &pbos[current_pbo];

  if(pbo->need_ids){
    if (verbose) cerr << "Computing ids for current_pbo = "<<current_pbo<<"\n";
    // Free old textures if necessary...
    glGenTextures(1, &pbo->texId);
    glGenBuffersARB(1, &pbo->bufId);
    pbo->need_ids = false;
  }

  bool resize_buffer = pbo->initialize(si);

  // Set up the buffer
  glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo->bufId);

  if (use_buffersubdata) {
    if (resize_buffer) {
      // Bind the buffer
      glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB,
                      pbo->texSize, si->getRawData(0),
                      GL_STREAM_DRAW_ARB);
    } else {
      glBufferSubDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0 /*offset*/,
                         pbo->texSize, si->getRawData(0));
    }
  } else {
    if (resize_buffer) {
      // Allocate more space for the buffer.

      // Reset the contnets of the texSize-sized buffer object
      glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo->texSize, NULL,
                      GL_STREAM_DRAW_ARB);
    }

    // Map the memory.  The contents are undefined, because of the
    // previous call to glBufferData.
    void* pboMemory = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,
                                     GL_WRITE_ONLY_ARB);

    if (pboMemory == NULL) {
      cerr << "pboMemory is null\n";
      have_pbo = false;
      return false;
    }
    // Copy the data
    memcpy(pboMemory, si->getRawData(0), pbo->texSize);

    // Unmap the texture image buffer
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
  }

  // Now use the other pbo, but only if it has been initialized
  int prev_pbo = 1 - current_pbo;
  // Cycle to the next pbo
  current_pbo = 1 - current_pbo;
  if (!single_buffered) {
    pbo = &pbos[prev_pbo];
    if(pbo->need_ids) {
      if (verbose) cerr << "Skipping rendering frame for prev_pbo = "<<prev_pbo<<"\n";
      return true;
    }
  }


  glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo->bufId);

  // Bind the texture and supply the parameters
  glBindTexture(GL_TEXTURE_RECTANGLE_EXT, pbo->texId);

  glTexParameteri (GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri (GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, pbo->rowLength);

  int xres = pbo->xres;
  int yres = pbo->yres;
  bool stereo = pbo->stereo;
  // Check to see if we need to rebind a new texture or do a SubImage
  if (resize_buffer                ||
      format     != pbo->format     ||
      type       != pbo->type       ||
      have_alpha != pbo->have_alpha)
    {

      glTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0,
                   have_alpha? GL_RGBA : GL_RGB,
                   xres, yres+(stereo? yres : 0), 0,
                   format, type,
                   NULL);
      pbo->format     = format;
      pbo->type       = type;
      pbo->have_alpha = have_alpha;
    } else {
      // Use SubImage
      glTexSubImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, 0, 0,
                      xres, yres+(stereo? yres : 0),
                      pbo->format, pbo->type, NULL);
    }

  glDrawBuffer(stereo? GL_BACK_LEFT : GL_BACK);
  glBegin(GL_QUADS);
  {
    glTexCoord2f(   0,    0); glVertex2f(0, 0);
    glTexCoord2f(xres,    0); glVertex2f(1, 0);
    glTexCoord2f(xres, yres); glVertex2f(1, 1);
    glTexCoord2f(   0, yres); glVertex2f(0, 1);
  }
  glEnd();
  if(stereo){
    glDrawBuffer(GL_BACK_RIGHT);
    glBegin(GL_QUADS);
    {
      glTexCoord2f(   0,      yres);  glVertex2f(0, 0);
      glTexCoord2f(xres,      yres);  glVertex2f(1, 0);
      glTexCoord2f(xres, yres+yres);  glVertex2f(1, 1);
      glTexCoord2f(   0, yres+yres);  glVertex2f(0, 1);
    }
    glEnd();
  }

  // Cleanup
  glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
  glDisable(GL_TEXTURE_RECTANGLE_EXT);
  return true;
}

void
PureOpenGLDisplay::gl_print_error(const char *file, int line)
{
  GLenum errcode = glGetError();
  if(errcode != GL_NO_ERROR)
    cerr << "PureOpenGLDisplay: "
         << file << ":"
         << line << " error: "
         << gluErrorString(errcode) << "\n";
}

bool
PureOpenGLDisplay::have_extension(const char* name)
{
  char* extensions = (char*)glGetString(GL_EXTENSIONS);
  if(!extensions)
    return false;

  istringstream in(extensions);
  while(in){
    string exname;
    in >> exname;
    if(in && exname == name)
      return true;
  }
  return false;
}

bool
PureOpenGLDisplay::glImageInfo(const Image* image,
                               GLenum& format, GLenum& type, bool& have_alpha)
{
  have_alpha = true;
  if(typeid(*image) == typeid(SimpleImage<RGBA8Pixel>)){
    format = GL_RGBA;
    type = big_endian? GL_UNSIGNED_INT_8_8_8_8 : GL_UNSIGNED_INT_8_8_8_8_REV;
  } else if(typeid(*image) == typeid(SimpleImage<RGB8Pixel>)){
    format = GL_RGB;
    type = GL_UNSIGNED_BYTE;
    have_alpha = false;
  } else if(typeid(*image) == typeid(SimpleImage<ABGR8Pixel>)){
    format = GL_RGBA;
    type = big_endian? GL_UNSIGNED_INT_8_8_8_8_REV : GL_UNSIGNED_INT_8_8_8_8;
  } else if(typeid(*image) == typeid(SimpleImage<ARGB8Pixel>)){
    format = GL_BGRA;
    type = big_endian? GL_UNSIGNED_INT_8_8_8_8_REV : GL_UNSIGNED_INT_8_8_8_8;
  } else if(typeid(*image) == typeid(SimpleImage<BGRA8Pixel>)){
    format = GL_BGRA;
    type = big_endian? GL_UNSIGNED_INT_8_8_8_8 : GL_UNSIGNED_INT_8_8_8_8_REV;
  } else if(typeid(*image) == typeid(SimpleImage<RGBAfloatPixel>)){
    format = GL_RGBA;
    type = GL_FLOAT;
  } else if(typeid(*image) == typeid(SimpleImage<RGBZfloatPixel>)){
      format = GL_RGBA;
      type = GL_FLOAT;
  } else if(typeid(*image) == typeid(SimpleImage<RGBfloatPixel>)){
    format = GL_RGB;
    type = GL_FLOAT;
  } else {
    return false;
  }

  return true;
}
