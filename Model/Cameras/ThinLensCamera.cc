
#include <MantaTypes.h>
#include <Model/Cameras/ThinLensCamera.h>
#include <Core/Util/Args.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Interface/Context.h>
#include <Interface/MantaInterface.h>
#include <Interface/RayPacket.h>
#include <Interface/SampleGenerator.h>
#include <Interface/Scene.h>
#include <Core/Geometry/BBox.h>
#include <Core/Geometry/AffineTransform.h>
#include <Core/Math/MiscMath.h>
#include <Core/Math/Trig.h>
#include <Core/Math/TrigSSE.h>
#include <Core/Util/Assert.h>
#include <MantaSSE.h>
#include <iostream>
#include <limits>

using namespace Manta;
using namespace std;

ThinLensCamera::ThinLensCamera( const Vector& eye_, const Vector& lookat_,
                                const Vector &up_, Real hfov_, Real vfov_,
                                Real focal_length_, Real aperture_)
  : eye( eye_ ), lookat( lookat_ ), up( up_ ),
    stereo_offset( 0.0 ), hfov( hfov_ ), vfov( vfov_),
    focal_length(focal_length_), aperture(aperture_), radius(aperture_/2.0)
{
  haveCamera = true;
  setup();
}

ThinLensCamera::ThinLensCamera(const vector<string>& args)
{
  haveCamera = false;
  stereo_offset = 0;

  eye    = Vector ( 3, 3, 2 );
  lookat = Vector ( 0, 0, (Real)0.3 );
  up     = Vector( 0, 0, 1 );
  hfov    = 60;
  vfov    = 60;
  aperture = 0;
  focal_length = 1;
  for(size_t i=0; i< args.size(); i++){
    string arg = args[i];
    if (arg == "-offset") {
      if(!getArg(i, args, stereo_offset))
        throw IllegalArgument("ThinLensCamera -offset", i, args);
    } else if(arg == "-eye"){
      if(!getVectorArg(i, args, eye))
        throw IllegalArgument("ThinLensCamera -eye", i, args);
      haveCamera = true;
    } else if(arg == "-lookat"){
      if(!getVectorArg(i, args, lookat))
        throw IllegalArgument("ThinLensCamera -lookat", i, args);
      haveCamera = true;
    } else if(arg == "-up"){
      if(!getVectorArg(i, args, up))
        throw IllegalArgument("ThinLensCamera -up", i, args);
      haveCamera = true;
    } else if(arg == "-fov"){
      if(!getArg<Real>(i, args, hfov))
        throw IllegalArgument("ThinLensCamera -fov", i, args);
      vfov = hfov;
      haveCamera = true;
    } else if(arg == "-hfov"){
      if(!getArg<Real>(i, args, hfov))
        throw IllegalArgument("ThinLensCamera -hfov", i, args);
      haveCamera = true;
    } else if(arg == "-vfov"){
      if(!getArg<Real>(i, args, vfov))
        throw IllegalArgument("ThinLensCamera -vfov", i, args);
      haveCamera = true;
    } else if(arg == "-aperture"){
      if(!getArg<Real>(i, args, aperture))
        throw IllegalArgument("ThinLensCamera -aperture", i, args);
      radius = aperture/2.0;
      haveCamera = true;
    } else if(arg == "-focal_length"){
      if(!getArg<Real>(i, args, focal_length))
        throw IllegalArgument("ThinLensCamera -focal_length", i, args);
      haveCamera = true;
    } else {
      throw IllegalArgument("ThinLensCamera", i, args);
    }
  }

  setup();
}

ThinLensCamera::~ThinLensCamera()
{
}

void ThinLensCamera::output( std::ostream &os ) {

  os << "thinlens( -eye " << eye
     << " -lookat " << lookat
     << " -up " << up
     << " -hfov " << hfov
     << " -vfov " << vfov
     << " -aperture " << aperture
     << " -focal_length " << focal_length;


  // Output the offset if one is set.
  if (stereo_offset != 0.0) {
    os << " -offset " << stereo_offset;
  }


  os << " )" << std::endl;
}

Camera* ThinLensCamera::create(const vector<string>& args)
{
  return new ThinLensCamera(args);
}

