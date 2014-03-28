
#ifndef Manta_Model_XWindowUI_h
#define Manta_Model_XWindowUI_h

#include <Interface/UserInterface.h>
#include <Core/Util/Callback.h>
#include <Interface/SetupCallback.h>
#include <Core/Geometry/Vector.h>
#include <Core/Thread/Runnable.h>
#include <Core/Thread/Mutex.h>
#include <Core/Thread/Semaphore.h>
#include <Interface/Object.h>
#include <X11/Xlib.h>

#include <vector>
#include <string>
#include <iosfwd>

namespace Manta {
  using namespace std;

  class MantaInterface;
  class TrackBall;
  class XWindow;
  class CameraPath;
  class TimeViewSampler;
  
  class XWindowUI: public UserInterface, public SetupCallback, public Runnable {
  public:
    XWindowUI(const vector<string>& args, MantaInterface *rtrt_interface);
    virtual ~XWindowUI();

    // From UserInterface
    virtual void startup();

    // From SetupCallback
    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);

    // From Runnable
    virtual void run();

    // Camera path
    CameraPath* getCameraPath(void) const;
    void setCameraPath(CameraPath* path);    

    static UserInterface* create(const vector<string>& args,
				 MantaInterface *rtrt_interface);

    void addConnection(int fd);
    void removeConnection(int fd);

    typedef CallbackBase_3Data<unsigned int, unsigned long, int> KeyEventCallbackType;
    // Args are (state, current_button, event, channel, mouse_x, mouse_y).
    typedef CallbackBase_6Data<unsigned int, unsigned int, int, int, int, int> MouseEventCallbackType;

    void register_key(unsigned int state, unsigned long key,
		      const string& description,
		      KeyEventCallbackType* keypress,
		      KeyEventCallbackType* keyrelease = 0);
    void register_mouse(unsigned int state, unsigned int button,
                        const string& description,
                        MouseEventCallbackType* events);

    // This is the callback function used to notify the engine that we
    // want a resize.
    void changeResolution(int, int, int channel, int new_xres, int new_yres);

    void shutdown(MantaInterface*);

    void gameTransform(int, Vector, Vector); // This method is called via a transaction.

  protected:
    void register_default_keys();
    void register_default_mouse();
    void register_game_mouse();
    void printhelp(std::ostream& out);

  private:
    void helpkey(unsigned int, unsigned long, int);
    void prockey(unsigned int, unsigned long, int);
    void quitkey(unsigned int, unsigned long, int);
    void autoview(unsigned int, unsigned long, int);

    void next_bookmark(unsigned int, unsigned long, int);		
    void add_bookmark(unsigned int, unsigned long, int);		
    void add_knot(unsigned int, unsigned long, int);
    void lights_helper(int, int);
	void lights(unsigned int, unsigned long, int);
    void write_knots(unsigned int, unsigned long, int);
    void animate(unsigned int, unsigned long, int);
    void reset_path(unsigned int, unsigned long, int);
    void output_camera(unsigned int, unsigned long, int);
    void output_window(unsigned int, unsigned long, int);

    // button_state (ShiftMask, ControlMask, Mod1Mask),
    // current_button (Button1,2,3,4,5),
    // event (ButtonPress, ButtonRelease, MotionNotify),
    // channel, mouse_x, mouse_y
    void mouse_fov(unsigned int, unsigned int, int, int, int, int);
    void mouse_translate(unsigned int, unsigned int, int, int, int, int);
    void mouse_dolly(unsigned int, unsigned int, int, int, int, int);
    void mouse_wheel_dolly(unsigned int, unsigned int, int, int, int, int);
    void mouse_rotate(unsigned int, unsigned int, int, int, int, int);
    void mouse_track_and_pan(unsigned int, unsigned int, int, int, int, int);
    void mouse_track(unsigned int, unsigned int, int, int, int, int);
    void mouse_pan(unsigned int, unsigned int, int, int, int, int);
    void mouse_debug_ray(unsigned int, unsigned int, int, int, int, int);
    // Transaction function
    void shootDebugRays(int channel, int mouse_x, int mouse_y, int numPixels);

    void animation_callback(int, int, bool&);

    void change_maxDepth(unsigned int, unsigned long, int);

    // This is a pointer to the interface we will use to interact with manta.
    MantaInterface *rtrt_interface;
    vector<XWindow*> windows;

    void lock_x();
    void unlock_x();
    void createWindow(XWindow* window, int channelIndex);
    Display* dpy;
    vector<int> xfds;
    int xpipe[2];
    Atom deleteWindow; // Used to get notification the window was
                       // closed by the window manager.

    CameraPath *path;

    // TimeViewSampler stuff
    void timeView_keyboard(unsigned int /*state*/,
                           unsigned long /*key*/,
                           int /*channel*/);
    void toggleTimeView();
    TimeViewSampler* timeView;
    
    Mutex xlock;
    Semaphore xsema;

    struct KeyTab {
      unsigned int state;
      unsigned long key;
      string description;
      KeyEventCallbackType* press;
      KeyEventCallbackType* release;
    };
    vector<KeyTab> keys;

    struct MouseTab {
      unsigned int state;
      unsigned int button;
      string description;
      MouseEventCallbackType* event;
    };
    vector<MouseTab> mouse;

    void handle_event(XEvent& event);

    // Use by the various mouse handlers
    struct InteractionState {
      MouseEventCallbackType* current_mouse_handler;
      unsigned int current_button;
      int orig_x, orig_y;
      int last_x, last_y;
      Vector rotate_from;
    };
    vector<InteractionState> interactions;

    // Interaction parameters
    Real fov_speed;
    Real translate_speed;
    Real invert;
    Real dolly_speed;
    Real rotate_speed;
    Real autoview_fov;
    Real trackball_radius;
    bool quitting;

    bool setup_game_mode;

    // The image size
    int width, height;
	
	//for visible lights
	vector<Object*> lightsRenderables;
	Object* originalObject;
	bool lightsVisible;
    bool waitingToDeleteLights;
	
    XWindowUI(const XWindowUI&);
    XWindowUI& operator=(const XWindowUI&);

  };
}

#endif
