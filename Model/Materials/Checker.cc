
#include <Model/Materials/Checker.h>
#include <Interface/RayPacket.h>

using namespace Manta;

Checker::Checker(Material* m1, Material* m2, const Vector& v1,
                 const Vector& v2)
  : m1(m1), m2(m2), v1(v1), v2(v2)
{
  if(v1.z() == 0 && v2.z() == 0)
    need_w = false;
  else
    need_w = true;
}

Checker::~Checker()
{
}

void Checker::preprocess(const PreprocessContext& context)
{
  m1->preprocess(context);
  m2->preprocess(context);
}

void Checker::shade(const RenderContext& context, RayPacket& rays) const
{
  if(need_w)
    rays.computeTextureCoordinates3(context);
  else
    rays.computeTextureCoordinates2(context);
  Real vv1 = Dot(rays.getTexCoords(0), v1);
  Real vv2 = Dot(rays.getTexCoords(0), v2);
  if(vv1<0)
    vv1=-vv1+1;
  if(vv2<0)
    vv2=-vv2+1;
  int i1 = (int)vv1;
  int i2 = (int)vv2;
  int which0 = (i1+i2)%2;
  for(int start = rays.begin(); start < rays.end();){
    int stop = start+1;
    // I'm not sure what the default value for "which" should be, but
    // this will not change the value of "which0" if "which" is never
    // changed.  - James Bigler
    int which = which0;
    while(stop < rays.end()){
      Real vv1 = Dot(rays.getTexCoords(stop), v1);
      Real vv2 = Dot(rays.getTexCoords(stop), v2);
      if(vv1<0)
        vv1=-vv1+1;
      if(vv2<0)
        vv2=-vv2+1;
      int i1 = (int)vv1;
      int i2 = (int)vv2;
      which = (i1+i2)%2;
      if(which != which0)
        break;
      stop++;
    }
    RayPacket subrays(rays, start, stop);
    (which0?m2:m1)->shade(context, subrays);
    which0 = which;
    start = stop;
  }
}

// TODO(bigler): the only difference between shade and
// attenuateShadows is the single function call to the children.
// There must be a way to pass in a function pointer or something.
void Checker::attenuateShadows(const RenderContext& context,
                               RayPacket& shadowRays) const
{
  if(need_w)
    shadowRays.computeTextureCoordinates3(context);
  else
    shadowRays.computeTextureCoordinates2(context);
  Real vv1 = Dot(shadowRays.getTexCoords(0), v1);
  Real vv2 = Dot(shadowRays.getTexCoords(0), v2);
  if(vv1<0)
    vv1=-vv1+1;
  if(vv2<0)
    vv2=-vv2+1;
  int i1 = (int)vv1;
  int i2 = (int)vv2;
  int which0 = (i1+i2)%2;
  for(int start = shadowRays.begin(); start < shadowRays.end();){
    int stop = start+1;
    // I'm not sure what the default value for "which" should be, but
    // this will not change the value of "which0" if "which" is never
    // changed.  - James Bigler
    int which = which0;
    while(stop < shadowRays.end()){
      Real vv1 = Dot(shadowRays.getTexCoords(stop), v1);
      Real vv2 = Dot(shadowRays.getTexCoords(stop), v2);
      if(vv1<0)
        vv1=-vv1+1;
      if(vv2<0)
        vv2=-vv2+1;
      int i1 = (int)vv1;
      int i2 = (int)vv2;
      which = (i1+i2)%2;
      if(which != which0)
        break;
      stop++;
    }
    RayPacket subshadowRays(shadowRays, start, stop);
    (which0?m2:m1)->attenuateShadows(context, subshadowRays);
    which0 = which;
    start = stop;
  }
}

