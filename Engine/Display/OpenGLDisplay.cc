
#include <Engine/Display/OpenGLDisplay.h>
#include <Engine/Display/PureOpenGLDisplay.h>
#include <Core/XUtils/XHelper.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Util/Args.h>
#include <Image/NullImage.h>
#include <Interface/Context.h>
#include <Interface/XWindow.h>

#include <Core/Exceptions/IllegalValue.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Thread/Thread.h>
#include <Core/Thread/Time.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <X11/Xutil.h>
#include <typeinfo>
#include <stdio.h>

using namespace Manta;
using namespace std;

ImageDisplay* OpenGLDisplay::create(const vector<string>& args)
{
  return new OpenGLDisplay(args);
}

OpenGLDisplay::OpenGLDisplay(const vector<string>& args)
{
  // Open X window
  setDefaults();
  for(size_t i=0;i<args.size();i++){
    if(args[i] == "-parentWindow"){
      long window;
      if(!getLongArg(i, args, window))
        throw IllegalArgument("OpenGLDisplay -parentWindow", i, args);
      else
        parentWindow = static_cast<Window>(window);
    } else if(args[i] == "-displayProc"){
      if(!getIntArg(i, args, displayProc))
        throw IllegalArgument("OpenGLDisplay -displayProc", i, args);
      if(displayProc < 0)
        throw IllegalValue<int>("Display processor should be >= 0", displayProc);
    } else if(args[i] == "-mode"){
      if(!getStringArg(i, args, mode))
        throw IllegalArgument("OpenGLDisplay -mode", i, args);
      if(mode != "image" && mode != "texture" && mode != "pbo")
        throw IllegalValue<string>("Illegal dispay mode", mode);
    } else if(args[i] == "-displayFrameRate"){
      displayFrameRate = true;
    } else if(args[i] == "-nodisplayFrameRate"){
      displayFrameRate = false;
    } else if(args[i] == "-v"){
      verbose = true;
    } else if(args[i] == "-singlebuffered"){
      single_buffered.set(true);
    } else if(args[i] == "-doublebuffered"){
      single_buffered.set(false);
    } else if(args[i] == "-buffersubdata"){
      use_buffersubdata.set(true);
    } else if(args[i] == "-mapbuffer"){
      use_buffersubdata.set(false);
    } else {
      throw IllegalArgument("OpenGLDisplay", i, args);
    }
  }
  vi = 0;
  win = 0;
  setup();
}

OpenGLDisplay::OpenGLDisplay(XVisualInfo* visualInfo, Window window)
{
  setDefaults();
  vi = visualInfo;
  win = window;
  setup();
}

void OpenGLDisplay::setDefaults()
{
  parentWindow = 0;
  displayProc = 0;
  displayFrameRate = true;
  mode = "texture";
  verbose = false;
  fpsDisplayLocation = LowerLeft;
}

void OpenGLDisplay::setup()
{
  madeCurrent = false;
  windowOpen = false;
  dpy = 0;
  // If this is zero then the value hasn't been used yet.
  last_frame_time = 0;
  ogl = new PureOpenGLDisplay(mode);
  if (verbose) ogl->verbose = verbose;
  if (single_buffered.isSet())
    ogl->single_buffered = single_buffered.get();
  if (use_buffersubdata.isSet())
    ogl->use_buffersubdata = use_buffersubdata.get();
}

OpenGLDisplay::~OpenGLDisplay()
{
  if(win)
    XDestroyWindow(dpy, win);
  if(dpy)
    XCloseDisplay(dpy);
}

void OpenGLDisplay::setupDisplayChannel(SetupContext& context)
{
  bool stereo;
  int xres, yres;
  context.getResolution(stereo, xres, yres);
  if(!windowOpen){
    createWindow(stereo, xres, yres, context.masterWindow);
    old_xres = xres;
    old_yres = yres;
  } else if(old_xres != xres || old_yres != yres){
    XResizeWindow(dpy, win, xres, yres);
    old_xres = xres;
    old_yres = yres;
  }
}

