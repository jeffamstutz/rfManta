
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

#include <Interface/MantaInterface.h>

#include <Engine/Factory/Factory.h>

#include <Core/Util/Args.h>
#include <Core/Util/Callback.h>
#include <Interface/Scene.h>
#include <Interface/Object.h>
#include <Interface/Camera.h>
#include <Interface/UserInterface.h>
#include <Interface/Context.h>
#include <Core/Geometry/BBox.h>
#include <Core/Exceptions/Exception.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/UnknownComponent.h>
#include <Core/Exceptions/UnknownColor.h>
#include <Core/Thread/Time.h>
#include <Core/Util/About.h>
#include <Engine/PixelSamplers/TimeViewSampler.h>

// Default scene includes.
#include <Core/Color/ColorDB.h>
#include <Interface/LightSet.h>
#include <Model/AmbientLights/ConstantAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Backgrounds/TextureBackground.h>
#include <Model/Lights/PointLight.h>
#include <Model/Textures/Constant.h>
#include <Model/Textures/CheckerTexture.h>
#include <Model/Textures/MarbleTexture.h>
#include <Model/Materials/Phong.h>
#include <Model/Materials/Flat.h>
#include <Model/Groups/Group.h>
#include <Model/Primitives/Parallelogram.h>
#include <Model/Primitives/Sphere.h>
#include <Model/TexCoordMappers/UniformMapper.h>
#include <Core/Thread/Thread.h>

#include <UseMPI.h>
#ifdef USE_MPI
# include <Engine/Display/NullDisplay.h>
# include <Engine/LoadBalancers/MPI_LoadBalancer.h>
# include <Engine/ImageTraversers/MPI_ImageTraverser.h>
# include <mpi.h>
#endif

#include <string>
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace Manta;

#if HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

Scene* createDefaultScene();
static void make_stack( ReadContext &context, const vector<string> &args );

static void
printList(ostream& out, const Factory::listType& list, int spaces=0)
{
  for(int i=0;i<spaces;i++)
    out << ' ';
  for(Factory::listType::const_iterator iter = list.begin();
      iter != list.end(); ++iter){
    if(iter != list.begin())
      out << ", ";
    out << *iter;
  }
  out << "\n";
}

static void usage(Factory* rtrt)
{

  cerr << "Manta Interactive Ray Tracer" << "\n\n";

  cerr << getLicenseString() << "\n";

  cerr << "Revision information:\n";

  cerr << getAboutString() << "\n\n";

  cerr << "Usage: manta [options]\n";
  cerr << "Valid options are:\n";
  cerr << " -[-]h[elp]      - Print this message and exit\n";
  cerr << " -bench [N [M]]  - Time N frames after an M frame warmup period and print out the framerate,\n";
  cerr << "                   default N=100, M=10\n";
  cerr << " -np N           - Use N processors\n";
  cerr << " -res NxM        - Use N by M pixels for rendering (needs the x).\n";
  cerr << " -imagedisplay S - Use image display mode named S, valid modes are:\n";
  printList(cerr, rtrt->listImageDisplays(), 4);
  cerr << " -imagetype S    - Use image display mode named S, valid modes are:\n";
  printList(cerr, rtrt->listImageTypes(), 4);
  cerr << " -ui S           - Use the user interface S, valid options are:\n";
  printList(cerr, rtrt->listUserInterfaces(), 4);
  cerr << " -shadows S      - Use S mode for rendering shadows, valid modes are:\n";
  printList(cerr, rtrt->listShadowAlgorithms(), 4);
  cerr << " -imagetraverser S - Use S method for image traversing, valid modes are:\n";
  printList(cerr, rtrt->listImageTraversers(), 4);
  cerr << " -pixelsampler S - Use S method for pixel sampling, valid modes are:\n";
  printList(cerr, rtrt->listPixelSamplers(), 4);
  cerr << " -camera S       - User camera model S, valid cameras are:\n";
  printList(cerr, rtrt->listCameras(), 4);
  cerr << " -bbcamera       - Positions the lookat in the center of the\n";
  cerr << "                   scene, and the eye point far enough away to\n";
  cerr << "                   see the entire scene.\n";
  cerr << " -renderer S     - Use renderer S, valid renderers are:\n";
  printList(cerr, rtrt->listRenderers(), 2);
  cerr << " -scene S        - Render Scene S\n";
  cerr << " -t, --timeview[=\"args\"] - Display a scaled view of the time it took to render some pixels.\n";
  cerr << " --bgcolor [option] - Change the color the background.  Options are:\n"
       << "           o [colorName]      (such as black or white)\n"
       << "           o [RGB8     r g b] (where components range [0,255])\n"
       << "           o [RGBfloat r g b] (where components range [0,1])\n";
  cerr << " --maxdepth [val] - The maximum ray depth\n";
  Thread::exitAll(1);
}

