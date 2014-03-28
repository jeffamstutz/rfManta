
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/IllegalValue.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Exceptions/OutputError.h>
#include <Core/Util/Args.h>
// #include <Engine/Factory/Factory.h>
#include <Engine/Renderers/Raytracer.h>
#include <Engine/Renderers/Raydumper.h>
#include <Engine/Shadows/HardShadows.h>
#include <Interface/Context.h>

#include <cfloat>

using namespace Manta;

// TODO:  Allow multi-threaded rendering
//        Construct actual ray trees

Renderer* Raydumper::create(const vector<string>& args)
{
  return new Raydumper(args);
}

Raydumper::Raydumper(const vector<string>& args) :
  renderer_string("raytracer"), shadow_string("hard"), ray_id(0)
{
  // Parse arguments
  string fname = "raydump.out";
  int argc = static_cast<int>(args.size());
  for(int i=0; i< argc; i++){
    string arg = args[i];
    if (arg == "-fname") {
      if (!getArg(i, args, fname))
        throw IllegalArgument("Raydumper -fname", i, args);
    } else if (arg == "-renderer") {
      if (!getArg(i, args, renderer_string))
        throw IllegalArgument("Raydumper -renderer", i, args);
    } else if (arg == "-shadows") {
      if (!getArg(i, args, shadow_string))
        throw IllegalArgument("Raydumper -shadows", i, args);
    } else {
      throw IllegalArgument("Raydumper", i, args);
      cerr<<"Valid options are:\n";
      cerr<<"  -fname <string>      output ray trees to specified file\n";
      cerr<<"  -renderer <string>   use specified renderer\n";
      cerr<<"  -shadows <string>    use specified shadow algorithm\n";
    }
  }

  // Open output file
  fp = fopen(fname.c_str(), "wb");
  if (!fp)
    throw OutputError("Failed to open \"" + fname + "\" for writing");
  cerr<<"Writing ray trees to \""<<fname<<"\"\n";
}

Raydumper::~Raydumper()
{
  delete renderer;
  delete shadows;
}

void Raydumper::setupBegin(const SetupContext& context, int integer)
{
  rtrt = context.rtrt_int;

  // Force single-threaded rendering
  if (rtrt->numWorkers() > 1)
    throw IllegalValue<int>("Raydumper does not yet support multi-threaded rendering",
                            rtrt->numWorkers());

  // Force ray tracer with hard shadows
  renderer = new Raytracer();
  shadows = new HardShadows();

  // Quit after rendering a single frame
  rtrt->addOneShotCallback(MantaInterface::Absolute, 1,
                           Callback::create(this, &Raydumper::quit));

  // Continue with setup
  renderer->setupBegin(context, integer);
}

void Raydumper::setupDisplayChannel(SetupContext& context)
{
  renderer->setupDisplayChannel(context);
}

void Raydumper::setupFrame(const RenderContext& context)
{
  renderer->setupFrame(context);
}

void Raydumper::traceEyeRays(const RenderContext& context, RayPacket& rays)
{
  renderer->traceEyeRays(context, rays);
  logRays(rays, RayInfo::PrimaryRay);
}

void Raydumper::traceRays(const RenderContext& context, RayPacket& rays)
{
  renderer->traceRays(context, rays);
  logRays(rays, RayInfo::UnknownRay);
}

bool Raydumper::computeShadows(const RenderContext& context,
                             const LightSet* lights, 
                             RayPacket& sourceRays,
                             RayPacket& shadowRays,
                             bool firstTime,
                             StateBuffer& stateBuffer)
{
  shadows->computeShadows(context, lights, sourceRays, shadowRays, firstTime,
                          stateBuffer);
  logRays(shadowRays, RayInfo::ShadowRay);
}

string Raydumper::getName() const
{
  return shadows->getName();
}

string Raydumper::getSpecs() const
{
  return shadows->getSpecs();
}

void Raydumper::logRays(RayPacket& rays, RayInfo::RayType type)
{
  int depth=rays.getDepth();
  for (int i = rays.begin(); i < rays.end(); ++i) {
    Vector o = rays.getOrigin(i);
    Vector d = rays.getDirection(i);
    float tmin = FLT_MAX;
    if (rays.wasHit(i))
      tmin = rays.getMinT(i);

    RayInfo node;
    node.setOrigin(o.x(), o.y(), o.z());
    node.setDirection(d.x(), d.y(), d.z());
    node.setTime(-1.f);
    node.setHit(tmin);
    node.setObjectID(-1);
    node.setMaterialID(-1);
    node.setSurfaceParams(-1.f, -1.f);
    node.setDepth(depth);
    node.setType(type);
    node.setRayID(ray_id++);
    node.setParentID(-1);

    forest.addChild(new RayTree(node));
  }
}

void Raydumper::quit(int, int)
{
  forest.writeToFile(fp);
  fclose(fp);

  rtrt->finish();
}
