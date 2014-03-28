#ifndef Manta_Model_Torus_h
#define Manta_Model_Torus_h

#include <Model/Primitives/PrimitiveCommon.h>
#include <Interface/TexCoordMapper.h>
#include <Core/Geometry/Vector.h>

namespace Manta
{
  class Torus : public PrimitiveCommon, public TexCoordMapper {
  public:
    Torus(Material* mat, double minor_radius, double major_radius);
    
    virtual Torus* clone(CloneDepth depth, Clonable* incoming);
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
    double minor_radius, major_radius;
    Torus(){ }
  };
}

#endif
