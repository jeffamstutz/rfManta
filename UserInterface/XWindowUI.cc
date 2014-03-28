#include <UserInterface/XWindowUI.h>
#include <Core/Util/Args.h>
#include <Core/Color/ColorSpace_fancy.h>
#include <Interface/Camera.h>
#include <Interface/CameraPath.h>
#include <Interface/Context.h>
#include <Interface/FrameState.h>
#include <Interface/MantaInterface.h>
#include <Interface/Transaction.h>
#include <Interface/XWindow.h>
#include <Interface/Scene.h>
#include <Interface/Object.h>
#include <Interface/Light.h>
#include <Model/Lights/PointLight.h>
#include <Model/Primitives/PrimaryRaysOnly.h>
#include <Interface/LightSet.h>
#include <Interface/RayPacket.h>
#include <Model/Groups/Group.h>
#include <Model/Primitives/Sphere.h>
#include <Model/Materials/CopyTextureMaterial.h>
#include <Engine/PixelSamplers/TimeViewSampler.h>
#include <Core/Exceptions/ErrnoException.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Geometry/AffineTransform.h>
#include <Core/Math/MiscMath.h>
#include <Core/Math/Expon.h>
#include <Core/Thread/Runnable.h>
#include <Core/Thread/Thread.h>
#include <Core/XUtils/XHelper.h>
#include <Core/Geometry/BBox.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>

#include <sstream>
#include <functional>
#include <algorithm>

// TODO:
// 1. Specify UI for only particular channels
// 2. Specify display
// 3. More orderly shutdown when rendering quits?
// 4. Expose events
// 5. Mouse events
// 6. Changing # of channels may not be thread safe

#define TOKEN_NOOP 1
#define TOKEN_LOCK 2
#define TOKEN_SHUTDOWN 3

using namespace Manta;

UserInterface* XWindowUI::create(const vector<string>& args,
                                 MantaInterface *rtrt_interface)
{
  UserInterface *newUI = new XWindowUI(args, rtrt_interface);
  return newUI;
}

static void connection_watch(Display*, XPointer data, int fd,
                             Bool opening, XPointer*)
{
  XWindowUI* ui = reinterpret_cast<XWindowUI*>(data);
  if(opening)
    ui->addConnection(fd);
  else
    ui->removeConnection(fd);
}

XWindowUI::XWindowUI(const vector<string>& args, MantaInterface *rtrt_interface) :
  rtrt_interface(rtrt_interface),
  path( 0 ),
  timeView(0),
  xlock("XWindowUI display lock"),
  xsema("XWindowUI semaphore", 0),
  quitting(false)
{
  bool quit=false;
  int max_count=0;
  int offset=0;
  CameraPath::IOMode mode = CameraPath::WriteKnots;
  string fname="";

  fov_speed = 10;
  translate_speed = 1;
  invert = 1.0;
  dolly_speed = 5;
  rotate_speed = 2;
  autoview_fov = 60;
  trackball_radius = 0.8;
  setup_game_mode = false;

  // Parse command line args.
  for (size_t i=0;i<args.size();++i) {
    const string &arg = args[i];
    if (arg=="-loop") {
      if (!getIntArg(i, args, max_count))
        throw IllegalArgument("XWindowUI -loop", i, args);
    } else if (arg=="-offset") {
      if (!getIntArg(i, args, offset))
        throw IllegalArgument("XWindowUI -offset", i, args);
    } else if (arg=="-opath") {
      if (!getStringArg(i, args, fname))
        throw IllegalArgument("XWindowUI -opath", i, args);
      mode=CameraPath::WriteKnots;
    } else if (arg=="-path") {
      if (!getStringArg(i, args, fname))
        throw IllegalArgument("XWindowUI -path", i, args);
      mode=CameraPath::ReadKnots;
    } else if (arg=="-game") {
      setup_game_mode = true;
      double speed = 1.0;
      getDoubleArg(i, args, speed);
      translate_speed = speed;
    } else if (arg=="-invert"){
      invert = -1.0;
    } else if (arg=="-quit") {
      quit=true;
    }
  }

  // Create a CameraPath (if necessary)
  if (fname != "") {
    CameraPath::LoopMode loopMode=CameraPath::Infinite;
    if (quit) {
      loopMode=CameraPath::CountAndQuit;
      if (max_count == 0)
        max_count = 1;
    } else if (max_count > 0) {
      loopMode=CameraPath::Count;
    }

    path = new CameraPath(rtrt_interface, fname, mode, loopMode, max_count,
                          offset);
  }

  lightsVisible = false;
  waitingToDeleteLights = false;
  originalObject = NULL;

  register_default_keys();
  if (setup_game_mode)
      register_game_mouse();
  else
      register_default_mouse();
  rtrt_interface->registerSetupCallback(this);
  rtrt_interface->registerSerialAnimationCallback(Callback::create(this, &XWindowUI::animation_callback));
  rtrt_interface->registerTerminationCallback(Callback::create(this, &XWindowUI::shutdown));

  // Open the display and setup file descriptors for threads
  if(pipe(xpipe) == -1){
    throw ErrnoException( "XWindowUI pipe", errno, __FILE__, __LINE__ );
  }
  dpy = XOpenDisplay(NULL);
  if(!dpy)
    throw InternalError("XOpenDisplay failed");
  if(XAddConnectionWatch(dpy, connection_watch, reinterpret_cast<XPointer>(this)) == 0)
    throw InternalError("XAddConnectionWatch failed");

  if(xfds.size() == 0){
    // XAddConnectionWatch must be broken.  To fix it, we look at the
    // second fd of the pipe, get a new fd, and assume that any X
    // file descriptors are between the two.
    int tmp_pipe[2];
    if(pipe(tmp_pipe) == -1)
      throw ErrnoException("XWindowUI pipe2", errno, __FILE__, __LINE__ );

    for(int i=xpipe[1]+1; i<tmp_pipe[0];i++)
      addConnection(i);
  }

  // Will create an atom if it doesn't exist
  bool only_if_exists = false;
  deleteWindow = XInternAtom( dpy, "WM_DELETE_WINDOW", only_if_exists);
  // Probably check some error codes or something.
}

