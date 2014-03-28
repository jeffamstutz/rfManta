
#include <UseStatsCollector.h>
#include <Core/Color/RegularColorMap.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Geometry/Vector.h>
#include <Core/Thread/Thread.h>
#include <Core/Util/Args.h>
#include <Core/Util/Preprocessor.h>
#include <DynLT/DynLTContext.h>
#include <DynLT/DynLTGridSpheres.h>
#include <DynLT/DynLTCGT.h>
#include <DynLT/DynLTParticles.h>
#include <DynLT/DynLTQueue.h>
#include <DynLT/DynLTStatsCollector.h>
#include <DynLT/DynLTWorker.h>
#include <Interface/CameraPath.h>
#include <Interface/Context.h>
#include <Interface/LightSet.h>
#include <Interface/MantaInterface.h>
#include <Interface/Scene.h>
#include <Model/AmbientLights/ConstantAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Backgrounds/EnvMapBackground.h>
#include <Model/Lights/PointLight.h>
#include <Model/Primitives/Sphere.h>
#include <Model/Readers/ParticleNRRD.h>


#include <iostream>

#include <float.h>

#define WORKER_THREAD_STACKSIZE 8*1024*1024

using namespace Manta;
using namespace std;

MANTA_PLUGINEXPORT
Scene* make_scene(ReadContext const& context, vector<string> const& args)
{
  Group* world=0;
  int cidx=0;
  string env_fname="";
  bool use_envmap=false;
  string fname="";
  int depth=1;
  bool dilate=false;
  bool fifo=false;
  bool lifo=false;
  int max_depth=3;
  int ncells=2;
  int ngroups=100;
  int nsamples=25;
  int nthreads=1;
  int qsize=16;
  double radius=1.;
  int ridx=-1;
  bool static_threads=false;
  bool stats=false;
  double runtime=1./25.;
  int minproc=0;
  int maxproc=INT_MAX;
  double lx=10.;
  double ly=10.;
  double lz=10.;
  string gridType = "spheres";
  string shadeType = "ambient";
  int shadeTypeInt = DynLTGridSpheres::AmbientOcclusion;
  string colorMapType = "InvRainbowIso";
  float kd = 0.5;
  float ka = 1.0;

  for(size_t i=0; i<args.size(); ++i) {
    string arg=args[i];
    if (arg=="-cidx") {
      if (!getIntArg(i, args, cidx))
        throw IllegalArgument("scene pnrrd -cidx", i, args);
    } else if (arg=="-depth") {
      if (!getIntArg(i, args, depth))
        throw IllegalArgument("scene dynlt -depth", i, args);
    } else if (arg=="-dilate") {
      dilate=true;
    } else if (arg=="-fifo") {
      fifo=true;
      lifo=false;
    } else if (arg=="-envmap") {
      string s;
      if (!getStringArg(i, args, s))
        throw IllegalArgument("scene dynlt -envmap", i, args);

      if (s[0] != '-') {
        if (s=="bg") {
          use_envmap=true;
          if (!getStringArg(i, args, env_fname))
            throw IllegalArgument("scene dynlt -envmap", i, args);
        } else {
          env_fname=s;
        }
      } else {
        throw IllegalArgument("scene dynlt -envmap", i, args);
      }
    } else if (arg=="-i") {
      if (!getStringArg(i, args, fname))
        throw IllegalArgument("scene dynlt -i", i, args);
    } else if (arg=="-lifo") {
      fifo=false;
      lifo=true;
    } else if (arg=="-light") {
      if (!getDoubleArg(i, args, lx))
        throw IllegalArgument("scene dynlt -light", i, args);
      if (!getDoubleArg(i, args, ly))
        throw IllegalArgument("scene dynlt -light", i, args);
      if (!getDoubleArg(i, args, lz))
        throw IllegalArgument("scene dynlt -light", i, args);
    } else if (arg=="-nbounces") {
      if (!getIntArg(i, args, max_depth))
        throw IllegalArgument("scene dynlt -nbounces", i, args);
      ++max_depth;
    } else if (arg=="-ncells") {
      if (!getIntArg(i, args, ncells))
        throw IllegalArgument("scene dynlt -ncells", i, args);
    } else if (arg=="-ngroups") {
      if (!getIntArg(i, args, ngroups))
        throw IllegalArgument("scene dynlt -ngroups", i, args);
    } else if (arg=="-nsamples") {
      if (!getIntArg(i, args, nsamples))
        throw IllegalArgument("scene dynlt -nsamples", i, args);
    } else if (arg=="-nthreads") {
      static_threads=true;
      if (!getIntArg(i, args, nthreads))
        throw IllegalArgument("scene dynlt -nthreads", i, args);
    } else if (arg=="-radius") {
      if (!getDoubleArg(i, args, radius))
        throw IllegalArgument("scene dynlt -radius", i, args);
    } else if (arg=="-ridx") {
      if (!getIntArg(i, args, ridx))
        throw IllegalArgument("scene dynlt -ridx", i, args);
    } else if (arg=="-qsize") {
      if (!getIntArg(i, args, qsize))
        throw IllegalArgument("scene dynlt -qsize", i, args);
#ifdef USE_STATS_COLLECTOR
    } else if (arg=="-stats") {
      stats=true;
#endif
    } else if (arg=="-timed") {
      string s;
      if (getStringArg(i, args, s)) {
        if (s[0] != '-') {
          runtime=atof(s.c_str());
          if (getStringArg(i, args, s)) {
            minproc=atoi(s.c_str());
            if (!getIntArg(i, args, maxproc))
              throw IllegalArgument("scene dynlt -nthreads", i, args);
          }
        }
      }
    } else if (arg=="-gridtype") {
      if (!getStringArg(i, args, gridType))
        throw IllegalArgument("scene dynlt -gridtype", i, args);
    } else if (arg=="-shadetype") {
      if (!getStringArg(i, args, shadeType))
        throw IllegalArgument("scene dynlt -shadetype", i , args);
    } else if (arg=="-colormap") {
      if (!getStringArg(i, args, colorMapType))
        throw IllegalArgument("scene dynlt -colormap", i, args);
    } else {
      cerr<<"Valid options for scene dynlt:\n";
      // cerr<<"  -bv <string>                     bounding volume {bvh|grid|group}\n";
      cerr<<"  -cidx <int>                       data value index for color mapping\n";
      cerr<<"  -depth <int>                      grid depth\n";
      cerr<<"  -dilate                           dilate textures during generation\n";
      cerr<<"  -fifo                             use fifo queue for texture requests\n";
      cerr<<"  -envmap [bg] <string>             environment map filename\n";
      cerr<<"  -i <string>                       particle data filename\n";
      cerr<<"  -lifo                             use lifo queue for texture requests\n";
      cerr<<"  -light <x> <y> <z>                light position\n";
      cerr<<"  -nbounces <int>                   number of indirect nbounces\n";
      cerr<<"  -ncells <int>                     grid resolution\n";
      cerr<<"  -ngroups <int>                    number of sample groups\n";
      cerr<<"  -nsamples <int>                   number of samples/texel\n";
      cerr<<"  -nthreads <int>                   number of static dynlt workers\n";
      cerr<<"  -qsize <int>                      maximum queue qsize\n";
      cerr<<"  -radius <float>                   particle radius\n";
      cerr<<"  -ridx <int>                       radius index\n";
#ifdef USE_STATS_COLLECTOR
      cerr<<"  -stats                            dump summary stats on exit\n";
#endif
      cerr<<"  -timed [<double> [<int> <int>]]   texgen thread range and run time (in seconds)\n";
      cerr<<"  -gridtype <string>                type of grid { gspheres | pcgt }";
      cerr<<"  -shadetype <string>               type of shading { ambient | global }\n";
      cerr<<"   -colormap <string>                type of colormap:  InvRainbowIso, \
      InvRainbow, \
      Rainbow, \
      InvGreyScale, \
      InvBlackBody, \
      BlackBody, \
      GreyScale\n";
    }
  }

  if (gridType == "gspheres")
    cerr<<"Using GridSpheres\n";
  else if (gridType == "pcgt")
    cerr<<"Using ParticleCGT\n";
  else {
    gridType = "gspheres";
    cerr<<"Invalid grid type:  using GridSpheres\n";
  }

  if (shadeType == "ambient") {
    shadeTypeInt = DynLTGridSpheres::AmbientOcclusion;
    cerr<<"Using Ambient Occlusion\n";
  } else if (shadeType == "global") {
    shadeTypeInt = DynLTGridSpheres::GlobalIllumination;
    cerr<<"Using Global Illumination\n";
  } else {
    shadeTypeInt = DynLTGridSpheres::AmbientOcclusion;
    cerr<<"Invalid shade type:  using Ambient Occlusion\n";
  }

  if (!world)
    world=new Group();

  // Create a scene
  Scene* scene=new Scene();

  // Create DynLT work queue
  DynLTQueue* queue;
  if (fifo) {
    queue=new DynLTFifoQ(nthreads*qsize);
    cerr<<"Using FIFO Queue\n";
  } else if (lifo) {
    queue=new DynLTLifoQ(nthreads*qsize);
    cerr<<"Using LIFO Queue\n";
  } else {
    queue=new DynLTPriorityQ(nthreads*qsize);
    cerr<<"Using Priority Queue\n";
  }

  Background* bg=0;
  if (env_fname != "")
    bg=new EnvMapBackground(env_fname);
  else
    bg=new ConstantBackground(Color(RGB(0, 0, 0)));

  if (static_threads) {
    // Static thread scheduling --> create DynLTWorker threads
    DynLTContext* dpltctx=new DynLTContext(context.manta_interface,
                                           queue, scene, ngroups, nsamples,
                                           max_depth, dilate, bg /*, runtime,
                                           minproc, maxproc, kd, ka */);
    for (unsigned int i=0; i<nthreads; ++i) {
      ostringstream name;
      name<<"DynLT Worker "<<i;
      DynLTWorker* worker=new DynLTWorker(dpltctx, i);
      Thread* thread=new Thread(worker, name.str().c_str(), 0,
                                Thread::NotActivated);
      thread->setStackSize(WORKER_THREAD_STACKSIZE);
      thread->activate(false);

      // Register termination callback
      context.manta_interface->registerTerminationCallback(Callback::create(worker, &DynLTWorker::terminate));
    }
  } else {
    // Dynamic thread scheduling --> register timed texgen function
    DynLTContext* dpltctx=new DynLTContext(context.manta_interface,
                                           queue, scene, ngroups, nsamples,
                                           max_depth, dilate, bg, runtime,
                                           minproc, maxproc, kd, ka );

    DynLTWorker* worker=new DynLTWorker(dpltctx, 0);
    context.manta_interface->registerParallelPreRenderCallback(Callback::create(worker, &DynLTWorker::timedRun));
  }

#ifdef USE_STATS_COLLECTOR
  // Register DynLTStatsCollector::resetPerFrameStats to reset per-frame stats
  context.manta_interface->registerSerialPreRenderCallback(Callback::create(DynLTStatsCollector::resetPerFrameStats));

  if (stats) {
    // Register DynLTStatsCollector::dump to output statistics on exit
    context.manta_interface->registerTerminationCallback(Callback::create(DynLTStatsCollector::dump));
  }
#else
  if (stats)
    cerr<<"Warning:  USE_STATS_COLLECTOR not defined\n";
#endif

  // Create color map
  unsigned int type=RegularColorMap::parseType(colorMapType.c_str());
  RegularColorMap* cmap=new RegularColorMap(type);
  Object* tsteps = NULL;

  tsteps = new DynLTParticles(gridType, fname, ncells, depth, radius, ridx,
                              cmap, cidx, queue, 0, INT_MAX, shadeTypeInt);
  queue->resizeHeapIdx(((DynLTParticles*)tsteps)->getNParticles(0));

  // Initialize the scene
  if (env_fname != "" && use_envmap)
    scene->setBackground(bg);
  else
    // scene->setBackground(new ConstantBackground(Color(RGB(0, 0, 0))));
    scene->setBackground(new ConstantBackground(Color(RGB(0.3, 0.3, 0.3))));
  scene->setObject(tsteps);

  // Add lights
  LightSet* lights=new LightSet();
  lights->add(new PointLight(Vector(lx, ly, lz), Color(RGB(1, 1, 1))));
  lights->add(new PointLight(Vector(-lx, -ly, -lz), Color(RGB(.0,0.1,0.9))));
  lights->add(new PointLight(Cross(Vector(lx,ly,lz),(Vector(-lx,-ly,-lz))), Color(RGB(0.5,0.6,0.0))));
  lights->setAmbientLight(new ConstantAmbient(Color(RGB(0.6, 0.7, 0.8))));
  scene->setLights(lights);

    // Dissertation --> Figure 6.14
    // bin/manta -np 8 -res 768x768 -scene "lib/libscene_dynlt.so(-i /home/sci/cgribble/csafe/particle/data/cylinder/spheredata022-crop.nrrd -radius 0.0005 -depth 2 -ncells 4 -cidx 6 -light 0.00382502 0.00941466 0.0387222 -timed 0.5)" -imagedisplay "file(-prefix dlt -fps)" -imagetype rgb8
    // bin/manta -np 8 -res 768x768 -scene "lib/libscene_dynlt.so(-i /home/sci/cgribble/csafe/particle/data/cylinder/spheredata022-crop.nrrd -radius 0.0005 -depth 2 -ncells 4 -cidx 7 -light 0.00382502 0.00941466 0.0387222 -timed 0.5)"
    // -light 0.00382502 0.00941466 0.0387222
    /*
    scene->addBookmark("cylinder zoom",
                       Vector(0.0258901, 0.0350857, 0.0101507),
                       Vector(0.0265856, -0.0189865, -0.00485091),
                       Vector(-0.987557, 0.157203, 0.00437984),
                       33.0838, 33.0838);
    scene->addBookmark("cylinder",
                       Vector(0.0341205, 0.0316136, -0.0175898),
                       Vector(0.0433734, -0.0201009, 0.00214181),
                       Vector(-1, 0, 0),
                       60.0, 60.0);
    scene->addBookmark("thunder zoom",
                       Vector(-0.0243489, -0.0387558, -0.0511661),
                       Vector(0.0457378, -0.0446916, 0.0355259),
                       Vector(0, 1, 0),
                       45.0, 45.0);
    scene->addBookmark("bullet",
                       Vector(-0.00660217, 0.0288114, 0.0100214),
                       Vector(0.00911079, -0.0242788, 0.000859511),
                       Vector(-0.161427, -0.0436313, 0.98592),
                       7.03459, 7.03459);
    */
    /*
      scene->addBookmark("debug 0", Vector(0.06, 26.9721, 0.06),
      Vector(0.06, 0.06, 0.06), Vector(0, 0, 1), 0.59, 0.59);
      scene->addBookmark("view 0", Vector(0.02, 1.02, 0.20),
      Vector(0.02, 0.02, 0.20), Vector(0, 0, 1),
      0.59, 0.59);
      scene->addBookmark("view 1", Vector(0.83, 0.85, 4.71),
      Vector(0.02, 0.01, 0.29), Vector(-0.54, -0.58, 0.59),
      0.59, 0.59);
      scene->addBookmark("view 2", Vector(-0.83, 0.85, 4.71),
      Vector(0.02, 0.01, 0.29), Vector(-0.59, -0.59, 0.59),
      0.59, 0.59);
      scene->addBookmark("view 3", Vector(1.01349, 0.0783629, 0.297803),
      Vector(0.02, 0.02, 0.2),
      Vector(-0.0890283, -0.13761, 0.986477), 0.204219, 0.204219);
    */

  return scene;
}
