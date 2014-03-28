
#include <Model/MiscObjects/Intersection.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/BBox.h>

using namespace Manta;

Intersection::Intersection(Object* object1, Object* object2)
  : object1(object1), object2(object2)
{
}

Intersection::~Intersection()
{
}

void Intersection::preprocess(const PreprocessContext& context)
{
  object1->preprocess(context);
  object2->preprocess(context);
}

void Intersection::computeBounds(const PreprocessContext& context, BBox& bbox) const
{
  BBox ibox1;
  object1->computeBounds(context, ibox1);
  BBox ibox2;
  object2->computeBounds(context, ibox2);
  bbox.extendByPoint(Max(ibox1.getMin(), ibox2.getMin()));
  bbox.extendByPoint(Min(ibox1.getMax(), ibox2.getMax()));
}

void Intersection::intersect(const RenderContext& context, RayPacket& rays) const
{
  RayPacketData raydata1;
  RayPacket object1_rays(raydata1, RayPacket::UnknownShape, rays.begin(), rays.end(),
                         rays.getDepth(), rays.getAllFlags());
  RayPacketData raydata2;
  RayPacket object2_rays(raydata2, RayPacket::UnknownShape, rays.begin(), rays.end(),
                         rays.getDepth(), rays.getAllFlags());

  for(int i = rays.begin();i<rays.end();i++){
    object1_rays.setRay(i, rays.getRay(i));
    object2_rays.setRay(i, rays.getRay(i));
    object1_rays.resetHit(i, rays.getMinT(i));
    object2_rays.resetHit(i, rays.getMinT(i));
  }
  object1->intersect(context, object1_rays);
  object2->intersect(context, object2_rays);
  for(int i=rays.begin();i<rays.end();i++){
    if(object1_rays.getMinT(i) > object2_rays.getMinT(i)){
      rays.hit(i, object1_rays.getMinT(i),
               object1_rays.getHitMaterial(i),
               object1_rays.getHitPrimitive(i),
               object1_rays.getHitTexCoordMapper(i));
    } else {
      rays.hit(i, object2_rays.getMinT(i),
               object2_rays.getHitMaterial(i),
               object2_rays.getHitPrimitive(i),
               object2_rays.getHitTexCoordMapper(i));
    }
  }
}