XWindowUI::~XWindowUI()
{
}

void XWindowUI::startup()
{
  Thread* t = new Thread(this, "Manta User Interface");
  t->setDaemon(true);
  t->detach();
}

void XWindowUI::shutdown(MantaInterface*)
{
  int token = TOKEN_SHUTDOWN;
  ssize_t s = write(xpipe[1], &token, sizeof(int));
  if(s != sizeof(int)){
    throw ErrnoException("XWindowUI write 3", errno, __FILE__, __LINE__ );
  }
}

void XWindowUI::setupBegin(const SetupContext&, int numChannels)
{
  if(setup_game_mode){
    BBox bbox;
    PreprocessContext ppc;
    rtrt_interface->getScene()->getObject()->computeBounds(ppc, bbox);
    translate_speed *= bbox.diagonal().length() * 0.6;
    setup_game_mode = false; // Avoid scaling translate_speed again if user hits 'p'
  }
  if(numChannels > static_cast<int>(windows.size())){
    int oldsize = static_cast<int>(windows.size());
    windows.resize(numChannels);
    for(int i=oldsize;i<numChannels;i++){
      windows[i] = new XWindow();
      windows[i]->window = 0;
      windows[i]->xres = windows[i]->yres = 0;
    }

    interactions.resize(numChannels);
    for(int i=oldsize;i<numChannels;i++){
      interactions[i].current_mouse_handler = 0;
    }
  } else if(numChannels < static_cast<int>(windows.size())){
    for(int i=numChannels;i<static_cast<int>(windows.size());i++){
      delete windows[i];
    }
    windows.resize(numChannels);
    interactions.resize(numChannels);
  }
}

void XWindowUI::setupDisplayChannel(SetupContext& context)
{
  XWindow* window = windows[context.channelIndex];
  if(context.masterWindow)
    cerr << "WARNING: XWindowUI not setting master window, already set\n";
  else
    context.masterWindow = window;

  lock_x();
  bool stereo;
  int xres, yres;
  context.getResolution(stereo, xres, yres);
  if(!window->window){
    window->xres = xres;
    window->yres = yres;
    createWindow(window, context.channelIndex);
  } else if(window->xres != xres || window->yres != yres){
    XResizeWindow(dpy, window->window, xres, yres);
    window->xres = xres;
    window->yres = yres;
  }
  unlock_x();
}

void XWindowUI::run()
{
  xlock.lock();
  for(;;){
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(xpipe[0], &readfds);
    int max = xpipe[0];
    for(vector<int>::iterator iter = xfds.begin(); iter != xfds.end(); iter++){
      FD_SET(*iter, &readfds);
      if(*iter > max)
        max = *iter;
    }
    int s;
    do {
      s = select(max+1, &readfds, 0, 0, 0);
      if(s == -1 && errno != EINTR)
        throw ErrnoException("XWindowUI select", errno, __FILE__, __LINE__ );
    } while(s <= 0);

    if(FD_ISSET(xpipe[0], &readfds)){
      // Process the pipe...
      int token;
      ssize_t s = read(xpipe[0], &token, sizeof(token));
      if(s != sizeof(token))
        throw ErrnoException("XWindowUI read pipe", errno,
                             __FILE__, __LINE__ );
      switch(token){
      case TOKEN_NOOP:
        break;
      case TOKEN_LOCK:
        xlock.unlock();
        xsema.down();
        xlock.lock();
        break;
      case TOKEN_SHUTDOWN:
        XCloseDisplay(dpy);
        return;
        break;
      default:
        throw InternalError("XWindowUI::Unknown token in pipe");
      }
    }
    bool have_some = false;
    for(vector<int>::iterator iter = xfds.begin(); iter != xfds.end(); iter++){
      if(FD_ISSET(*iter, &readfds)){
        XProcessInternalConnection(dpy, *iter);
        have_some = true;
      }
    }
    if(have_some){
      while(XEventsQueued(dpy, QueuedAfterReading)){
        XEvent e;
        XNextEvent(dpy, &e);
        handle_event(e);
      }
    }
  }
}

void XWindowUI::addConnection(int fd)
{
  xfds.push_back(fd);
}

void XWindowUI::removeConnection(int fd)
{
  xfds.erase(remove_if(xfds.begin(), xfds.end(), bind2nd(equal_to<int>(), fd)));
}

CameraPath* XWindowUI::getCameraPath(void) const
{
  return path;
}

void XWindowUI::setCameraPath(CameraPath* newPath)
{
  path=newPath;
}



void XWindowUI::lock_x()
{
  int token = TOKEN_LOCK;
  ssize_t s = write(xpipe[1], &token, sizeof(int));
  if(s != sizeof(int)){
    throw ErrnoException("XWindowUI write 3", errno, __FILE__, __LINE__ );
  }
  xlock.lock();
}

void XWindowUI::unlock_x()
{
  xlock.unlock();
  xsema.up();
}

