
#include <GL/glu.h>
#include <X11/Xutil.h>

#include <Core/Exceptions/Exception.h>
#include <Core/Exceptions/InternalError.h>
#include <Engine/Display/GLXImageDisplay.h>
#include <Image/SimpleImage.h>
#include <Image/Pixel.h>
#include <Interface/Image.h>
#include <Interface/Context.h>
#include <Core/XUtils/XHelper.h>

#include <iostream>
#include <vector>

using namespace Manta;
using std::vector;

// Note that the GLXImage display must be passed a GLX context by the
// application it is embedded inside of.
GLXImageDisplay::GLXImageDisplay( XVisualInfo *x_visual_info_, 
																	GLXDrawable glx_drawable_ ) 
	: x_visual_info( x_visual_info_ ), 
		glx_drawable( glx_drawable_ ),
		x_display( 0 )
{
  int x = 0x12345678;
  char* p = (char*)&x;
  if(p[0] == 0x12)
    big_endian = true;
  else
    big_endian = false;		
}

GLXImageDisplay::GLXImageDisplay( bool use_stereo_, GLXDrawable glx_drawable_ ) 
	:
  use_stereo( use_stereo_ ),
  x_visual_info( 0 ), 
  glx_drawable( glx_drawable_ ),
  x_display( 0 )
{
  int x = 0x12345678;
  char* p = (char*)&x;
  if(p[0] == 0x12)
    big_endian = true;
  else
    big_endian = false;		
}

GLXImageDisplay::~GLXImageDisplay() {
	
	// Close the X display.
	if (x_display != 0) {
	
		// Destroy the gl context.
		glXDestroyContext( x_display, glx_context );
		
		// Disconnect from X server.
		XCloseDisplay( x_display );
		x_display = 0;
	}
}

// Manta ImageDisplay interface.
void GLXImageDisplay::setupDisplayChannel( SetupContext &context ) {


  // The setup for the window should be done in the driving application.
  // Note that if stereo is desired it must be specified when creating the visual
  // in the driving application.
  
	// std::cout << "setupDisplayChannel " << context.proc << std::endl;

	// Only the display processor.
	if (context.proc != 0)
		return;
			
	// Check to see if we are already connected to the X server.
	if (x_display == 0) {
		// std::cout << "setupDisplayChannel: Connecting to server. " << context.proc << std::endl;
		if ((x_display = XOpenDisplay( 0 )) == 0) {
      throw InternalError( "Could not connect to X display", __FILE__, __LINE__ );
    }
	}
	

  // Check to see if a visual info was specified.
  if (x_visual_info == 0) {

    // Get the default screen for the display.
    int x_screen = DefaultScreen( x_display );
    
    // Form attributes for the visual
    vector<int> attributes;
    attributes.push_back(GLX_RGBA);
    attributes.push_back(GLX_RED_SIZE);   attributes.push_back(1);
    attributes.push_back(GLX_GREEN_SIZE); attributes.push_back(1);
    attributes.push_back(GLX_BLUE_SIZE);  attributes.push_back(1);
    attributes.push_back(GLX_ALPHA_SIZE); attributes.push_back(0);
    attributes.push_back(GLX_DEPTH_SIZE); attributes.push_back(0);
    
    // Require stereo.
    if (use_stereo) {
      attributes.push_back(GLX_STEREO);
    }
    
    attributes.push_back(GLX_DOUBLEBUFFER); // This must be the last one
    attributes.push_back(None);

    // Choose the visual
    if ((x_visual_info = glXChooseVisual(x_display, x_screen, &attributes[0])) == 0) {
      throw InternalError( "Could not find glx visual.", __FILE__, __LINE__ );
    }
  }
    
  // Create the glx context.
	if ((glx_context = glXCreateContext( x_display, x_visual_info, 0, true )) == 0) {
    throw InternalError( "Could not create glx context.", __FILE__, __LINE__ );
  }

    
	// Call make current to associate with this thread
	// NOTE THIS MUST BE CALLED EACH TIME THE DISPLAY THREAD CHANGES!!

	if (!glXMakeCurrent( x_display, glx_drawable, glx_context )) {
    throw InternalError( "Could not make glx context current", __FILE__, __LINE__ );
  }
	
	// Copy out the manta channel.
	manta_channel = context.channelIndex;

  // Get the fonts.  You need to call this with a current GL context.
  if (!(font_info = XHelper::getX11Font( x_display )))
    throw new InternalError("getX11Font failed!\n", __FILE__, __LINE__ );

  if (!(fontbase = XHelper::getGLFont(font_info))) 
    throw new InternalError("getGLFont failed!\n", __FILE__, __LINE__ );

  // Setup opengl.
  glDisable( GL_DEPTH_TEST );
  
  // Clear the screen.
  glClearColor(.05, .1, .2, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glXSwapBuffers( x_display, glx_drawable );
  glFinish();
}

void GLXImageDisplay::displayImage( const DisplayContext &context, const Image* image ) {

  // Check to see if this processor should display the image.
  if (context.proc != 0)
    return;
  const SimpleImageBase* si = dynamic_cast<const SimpleImageBase*>(image);
  if(!si)
    return;
	
  // Determine the image resolution.
  int xres, yres;
  bool stereo;

  image->getResolution( stereo, xres, yres );
		
  // This code is from OpenGLImageDisplay.
  glViewport(0, 0, xres, yres);
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
  } else if(typeid(*image) == typeid(SimpleImage<RGBAfloatPixel>)){
    format = GL_RGBA;
    type = GL_FLOAT;
  } else if(typeid(*image) == typeid(SimpleImage<RGBfloatPixel>)){
    format = GL_RGB;
    type = GL_FLOAT;
  } else {
    return false;
  }
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
    
  // Output fps.
  // display_frame_rate( 1.0 );
    
  // Swap buffers.
  glXSwapBuffers( x_display, glx_drawable );
    
  // Check for any gl error.
  GLenum errcode = glGetError();
  if(errcode != GL_NO_ERROR) {
    std::cerr << "GLXImageDisplay: Error code from OpenGL: " << gluErrorString(errcode) << std::endl;
  }
}

void GLXImageDisplay::display_frame_rate(double framerate) {

  // Display textual information on the screen:
  char buf[200];
  if (framerate > 1)
    sprintf( buf, "%3.1lf fps", framerate);
  else
    sprintf( buf, "%2.2lf fps - %3.1lf spf", framerate , 1.0f/framerate);

  // Figure out how wide the string is
  int width = XHelper::calc_width(font_info, buf);

  // Now we want to draw a gray box beneth the font using blending. :)
  glRasterPos2i(0,0);
  glEnable(GL_BLEND);
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glColor4f(0.5,0.5,0.5,0.5);
  glRecti(8,3-font_info->descent-2,12+width,font_info->ascent+3);
  glDisable(GL_BLEND);

  XHelper::printString(fontbase, 10, 3, buf, RGBColor(1,1,1));
}
