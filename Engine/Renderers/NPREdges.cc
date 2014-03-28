#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Util/Args.h>
#include <Core/Util/Preprocessor.h>
#include <Engine/Renderers/NPREdges.h>
#include <Engine/Renderers/Raytracer.h>
#include <Interface/Background.h>
#include <Interface/Camera.h>
#include <Interface/Context.h>
#include <Interface/Material.h>
#include <Interface/RayPacket.h>
#include <Interface/Scene.h>
#include <Model/Primitives/Cube.h>
#include <Model/Primitives/QuadFacedHexahedron.h>
#include <Model/Primitives/ValuePrimitive.h>
#include <Model/Instances/Instance.h>

#include <iostream>
using namespace std;

using namespace Manta;

Renderer* NPREdges::create(const vector<string>& args)
{
  return new NPREdges(args);
}

NPREdges::NPREdges(const vector<string>& args)
  : raytracer(new Raytracer),
    imageX(0.0), imageY(0.0),
    lineWidth(2.0), normalThreshold(0.0),
    computeCreases(true),
    noshade(false),
    N(1)
{
  bool threshold_set = false;

  for(size_t i=0; i<args.size(); i++){
    const string& arg = args[i];
    if(arg == "-width"){
      if(!getArg(i, args, lineWidth))
        throw IllegalArgument("NPREdges -width", i, args);
    }
    else if(arg == "-normal-threshold"){
      Real val=0;
      if(!getArg(i, args, val))
        throw IllegalArgument("NPREdges -normal-threshold", i, args);

      this->setNormalThreshold(val);
      threshold_set = true;
    }
    else if(arg == "-nocrease"){
      computeCreases = false;
    }
    else if(arg == "-noshade"){
      noshade = true;
    }
    else if(arg == "-N"){
      if(!getArg(i, args, N))
        throw IllegalArgument("NPREdges -N", i, args);
    }
    else {
      throw IllegalArgument("NPREdges", i, args);
    }
  }

  if(!threshold_set)
    this->setNormalThreshold(0.1);

  // M is one more than the square root of the number of total
  // samples (the "middle" sample should be omitted).
  M = 1+2*N;
  stencil_size = M*M-1;

  // last_ring is the stencil index of the first element of the final
  // ring of stencil rays.  This is used to compute the stencil
  // indices of the rays to be used for the normal gradient
  // computation.
  last_ring = (2*N-1)*(2*N-1) - 1;
}

NPREdges::~NPREdges()
{
  delete raytracer;
}

void NPREdges::setupBegin(const SetupContext& context, int)
{
}

void NPREdges::setupDisplayChannel(SetupContext& context)
{
  bool stereo;
  int x, y;
  context.getResolution(stereo, x, y);

  // image{X|Y} are the distnaces in screen coordinates corresponding
  // to lineWidth pixels.  This is used as the stencil width.  The
  // screen coordinates map to [-1,1], so we want lineWidth times
  // 2/res.
  imageX = 2*lineWidth/x;
  imageY = 2*lineWidth/y;
}

void NPREdges::setupFrame(const RenderContext&)
{
}

void NPREdges::traceEyeRays(const RenderContext& context, RayPacket& rays)
{
  ASSERT(rays.getFlag(RayPacket::HaveImageCoordinates));
  context.camera->makeRays(context, rays);
  rays.initializeImportance();
  traceRays(context, rays);
}