void XWindowUI::createWindow(XWindow* window, int channel)
{
  XHelper::Xlock.lock();
  int screen=DefaultScreen(dpy);

  XSetWindowAttributes atts;
  int flags=CWEventMask;
  atts.event_mask=StructureNotifyMask|ExposureMask|ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|KeyPressMask|KeyReleaseMask;
  Window parent = RootWindow(dpy, screen);
  window->window = XCreateWindow(dpy, parent, 100, 100,
                                 window->xres, window->yres, 0,
                                 DefaultDepth(dpy, screen), InputOutput,
                                 DefaultVisual(dpy, screen), flags, &atts);

  width = window->xres;
  height = window->yres;

  XTextProperty tp;
  ostringstream title;
  title << "Manta ray tracer (channel " << channel << ")";
  // This used to be a const_cast<char*>(title...), but it somehow
  // data in name pointed to was corrupted.
  char* name = strdup(title.str().c_str());
  XStringListToTextProperty(&name, 1, &tp);
  free(name);

  XSizeHints sh;
  sh.flags = USPosition|USSize;

  XSetWMProperties(dpy, window->window, &tp, &tp, 0, 0, &sh, 0, 0);
  // The last arg is the number of atom &deleteWindow points to.
  XSetWMProtocols(dpy, window->window, &deleteWindow, 1);
  XMapWindow(dpy, window->window);

  XHelper::Xlock.unlock();

  // Wait for the window to appear before proceeding
  for(;;){
    XEvent e;
    XNextEvent(dpy, &e);
    if(e.type == MapNotify)
      break;
  }
}

void XWindowUI::register_key(unsigned int state, unsigned long key,
                             const string& description,
                             KeyEventCallbackType* press,
                             KeyEventCallbackType* release)
{
  // cerr << description << ": " << (void*)key << '\n';
  if(key == 0)
    throw InternalError("tried to register invalid key: "+description);

  for(vector<KeyTab>::iterator iter = keys.begin();
      iter != keys.end(); iter++){
    if(key == iter->key && state == iter->state)
      throw InternalError("Two functions mapped to the same key: "+description+" and "+iter->description);
  }
  KeyTab entry;
  entry.key = key;
  entry.state = state;
  entry.description = description;
  entry.press = press;
  entry.release = release;
  keys.push_back(entry);
}

void XWindowUI::register_mouse(unsigned int state, unsigned int button,
                               const string& description,
                               MouseEventCallbackType* event)
{
  // cerr << description << ": " << button << '\n';
  for(vector<MouseTab>::iterator iter = mouse.begin();
      iter != mouse.end(); iter++){
    if(button == iter->button && state == iter->state)
      throw InternalError("Two functions mapped to the same mouse button: "+description+" and "+iter->description);
  }
  MouseTab entry;
  entry.state = state;
  entry.button = button;
  entry.description = description;
  entry.event = event;
  mouse.push_back(entry);
}

void XWindowUI::changeResolution(int, int, int channel,
                                 int new_xres, int new_yres)
{
  // cerr << "Resolution: " << new_xres << " " << new_yres << endl;
  width = new_xres;
  height = new_yres;
  rtrt_interface->changeResolution(channel, new_xres, new_yres, true);
}

void XWindowUI::printhelp(ostream& out)
{
  int desccol = 20;
  for(vector<KeyTab>::iterator iter = keys.begin();
      iter != keys.end(); iter++){
    streampos start = out.tellp();
    if(iter->state & Mod1Mask)
      out << "Alt-";
    if(iter->state & ControlMask)
      out << "Control-";
    if(iter->state & ShiftMask)
      out << "Shift-";
    if(iter->state & ~(ShiftMask|ControlMask|Mod1Mask))
      out << "OtherModifier-";
    char* c = XKeysymToString(iter->key);
    if(!c)
      out << "(Unknown keysym " << iter->key << ')';
    else
      out << c;
    int diff = desccol-(out.tellp()-start);
    if(diff < 1)
      diff=1;
    for(int i=0;i<diff;i++)
      out << ' ';
    out << iter->description << '\n';
  }

  for(vector<MouseTab>::iterator iter = mouse.begin();
      iter != mouse.end(); iter++){
    streampos start = out.tellp();
    if(iter->state & Mod1Mask)
      out << "Alt-";
    if(iter->state & ControlMask)
      out << "Control-";
    if(iter->state & ShiftMask)
      out << "Shift-";
    if(iter->state & ~(ShiftMask|ControlMask|Mod1Mask))
      out << "OtherModifier-";
    out << "mouse";
    out << iter->button;
    int diff = desccol-(out.tellp()-start);
    if(diff < 1)
      diff=1;
    for(int i=0;i<diff;i++)
      out << ' ';
    out << iter->description << '\n';
  }
}