class BenchHelper {
public:
  BenchHelper(MantaInterface* rtrt, long num_frames, int output_format);
  void start(int, int);
  void stop(int, int);

  enum {
    fps_only,
    dart_format,
    default_format // leave me last
  };
private:
  MantaInterface* rtrt;
  double start_time;
  long num_frames;
  int output_format;
};

BenchHelper::BenchHelper(MantaInterface* rtrt, long num_frames, int output_format)
  : rtrt(rtrt), num_frames(num_frames), output_format(output_format)
{
}

void BenchHelper::start(int, int)
{
  start_time = Time::currentSeconds();
}

void BenchHelper::stop(int, int)
{
  double dt = Time::currentSeconds()-start_time;
  double fps = num_frames/dt;
  switch(output_format) {
  case fps_only:
    std::cout << fps << std::endl;
    break;
  case dart_format:
    std::cout << "<DartMeasurement name=\"frames_per_second\" type=\"numeric/double\">"<<fps<<"</DartMeasurement>\n";
    std::cout << "<DartMeasurement name=\"total_time\" type=\"numeric/double\">"<<dt<<"</DartMeasurement>\n";
    break;
  case default_format:
  default:
    cout << "Benchmark completed in " << dt
         << " seconds (" << num_frames << " frames, "
         << fps << " frames per second)" << std::endl;
  }
  rtrt->finish();
  delete this;
}

Factory *factory = 0;

ImageDisplay* createImageDisplay(Factory *factory, const string& spec)
{
#ifdef USE_MPI
  int rank = MPI::COMM_WORLD.Get_rank();
  if (rank != 0) {
    vector<string> dummy_args;
    return new NullDisplay(dummy_args);
  }
#endif
  return factory->createImageDisplay( spec );
}


