
#ifndef Manta_Model_SphericalMapper_h
#define Manta_Model_SphericalMapper_h

#include <Interface/TexCoordMapper.h>
#include <Core/Geometry/Vector.h>

namespace Manta {

  class SphericalMapper : public TexCoordMapper {
  public:
    SphericalMapper(const Vector& center, Real radius);
    virtual ~SphericalMapper();

    virtual void computeTexCoords2(const RenderContext& context,
			   RayPacket& rays) const;
    virtual void computeTexCoords3(const RenderContext& context,
			    RayPacket& rays) const;
  private:
    SphericalMapper(const SphericalMapper&);
    SphericalMapper& operator=(const SphericalMapper&);

    Vector center;
    Real radius;
    Real inv_radius;
  };
}

#endif