void XWindowUI::register_default_keys()
{
  register_key(0, XStringToKeysym("b"),
               "next bookmark",
               Callback::create(this, &XWindowUI::next_bookmark));
  register_key(0, XStringToKeysym("B"),
               "add bookmark",
               Callback::create(this, &XWindowUI::add_bookmark));
  register_key(0, XStringToKeysym("c"),
               "output camera",
               Callback::create(this, &XWindowUI::output_camera));
  register_key(0, XStringToKeysym("d"),
               "increase maxDepth by one",
               Callback::create(this, &XWindowUI::change_maxDepth));
  register_key(0, XStringToKeysym("D"),
               "decrease maxDepth by one",
               Callback::create(this, &XWindowUI::change_maxDepth));
  register_key(0, XStringToKeysym("h"),
               "print help message to stderr",
               Callback::create(this, &XWindowUI::helpkey));
  register_key(0, XStringToKeysym("k"),
               "add knot",
               Callback::create(this, &XWindowUI::add_knot));
  register_key(0, XStringToKeysym("K"),
               "write knots",
               Callback::create(this, &XWindowUI::write_knots));
  register_key(0, XStringToKeysym("l"),
			   "display lights",
			   Callback::create(this, &XWindowUI::lights));
  register_key(0, XStringToKeysym("p"),
               "increase/decrease number of processors",
               Callback::create(this, &XWindowUI::prockey));
  register_key(0, XStringToKeysym("P"),
               "increase/decrease number of processors",
               Callback::create(this, &XWindowUI::prockey));
  register_key(0, XStringToKeysym("q"),
               "quit after this frame, press again to immediately quit",
               Callback::create(this, &XWindowUI::quitkey));
  register_key(0, XStringToKeysym("r"),
               "reset path",
               Callback::create(this, &XWindowUI::reset_path));
  register_key(0, XStringToKeysym("t"),
               "time profile display",
               Callback::create(this, &XWindowUI::timeView_keyboard));
  register_key(ControlMask, XStringToKeysym("t"),
               "time profile scale increase",
               Callback::create(this, &XWindowUI::timeView_keyboard));
  register_key(ControlMask, XStringToKeysym("T"),
               "time profile scale decrease",
               Callback::create(this, &XWindowUI::timeView_keyboard));
  register_key(0, XStringToKeysym("v"),
               "autoview",
               Callback::create(this, &XWindowUI::autoview));
  register_key(0, XStringToKeysym("w"),
               "output window",
               Callback::create(this, &XWindowUI::output_window));
  register_key(0, XK_space,
               "animate path or start/stop animation",
               Callback::create(this, &XWindowUI::animate));
  register_key(0, XStringToKeysym("Escape"),
               "quit after this frame, press again to immediately quit",
               Callback::create(this, &XWindowUI::quitkey));
  register_key(0, XStringToKeysym("question"),
               "print help message to stderr",
               Callback::create(this, &XWindowUI::helpkey));
}

void XWindowUI::register_default_mouse()
{
  register_mouse(ShiftMask, Button3,
                 "change field of view",
                 Callback::create(this, &XWindowUI::mouse_fov));
  register_mouse(0, Button3,
                 "dolly to/from lookat",
                 Callback::create(this, &XWindowUI::mouse_dolly));
  register_mouse(0, Button4,
                 "dolly to/from lookat",
                 Callback::create(this, &XWindowUI::mouse_wheel_dolly));
  register_mouse(0, Button5,
                 "dolly to/from lookat",
                 Callback::create(this, &XWindowUI::mouse_wheel_dolly));
  register_mouse(0, Button1,
                 "rotate",
                 Callback::create(this, &XWindowUI::mouse_rotate));
  register_mouse(0, Button2,
                 "translate",
                 Callback::create(this, &XWindowUI::mouse_translate));
  register_mouse(ControlMask, Button1,
                 "Debug ray",
                 Callback::create(this, &XWindowUI::mouse_debug_ray));
  register_mouse(ShiftMask, Button1,
                 "Debug SSE ray",
                 Callback::create(this, &XWindowUI::mouse_debug_ray));
}

void XWindowUI::register_game_mouse()
{
  register_mouse(0, Button1,
                 "track and pan",
                 Callback::create(this, &XWindowUI::mouse_track_and_pan));
  register_mouse(0, Button2,
                 "track",
                 Callback::create(this, &XWindowUI::mouse_track));
  register_mouse(0, Button3,
                 "pan",
                 Callback::create(this, &XWindowUI::mouse_pan));
  register_mouse(ShiftMask, Button3,
                 "change field of view",
                 Callback::create(this, &XWindowUI::mouse_fov));
  register_mouse(ControlMask, Button1,
                 "Debug ray",
                 Callback::create(this, &XWindowUI::mouse_debug_ray));
  register_mouse(ShiftMask, Button1,
                 "Debug SSE ray",
                 Callback::create(this, &XWindowUI::mouse_debug_ray));
}

void XWindowUI::helpkey(unsigned int, unsigned long, int)
{
  cerr << "Keymapping:\n";
  printhelp(cerr);
  cerr << "\n";
}

struct changeProcs {
  changeProcs(int value) : value(value) {}
  int value;
  int operator()(int oldProcs) const {
    int newProcs = oldProcs+value;
    if(newProcs < 1)
      newProcs = 1;
    cerr << "Setting number of processors to: " << newProcs << endl;
    return newProcs;
  }
};


void XWindowUI::prockey(unsigned int, unsigned long key, int)
{
  int value = 1;
  if(key == XStringToKeysym("P"))
    value = -1;
  rtrt_interface->addTransaction("numWorkers",
                                 rtrt_interface->numWorkers(),
                                 changeProcs(value));
}

void XWindowUI::quitkey(unsigned int, unsigned long, int)
{
  if(quitting){
    // Pressed twice, do a faster shutdown
    cerr << "Fast quit\n";
    Thread::exitAll(1);
  } else {
    quitting = true;
    rtrt_interface->finish();
  }
}

void XWindowUI::autoview(unsigned int, unsigned long, int channel)
{
  Camera* camera = rtrt_interface->getCamera(channel);
  BBox bbox;
  PreprocessContext ppc;
  rtrt_interface->getScene()->getObject()->computeBounds(ppc, bbox);
  rtrt_interface->addTransaction("autoview",
                                 Callback::create(camera, &Camera::autoview,
                                                  bbox));
}

