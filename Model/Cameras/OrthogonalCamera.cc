
#include <Model/Cameras/OrthogonalCamera.h>
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
#include <Core/Util/NotFinished.h>

using namespace Manta;
using namespace std;

OrthogonalCamera::OrthogonalCamera(const Vector& eye_, const Vector& lookat_,
                                   const Vector& up_, Real hscale_, Real vscale_ )
  : eye( eye_ ), lookat( lookat_ ), up( up_ ), hscale( hscale_ ), vscale(vscale_)
{
  haveCamera = true;
  setup();
}

OrthogonalCamera::OrthogonalCamera(const vector<string>& args)
{
  haveCamera = false;
  bool gotEye = false;
  bool gotLookat = false;
  bool gotScale = false;
  bool gotUp = false;
  for(size_t i=0; i< args.size(); i++){
    string arg = args[i];
    if(arg == "-eye"){
      if(!getVectorArg(i, args, eye))
        throw IllegalArgument("OrthogonalCamera -eye", i, args);
      gotEye = true;
      haveCamera = true;
    } else if(arg == "-lookat"){
      if(!getVectorArg(i, args, lookat))
        throw IllegalArgument("OrthogonalCamera -lookat", i, args);
      gotLookat = true;
      haveCamera = true;
    } else if(arg == "-up"){
      if(!getVectorArg(i, args, up))
        throw IllegalArgument("OrthogonalCamera -up", i, args);
      gotUp = true;
      haveCamera = true;
    } else if(arg == "-scale"){
      if(!getArg<Real>(i, args, hscale))
        throw IllegalArgument("OrthogonalCamera -scale", i, args);
      vscale = hscale;
      gotScale = true;
      haveCamera = true;
    } else if(arg == "-hscale"){
      if(!getArg<Real>(i, args, hscale))
        throw IllegalArgument("OrthogonalCamera -hscale", i, args);
      gotScale = true;
      haveCamera = true;
    } else if(arg == "-vscale"){
      if(!getArg<Real>(i, args, vscale))
        throw IllegalArgument("OrthogonalCamera -vscale", i, args);
      gotScale = true;
      haveCamera = true;
    } else {
      throw IllegalArgument("OrthogonalCamera", i, args);
    }
  }

  // TODO(boulos): Separate gotScale into gotHScale and gotVScale
  if(!gotEye || !gotLookat || !gotUp || !gotScale)
    throw IllegalArgument("OrthogonalCamera needs -eye -lookat -up and -scale", 0, args);
  setup();
}

OrthogonalCamera::~OrthogonalCamera()
{
}

Camera* OrthogonalCamera::create(const vector<string>& args)
{
  return new OrthogonalCamera(args);
}

void OrthogonalCamera::setup()
{
  direction=lookat-eye;
  direction.normalize();
  v=Cross(direction, up);
  if(v.length2() == 0){
    cerr << "Ambiguous up direction...\n";
  }
  v.normalize();

  u=Cross(v, direction);
  u.normalize();

  u*=vscale;
  v*=hscale;
}

void OrthogonalCamera::makeRays(const RenderContext& context, RayPacket& rays) const
{
  ASSERT(rays.getFlag(RayPacket::HaveImageCoordinates));

  for(int i=rays.begin();i<rays.end();i++){
    Vector rayposition(eye +
                       v*rays.getImageCoordinates(i, 0) +
                       u*rays.getImageCoordinates(i, 1));
    rays.setRay(i, rayposition, direction);
  }
  rays.setFlag(RayPacket::NormalizedDirections);
}

// FOV doesn't quite make sense here - orthogonal cameras don't have FOV's.
// But in the context of the camera interface, adjusting the scale makes
// the most sense, so we'll do that.
void OrthogonalCamera::scaleFOV(Real scale)
{
  hscale *= scale;
  vscale *= scale;
  setup();
}

void OrthogonalCamera::translate(Vector t)
{
  Vector trans(u*t.y()+v*t.x());

  eye += trans;
  lookat += trans;
  setup();
}

void OrthogonalCamera::dolly(Real scale)
{
  Vector dir = lookat - eye;
  eye += dir*scale;
  setup();
}

void OrthogonalCamera::transform(AffineTransform t, TransformCenter center)
{
  Vector cen;
  switch(center){
  case Eye:
    cen = eye;
    break;
  case LookAt:
    cen = lookat;
    break;
  case Origin:
    cen = Vector(0,0,0);
    break;
  }

  Vector lookdir(eye-lookat);
  Real length = lookdir.length();

  AffineTransform frame;
  frame.initWithBasis(v.normal()*length, u.normal()*length, lookdir, cen);

  AffineTransform frame_inv(frame);
  frame_inv.invert();

  AffineTransform t2 = frame * t * frame_inv;

  up     = t2.multiply_vector(up);
  eye    = t2.multiply_point(eye);
  lookat = t2.multiply_point(lookat);
  setup();
}

void OrthogonalCamera::autoview(const BBox bbox)
{
  output(cerr);
  Vector diag(bbox.diagonal());
  Real w=diag.length();
  Vector lookdir(eye-lookat);
  lookdir.normalize();
  lookat = bbox.center();
  eye = lookat+lookdir*w;
  setup();
}

void OrthogonalCamera::output( std::ostream &os ) {
  os << "orthogonal( -eye " << eye
     << " -lookat " << lookat
     << " -up " << up
     << " -hscale " << hscale
     << " -vscale " << vscale
     << " )"
     << std::endl;
}

void OrthogonalCamera::reset(const Vector& eye_, const Vector& up_,
                             const Vector& lookat_ )
{
  eye    = eye_;
  up     = up_;
  lookat = lookat_;

  setup();
}

void OrthogonalCamera::getBasicCameraData(BasicCameraData& cam) const
{
  // TODO(sparker): fov doesn't make sense for this camera - use a
  // reasonable default hscale isn't the right thing to use for the
  // fov.  We should do the math and fix this.
  cam = BasicCameraData(eye, lookat, up, hscale, vscale);
}

void OrthogonalCamera::setBasicCameraData(BasicCameraData cam)
{
  eye = cam.eye;
  lookat = cam.lookat;
  up = cam.up;
  hscale = cam.hfov;
  vscale = cam.vfov;
  setup();
}


void OrthogonalCamera::preprocess(const PreprocessContext& context)
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