void OpenGLDisplay::createWindow(bool stereo, int xres, int yres, XWindow* masterWindow)
{
  XHelper::Xlock.lock();
  // Open the display and make sure it has opengl
  dpy = XOpenDisplay(NULL);
  if(!dpy) {
    XHelper::Xlock.unlock();
    throw InternalError("Error opening display");
  }
  int error, event;
  if ( !glXQueryExtension( dpy, &error, &event) ) {
    XCloseDisplay(dpy);
    dpy=0;
    XHelper::Xlock.unlock();
    throw InternalError("GL extension NOT available!");
  }
  int screen=DefaultScreen(dpy);
    
  // Form the criteria for the visual
  vector<int> attribList;
  attribList.push_back(GLX_RGBA);
  attribList.push_back(GLX_RED_SIZE); attribList.push_back(1);
  attribList.push_back(GLX_GREEN_SIZE); attribList.push_back(1);
  attribList.push_back(GLX_BLUE_SIZE); attribList.push_back(1);
  attribList.push_back(GLX_ALPHA_SIZE); attribList.push_back(0);
  attribList.push_back(GLX_DEPTH_SIZE); attribList.push_back(0);

  // Require stereo.
  if (stereo) {
    attribList.push_back(GLX_STEREO);
  }
  
  attribList.push_back(GLX_DOUBLEBUFFER); // This must be the last one
  attribList.push_back(None);

  bool free_vi = (vi == 0?true:false);
  vi = glXChooseVisual(dpy, screen, &attribList[0]);
  if(!vi){
    // We failed to choose a visual.  Try again without the double-buffer flag
    attribList.pop_back();
    attribList.pop_back();
    attribList.push_back(None);
    vi = glXChooseVisual(dpy, screen, &attribList[0]);
    if(!vi){
      // Cannot find anything suitable
      XHelper::Xlock.unlock();
      throw InternalError("Error selecting OpenGL visual");
    }
  }

  if(!win){
    Colormap cmap = XCreateColormap(dpy, RootWindow(dpy, screen),
                                    vi->visual, AllocNone);

    XSetWindowAttributes atts;
    int flags=CWColormap|CWEventMask|CWBackPixmap|CWBorderPixel;
    atts.background_pixmap = None;
    atts.border_pixmap = None;
    atts.border_pixel = 0;
    atts.colormap=cmap;
    atts.event_mask=StructureNotifyMask;
    Window parent = parentWindow;
    if(!parent){
      if(masterWindow){
        parent = masterWindow->window;
      } else {
        parent = RootWindow(dpy, screen);
      }
    }
    win = XCreateWindow(dpy, parent, 0, 0, xres, yres, 0, vi->depth,
                        InputOutput, vi->visual, flags, &atts);

    if(!parentWindow){
      XTextProperty tp;
      const char* name = "Manta";
      XStringListToTextProperty(const_cast<char**>(&name), 1, &tp);
      XSizeHints sh;
      sh.flags = USPosition|USSize;
      
      XSetWMProperties(dpy, win, &tp, &tp, 0, 0, &sh, 0, 0);
    }
  
    XMapWindow(dpy, win);
  
    // Wait for the window to appear before proceeding
    for(;;){
      XEvent e;
      XNextEvent(dpy, &e);
      if(e.type == MapNotify)
        break;
    }

    // Turn off events from this window
    atts.event_mask = 0;
    XChangeWindowAttributes(dpy, win, CWEventMask, &atts);
  }
  windowOpen = true;

  cx = glXCreateContext(dpy, vi, NULL, True);
  if(free_vi){
    XFree(vi);
    vi = 0;
  }

  if(!glXMakeCurrent(dpy, win, cx)) {
    XHelper::Xlock.unlock();
    throw InternalError("glXMakeCurrent failed!");
  }

  ogl->clearScreen(.05, .1, .2, 0);
  glXSwapBuffers(dpy, win);
  glFinish();

  // Get the fonts.  You need to call this with a current GL context.
  fontInfo = XHelper::getX11Font(dpy);
  if (!fontInfo) {
    XHelper::Xlock.unlock();
    throw InternalError("getX11Font failed!");
  }
  fontbase = XHelper::getGLFont(fontInfo);
  if (fontbase == 0) {
    XHelper::Xlock.unlock();
    throw InternalError("getGLFont failed!");
  }

  ogl->init();
  
  if(!glXMakeCurrent(dpy, None, NULL)) {
    XHelper::Xlock.unlock();
    throw InternalError("glXMakeCurrent failed!");
  }

  XHelper::Xlock.unlock();
}

void OpenGLDisplay::displayImage(const DisplayContext& context,
                                 const Image* image)
{
  if(context.proc != displayProc%context.numProcs)
    return;
  // We will look at the typeid of the image to determine what kind it
  // is and avoid extra dynamic_casts.  We do that instead of using a
  // virtual function in image to avoid having a zillion virtual functions
  // in the Image interface.  This assumes that there will be more display
  // types that image types.

  if(typeid(*image) == typeid(NullImage))
    return;

  // Compute the framerate
  double currentTime = Time::currentSeconds();

  if(!madeCurrent){
    if(!glXMakeCurrent(dpy, win, cx))
      throw InternalError("glXMakeCurrent failed!");
    madeCurrent=true;
  }

  bool stereo;
  int xres, yres;
  image->getResolution(stereo, xres, yres);

  ogl->displayImage(image);

  // Display the frame rate
  if(displayFrameRate) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, xres, 0, yres);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    display_frame_rate(1.0/(currentTime-last_frame_time), xres, yres);
  }
  last_frame_time = currentTime;
  
  glXSwapBuffers(dpy, win);
  gl_print_error(__FILE__,__LINE__);

  // Suck up X events to keep opengl happy
  while (XPending(dpy)) {
    XEvent e;
    XNextEvent(dpy, &e);
  }
}

void OpenGLDisplay::display_frame_rate(double framerate, int xres, int yres)
{
  // Display textual information on the screen:
  char buf[200];
  if (framerate > 1)
    sprintf( buf, "%3.1lf fps", framerate);
  else
    sprintf( buf, "%2.2lf fps - %3.1lf spf", framerate , 1.0f/framerate);
  // Figure out how wide the string is
  int width = XHelper::calc_width(fontInfo, buf);
  // Now we want to draw a gray box beneth the font using blending. :)
  glEnable(GL_BLEND);
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glColor4f(0.5,0.5,0.5,0.5);
  int yoffset = 3;
  int xoffset = 8;
  switch (fpsDisplayLocation) {
  case LowerRight:
    xoffset = xres-width-14;
    break;
  case UpperLeft:
    yoffset = yres - fontInfo->ascent - 3;
    break;
  case UpperRight:
    yoffset = yres - fontInfo->ascent - 3;
    xoffset = xres-width-14;
    break;
    // Default and LowerLeft cases don't need to do anything
  }
  glRecti(xoffset,         yoffset-fontInfo->descent-2,
          xoffset+6+width, yoffset+fontInfo->ascent);
  glDisable(GL_BLEND);
  XHelper::printString(fontbase, xoffset+2, yoffset+1, buf, RGBColor(1,1,1));
}

void OpenGLDisplay::gl_print_error(const char *file, int line)
{
  GLenum errcode = glGetError();
  if(errcode != GL_NO_ERROR)
    std::cerr << "OpenGLDisplay: "
              << file << ":"
              << line << " error: "
              << gluErrorString(errcode) << std::endl;
}

