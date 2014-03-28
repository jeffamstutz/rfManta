
#include <MantaTypes.h>
#include <Model/Cameras/PinholeCamera.h>
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
#include <Core/Util/Assert.h>
#include <MantaSSE.h>
#include <iostream>
#include <limits>

using namespace Manta;
using namespace std;

PinholeCamera::PinholeCamera( const Vector& eye_, const Vector& lookat_,
                              const Vector &up_, Real hfov_, Real vfov_ )
  : eye( eye_ ), lookat( lookat_ ), up( up_ ),
    stereo_offset( 0.0 ), hfov( hfov_ ), vfov( vfov_)
{
  haveCamera = true;
  setup();
}

PinholeCamera::PinholeCamera(const vector<string>& args)
{
  haveCamera = false;
  normalizeRays = false;
  createCornerRays = false;
  stereo_offset = 0;

  // pinhole(-eye 3 3 2 -lookat 0 0 0.3 -up 0 0 1 -fov 60
  eye    = Vector ( 3, 3, 2 );
  lookat = Vector ( 0, 0, (Real)0.3 );
  up     = Vector( 0, 0, 1 );
  hfov    = 60;
  vfov    = 60;
  for(size_t i=0; i< args.size(); i++){
    string arg = args[i];
    if (arg == "-offset") {
      if(!getArg(i, args, stereo_offset))
        throw IllegalArgument("PinholeCamera -offset", i, args);
    } else if(arg == "-eye"){
      if(!getVectorArg(i, args, eye))
        throw IllegalArgument("PinholeCamera -eye", i, args);
      haveCamera = true;
    } else if(arg == "-lookat"){
      if(!getVectorArg(i, args, lookat))
        throw IllegalArgument("PinholeCamera -lookat", i, args);
      haveCamera = true;
    } else if(arg == "-up"){
      if(!getVectorArg(i, args, up))
        throw IllegalArgument("PinholeCamera -up", i, args);
      haveCamera = true;
    } else if(arg == "-fov"){
      if(!getArg<Real>(i, args, hfov))
        throw IllegalArgument("PinholeCamera -fov", i, args);
      vfov = hfov;
      haveCamera = true;
    } else if(arg == "-hfov"){
      if(!getArg<Real>(i, args, hfov))
        throw IllegalArgument("PinholeCamera -hfov", i, args);
      haveCamera = true;
    } else if(arg == "-vfov"){
      if(!getArg<Real>(i, args, vfov))
        throw IllegalArgument("PinholeCamera -vfov", i, args);
      haveCamera = true;
    } else if(arg == "-normalizeRays"){
      normalizeRays = true;
    } else if(arg == "-createCornerRays"){
      createCornerRays = true;
    } else if(arg == "-maintainAspectRatio"){
      string keep;
      if(!getStringArg(i, args, keep))
        throw IllegalArgument("PinholeCamera -maintainAspectRatio", i, args);

      if(keep == "horizontal")
        aspectRatioMode = Camera::KeepHorizontal;
      else if(keep == "vertical")
        aspectRatioMode = Camera::KeepVertical;
      else if(keep == "none")
        aspectRatioMode = Camera::KeepNone;
      else
        throw IllegalArgument("PinholeCamera -maintainAspectRatio", i, args);
    } else {
      throw IllegalArgument("PinholeCamera", i, args);
    }
  }

  setup();
}

PinholeCamera::~PinholeCamera()
{
}

void PinholeCamera::output( std::ostream &os ) {

  os << "pinhole( -eye " << eye
     << " -lookat " << lookat
     << " -up " << up
     << " -hfov " << hfov
     << " -vfov " << vfov;


  // Output the offset if one is set.
  if (stereo_offset != 0.0) {
    os << " -offset " << stereo_offset;
  }

  if (normalizeRays) {
    os << " -normalizeRays";
  }

  if (createCornerRays) {
    os << " -createCornerRays";
  }

  os << " )" << std::endl;
}

void PinholeCamera::setAspectRatio(float ratio){
  // This method modifies the "variable" fov to correctly reflect the
  // aspect ratio passed in.

  float realRatio = ratio;

  const Real *inputAngle = 0;
  Real *outputAngle = 0;
  switch(aspectRatioMode){
  case Camera::KeepHorizontal:
    inputAngle = &hfov;
    outputAngle = &vfov;
    realRatio = 1./ratio;
    break;

  case Camera::KeepVertical:
    inputAngle = &vfov;
    outputAngle = &hfov;
    break;

  case Camera::KeepNone:
    return;
    break;
  }

  *outputAngle = RtoD(2*atan(realRatio*tan(DtoR(0.5*(*inputAngle)))));
  setup();

  // Choudhury: This is a hack to solve the initially non-square image
  // problem.
  savedRatio = ratio;
}