void XWindowUI::next_bookmark(unsigned int, unsigned long, int channel)
{
  Camera* camera = rtrt_interface->getCamera(channel);
  const BasicCameraData* bookmark = rtrt_interface->getScene()->nextBookmark();
  if(bookmark){
    rtrt_interface->addTransaction("next bookmark",
                                   Callback::create(camera, &Camera::setBasicCameraData,
                                                    *bookmark));
  }
}

void XWindowUI::add_bookmark(unsigned int, unsigned long, int channel)
{
  Camera* camera = rtrt_interface->getCamera(channel);
  BasicCameraData bookmark;
  camera->getBasicCameraData(bookmark);
  rtrt_interface->getScene()->addBookmark("user added", bookmark);
}

void XWindowUI::add_knot(unsigned int, unsigned long, int channel)
{
  if (!path) {
    cerr<<"Creating a new CameraPath; writing knots to \"out.path\"\n";
    path=new CameraPath(rtrt_interface, "out.path", CameraPath::WriteKnots);
  }

  BasicCameraData data;
  rtrt_interface->getCamera(channel)->getBasicCameraData(data);
  path->addKnot(rtrt_interface->getFrameState().frameSerialNumber, data);
}

void XWindowUI::write_knots(unsigned int, unsigned long, int)
{
  if (!path)
    return;

  path->writeKnots();
}

void XWindowUI::lights_helper(int, int)
{
    for (vector<Object*>::iterator itr = lightsRenderables.begin(); itr != lightsRenderables.end(); itr++)
        delete *itr;
    lightsRenderables.clear();
    waitingToDeleteLights = false;
}

void XWindowUI::lights(unsigned int, unsigned long, int)
{
    if (waitingToDeleteLights)
        return;
	if (lightsVisible)
	{
		lightsVisible = false;
		rtrt_interface->getScene()->setObject(originalObject);
		//TODO: I don't delete the object itself as it contains the original object... does its destructor destroy its children?
        rtrt_interface->addOneShotCallback( MantaInterface::Relative, 2, Callback::create(this, &XWindowUI::lights_helper));
        waitingToDeleteLights = true;
	}
	else 
	{
		lightsVisible = true;
		Scene* scene = rtrt_interface->getScene();
		Group* group = new Group();
		originalObject = scene->getObject();
		group->add(originalObject);
		LightSet* lights = scene->getLights();
		for(unsigned int i = 0; i < lights->numLights(); i++)
		{
            PointLight* light = dynamic_cast<PointLight*>(lights->getLight(i));
            if (light)
            {
                Object* renderable = new PrimaryRaysOnly(new Sphere(new CopyTextureMaterial(light->getColor()), light->getPosition(), 0.5f));
                if (renderable)
                {
                    group->add(renderable);
                    lightsRenderables.push_back(renderable);
                }
            }
		}
		scene->setObject(group);
	}
}

void XWindowUI::animate(unsigned int, unsigned long, int channel)
{
  if (!path){
    // Start/stop time
    if(rtrt_interface->timeIsStopped()){
      rtrt_interface->startTime();
    } else {
      rtrt_interface->stopTime();
    }
  } else {
    rtrt_interface->addTransaction("animate path",
                                   Callback::create(path, &CameraPath::animate));
  }
}

void XWindowUI::reset_path(unsigned int, unsigned long, int channel)
{
  if (!path)
    return;

  rtrt_interface->addTransaction("reset path",
                                 Callback::create(path, &CameraPath::reset));
}

void XWindowUI::toggleTimeView()
{
  TimeViewSampler* currentTimeView = TimeViewSampler::toggleTimeView(rtrt_interface, timeView);
  if (timeView != currentTimeView) {
    // TODO: (bigler) should we delete the old one?
    timeView = currentTimeView;
  }
}

void XWindowUI::timeView_keyboard(unsigned int state,
                                  unsigned long key,
                                  int /*channel*/)
{
  if (timeView == NULL) {
    // We are in an uninitialized state.  Check to see if we can grab
    // a TimeViewSampler from the MantaInterface or if we need to
    // allocate one.
    timeView = dynamic_cast<TimeViewSampler*>(rtrt_interface->getPixelSampler());
    if (timeView == NULL)
      timeView = new TimeViewSampler(vector<string>());
  }
  if (state & ControlMask) {
    ColorComponent factor = static_cast<ColorComponent>(1.1);
    if(key == XStringToKeysym("T"))
      factor = 1/factor;
    rtrt_interface->addTransaction("TimeView scale change",
                                   Callback::create(timeView,
                                                    &TimeViewSampler::changeScale,
                                                    factor));
  } else {
    rtrt_interface->addTransaction("TimeView toggle",
                                   Callback::create(this,
                                                    &XWindowUI::toggleTimeView));
  }
}

void XWindowUI::output_camera(unsigned int, unsigned long, int channel)
{
  Camera* camera = rtrt_interface->getCamera(channel);
  rtrt_interface->addTransaction("output camera",
                                 Callback::create<Camera,std::ostream &>(camera, &Camera::output, std::cout));
}

void XWindowUI::output_window(unsigned int, unsigned long, int)
{
  std::cout << "window resolution: " << width << " " << height << std::endl;
}

void XWindowUI::mouse_fov(unsigned int, unsigned int,
                          int event, int channel, int mouse_x, int mouse_y)
{
  InteractionState& ias = interactions[channel];
  if(event == ButtonPress){
    ias.last_x = mouse_x;
    ias.last_y = mouse_y;
  } else {
    // Zoom in/out.
    XWindow* window = windows[channel];
    Real xmotion= Real(ias.last_x - mouse_x)/window->xres;
    Real ymotion= Real(ias.last_y - mouse_y)/window->yres;
    Real scale;
    if (Abs(xmotion) > Abs(ymotion))
      scale = xmotion;
    else
      scale = ymotion;
    scale *= fov_speed;

    if (scale < 0)
      scale = 1/(1-scale);
    else
      scale += 1;

    Camera* camera = rtrt_interface->getCamera(channel);
    rtrt_interface->addTransaction("scale fov",
                                   Callback::create(camera, &Camera::scaleFOV,
                                                    scale));

    ias.last_x = mouse_x;
    ias.last_y = mouse_y;
  }
}

