
#include <MantaTypes.h>
#include <Model/Cameras/EnvironmentCamera.h>
#include <Core/Util/Args.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Interface/Context.h>
#include <Interface/MantaInterface.h>
#include <Interface/RayPacket.h>
#include <Interface/Scene.h>
#include <Core/Geometry/BBox.h>
#include <Core/Geometry/AffineTransform.h>
#include <Core/Math/MiscMath.h>
#include <Core/Math/Trig.h>
#include <Core/Util/Assert.h>
#include <Core/Util/NotFinished.h>
#include <iostream>

using namespace Manta;
using namespace std;

EnvironmentCamera::EnvironmentCamera(const Vector& eye_,
                                     const Vector& lookat_,
                                     const Vector& up_ )
  : eye( eye_ ), lookat( lookat_ ), up( up_ )
{
  haveCamera = true;
  setup();
}

EnvironmentCamera::EnvironmentCamera(const vector<string>& args)
{
  haveCamera = false;
  bool gotEye=false;
  bool gotLookat=false;
  bool gotUp=false;
  normalizeRays=false;
  for (size_t i=0; i<args.size(); i++) {
    string arg=args[i];
    if (arg=="-eye") {
      if (!getVectorArg(i, args, eye))
        throw IllegalArgument("EnvironmentCamera -eye", i, args);
      gotEye=true;
      haveCamera = true;
    } else if (arg=="-lookat") {
      if (!getVectorArg(i, args, lookat))
        throw IllegalArgument("EnvironmentCamera -lookat", i, args);
      gotLookat=true;
      haveCamera = true;
    } else if (arg=="-up") {
      if (!getVectorArg(i, args, up))
        throw IllegalArgument("EnvironmentCamera -up", i, args);
      gotUp=true;
      haveCamera = true;
    } else if (arg=="-normalizeRays") {
      normalizeRays=true;
    } else {
      throw IllegalArgument("EnvironmentCamera", i, args);
    }
  }
  if (!gotEye || !gotLookat || !gotUp)
    throw IllegalArgument("EnvironmentCamera needs -eye -lookat and -up", 0, args);
  setup();
}

EnvironmentCamera::~EnvironmentCamera()
{
}

Camera* EnvironmentCamera::create(const vector<string>& args)
{
  return new EnvironmentCamera(args);
}

void EnvironmentCamera::preprocess(const PreprocessContext& context)
{
  Scene* scene = context.manta_interface->getScene();
  if(!haveCamera){
    const BasicCameraData* bookmark = scene->currentBookmark();
    if(bookmark){
      if (context.proc == 0) {
        setBasicCameraData(*bookmark);
      }
    } else {
      BBox bbox;
      scene->getObject()->computeBounds(context, bbox);

      if (context.proc == 0) {
        autoview(bbox);
      }
    }
    context.done();
    haveCamera = true;
  }
}

void EnvironmentCamera::getBasicCameraData(BasicCameraData& cam) const
{
  // fov doesn't make sense for this camera - use a reasonable default
  cam = BasicCameraData(eye, lookat, up, 60, 60);
}

void EnvironmentCamera::setBasicCameraData(BasicCameraData cam)
{
  eye = cam.eye;
  lookat = cam.lookat;
  up = cam.up;
  setup();
}

void EnvironmentCamera::reset( const Vector &eye_, const Vector &up_,
                               const Vector &lookat_ )
{
  eye = eye_;
  up = up_;
  lookat = lookat_;
  setup();
}

void EnvironmentCamera::setup()
{
  direction=lookat - eye;
  n=direction;
  n.normalize();

  v=Cross(direction, up);
  if (v.length2()==0.0) {
    std::cerr << __FILE__ << " line: " << __LINE__ << " Ambiguous up direciton...\n";
  }
  v.normalize();

  u=Cross(v, direction);
  u.normalize();

  v=-v;
}

void EnvironmentCamera::makeRays(const RenderContext& context, RayPacket& rays) const
{
  ASSERT(rays.getAllFlags() & RayPacket::HaveImageCoordinates);
  rays.setFlag(RayPacket::ConstantOrigin);
  if (normalizeRays) {
    for (int i=rays.begin(); i<rays.end(); i++) {
      Real theta = (Real)0.5 * ((Real)M_PI - (Real)M_PI * rays.getImageCoordinates(i, 1));
      Real phi = (Real)M_PI * rays.getImageCoordinates(i, 0) + (Real)M_PI;
      Vector xyz(Sin(theta)*Cos(phi), Sin(theta)*Sin(phi),
                 Cos(theta));
      Vector raydir(Dot(xyz, v),
                    Dot(xyz, n),
                    Dot(xyz, u));
      raydir.normalize();
      rays.setRay(i, eye, raydir);
    }
    rays.setFlag(RayPacket::NormalizedDirections);
  } else {
    for (int i=rays.begin(); i<rays.end(); i++) {
      Real theta = (Real)0.5 * ((Real)M_PI - (Real)M_PI * rays.getImageCoordinates(i, 1));
      Real phi = (Real)M_PI * rays.getImageCoordinates(i, 0) + (Real)M_PI;
      Vector xyz(Sin(theta)*Cos(phi), Sin(theta)*Sin(phi),
                 Cos(theta));
      Vector raydir(Dot(xyz, v),
                    Dot(xyz, n),
                    Dot(xyz, u));
      rays.setRay(i, eye, raydir);
    }
  }
}

void EnvironmentCamera::scaleFOV(Real /*scale*/)
{
  // This functionality doesn't make much sense with the environment camera
}

void EnvironmentCamera::translate(Vector t)
{
  Vector trans(u*t.y() + v*t.x());

  eye += trans;
  lookat += trans;
  setup();
}

void EnvironmentCamera::dolly(Real scale)
{
  Vector dir=lookat - eye;
  eye += dir*scale;
  setup();
}

void EnvironmentCamera::transform(AffineTransform t, TransformCenter center)
{
  Vector cen;
  switch(center) {
  case Eye:
    cen=eye;
    break;
  case LookAt:
    cen=lookat;
    break;
  case Origin:
    cen=Vector(0, 0, 0);
    break;
  }

  Vector lookdir(eye - lookat);

  AffineTransform frame;
  frame.initWithBasis(v.normal(), u.normal(), lookdir.normal(), cen);

  AffineTransform frame_inv=frame;
  frame_inv.invert();

  AffineTransform t2=frame*t*frame_inv;
  up     = t2.multiply_vector(up);
  eye    = t2.multiply_point(eye);
  lookat = t2.multiply_point(lookat);
  setup();
}

void EnvironmentCamera::autoview(const BBox /*bbox*/)
{
  // This functionality doesn't make much sense with the environment camera
}

void EnvironmentCamera::output( std::ostream &os ) {

        os << "environment( -eye " << eye
           << " -lookat " << lookat
           << " -up " << up
           << " )"
           << std::endl;
}

