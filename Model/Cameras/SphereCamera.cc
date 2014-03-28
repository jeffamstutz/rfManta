#include <Model/Cameras/SphereCamera.h>
#include <Core/Util/Args.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Interface/Context.h>
#include <Interface/MantaInterface.h>
#include <Interface/RayPacket.h>
#include <Interface/Scene.h>
#include <Core/Geometry/BBox.h>
#include <Core/Math/Trig.h>
#include <Core/Util/Assert.h>
#include <iostream>

using namespace Manta;
using namespace std;

SphereCamera::SphereCamera(const Vector& eye)
  : eye(eye)
{
  haveCamera = true;
}

SphereCamera::SphereCamera(const vector<string>& args)
{
  haveCamera = false;
  eye = Vector ( 3, 3, 2 );
  for(size_t i=0; i< args.size(); i++){
    string arg = args[i];
    if(arg == "-eye"){
      if(!getVectorArg(i, args, eye))
        throw IllegalArgument("SphereCamera -eye", i, args);
      haveCamera = true;
    } else {
      throw IllegalArgument("SphereCamera", i, args);
    }
  }
}

SphereCamera::~SphereCamera()
{
}

Camera* SphereCamera::create(const vector<string>& args)
{
  return new SphereCamera(args);
}

void SphereCamera::makeRays(const RenderContext& context, RayPacket& rays) const
{
  ASSERT(rays.getFlag(RayPacket::HaveImageCoordinates) );
  rays.setFlag(RayPacket::ConstantOrigin | RayPacket::NormalizedDirections);
  //TODO (thiago): If performance is an issue, try using the sse code
  //from Sphere.cc.
  for(int i=rays.begin();i<rays.end();i++){
    // u and v go roughly from [-1,1].  They might be a little more than that
    // if super sampling pixels.
    Real u = rays.getImageCoordinates(i, 0);
    Real v = rays.getImageCoordinates(i, 1);
    Real z = u;
    Real r = Sqrt(fabs(1 - z*z)); // fabs since z might be slightly bigger than
                                  // 1.  There's probably a better way to
                                  // handle this so it doesn't wrap...
    Real phi = v*M_PI;
    Real x = r*Cos(phi);
    Real y = r*Sin(phi);
    rays.setRay(i, eye, Vector(x, y, z));
    rays.data->time[i] = 0;
  }
}

void SphereCamera::output( std::ostream &os ) {
  os << "sphere( -eye " << eye
     << ")"
     << std::endl;
}

void SphereCamera::reset(const Vector& eye_, const Vector& up_,
                          const Vector& lookat_ )
{
  eye = eye_;
}

void SphereCamera::getBasicCameraData(BasicCameraData& cam) const
{
  // fov doesn't make sense for this camera - use a reasonable default
  cam = BasicCameraData(eye, Vector(0,0,0), Vector(0,1,0), 360, 360);
}

void SphereCamera::setBasicCameraData(BasicCameraData cam)
{
  eye = cam.eye;
}

void SphereCamera::preprocess(const PreprocessContext& context)
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