Camera* PinholeCamera::create(const vector<string>& args)
{
  return new PinholeCamera(args);
}

void PinholeCamera::setup()
{
  direction=lookat-eye; // the normal vector
  nearZ=direction.length(); // lenghth to near plane

  Vector n = direction;
  n.normalize();
  up.normalize();

  v=Cross(direction, up);
  if(v.length2() == 0){
    std::cerr << __FILE__ << " line: " << __LINE__
              << " Ambiguous up direciton... "
              << "lookat = "<< lookat
              << ", eye = "<< eye
              << ", direciton = "<< direction
              << ", n = " << n
              << ", up = "<< up
              << ", v = " << v
              << "\n";
  }
  v.normalize();

  u=Cross(v, direction);
  u.normalize();

  height=nearZ*tan(DtoR(0.5*vfov));
  u*=height;

  width=nearZ*tan(DtoR(0.5*hfov));
  v*=width;
}

void PinholeCamera::makeRays(const RenderContext& context, RayPacket& rays) const
{
  ASSERT(rays.getFlag(RayPacket::HaveImageCoordinates));
  ASSERT(rays.getFlag(RayPacket::ConstantEye));
  rays.setFlag(RayPacket::ConstantOrigin);

  if (normalizeRays)
    if (createCornerRays)
      makeRaysSpecialized<true, true>(rays);
    else
      makeRaysSpecialized<true, false>(rays);
  else
    if (createCornerRays)
      makeRaysSpecialized<false, true>(rays);
    else
      makeRaysSpecialized<false, false>(rays);

  // TODO(boulos): Provide SSE version and somehow avoid this overhead
  // when people don't need time? rays.computeTimeValues would work
  // for the first bounce, but if we don't compute it by then, we'll
  // need to have some sort of "tree of parents" that we can follow up
  Packet<Real> time_seeds;
  context.sample_generator->nextSeeds(context, time_seeds, rays);
  // NOTE(boulos): Call twice to make sure that seeds are pulled off 2
  // at a time... This is an ugly hack but makes the rest of the stuff
  // work better
  context.sample_generator->nextSeeds(context, time_seeds, rays);
  for (int i = rays.begin(); i < rays.end(); i++) {
    rays.data->time[i] = time_seeds.get(i);
  }
}