void XWindowUI::mouse_rotate(unsigned int, unsigned int,
                             int event, int channel, int mouse_x, int mouse_y)
{
  InteractionState& ias = interactions[channel];
  XWindow* window = windows[channel];
  Real xpos = (Real)2*mouse_x/window->xres - 1;
  Real ypos = 1 - (Real)2*mouse_y/window->yres;
  if(event == ButtonPress){
    ias.rotate_from = projectToSphere(xpos, ypos, trackball_radius);
  } else {
    Vector to(projectToSphere(xpos, ypos, trackball_radius));
    AffineTransform trans;

    trans.initWithRotation(to, ias.rotate_from);
    ias.rotate_from = to;

    Camera* camera = rtrt_interface->getCamera(channel);
    rtrt_interface->addTransaction("rotate",
                                   Callback::create(camera, &Camera::transform,
                                                    trans, Camera::LookAt));
    ias.last_x = mouse_x;
    ias.last_y = mouse_y;
  }
}

void XWindowUI::mouse_translate(unsigned int, unsigned int,
                                int event, int channel,
                                int mouse_x, int mouse_y)
{
  InteractionState& ias = interactions[channel];
  if(event == ButtonPress){
    ias.last_x = mouse_x;
    ias.last_y = mouse_y;
  } else {
    XWindow* window = windows[channel];
    Real xmotion =  Real(ias.last_x-mouse_x)/window->xres;
    Real ymotion = -Real(ias.last_y-mouse_y)/window->yres;
    Vector translation(xmotion*translate_speed, ymotion*translate_speed, 0);

    Camera* camera = rtrt_interface->getCamera(channel);
    rtrt_interface->addTransaction("translate",
                                   Callback::create(camera, &Camera::translate,
                                                    translation));
    ias.last_x = mouse_x;
    ias.last_y = mouse_y;
  }
}

void XWindowUI::mouse_dolly(unsigned int, unsigned int,
                            int event, int channel, int mouse_x, int mouse_y)
{
  InteractionState& ias = interactions[channel];
  if(event == ButtonPress){
    ias.last_x = mouse_x;
    ias.last_y = mouse_y;
  } else {
    XWindow* window = windows[channel];
    Real xmotion = -Real(ias.last_x-mouse_x)/window->xres;
    Real ymotion = -Real(ias.last_y-mouse_y)/window->yres;

    Real scale;
    if (Abs(xmotion)>Abs(ymotion))
      scale=xmotion;
    else
      scale=ymotion;
    scale *= dolly_speed;

    Camera* camera = rtrt_interface->getCamera(channel);
    rtrt_interface->addTransaction("dolly",
                                   Callback::create(camera, &Camera::dolly,
                                                    scale));
    ias.last_x = mouse_x;
    ias.last_y = mouse_y;
  }
}

void XWindowUI::mouse_wheel_dolly(unsigned int /*button_state*/,
                                  unsigned int current_button,
                                  int /*event*/, int channel,
                                  int /*mouse_x*/, int /*mouse_y*/)
{
  Real scale = 0.01 * dolly_speed;
  if (current_button == Button4) {
    // This will undo our actions
    scale = scale/(scale-1);
  } else if (current_button == Button5) {
    // Keep as is
  } else {
    // Don't recongnize this event
    return;
  }

  Camera* camera = rtrt_interface->getCamera(channel);
  rtrt_interface->addTransaction("dolly",
                                 Callback::create(camera, &Camera::dolly,
                                                  scale));
}

void XWindowUI::gameTransform(int channel, Vector rotation, Vector translation)
{
  // This code is called by a transaction so it is thread safe.
  BasicCameraData cd = rtrt_interface->getCamera(channel)->getBasicCameraData();

  Vector gaze( cd.eye - cd.lookat );
  Vector side( Cross( cd.up, gaze ) );
  Vector projectedGaze( gaze - cd.up * Dot( cd.up, gaze ) );
  Vector projectedSide( side - cd.up * Dot( cd.up, side ) );
  gaze.normalize();
  side.normalize();
  projectedGaze.normalize();
  projectedSide.normalize();
  AffineTransform frame;
  frame.initWithTranslation( -cd.eye );
  frame.rotate( side, rotation.y() );
  frame.rotate( cd.up, rotation.x() );
  frame.translate( projectedSide * translation.x() );
  frame.translate( cd.up * translation.y() );
  frame.translate( projectedGaze * translation.z() );
  frame.translate( cd.eye );
  cd.eye = frame.multiply_point( cd.eye );
  cd.lookat = frame.multiply_point( cd.lookat );
  frame.initWithRotation( gaze, rotation.z() );
  cd.up = frame.multiply_vector( cd.up );

  rtrt_interface->getCamera(channel)->setBasicCameraData( cd );
}


