
#include <MantaTypes.h>
#include <Model/Cameras/FisheyeCamera.h>
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
#include <Core/Math/Expon.h>
#include <Core/Util/Assert.h>
#include <iostream>

using namespace Manta;
using namespace std;

FisheyeCamera::FisheyeCamera(const Vector& eye_, const Vector& lookat_,
                             const Vector& up_,  Real hfov_, Real vfov_)
  : eye( eye_ ), lookat( lookat_ ), up( up_ ), hfov( hfov_ ), vfov( vfov_)
{
  haveCamera = true;
  setup();
}

FisheyeCamera::FisheyeCamera(const vector<string>& args)
{
  haveCamera = false;
  eye = Vector ( 3, 3, 2 );
  lookat = Vector ( 0, 0, (Real)0.3 );
  up     = Vector( 0, 0, 1 );
  vfov = hfov = 60;
  for(size_t i=0; i< args.size(); i++){
    string arg = args[i];
    if(arg == "-eye"){
      if(!getVectorArg(i, args, eye))
        throw IllegalArgument("FisheyeCamera -eye", i, args);
      haveCamera = true;
    } else if(arg == "-lookat"){
      if(!getVectorArg(i, args, lookat))
        throw IllegalArgument("FisheyeCamera -lookat", i, args);
      haveCamera = true;
    } else if(arg == "-up"){
      if(!getVectorArg(i, args, up))
        throw IllegalArgument("FisheyeCamera -up", i, args);
      haveCamera = true;
    } else if(arg == "-fov"){
      if(!getArg<Real>(i, args, hfov))
        throw IllegalArgument("FisheyeCamera -fov", i, args);
      vfov = hfov;
      haveCamera = true;
    } else if(arg == "-hfov"){
      if(!getArg<Real>(i, args, hfov))
        throw IllegalArgument("FisheyeCamera -hfov", i, args);
      haveCamera = true;
    } else if(arg == "-vfov"){
      if(!getArg<Real>(i, args, vfov))
        throw IllegalArgument("FisheyeCamera -vfov", i, args);
      haveCamera = true;
    } else {
      throw IllegalArgument("FisheyeCamera", i, args);
    }
  }
  setup();
}

FisheyeCamera::~FisheyeCamera()
{
}

Camera* FisheyeCamera::create(const vector<string>& args)
{
  return new FisheyeCamera(args);
}

void FisheyeCamera::setup()
{
  direction=lookat-eye;
  nearZ=direction.length();

  n = direction;
  n.normalize();

  v=Cross(direction, up);
  if(v.length2() == 0.0){
    std::cerr << __FILE__ << " line: " << __LINE__ << " Ambiguous up direciton...\n";
  }
  v.normalize();

  u=Cross(v, direction);
  u.normalize();
}

void FisheyeCamera::makeRays(const RenderContext& context, RayPacket& rays) const
{
  ASSERT(rays.getFlag(RayPacket::HaveImageCoordinates) );
  rays.setFlag(RayPacket::ConstantOrigin|RayPacket::NormalizedDirections);
  for(int i=rays.begin();i<rays.end();i++){
    // TODO(boulos): Determine if vfov should be used in here...
    Real imageX = rays.getImageCoordinates(i, 0);
    Real imageY = rays.getImageCoordinates(i, 1);
    Real z = Sqrt( 2 - imageX * imageX - imageY * imageY );
    Real theta = Atan2( imageY, imageX );
    Real phi = Acos( z * (Real)(M_SQRT1_2) ) * hfov * (Real)0.0111111111111111;
    Real x = Cos( theta ) * Sin( phi );
    Real y = Sin( theta ) * Sin( phi );
    z = Cos( phi );
    rays.setRay(i, eye, v*x+u*y+n*z);
  }
}

void FisheyeCamera::scaleFOV(Real scale)
{
  Real fov_min = 0;
  Real fov_max = 360;
  hfov = scale*hfov;
  hfov = Clamp(hfov, fov_min, fov_max);
  vfov = scale*vfov;
  vfov = Clamp(vfov, fov_min, fov_max);
}

void FisheyeCamera::translate(Vector t)
{
  Vector trans(u*t.y()+v*t.x());

  eye += trans;
  lookat += trans;
  setup();
}

void FisheyeCamera::dolly(Real scale)
{
  Vector dir = lookat - eye;
  eye += dir*scale;
  setup();
}

void FisheyeCamera::transform(AffineTransform t, TransformCenter center)
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

  AffineTransform frame;
  frame.initWithBasis(v.normal(), u.normal(), lookdir.normal(), cen);

  AffineTransform frame_inv = frame;
  frame_inv.invert();

  AffineTransform t2        = frame * t * frame_inv;
  up     = t2.multiply_vector(up);
  eye    = t2.multiply_point(eye);
  lookat = t2.multiply_point(lookat);
  setup();
}

void FisheyeCamera::autoview(const BBox bbox)
{
  output(cerr);
  Vector diag(bbox.diagonal());
  Real w=diag.length();
  Vector lookdir(eye-lookat);
  lookdir.normalize();
  Real scale = 1/(2*tan(DtoR(hfov*.5)));
  Real length = w*scale;
  lookat = bbox.center();
  eye = lookat+lookdir*length;
  setup();
}

void FisheyeCamera::output( std::ostream &os ) {
  os << "fisheye( -eye " << eye
     << " -lookat " << lookat
     << " -up " << up
     << " -hfov " << hfov
     << " -vfov " << vfov
     << ")"
     << std::endl;
}

void FisheyeCamera::reset(const Vector& eye_, const Vector& up_,
                          const Vector& lookat_ )
{
  eye    = eye_;
  up     = up_;
  lookat = lookat_;

  setup();
}

void FisheyeCamera::getBasicCameraData(BasicCameraData& cam) const
{
  // fov doesn't make sense for this camera - use a reasonable default
  cam = BasicCameraData(eye, lookat, up, hfov, vfov);
}

void FisheyeCamera::setBasicCameraData(BasicCameraData cam)
{
  eye = cam.eye;
  lookat = cam.lookat;
  up = cam.up;
  hfov = cam.hfov;
  vfov = cam.vfov;
  setup();
}


void FisheyeCamera::preprocess(const PreprocessContext& context)
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
        cout <<hfov<<"  ";
        hfov = 90;
        cout <<hfov<<"\n";
        autoview(bbox);
      }
    }
    context.done();
    haveCamera = true;
  }
}