template <bool NORMALIZE_RAYS, bool CREATE_CORNER_RAYS>
void PinholeCamera::makeRaysSpecialized(RayPacket& rays) const
{

  //we need to find the max ray extents so that we can calculate the
  //corner rays.  These are used by the WaldTriangle intersector and
  //CGT acceleration structure to do frustum culling/traversal, and
  //possibly elsewhere.
  float min_v = std::numeric_limits<float>::max();
  float max_v = -std::numeric_limits<float>::max();
  float min_u = std::numeric_limits<float>::max();
  float max_u = -std::numeric_limits<float>::max();

#ifdef MANTA_SSE
    int b = (rays.rayBegin + 3) & (~3);
    int e = rays.rayEnd & (~3);
    if(b >= e){
      for(int i = rays.begin(); i < rays.end(); i++){
        Vector raydir(v*rays.getImageCoordinates(i, 0)+u*rays.getImageCoordinates(i, 1)+direction);
        if (NORMALIZE_RAYS)
          raydir.normalize();
        rays.setRay(i, eye, raydir);
        rays.data->ignoreEmittedLight[i] = 0;

        if (CREATE_CORNER_RAYS) {
          //find max ray extents.
          //TODO: double check that calling getImageCoordinates again
          //won't return different values and isn't slow.
          min_v = Min(min_v, rays.getImageCoordinates(i, 0));
          max_v = Max(max_v, rays.getImageCoordinates(i, 0));
          min_u = Min(min_u, rays.getImageCoordinates(i, 1));
          max_u = Max(max_u, rays.getImageCoordinates(i, 1));
        }
      }
    } else {
      int i = rays.rayBegin;
      for(;i<b;i++){
        Vector raydir(v*rays.getImageCoordinates(i, 0)+u*rays.getImageCoordinates(i, 1)+direction);
        if (NORMALIZE_RAYS)
          raydir.normalize();
        rays.setRay(i, eye, raydir);
        rays.data->ignoreEmittedLight[i] = 0;

        if (CREATE_CORNER_RAYS) {
          //find max ray extents
          min_v = Min(min_v, rays.getImageCoordinates(i, 0));
          max_v = Max(max_v, rays.getImageCoordinates(i, 0));
          min_u = Min(min_u, rays.getImageCoordinates(i, 1));
          max_u = Max(max_u, rays.getImageCoordinates(i, 1));
        }
      }
      for(i=e;i<rays.rayEnd;i++){
        Vector raydir(v*rays.getImageCoordinates(i, 0)+u*rays.getImageCoordinates(i, 1)+direction);
        if (NORMALIZE_RAYS)
          raydir.normalize();
        rays.setRay(i, eye, raydir);
        rays.data->ignoreEmittedLight[i] = 0;

        if (CREATE_CORNER_RAYS) {
          //find max ray extents
          min_v = Min(min_v, rays.getImageCoordinates(i, 0));
          max_v = Max(max_v, rays.getImageCoordinates(i, 0));
          min_u = Min(min_u, rays.getImageCoordinates(i, 1));
          max_u = Max(max_u, rays.getImageCoordinates(i, 1));
        }
      }
      sse_t min_vs = set4(min_v);
      sse_t max_vs = set4(max_v);
      sse_t min_us = set4(min_u);
      sse_t max_us = set4(max_u);

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

      RayPacketData* data = rays.data;
      for(i=b;i<e;i+=4){
        const sse_t imagev = load44(&data->image[0][i]);
        const sse_t imageu = load44(&data->image[1][i]);
        const sse_t xd = add4(add4(mul4(v0, imagev), mul4(u0, imageu)), dirx);
        const sse_t yd = add4(add4(mul4(v1, imagev), mul4(u1, imageu)), diry);
        const sse_t zd = add4(add4(mul4(v2, imagev), mul4(u2, imageu)), dirz);
        if (NORMALIZE_RAYS) {
          const sse_t length2 = add4(add4(mul4(xd, xd), mul4(yd, yd)), mul4(zd, zd));
          const sse_t scale = accurateReciprocalSqrt(length2);
          store44(&data->direction[0][i], mul4(xd, scale));
          store44(&data->direction[1][i], mul4(yd, scale));
          store44(&data->direction[2][i], mul4(zd, scale));
        }
        else {
          store44(&data->direction[0][i], xd);
          store44(&data->direction[1][i], yd);
          store44(&data->direction[2][i], zd);
        }
        store44(&data->origin[0][i], eyex);
        store44(&data->origin[1][i], eyey);
        store44(&data->origin[2][i], eyez);

        _mm_store_si128((__m128i*)&(data->ignoreEmittedLight[i]), _mm_setzero_si128());

        if (CREATE_CORNER_RAYS) {
          min_vs = min4(min_vs, imagev);
          max_vs = max4(max_vs, imagev);
          min_us = min4(min_us, imageu);
          max_us = max4(max_us, imageu);
        }
      }

      if (CREATE_CORNER_RAYS) {
        min_v = min4f(min_vs);
        max_v = max4f(max_vs);
        min_u = min4f(min_us);
        max_u = max4f(max_us);
        const sse_t imageu = set44(max_u, max_u, min_u, min_u);
        const sse_t imagev = set44(max_v, min_v, max_v, min_v);
        const sse_t xd = add4(add4(mul4(v0, imagev), mul4(u0, imageu)), dirx);
        const sse_t yd = add4(add4(mul4(v1, imagev), mul4(u1, imageu)), diry);
        const sse_t zd = add4(add4(mul4(v2, imagev), mul4(u2, imageu)), dirz);
        if (NORMALIZE_RAYS) {
          const sse_t length2 = add4(add4(mul4(xd, xd), mul4(yd, yd)), mul4(zd, zd));
          const sse_t scale = accurateReciprocalSqrt(length2);
          store44(data->corner_dir[0], mul4(xd, scale));
          store44(data->corner_dir[1], mul4(yd, scale));
          store44(data->corner_dir[2], mul4(zd, scale));
        }
        else {
          store44(data->corner_dir[0], xd);
          store44(data->corner_dir[1], yd);
          store44(data->corner_dir[2], zd);
        }
        rays.setFlag(RayPacket::HaveCornerRays);
      }
    }
#else
    for(int i = rays.begin(); i < rays.end(); i++){
      const float v_imageCoord = rays.getImageCoordinates(i, 0);
      const float u_imageCoord = rays.getImageCoordinates(i, 1);
      Vector raydir(v*v_imageCoord + u*u_imageCoord + direction);
      if (NORMALIZE_RAYS)
        raydir.normalize();
      rays.setRay(i, eye, raydir);
      rays.data->ignoreEmittedLight[i] = 0;

      if (CREATE_CORNER_RAYS) {
          min_v = Min(min_v, v_imageCoord);
          max_v = Max(max_v, v_imageCoord);
          min_u = Min(min_u, u_imageCoord);
          max_u = Max(max_u, u_imageCoord);
      }
    }

    if (CREATE_CORNER_RAYS) {
      Vector raydir[4] = { v*min_v + u*min_u + direction,
                           v*min_v + u*max_u + direction,
                           v*max_v + u*min_u + direction,
                           v*max_v + u*max_u + direction};
      for (int i=0; i < 4; ++i) {
        if (NORMALIZE_RAYS)
          raydir[i].normalize();
        rays.data->corner_dir[0][i] = raydir[i][0];
        rays.data->corner_dir[1][i] = raydir[i][1];
        rays.data->corner_dir[2][i] = raydir[i][2];
      }
      rays.setFlag(RayPacket::HaveCornerRays);
    }
#endif
    if (NORMALIZE_RAYS)
      rays.setFlag(RayPacket::NormalizedDirections);
}

