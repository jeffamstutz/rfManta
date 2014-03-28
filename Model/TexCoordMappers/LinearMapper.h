
#ifndef Manta_Model_LinearMapper_h
#define Manta_Model_LinearMapper_h

#include <Interface/TexCoordMapper.h>
#include <Core/Geometry/Vector.h>
#include <Core/Geometry/AffineTransform.h>

namespace Manta {

  class LinearMapper : public TexCoordMapper {
  public:
    LinearMapper(const Vector& origin, const Vector& v1,
                 const Vector& v2, const Vector& v3);
    LinearMapper(const AffineTransform& transform);
    virtual ~LinearMapper();

    virtual void computeTexCoords2(const RenderContext& context,
			   RayPacket& rays) const;
    virtual void computeTexCoords3(const RenderContext& context,
			    RayPacket& rays) const;
  private:
    LinearMapper(const LinearMapper&);
    LinearMapper& operator=(const LinearMapper&);

    AffineTransform transform;
  };
}

#endif
