
#ifndef Manta_Model_Cylinder_h
#define Manta_Model_Cylinder_h

#include <Model/Primitives/PrimitiveCommon.h>
#include <Interface/TexCoordMapper.h>
#include <Core/Geometry/Vector.h>
#include <Core/Geometry/AffineTransform.h>

namespace Manta
{

  class Cylinder : public PrimitiveCommon, public TexCoordMapper {
  public:
    Cylinder(Material* mat, const Vector& bottom, const Vector& top,
	     Real radius);
    virtual ~Cylinder();
    
    virtual void computeBounds(const PreprocessContext& context,
                               BBox& bbox) const;
    virtual void intersect(const RenderContext& context, RayPacket& rays) const ;
    virtual void computeNormal(const RenderContext& context, RayPacket &rays) const;    
    virtual void computeTexCoords2(const RenderContext& context,
				   RayPacket& rays) const;
    virtual void computeTexCoords3(const RenderContext& context,
				   RayPacket& rays) const;
    
  private:
    Vector bottom, top;
    Real radius;
    AffineTransform xform;
    AffineTransform ixform;
  };
}

#endif


