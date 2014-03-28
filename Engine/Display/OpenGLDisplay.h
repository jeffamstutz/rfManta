
#ifndef Manta_Engine_OpenGLDisplay_h
#define Manta_Engine_OpenGLDisplay_h

#include <Interface/ImageDisplay.h>
#include <Core/Util/Assert.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#include <string>
#include <iostream>
#include <vector>

namespace Manta {
  using namespace std;
  class XWindow;
  class PureOpenGLDisplay;
  
  class OpenGLDisplay : public ImageDisplay {
  public:
    OpenGLDisplay(const vector<string>& args);
    OpenGLDisplay(XVisualInfo* visualInfo, Window window);
    virtual ~OpenGLDisplay();
    virtual void setupDisplayChannel(SetupContext&);
    virtual void displayImage(const DisplayContext& context, const Image* image);
    static ImageDisplay* create(const vector<string>& args);

    // Accessors.
    bool getDisplayFrameRate() const { return displayFrameRate; }
    void setDisplayFrameRate( bool b ) { displayFrameRate = b; }

    // The location of the fps display.
    enum {
      LowerLeft  = 0,
      LowerRight = 1,
      UpperLeft  = 2,
      UpperRight = 3
    };

    int getFPSDisplayLocation() const { return fpsDisplayLocation; }
    void setFPSDisplayLocation( int new_loc ) { fpsDisplayLocation = new_loc; }

  private:
    OpenGLDisplay(const OpenGLDisplay&);
    OpenGLDisplay& operator=(const OpenGLDisplay&);

    void createWindow(bool stereo, int xres, int yres, XWindow* masterWindow);
    Window parentWindow;
    XVisualInfo* vi;
    Window win;
    Display* dpy;
    int screen;
    bool windowOpen;
    bool madeCurrent;
    GLXContext cx;
    int old_xres, old_yres;
    // These contain stuff for font stuff.
    XFontStruct* fontInfo;
    GLuint fontbase;

    int displayProc;
    string mode;
    PureOpenGLDisplay* ogl;
    bool verbose;

#ifndef SWIG
    // Handy little class to test whether a variable has been set.
    // This assumes that you only want to use the value if you called
    // set.
    template<class T>
    class MaybeSet {
    public:
      MaybeSet(): isset_(false) {}
      void set(const T& new_val) {
        isset_ = true;
        val = new_val;
      }
      T& get() { ASSERT(isset_); return val; }
      const T& get() const { ASSERT(isset_); return val; }
      
      bool isSet() { return isset_; }
    private:
      T val;
      bool isset_;
    };
    MaybeSet<bool> single_buffered;
    MaybeSet<bool> use_buffersubdata;
#endif

    bool displayFrameRate;
    int fpsDisplayLocation;

    // This is when the last frame started displaying.  Use it to
    // compute the framerate.
    double last_frame_time;

    // Used to display the frame rate on the screen
    void display_frame_rate(double framerate, int xres, int yres);

    void gl_print_error(const char *file, int line);

    void setDefaults();
    void setup();
  };
}

#endif
