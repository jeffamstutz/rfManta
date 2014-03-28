#ifndef Model_Primitives_ConvexQuad_h
#define Model_Primitives_ConvexQuad_h

#include <Model/Primitives/PrimitiveCommon.h>

namespace Manta{
  class Material;

  class ConvexQuad : public PrimitiveCommon {
  public:
    ConvexQuad(Material *mat,
	       const Vector& v0, const Vector& v1, const Vector& v2, const Vector& v3);

    ~ConvexQuad();

    void computeBounds(const PreprocessContext& context,
		       BBox& bbox_) const;

    void intersect(const RenderContext& context,
		   RayPacket& rays) const;

    void computeNormal(const RenderContext& context,
		       RayPacket& rays) const;

  private:
    Vector v[4];
    Vector normal;
  };
}

#endif
