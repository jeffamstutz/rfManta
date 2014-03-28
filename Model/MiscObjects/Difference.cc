
#include <Model/MiscObjects/Difference.h>
#include <Interface/RayPacket.h>
#include <Core/Exceptions/InternalError.h>

using namespace Manta;

Difference::Difference(Object* object1, Object* object2)
  : object1(object1), object2(object2)
{
  throw InternalError("Difference not yet implemented");
}

Difference::~Difference()
{
}

void Difference::preprocess(const PreprocessContext& context)
{
  object1->preprocess(context);
  object2->preprocess(context);
}

void Difference::computeBounds(const PreprocessContext& context, BBox& bbox) const
{
  object1->computeBounds(context, bbox);
}

void Difference::intersect(const RenderContext& context, RayPacket& rays) const
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
}