void ThinLensCamera::setup()
{
  direction=lookat-eye; // the normal vector

  direction.normalize();
  up.normalize();

  v=Cross(direction, up);
  if(v.length2() == 0){
    std::cerr << __FILE__ << " line: " << __LINE__
              << " Ambiguous up direciton... "
              << "lookat = "<< lookat
              << ", eye = "<< eye
              << ", direciton = "<< direction
              << ", up = "<< up
              << ", v = " << v
              << "\n";
  }
  v.normalize();

  u=Cross(v, direction);
  u.normalize();

  height=focal_length*tan(vfov*(Real)(0.5*M_PI/180.0));
  u*=height;
  width=focal_length*tan(hfov*(Real)(0.5*M_PI/180.0));
  v*=width;

}

void ThinLensCamera::makeRays(const RenderContext& context, RayPacket& rays) const
{
  ASSERT(rays.getFlag(RayPacket::HaveImageCoordinates));
  ASSERT(rays.getFlag(RayPacket::ConstantEye));
  Packet<Real> lens_coord_x;
  Packet<Real> lens_coord_y;

  context.sample_generator->nextSeeds(context, lens_coord_x, rays);
  context.sample_generator->nextSeeds(context, lens_coord_y, rays);

#ifdef MANTA_SSE
  int b = (rays.rayBegin + 3) & (~3);
  int e = rays.rayEnd & (~3);

  if(b>=e) {
    for(int i = rays.begin(); i < rays.end(); ++i) {
      Real imageX = rays.getImageCoordinates(i, 0);
      Real imageY = rays.getImageCoordinates(i, 1);

      Real theta = 2.0 * M_PI * lens_coord_x.get(i);
      Real r = radius * Sqrt( lens_coord_y.get(i) );

      Vector origin = r*Cos(theta)*u + r*Sin(theta)*v;
      Vector on_film = imageX*v+imageY*u+direction*focal_length;

      rays.setRay(i, eye+origin, on_film-origin);
    }
  } else {
    int i = rays.rayBegin;
    for(;i<b;i++) {
      Real imageX = rays.getImageCoordinates(i, 0);
      Real imageY = rays.getImageCoordinates(i, 1);

      Real theta = 2.0 * M_PI * lens_coord_x.get(i);
      Real r = radius * Sqrt( lens_coord_y.get(i) );

      Vector origin = r*Cos(theta)*u + r*Sin(theta)*v;
      Vector on_film = imageX*v+imageY*u+direction*focal_length;

      rays.setRay(i, eye+origin, on_film-origin);
    }
    for(i=e;i<rays.rayEnd;i++) {
      Real imageX = rays.getImageCoordinates(i, 0);
      Real imageY = rays.getImageCoordinates(i, 1);

      Real theta = 2.0 * M_PI * lens_coord_x.get(i);
      Real r = radius * Sqrt( lens_coord_y.get(i) );

      Vector origin = r*Cos(theta)*u + r*Sin(theta)*v;
      Vector on_film = imageX*v+imageY*u+direction*focal_length;

      rays.setRay(i, eye+origin, on_film-origin);
    }

    RayPacketData* data = rays.data;

    const sse_t eyex = set4(eye.data[0]);
    const sse_t eyey = set4(eye.data[1]);
    const sse_t eyez = set4(eye.data[2]);

    const sse_t v0 = set4(v[0]);
    const sse_t v1 = set4(v[1]);
    const sse_t v2 = set4(v[2]);

    const sse_t u0 = set4(u[0]);
    const sse_t u1 = set4(u[1]);
    const sse_t u2 = set4(u[2]);

    const sse_t dirx = set4(direction[0]);
    const sse_t diry = set4(direction[1]);
    const sse_t dirz = set4(direction[2]);

    const sse_t twoPI = set4(2.0*M_PI);
    const sse_t radius4 = set4(radius);
    const sse_t focal4 = set4(focal_length);

    for(i=b;i<e;i+=4){
      const sse_t imageX = load44(&data->image[0][i]);
      const sse_t imageY = load44(&data->image[1][i]);
      const sse_t lensCoordsX = load44(&lens_coord_x.data[i]);
      const sse_t lensCoordsY = load44(&lens_coord_y.data[i]);
      const sse_t theta4 = mul4(twoPI, lensCoordsX);
      const sse_t r4 = mul4(radius4, sqrt4( lensCoordsY ));

#if 0
      const sse_t rCosTheta = mul4(r4, cos4(theta4));
      const sse_t rSinTheta = mul4(r4, sin4(theta4));
#else
      sse_t rCosTheta, rSinTheta;
      sincos4(theta4, &rSinTheta, &rCosTheta);
      rCosTheta = mul4(r4, rCosTheta);
      rSinTheta = mul4(r4, rSinTheta);
#endif

      const sse_t origin_x = add4( mul4(rCosTheta, u0),
                                   mul4(rSinTheta, v0));
      const sse_t origin_y = add4( mul4(rCosTheta, u1),
                                   mul4(rSinTheta, v1));
      const sse_t origin_z = add4( mul4(rCosTheta, u2),
                                   mul4(rSinTheta, v2));

      store44(&data->origin[0][i], add4(eyex, origin_x));
      store44(&data->origin[1][i], add4(eyey, origin_y));
      store44(&data->origin[2][i], add4(eyez, origin_z));

      const sse_t onfilm_x = add4( add4( mul4( imageX, v0 ),
                                            mul4( imageY, u0 ) ),
                                      mul4( dirx, focal4 ) );
      const sse_t onfilm_y = add4( add4( mul4( imageX, v1 ),
                                            mul4( imageY, u1 ) ),
                                      mul4( diry, focal4 ) );
      const sse_t onfilm_z = add4( add4( mul4( imageX, v2 ),
                                            mul4( imageY, u2 ) ),
                                      mul4( dirz, focal4 ) );

      store44(&data->direction[0][i], sub4(onfilm_x, origin_x));
      store44(&data->direction[1][i], sub4(onfilm_y, origin_y));
      store44(&data->direction[2][i], sub4(onfilm_z, origin_z));
    }
  }
#else
  for(int i = rays.begin(); i < rays.end(); ++i) {
    Real imageX = rays.getImageCoordinates(i, 0);
    Real imageY = rays.getImageCoordinates(i, 1);

    Real theta = 2.0 * M_PI * lens_coord_x.get(i);
    Real r = radius * Sqrt( lens_coord_y.get(i) );

    Vector origin = r*Cos(theta)*u + r*Sin(theta)*v;
    Vector on_film = imageX*v+imageY*u+direction*focal_length;

    rays.setRay(i, eye+origin, on_film-origin);
  }
#endif
}