void NPREdges::traceRays(const RenderContext& context, RayPacket& rays)
{
  const int debugFlag = rays.getAllFlags() & RayPacket::DebugPacket;

  // Shade the scene as normal (NPR lines will go as an overlay on
  // top).  Create a render context for default shading.
  RenderContext subContext(context.rtrt_int,
                           context.channelIndex, context.proc, context.numProcs,
                           context.frameState,
                           context.loadBalancer, context.pixelSampler,
                           raytracer, context.shadowAlgorithm,
                           context.camera, context.scene,
                           context.storage_allocator,
                           context.rng,
                           context.sample_generator);
  raytracer->traceRays(subContext, rays);

  if(noshade)
    for(int i=rays.begin(); i<rays.end(); i++)
      rays.setColor(i, Color::white());

  // Create parallel ray packets, each containing one member of a
  // "surround" for each ray in the rays parameter.
  RayPacketData *stencil_data = 0;
  RayPacket *stencil = 0;

  stencil_data = MANTA_STACK_ALLOC(RayPacketData, stencil_size);
  stencil = MANTA_STACK_ALLOC(RayPacket, stencil_size);
  
  // Compute the stencil locations.
  computeCircleStencil(stencil, stencil_data, rays);

  // Generate camera rays for the stencil and trace them.
  for(int i=0; i<stencil_size; i++){
    context.camera->makeRays(context, stencil[i]);
    stencil[i].resetHits();
    context.scene->getObject()->intersect(context, stencil[i]);
  }

  // TODO(choudhury): amortize the calls to computeFFNormal in the
  // Shade case by looking for a run of Shade cases in the ray packet.
  for(int i=rays.begin(); i<rays.end(); ){
    int count;

    switch(type(rays, stencil, i, count)){
    case Intersection:
      {
        if(debugFlag)
          cerr << "count was " << count << endl;

        float lightness = Abs(count - stencil_size/2.0) / (stencil_size/2.0);
        rays.setColor(i, rays.getColor(i)*lightness);

        ++i;
      }
      break;

    case Background:
      {
        if(debugFlag)
          cerr << "Ray is background all-surround" << endl;

        ++i;
      }
      break;

    case Shade:
      {
        if(debugFlag){
          cerr << "Ray is primitive all-surround" << endl;
        }

        // Find a run of Shade samples.
        //
        // TODO(choudhury): Look for a run of Shade samples *of the
        // same primitive*, and do only one test per run for
        // crease-type primitive.
        int j;
        for(j=i+1; j<rays.end(); j++)
          if(type(rays, stencil, j) != Shade)
            break;

        // TODO(choudhury): this procedure only works properly for
        // square pixels (i.e., for imageX == imageY).  I think with
        // the fov stuff that keeps the image from distorting this is
        // the case but need to make sure.
        //
        // Check for crease line.
        RayPacket normRight(stencil[last_ring + 0], i, j);
        RayPacket normUp(stencil[last_ring + 2*N], i, j);
        RayPacket normLeft(stencil[last_ring + 4*N], i, j);
        RayPacket normDown(stencil[last_ring + 6*N], i, j);

        normRight.computeFFNormals<true>(context);
        normLeft.computeFFNormals<true>(context);
        normUp.computeFFNormals<true>(context);
        normDown.computeFFNormals<true>(context);

        for(int k=i; k<j; k++){
          bool checkSelfOccluding = true;

          const Primitive *hitprim = rays.getHitPrimitive(k);
          if(creasable(hitprim)){
            // Compute the gradient of the normal field.
            const Vector v1 = normRight.getFFNormal(k) - normLeft.getFFNormal(k);
            const Vector v2 = normUp.getFFNormal(k) - normDown.getFFNormal(k);

            if(debugFlag){
              cerr << "right normal: " << normRight.getFFNormal(k) << endl
                   << "left normal: " << normLeft.getFFNormal(k) << endl
                   << "up normal: " << normUp.getFFNormal(k) << endl
                   << "down normal: " << normDown.getFFNormal(k) << endl
                   << "v1 (right minus left): " << v1 << endl
                   << "v2 (right minus left): " << v2 << endl;
            }

            if(v1.length2() + v2.length2() > normalThreshold){
              rays.setColor(k, Color::black());
              checkSelfOccluding = false;
            }
          }

          if(checkSelfOccluding){
            const Real t = rays.getMinT(k);

            // Count the number of stencil rays whose t-values are far
            // from the t-value of the sample ray.
            int count = 0;
            for(int l=0; l<stencil_size; l++){
              const Real diff = Abs(t - stencil[l].getMinT(k));
              // NOTE(choudhury): the value by which to scale t is
              // arbitrary.  0.16667 and 0.2 both work well for the
              // torus as it appears in the primtest scene.

              //const Real threshold = 0.16667*t;
              const Real threshold = 0.2*t;
              if(debugFlag){
                std::cout << "diff[" << l << "] = " << diff << (diff > threshold ? " [check]" : "") << std::endl;
              }
              count = diff > threshold ? count + 1 : count;
            }

            float lightness = Abs(count - stencil_size/2.0) / (stencil_size/2.0);
            rays.setColor(k, rays.getColor(k)*lightness);
          }

        }

        i = j;
      }
      break;
    }
  }
}

void NPREdges::traceRays(const RenderContext& context, RayPacket& rays, Real)
{
  traceRays(context, rays);
}

NPREdges::PixelType NPREdges::type(const RayPacket& sample, const RayPacket *stencil, int i) const {
  if(sample.wasHit(i)){
    // The sample ray hit a primitive; check if this is an intersection sample.
    const Primitive *hit = sample.getHitPrimitive(i);
    for(int j=0; j<stencil_size; j++)
      if(!(stencil[j].wasHit(i) && (stencil[j].getHitPrimitive(i) == hit)))
        return Intersection;
  }
  else{
    // The sample ray struck the background; check if this is an intersection sample.
    for(int j=0; j<stencil_size; j++)
      if(stencil[j].wasHit(i))
        return Intersection;
    
    // If all the stencil rays strike the background, this is a background sample.
    return Background;
  }

  // If we reach here, the sample ray struck a primitive, and so did
  // all the stencil rays.
  return Shade;
}

