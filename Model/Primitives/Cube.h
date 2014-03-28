
#ifndef Manta_Model_Cube_h
#define Manta_Model_Cube_h

#include <Model/Primitives/PrimitiveCommon.h>
#include <Core/Geometry/Vector.h>
#include <Core/Geometry/BBox.h>

namespace Manta
{

  class Cube : public PrimitiveCommon {
  public:
    Cube(Material* mat, const Vector& min_, const Vector& max_ );
    ~Cube();
    void setMinMax(const Vector&  p0, const Vector& p1);
    virtual void computeBounds(const PreprocessContext& context,
                               BBox& bbox) const;
    virtual void intersect(const RenderContext& context, RayPacket& rays) const ;
    virtual void computeNormal(const RenderContext& context, RayPacket &rays) const;    
    
  private:
    BBox bbox;
  };
}

#endif
