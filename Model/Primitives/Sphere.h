
#ifndef Manta_Model_Sphere_h
#define Manta_Model_Sphere_h

#include <Model/Primitives/PrimitiveCommon.h>
#include <Interface/TexCoordMapper.h>
#include <Core/Geometry/Vector.h>

namespace Manta {
  class ArchiveElement;

  class Sphere : public PrimitiveCommon, public TexCoordMapper {
  public:
    Sphere();
    Sphere(Material* material, const Vector& center, Real radius);
    virtual ~Sphere();

#ifndef SWIG
    virtual Sphere* clone(CloneDepth depth, Clonable* incoming);
    virtual InterpErr serialInterpolate(const std::vector<keyframe_t> &keyframes);
#endif

    virtual void computeBounds(const PreprocessContext& context,
                               BBox& bbox) const;
    virtual void intersect(const RenderContext& context, RayPacket& rays) const;
    virtual void computeNormal(const RenderContext& context, RayPacket& rays) const;
    virtual void computeTexCoords2(const RenderContext& context,
                                   RayPacket& rays) const;
    virtual void computeTexCoords3(const RenderContext& context,
                                   RayPacket& rays) const;

    virtual void getRandomPoints(Packet<Vector>& points,
                                 Packet<Vector>& normals,
                                 Packet<Real>& pdfs,
                                 const RenderContext& context,
                                 RayPacket& rays) const;

    Vector getCenter(void) const
    {
      return center;
    }
    void setCenter(const Vector& center) {
      this->center = center;
    }

    Real getRadius(void) const
    {
      return radius;
    }
    void setRadius(Real radius) {
      this->radius = radius;
      inv_radius = 1 / radius;
    }

    void readwrite(ArchiveElement* archive);
  private:
    Vector center;
    Real radius;
    Real inv_radius;
  };
}

#endif
