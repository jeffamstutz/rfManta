
#ifndef Manta_Model_Cone_h
#define Manta_Model_Cone_h

#include <Model/Primitives/PrimitiveCommon.h>
#include <Interface/TexCoordMapper.h>
#include <Core/Geometry/Vector.h>

namespace Manta
{
  class Cone : public PrimitiveCommon, public TexCoordMapper {
  public:
    Cone(Material* mat, Real radius, Real height);
    virtual ~Cone();
    
    virtual Cone* clone(CloneDepth depth, Clonable* incoming);
    virtual InterpErr serialInterpolate(const std::vector<keyframe_t> &keyframes);

    virtual void computeBounds(const PreprocessContext& context,
                               BBox& bbox) const;
    virtual void intersect(const RenderContext& context, RayPacket& rays) const;
    virtual void computeNormal(const RenderContext& context, RayPacket &rays) const;    
    virtual void computeTexCoords2(const RenderContext& context,
                   RayPacket& rays) const;
    virtual void computeTexCoords3(const RenderContext& context,
                   RayPacket& rays) const;
    
  private:
    Real r, h;
    Cone(){ } 
  };
}

#endif