void ThinLensCamera::scaleFOV(Real scale)
{
  Real fov_min = 0;
  Real fov_max = 180;
  hfov = RtoD(2*Atan(scale*Tan(DtoR(hfov/2))));
  hfov = Clamp(hfov, fov_min, fov_max);
  vfov = RtoD(2*Atan(scale*Tan(DtoR(vfov/2))));
  vfov = Clamp(vfov, fov_min, fov_max);
  setup();
}

void ThinLensCamera::translate(Vector t)
{
  Vector trans(u*t.y()+v*t.x());

  eye += trans;
  lookat += trans;
  setup();
}

// Here scale will move the eye point closer or farther away from the
// lookat point.  If you want an invertable value feed it
// (previous_scale/(previous_scale-1)
void ThinLensCamera::dolly(Real scale)
{
  // Better make sure the scale isn't exactly one.
  if (scale == 1) return;
  Vector d = (lookat - eye) * scale;
  eye    += d;

  setup();
}

void ThinLensCamera::transform(AffineTransform t, TransformCenter center)
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

void ThinLensCamera::autoview(const BBox bbox)
{
  Vector diag(bbox.diagonal());
  Real w=diag.length();
  Vector lookdir(eye-lookat);
  lookdir.normalize();
  Real scale = 1/(2*Tan(DtoR(hfov/2)));
  Real length = w*scale;
  lookat = bbox.center();
  eye = lookat+lookdir*length;
  setup();
}

void ThinLensCamera::reset( const Vector& eye_, const Vector& up_,
                           const Vector& lookat_ )
{
  eye = eye_;
  up = up_;
  lookat = lookat_;

  setup();
}

void ThinLensCamera::getBasicCameraData(BasicCameraData& cam) const
{
  cam = BasicCameraData(eye, lookat, up, hfov, vfov);
}

void ThinLensCamera::setBasicCameraData(BasicCameraData cam)
{
  eye = cam.eye;
  lookat = cam.lookat;
  up = cam.up;
  hfov = cam.hfov;
  vfov = cam.vfov;
  setup();
}

void ThinLensCamera::preprocess(const PreprocessContext& context)
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
        eye = Vector(0,1,0);
        lookat = Vector(0,0,0);
        up = Vector(0, 0, 1);
        hfov = 60;
        vfov = 60;
        autoview(bbox);
      }
    }

    context.done();
    haveCamera = true;
  }
}