void XWindowUI::mouse_track_and_pan(unsigned int, unsigned int,
                                    int event, int channel, int mouse_x, int mouse_y)
{
  InteractionState& ias = interactions[channel];
  if(event == ButtonPress){
    ias.last_x = mouse_x;
    ias.last_y = mouse_y;
  } else {
    XWindow* window = windows[channel];
    Real xmotion =  Real(ias.last_x-mouse_x)/window->xres;
    Real ymotion = -Real(ias.last_y-mouse_y)/window->yres;
    Vector rotation(xmotion*rotate_speed, 0, 0);
    Vector translation(0, 0, ymotion*translate_speed);

    rtrt_interface->addTransaction("game",
                                   Callback::create(this, &XWindowUI::gameTransform,
                                                    channel, rotation, translation));
    ias.last_x = mouse_x;
    ias.last_y = mouse_y;
  }
}

void XWindowUI::mouse_track(unsigned int, unsigned int,
                            int event, int channel, int mouse_x, int mouse_y)
{
  InteractionState& ias = interactions[channel];
  if(event == ButtonPress){
    ias.last_x = mouse_x;
    ias.last_y = mouse_y;
  } else {
    XWindow* window = windows[channel];
    Real xmotion = -Real(ias.last_x-mouse_x)/window->xres;
    Real ymotion =  Real(ias.last_y-mouse_y)/window->yres;
    Vector rotation(0, 0, 0);
    Vector translation(xmotion*translate_speed, ymotion*translate_speed, 0);

    rtrt_interface->addTransaction("game",
                                   Callback::create(this, &XWindowUI::gameTransform,
                                                    channel, rotation, translation));

    ias.last_x = mouse_x;
    ias.last_y = mouse_y;
  }
}

void XWindowUI::mouse_pan(unsigned int, unsigned int,
                            int event, int channel, int mouse_x, int mouse_y)
{
  InteractionState& ias = interactions[channel];
  if(event == ButtonPress){
    ias.last_x = mouse_x;
    ias.last_y = mouse_y;
  } else {
    XWindow* window = windows[channel];
    Real xmotion = Real(ias.last_x-mouse_x)/window->xres;
    Real ymotion = invert*Real(ias.last_y-mouse_y)/window->yres;
    Vector rotation(xmotion*rotate_speed, ymotion*rotate_speed, 0);
    Vector translation(0, 0, 0);

    rtrt_interface->addTransaction("game",
                                   Callback::create(this, &XWindowUI::gameTransform,
                                                    channel, rotation, translation));    
    
    ias.last_x = mouse_x;
    ias.last_y = mouse_y;
  }
}

void XWindowUI::shootDebugRays(int channel,
                               int mouse_x, int mouse_y,
                               int numPixels)
{
  XWindow* window = windows[channel];
  std::cerr << "Debug ray at ("<<mouse_x<<", "<<mouse_y<<")\n";
  if (numPixels > RayPacket::MaxSize) {
    std::cerr << "XWindowUI::shootDebugRays: numPixels("<<numPixels<<") > RayPacket::MaxSize("<<RayPacket::MaxSize<<")\n";
    return;
  }

  RayPacketData data;
  RayPacket rays( data, RayPacket::LinePacket, 0, numPixels, 0, 0 );

  Real image_x = (2.0*mouse_x+1)/window->xres - 1.0;
  Real image_y = 1.0 - (2.0*mouse_y+1)/window->yres;
  for(int i = 0; i < numPixels; ++i) {
    Real x = (2.0*mouse_x+i+1)/window->xres - 1.0;
    rays.setPixel(i, 0, x, image_y);
  }

  // TODO(bigler): shouldn't ConstantEye be set in the camera?
  rays.setFlag( RayPacket::DebugPacket | RayPacket::ConstantEye );

        // Shoot the rays.
        Color color;
        rtrt_interface->shootOneRay( color, rays, image_x, image_y, channel );

  for(int i = 0; i < numPixels; ++i) {
    if (rays.wasHit(i)) {
      std::cerr << "t("<<i<<"):       " << rays.getMinT(i) << "\n";
      std::cerr << "hit pos("<<i<<"): " << rays.getHitPosition(i) << "\n";
      std::cerr << "color  : "          << rays.getColor(i).toString() << "\n";
    }
  }

}

void XWindowUI::mouse_debug_ray(unsigned int state, unsigned int,
                                int event, int channel,
                                int mouse_x, int mouse_y)
{
  if(event != ButtonPress) {
    return;
  }
  int numPixels = 1;
  if (state & ControlMask) {
    numPixels = 1;
  } else if (state & ShiftMask) {
    numPixels = 4;
  }
  rtrt_interface->addTransaction("XWindowUI::shootDebugRays",
                                 Callback::create(this,
                                                  &XWindowUI::shootDebugRays,
                                                  channel,
                                                  mouse_x, mouse_y, numPixels)
                                 );
}

