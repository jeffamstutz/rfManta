
#ifndef Manta_Model_Parallelogram_h
#define Manta_Model_Parallelogram_h

#include <Model/Primitives/PrimitiveCommon.h>
#include <Interface/TexCoordMapper.h>
#include <Core/Geometry/Vector.h>

namespace Manta {
  class ArchiveElement;

  class Parallelogram : public PrimitiveCommon, public TexCoordMapper {
  public:
    Parallelogram();
    Parallelogram(Material* material, const Vector& anchor,
                  const Vector& v1, const Vector& v2);
    virtual ~Parallelogram();

    // NOTE(boulos): This function doesn't take Vector& so that it can
    // be used for callbacks.
    void changeGeometry(Vector new_anchor,
                        Vector new_v1,
                        Vector new_v2);

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

    void readwrite(ArchiveElement* archive);
  private:
    Vector anchor;
    Vector v1, v2;
    Vector v1_unscaled;
    Vector v2_unscaled;
    Vector normal;
    Real d;
    Real inv_area;
  };
}

#endif