NPREdges::PixelType NPREdges::type(const RayPacket& sample, const RayPacket *stencil, int i, int& count) const {
  const int debugFlag = sample.getAllFlags() & RayPacket::DebugPacket;

  if(sample.wasHit(i)){
    // The sample ray hit a primitive; check if this is an intersection sample.
    count = 0;
    const Primitive *hit = sample.getHitPrimitive(i);

    if(debugFlag){
      std::cout << "hit = " << hit << std::endl;
    }

    for(int j=0; j<stencil_size; j++)
      if(!(stencil[j].wasHit(i) && (stencil[j].getHitPrimitive(i) == hit))){
        if(debugFlag){
          std::cout << "stencil[" << j << "] off (" << stencil[j].getHitPrimitive(i) << ")" << std::endl;
        }
        ++count;
      }
      else if(debugFlag){
        std::cout << "stencil[" << j << "] on" << std::endl;
      }

    if(debugFlag)
      std::cout << "count = " << count << std::endl;

    if(count > 0)
      return Intersection;
  }
  else{
    // The sample ray struck the background; check if this is an intersection sample.
    count = 0;
    for(int j=0; j<stencil_size; j++)
      if(stencil[j].wasHit(i)){
        if(debugFlag){
          std::cout << "stencil[" << j << "] struck " << stencil[j].getHitPrimitive(i) << std::endl;
        }
        ++count;
      }
      else if(debugFlag){
        std::cout << "stencil[" << j << "] struck background" << std::endl;
      }

    if(count > 0)
      return Intersection;
    
    // If all the stencil rays strike the background, this is a background sample.
    return Background;
  }

  // If we reach here, the sample ray struck a primitive, and so did
  // all the stencil rays.
  return Shade;
}

bool NPREdges::creasable(const Primitive *prim){
  if(dynamic_cast<const ValuePrimitive<double> *>(prim)){
    return creasable(dynamic_cast<const ValuePrimitive<double> *>(prim)->getPrimitive());
  }
  else if(dynamic_cast<const Instance *>(prim)){
    const Object *inst = dynamic_cast<const Instance *>(prim)->getInstance();
    const Primitive *p = dynamic_cast<const Primitive *>(inst);
    if(!p){
      return false;
    }
    else{
      return creasable(p);
    }
  }
  else{
    return (dynamic_cast<const Cube *>(prim) ||
            dynamic_cast<const QuadFacedHexahedron *>(prim));
  }
}

void NPREdges::setNormalThreshold(Real val){
  normalThreshold = val*val*lineWidth*lineWidth;
}

void NPREdges::computeCircleStencil(RayPacket *stencil, RayPacketData *stencil_data, const RayPacket& rays) const {
  static Real *x = 0, *y = 0;
  if(!x){
    // TODO(choudhury): x and y should be data members so they can be
    // delete'ed at destruction time.
    x = new Real[stencil_size];
    y = new Real[stencil_size];

#ifndef NDEBUG
    std::cout << "Stencil ray positions:" << std::endl;
#endif

    int j=0;
    for(int r=1; r<=N; r++){
      const int ringsize = 8*r;
      const int start = j;
      const int end = j+ringsize;

      const float angle_inc = 2*M_PI/(ringsize);
      const float radius = r*imageX*0.5 / N;

      for(; j<end; j++){
        x[j] = radius*cos(angle_inc*(j-start));
        y[j] = radius*sin(angle_inc*(j-start));

#ifndef NDEBUG
        std::cout << x[j] << " " << y[j] << std::endl;
#endif
      }
    }
  }

  for(int i=0; i<stencil_size; i++){
    new (stencil_data+i)RayPacketData;
    new (stencil+i)RayPacket(stencil_data[i], rays.shape, rays.begin(), rays.end(), rays.getDepth(), RayPacket::HaveImageCoordinates);
  }

  for(int i=rays.begin(); i<rays.end(); i++){
    int eye = rays.getWhichEye(i);

    for(int j=0; j<stencil_size; j++)
      stencil[j].setPixel(i-rays.begin(), eye,
                          rays.getImageCoordinates(i, 0) + x[j],
                          rays.getImageCoordinates(i, 1) + y[j]);
  }
}