void XWindowUI::handle_event(XEvent& e)
{
  // Find out which channel the event came from. Use a linear search
  // because there will only be a few channels
  int channel = -1;
  int numChannels = (int)windows.size();
  for(int i=0;i<numChannels;i++){
    if(windows[i]->window == e.xany.window){
      channel = i;
      break;
    }
  }
  if(channel == -1){
    cerr << "Warning: event received for unknown window: " << e.xany.window << '\n';
    return;
  }
  InteractionState& ias = interactions[channel];
  switch(e.type){
  case KeyPress:
    {
      bool matched = false;
      unsigned long key = XKeycodeToKeysym(dpy, e.xkey.keycode, 0);
      e.xkey.state &= ShiftMask|ControlMask|Mod1Mask;
      if(e.xkey.state & ShiftMask){
        unsigned long key2 = XKeycodeToKeysym(dpy, e.xkey.keycode, 1);
        if(key2 != key){
          key = key2;
          e.xkey.state &= ~ShiftMask;
        }
      }
      for(vector<KeyTab>::iterator iter = keys.begin();
          iter != keys.end(); iter++){
        if(key == iter->key && e.xkey.state == iter->state){
          if(iter->press){
            matched=true;
            iter->press->call(e.xkey.state, key, channel);
          }
        }
      }
      if(!matched){
        if(IsModifierKey(key)){
          // Don't print help for these keys
        } else {
          cerr << "Unknown key: ";
          char* s = XKeysymToString(key);
          if(!s)
            cerr << "Unknown keysym: " << key;
          else
            cerr << s << " (" << key << ")";
          if(e.xkey.state){
            cerr << " (";
            if(e.xkey.state & Mod1Mask)
              cerr << "Alt ";
            if(e.xkey.state & ControlMask)
              cerr << "Control ";
            if(e.xkey.state & ShiftMask)
              cerr << "Shift ";
            if(e.xkey.state & ~(Mod1Mask|ControlMask|ShiftMask))
              cerr << "OtherModifier ";
            cerr << ")";
          }
          cerr << '\n';
          helpkey(e.xkey.state, key, channel);
        }
      }
    }
    break;
  case KeyRelease:
    {
      unsigned long key = XKeycodeToKeysym(dpy, e.xkey.keycode, 0);
      for(vector<KeyTab>::iterator iter = keys.begin();
          iter != keys.end(); iter++){
        if(key == iter->key && e.xkey.state == iter->state){
          if(iter->release)
            iter->release->call(e.xkey.state, key, channel);
        }
      }
    }
    break;
  case ButtonPress:
    if(ias.current_mouse_handler){
      // Release the old one first...
      ias.current_mouse_handler->call(e.xbutton.state, ias.current_button,
                                      ButtonRelease, channel,
                                      e.xbutton.x, e.xbutton.y);
      ias.current_mouse_handler = 0;
    }
    ias.current_button = e.xbutton.button;

    // This will make sure that the mouse functions are only sensitive
    // to when the Shift, Control, or Mod1Mask is set.  Similar to the
    // KeyPress event.

    //cerr << "e.xbutton.state = "<<e.xbutton.state<<" Mod1Mask = "<<Mod1Mask;
    e.xbutton.state &= ShiftMask|ControlMask|Mod1Mask;
    //cerr << " e.xbutton.state = "<<e.xbutton.state<<"\n";
    for(vector<MouseTab>::iterator iter = mouse.begin();
        iter != mouse.end(); iter++){
      if(e.xbutton.button == iter->button && e.xbutton.state == iter->state){
        ias.current_mouse_handler = iter->event;
        ias.current_mouse_handler->call(e.xbutton.state, ias.current_button,
                                        ButtonPress, channel,
                                        e.xbutton.x, e.xbutton.y);
        break;
      }
    }
    if(!ias.current_mouse_handler){
      cerr << "Warning, didn't find mouse handler for button "
           << ias.current_button << ", state=" << e.xbutton.state << '\n';
    }
    break;
  case ButtonRelease:
    if(ias.current_mouse_handler){
      ias.current_mouse_handler->call(e.xbutton.state, ias.current_button,
                                      ButtonRelease, channel,
                                      e.xbutton.x, e.xbutton.y);
      ias.current_button = 0;
      ias.current_mouse_handler = 0;
    } else {
      //cerr << "Dropped button relase event for button " << e.xbutton.button
      //<< ", probably because multiple mouse buttons pressed\n";
    }
    break;
  case MotionNotify:
    if(ias.current_mouse_handler){
      ias.current_mouse_handler->call(e.xmotion.state, ias.current_button,
                                      MotionNotify, channel,
                                      e.xmotion.x, e.xmotion.y);
    } else {
      //cerr << "Dropped button motion event for button " << ias.current_button
      //<< ", probably because multiple mouse buttons pressed\n";
    }
    break;
  case ConfigureNotify:
    {
      int new_xres = e.xconfigure.width;
      int new_yres = e.xconfigure.height;
      XWindow* window = windows[channel];
      if(new_xres != window->xres || new_yres != window->yres)
        rtrt_interface->addOneShotCallback(MantaInterface::Relative, 0,
                                           Callback::create(this, &XWindowUI::changeResolution,
                                                            channel, new_xres, new_yres));
    } // case ConfigureNotify
    break;
  case Expose:
    //    cerr << "Expose event found.\n";
    break;
  case ClientMessage:
    {
      // The case was being done implicitly.  Now making it explicit.
      // This is how wxWidgets does it, at least.
      if (static_cast<Atom>(e.xclient.data.l[0]) == deleteWindow) {
        //        cerr << "Delete window event caught\n";
        // Takes three arguments that are simply ignored
        quitkey(0,0,0);
      }
    } // case ClientMessage
    break;
  default:
    cerr << "Unknown X event: " << e.type << '\n';
    break;
  }
}

void XWindowUI::animation_callback(int, int, bool& changed)
{
  for(size_t i=0;i<interactions.size();i++)
    if(interactions[i].current_mouse_handler){
      changed=true;
      break;
    }
}

static void change_maxDepth_callback(Scene* scene,
                                     int change)
{
  scene->getRenderParameters().changeMaxDepth(change);
  cerr << "New ray maxDepth = "<<scene->getRenderParameters().maxDepth<<"\n";
}

void XWindowUI::change_maxDepth(unsigned int /*state*/,
                                unsigned long key,
                                int /*chanel*/)
{
  int change = 1;
  if (key == XStringToKeysym("D"))
    change = -1;
  rtrt_interface->addTransaction("maxDepth change",
                                 Callback::create(&change_maxDepth_callback,
                                                  rtrt_interface->getScene(),
                                                  change));
}