void PinholeCamera::scaleFOV(Real scale)
{
  Real fov_min = 0;
  Real fov_max = 180;
  hfov = RtoD(2*Atan(scale*Tan(DtoR(hfov/2))));
  hfov = Clamp(hfov, fov_min, fov_max);
  vfov = RtoD(2*Atan(scale*Tan(DtoR(vfov/2))));
  vfov = Clamp(vfov, fov_min, fov_max);
  setup();
}

void PinholeCamera::translate(Vector t)
{
  Vector trans(u*t.y()+v*t.x());

  eye += trans;
  lookat += trans;
  setup();
}

// Here scale will move the eye point closer or farther away from the
// lookat point.  If you want an invertable value feed it
// (previous_scale/(previous_scale-1)
void PinholeCamera::dolly(Real scale)
{
  // Better make sure the scale isn't exactly one.
  if (scale == 1) return;
  Vector d = (lookat - eye) * scale;
  eye    += d;

  setup();
}

void PinholeCamera::transform(AffineTransform t, TransformCenter center)
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

void PinholeCamera::autoview(const BBox bbox)
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

void PinholeCamera::reset( const Vector& eye_, const Vector& up_,
                           const Vector& lookat_ )
{
  eye = eye_;
  up = up_;
  lookat = lookat_;

  setup();
}

void PinholeCamera::reset( const Vector& eye_, const Vector& up_,
                           const Vector& lookat_, Real hfov_, Real vfov_ )
{
  hfov = hfov_;
  vfov = vfov_;
  reset(eye_, up_, lookat_);
}


void PinholeCamera::getBasicCameraData(BasicCameraData& cam) const
{
  cam = BasicCameraData(eye, lookat, up, hfov, vfov);
}

void PinholeCamera::setBasicCameraData(BasicCameraData cam)
{
  eye = cam.eye;
  lookat = cam.lookat;
  up = cam.up;
  hfov = cam.hfov;
  vfov = cam.vfov;
  setup();
}

void PinholeCamera::preprocess(const PreprocessContext& context)
{

  if(!haveCamera) {
    Scene* scene = context.manta_interface->getScene();
    const BasicCameraData* bookmark = scene->currentBookmark();
    if(bookmark) {
      if (context.proc == 0) {
        setBasicCameraData(*bookmark);

        // Choudhury: the following line is a hack to get PinholeCameras
        // to properly display non-square images at startup (as by
        // passing -res MxY to bin/manta).
        this->setAspectRatio(savedRatio);
      }
    } else {
      if (context.proc == 0) {
        eye = Vector(0,1,0);
        lookat = Vector(0,0,0);
        up = Vector(0, 0, 1);
        hfov = 60;
        vfov = 60;
      }

      BBox bbox;

      // This compute bounds is done in parallel and so we need to make sure
      // all the threads get to here.
      scene->getObject()->computeBounds(context, bbox);
      if (context.proc == 0)
        autoview(bbox);
    }

    // Need to wait to make sure all threads have entered the if(!haveCamera)
    // block before we go and set haveCamera to true.  Otherwise some late
    // threads might never enter the block and the context.done() barrier will
    // block forever.
    context.done();
    haveCamera = true; // should be ok for all threads to write the same value
                       // to a bool.  We have all the threads do this instead
                       // of thread 0 in order to ensure that haveCamera is
                       // always true for all the threads, even if a thread
                       // were to return out of this function before thread 0
                       // got a chance to update haveCamera.  I (thiago) think
                       // this is simpler than using another context.done(),
                       // but I could be wrong...
  }
}
