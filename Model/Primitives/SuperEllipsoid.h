
#ifndef Manta_SuperEllipsoid_h
#define Manta_SuperEllipsoid_h

#include <Model/Primitives/PrimitiveCommon.h>
#include <Interface/TexCoordMapper.h>
#include <Core/Geometry/Vector.h>

namespace Manta {
  class SuperEllipsoid : public PrimitiveCommon, public TexCoordMapper {
  public:
    SuperEllipsoid(Material* material, const Vector& center, Real radius,
                   Real alpha, Real beta);
    virtual ~SuperEllipsoid();

    virtual void computeBounds(const PreprocessContext& context,
                               BBox& bbox) const;
    virtual void intersect(const RenderContext& context, RayPacket& rays) const;
    virtual void computeNormal(const RenderContext& context, RayPacket& rays) const;
    virtual void computeTexCoords2(const RenderContext& context,
                                   RayPacket& rays) const;
    virtual void computeTexCoords3(const RenderContext& context,
                                   RayPacket& rays) const;
  private:
    Real functionValue(const Vector& location) const;
    inline Vector functionGradient(const Vector& location, Real& value ) const;
    inline Vector logarithmFunctionGradient(const Vector& location, Real& value ) const;
    Vector center;
    Real radius;
    Real inv_radius;
    Real two_over_alpha;
    Real two_over_beta;
    Real alpha_over_beta;
  };
}

#endif