int
main(int argc, char* argv[])
{
#if HAVE_IEEEFP_H
  fpsetmask(FP_X_OFL|FP_X_DZ|FP_X_INV);
#endif

  // Copy args into a vector<string>
  vector<string> args;

#ifdef USE_MPI
  int provided=-1;
  int requested = MPI_THREAD_MULTIPLE;
  MPI_Init_thread(&argc, &argv, requested, &provided);

  int rank = MPI::COMM_WORLD.Get_rank();

  if (rank == 0 && (provided != requested))
    cerr << "Error: your MPI does not support MPI_THREAD_MULTIPLE!\n"
         << "Bad things might happen if using manta with more than 1 thread...\n";

  args.push_back("-loadbalancer"); args.push_back("MPI_LoadBalancer");
  args.push_back("-imagetraverser"); args.push_back("MPI_ImageTraverser(-square)");
  args.push_back("-imagetype"); args.push_back("rgb8");

  if (rank == 0) {
    /* nothing */
  }
  else {
    args.push_back("-ui"); args.push_back("null");
  }
#endif

  for(int i=1;i<argc;i++) {
#ifdef USE_MPI
    // Only the rank 0 should have this option turned on.
    if (rank != 0) {
      if (string(argv[i]) == "-nodisplaybench" ||
          string(argv[i]) == "-bench") {
        for (++i; i < argc && argv[i][0] != '-'; ++i);
        --i;
        continue;
      }
    }
#endif

    args.push_back(argv[i]);
  }

  try {

    ///////////////////////////////////////////////////////////////////////////
    // Create Manta.
    MantaInterface* rtrt = createManta();

    // Create a Manta Factory.
    factory = new Factory( rtrt );

    // Set the scene search path based on an env variable.
    if(getenv("MANTA_SCENEPATH"))
      factory->setScenePath(getenv("MANTA_SCENEPATH"));
    else
      factory->setScenePath("");

    // Use one worker by default.
    rtrt->changeNumWorkers(1);

    // Default options.
    if(!factory->selectImageType("argb8"))
      throw InternalError("default image not found");

    if(!factory->selectShadowAlgorithm("hard"))
      throw InternalError("default shadow algorithm not found" );

    // Set camera for the default scene.
    Camera* currentCamera = factory->createCamera("pinhole(-normalizeRays)");
    if(!currentCamera)
      throw InternalError("cannot create default camera");

    // Defaults for command line args.
    int xres = 512, yres = 512;
    string stack_file = "";
    bool haveUI = false;
    bool channelCreated=false;
    bool stereo = false;
    bool timeView = false;
    vector<string> timeViewArgs;
    Color bgcolor;
    bool override_scene_bgcolor = false;
    int maxDepth = -1;          // -1 is invalid and represent unset state.


    // Parse command line args.
    try {
      for(size_t i=0;i < args.size();i++){
        string arg = args[i];
        if( (arg == "-help") || (arg == "--help") || (arg == "-h") || (arg == "--h") ) {
          usage(factory);
        }
        else if( arg == "-bench" ||
                 arg == "-quietbench" ||
                 arg == "-nodisplaybench" ||
                 arg == "-nodisplaybench-dart" ) {

          ///////////////////////////////////////////////////////////////////////
          // Benchmark Helper.
          int output_format = BenchHelper::default_format;
          if(arg == "-quietbench")
            output_format = BenchHelper::fps_only;
          if(arg == "-nodisplaybench-dart")
            output_format = BenchHelper::dart_format;
          long numFrames = 100;
          long warmup = 10;
          if(getLongArg(i, args, numFrames)){
            getLongArg(i, args, warmup);
          }
          BenchHelper* b = new BenchHelper(rtrt, numFrames, output_format);
          // Ask for two callbacks, one at frame "warmup", and one at
          // frame warmup+numFrames
          rtrt->addOneShotCallback(MantaInterface::Absolute, warmup,
                                   Callback::create(b, &BenchHelper::start));
          rtrt->addOneShotCallback(MantaInterface::Absolute, warmup+numFrames,
                                   Callback::create(b, &BenchHelper::stop));

          if (arg.find("-nodisplaybench") == 0) {
            // setup the ui and imagedisplay to both be null

            if(!channelCreated) {
              ImageDisplay *display = createImageDisplay(factory, "null" );
              rtrt->createChannel(display, currentCamera, stereo, xres, yres);
              channelCreated=true;
            } else {
              cerr << "ERROR: Already created channel.\n"
                   << "Currently only one channel is supported in bin/manta.\n"
                   << "You can create channels with -imagedisplay or a -nodisplaybench command.\n";
              throw IllegalArgument( string(argv[0]), i, args );
            }
            UserInterface* ui = factory->createUserInterface("null");
            if (!ui) throw UnknownComponent( "Null user interface not found", "");
            ui->startup();
            haveUI = true;
          }

        } else if(arg == "-camera"){
          string s;
          if(!getStringArg(i, args, s))
            usage(factory);
          currentCamera = factory->createCamera(s);
          if(!currentCamera){
            cerr << "Error creating camera: " << s << ", available cameras are:\n";
            printList(cerr, factory->listCameras());
            throw IllegalArgument( s, i, args );
          }
        } else if(arg == "-res"){
          if (channelCreated) {
            cerr << "ERROR: Already created channel.\n"
                 << "Please specify resolution before creating channel (-imagedisplay or a -nodisplaybench command).\n";
            throw IllegalArgument( string(argv[0]), i, args );
          }
          if(!getResolutionArg(i, args, xres, yres)){
            cerr << "Error parsing resolution: " << args[i+1] << '\n';
            usage(factory);
          }
        } else if(arg == "-stereo") {
          stereo = true;

        } else if(arg == "-imagedisplay"){
          string s;
          if(!getStringArg(i, args, s))
            usage(factory);

          // Create the channel.
          try {
            if(!channelCreated) {
              ImageDisplay* display = createImageDisplay(factory, s );

              rtrt->createChannel(display, currentCamera, stereo, xres, yres);
            } else {
              cerr << "Already created channel.\n"
                   << "Currently only one channel is supported in bin/manta.\n"
                   << "You can create channels with -imagedisplay or a -nodisplaybench command.\n";
              throw IllegalArgument( s, i, args );
            }

          } catch (UnknownComponent e) {
            cerr << e.message() << "\n"
                 << "Available image displays: ";
            printList(cerr, factory->listImageDisplays());
            throw e;
          }
          channelCreated=true;

        } else if(arg == "-imagetype"){
          string s;
          if(!getStringArg(i, args, s))
            usage(factory);
          if(!factory->selectImageType(s)){
            cerr << "Invalid image type: " << s << ", available image types are:\n";
            printList(cerr, factory->listImageTypes());
            throw IllegalArgument( s, i, args );
          }

        } else if(arg == "-idlemode"){
          string s;
          if(!getStringArg(i, args, s))
            usage(factory);
          // NOTE(boulos): This is a very bad way to do this...
          if(factory->addIdleMode(s) == static_cast<unsigned int>(-1)){
            cerr << "Invalid idle mode: " << s << ", available idle modes are:\n";
            printList(cerr, factory->listIdleModes());
            throw IllegalArgument( s, i, args );
          }
        } else if(arg == "-np"){
          long np;
          if(!getLongArg(i, args, np))
            usage(factory);
          rtrt->changeNumWorkers(static_cast<int>(np));

        } else if(arg == "-ui"){
          string s;
          if(!getStringArg(i, args, s))
            usage(factory);
          UserInterface* ui = factory->createUserInterface(s);
          if(!ui){
            cerr << "Unknown user interface: " << s << ", available user interfaces are:\n";
            printList(cerr, factory->listUserInterfaces());
            throw IllegalArgument( s, i, args );
          }
          ui->startup();
          haveUI = true;

        } else if(arg == "-shadows"){
          string s;
          if(!getStringArg(i, args, s))
            usage(factory);
          if(!factory->selectShadowAlgorithm(s)){
            cerr << "Invalid shadow algorithm: " << s << ", available shadow algorithms are:\n";
            printList(cerr, factory->listShadowAlgorithms());
            throw IllegalArgument( s, i, args );
          }

        } else if(arg == "-scene"){
          if(rtrt->getScene()!=0)
            cerr << "WARNING: multiple scenes specified, will use last one\n";
          string scene;
          if(!getStringArg(i, args, scene))
            usage(factory);
          if(!factory->readScene(scene)){
            cerr << "Error reading scene: " << scene << '\n';
            throw IllegalArgument( "-scene", i, args );
          }
        }
        else if (arg == "-stack") {
          if (!getStringArg(i,args,stack_file))
            usage(factory);
        }
        else if (arg == "-t") {
          timeView = true;
        }
        else if (arg.find("--timeview") == 0) {
          timeView = true;
          // Pull out the args
          if (arg.size() > 10) {
            string argString(arg, 11, arg.size()-9);
            parseArgs(argString, timeViewArgs);
          }
        }
        else if (arg == "--bgcolor" || arg == "-bgcolor" || arg == "-bgc" ) {
          try {
            if (!getColorArg(i, args, bgcolor)) {
              cerr << "Error parsing bgcolor: " << args[i+1] << '\n';
              usage(factory);
            } else {
              override_scene_bgcolor = true;
            }
          } catch (UnknownColor& e) {
            cerr << "Error parsing bgcolor: "<<e.message()<<"\n";
          }
        }
        else if (arg == "--maxdepth" || arg == "-maxdepth" ) {
          if (!getArg<int>(i, args, maxDepth)) {
            cerr << "Error reading max depth\n";
            usage(factory);
          }
        }
        else {
          // We should barf about unknown commands.  For the stack stuff, args
          // should probably be parsed out some other way instead of on the
          // general command line.

          // Check if the "unknown" arg is really an arg used in make_stack.
          // This is a nasty kludge, but oh well...
          if (arg == "-imagetraverser" ||
              arg == "-loadbalancer" ||
              arg == "-pixelsampler" ||
              arg == "-renderer") {
            // ignore the argument's options
            for (++i; i < args.size() && args[i][0] != '-'; ++i);
            --i;
            continue;
          }
          cerr << "ERROR: Unknown argument: " << arg << "\n";
#if 0
          throw IllegalArgument( string(argv[0]), i, args );
#endif
        }
      }
    }
    catch (Exception& e) {
      cerr << "manta.cc (argument parsing): Caught exception: " << e.message() << '\n';
      Thread::exitAll( 1 );

    } catch (std::exception e){
      cerr << "manta.cc (argument parsing): Caught std exception: "
           << e.what()
           << '\n';
      Thread::exitAll(1);

    } catch(...){
      cerr << "manta.cc (argument parsing): Caught unknown exception\n";
      Thread::exitAll(1);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Use defaults if necessary.

    // Check to see if a channel  was created.
    if(!channelCreated){
      ImageDisplay *display = createImageDisplay(factory, "opengl" );
      rtrt->createChannel(display, currentCamera, stereo, xres, yres);
    }

    // Check to see if a UI was specified
    if(!haveUI){
      UserInterface* ui = factory->createUserInterface("X");
      if(!ui){
        cerr << "Cannot find default user interface: X, available user interfaces are:\n";
        printList(cerr, factory->listUserInterfaces());
        Thread::exitAll(1);
      }
      ui->startup();
    }

#ifdef USE_MPI
    MPI_LoadBalancer::setNumRenderThreadsPerNode(rtrt->numWorkers());

    if (rank == 0) {
      const int minThreads = 2; // We need at least 2 for it to run.

      // subtract one to make space for load balancer process in case it is on
      // the same node.  We take the min of what the user selected and 4, since
      // more than 4 is unlikely to help and it is likely that these might be
      // hyper threaded which would hurt performance.  Ideally we would let the
      // user select both the number of render threads and master threads,
      // where render threads can use hyper threading and master threads use
      // only 1 thread per physical core...
      const int maxThreads = Min(4, rtrt->numWorkers()-1);
      int numThreads = Max(minThreads, maxThreads);
      rtrt->changeNumWorkers(numThreads);
      printf("master node is using %d threads\n", numThreads);
    }
    else if (rank == 1) {
      rtrt->changeNumWorkers(1);
    }
#endif

    // Check if default scene should be used.
    if(rtrt->getScene()==0){
      rtrt->setScene(createDefaultScene());
    }


    ///////////////////////////////////////////////////////////////////////////
    // Configure the rendering stack from a specified file.
    if (stack_file.size()) {
      factory->readStack( stack_file, args );
    }
    else {
      // Pass a read context and all of the command line args to the scene
      // configuration function. The function will ignore some arguments.
      ReadContext context( rtrt );
      make_stack( context, args );
    }

    if (timeView) {
      // Pull out the current pixel sampler and replace it.
      PixelSampler* currentPixelSampler = rtrt->getPixelSampler();
      TimeViewSampler* tvs = new TimeViewSampler(timeViewArgs);
      tvs->setPixelSampler(currentPixelSampler);
      rtrt->setPixelSampler(tvs);
    }

    if (override_scene_bgcolor) {
      Scene* scene = rtrt->getScene();
      Background* bg = scene->getBackground();
      if (bg) delete bg;
      scene->setBackground(new ConstantBackground(bgcolor));
    }

    // If maxDepth is < 0, this function has no effect.
    rtrt->getScene()->getRenderParameters().setMaxDepth(maxDepth);

    rtrt->beginRendering(true);

    //manually delete
#ifdef USE_MPI
    if (rank == 0) {
      bool exit = true;
      MPI::COMM_WORLD.Bcast(&exit, sizeof(exit), MPI_BYTE, 0);
      // Assume first element is the group.
      const Group* world = dynamic_cast<Group*>(rtrt->getScene()->getObject());
      Group* world_cc = const_cast<Group*>(world);
      world_cc->remove(world_cc->get(0), true);
      MPI_Finalize();
    }
#else
    delete rtrt;
#endif

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

  } catch (Exception& e) {
    cerr << "manta.cc (top level): Caught exception: " << e.message() << '\n';

#ifdef USE_MPI
    if (rank == 0) {
      bool exit = true;
      MPI::COMM_WORLD.Bcast(&exit, sizeof(exit), MPI_BYTE, 0);
      MPI_Finalize();
    }
#endif

    Thread::exitAll( 1 );

  } catch (std::exception e){
    cerr << "manta.cc (top level): Caught std exception: "
         << e.what()
         << '\n';

#ifdef USE_MPI
    if (rank == 0) {
      bool exit = true;
      MPI::COMM_WORLD.Bcast(&exit, sizeof(exit), MPI_BYTE, 0);
      MPI_Finalize();
    }
#endif

    Thread::exitAll(1);

  } catch(...){
    cerr << "manta.cc (top level): Caught unknown exception\n";

#ifdef USE_MPI
    if (rank == 0) {
      bool exit = true;
      MPI::COMM_WORLD.Bcast(&exit, sizeof(exit), MPI_BYTE, 0);
      MPI_Finalize();
    }
#endif

    Thread::exitAll(1);
  }

  Thread::exitAll(0);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Default stack setup.

void make_stack( ReadContext &context, const vector<string> &args ) {
  // Defaults.

  if(!factory->selectLoadBalancer("workqueue"))
    throw InternalError("default load balancer not found");

  if(!factory->selectImageTraverser("tiled(-square)"))
    throw InternalError("default image traverser not found");

  if(!factory->selectPixelSampler("singlesample"))
    throw InternalError("default pixel sampler not found");

  if(!factory->selectRenderer("raytracer"))
    throw InternalError("default renderer not found");

  // Parse command line args.
  for (size_t i=0; i < args.size(); ++i) {
    string arg = args[i];

    if(arg == "-imagetraverser"){
      string s;
      if(!getStringArg(i, args, s))
        usage(factory);
      if(!factory->selectImageTraverser(s)){
        cerr << "Invalid image traverser: " << s << ", available image traversers are:\n";
        printList(cerr, factory->listImageTraversers());
        throw IllegalArgument( s, i, args );
      }

    } else if(arg == "-loadbalancer"){
      string s;
      if(!getStringArg(i, args, s))
        usage(factory);
      if(!factory->selectLoadBalancer(s)){
        cerr << "Invalid load balancer: " << s << ", available load balancers are:\n";
        printList(cerr, factory->listLoadBalancers());
        throw IllegalArgument( s, i, args );
      }
    } else if(arg == "-pixelsampler"){
      string s;
      if(!getStringArg(i, args, s))
        usage(factory);
      if(!factory->selectPixelSampler(s)){
        cerr << "Invalid pixel sampler: " << s << ", available pixel samplers are:\n";
        printList(cerr, factory->listPixelSamplers());
        throw IllegalArgument( s, i, args );
      }
    } else if(arg == "-renderer"){
      string s;
      if(!getStringArg(i, args, s)){
        usage(factory);
        cerr << "oops\n";
      }
      if(!factory->selectRenderer(s)){
        cerr << "Invalid renderer: " << s << ", available renderers are:\n";
        printList(cerr, factory->listRenderers());
        throw IllegalArgument( s, i, args );
      }
    }
  }
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Default Scene.

Scene* createDefaultScene()
{
  // Create a default scene.  This scene is used for benchmarks, so
  // please do not change it.  Please create a new scene instead.
  // Don't even put any #ifdefs in it or code that is commented out.
  Scene* scene = new Scene();

  scene->setBackground(new ConstantBackground(ColorDB::getNamedColor("SkyBlue3")*0.5));

  Material* red=new Phong(Color(RGBColor(.6,0,0)),
                          Color(RGBColor(.6,.6,.6)), 32, (ColorComponent)0.4);

  Material* plane_matl = new Phong(new CheckerTexture<Color>(Color(RGBColor(.6,.6,.6)),
                                                             Color(RGBColor(0,0,0)),
                                                             Vector(1,0,0),
                                                             Vector(0,1,0)),
                                   new Constant<Color>(Color(RGBColor(.6,.6,.6))),
                                   32,
                                   new CheckerTexture<ColorComponent>
                                   ((ColorComponent)0.2,
                                    (ColorComponent)0.5,
                                    Vector(1,0,0),
                                    Vector(0,1,0)));


  Group* world = new Group();
  Primitive* floor = new Parallelogram(plane_matl, Vector(-20,-20,0),
                                       Vector(40,0,0), Vector(0,40,0));
  // Setup world-space texture coordinates for the checkerboard floor
  UniformMapper* uniformmap = new UniformMapper();
  floor->setTexCoordMapper(uniformmap);
  world->add(floor);
  world->add(new Sphere(red, Vector(0,0,1.2), 1.0));
  scene->setObject(world);

  LightSet* lights = new LightSet();
  lights->add(new PointLight(Vector(0,5,8), Color(RGBColor(.6,.1,.1))));
  lights->add(new PointLight(Vector(5,0,8), Color(RGBColor(.1,.6,.1))));
  lights->add(new PointLight(Vector(5,5,2), Color(RGBColor(.2,.2,.2))));
  lights->setAmbientLight(new ConstantAmbient(Color::black()));
  scene->setLights(lights);
  scene->getRenderParameters().maxDepth = 5;

  scene->addBookmark("default view", Vector(3, 3, 2), Vector(0, 0, 0.3), Vector(0, 0, 1), 60, 60);
  return scene;
}
