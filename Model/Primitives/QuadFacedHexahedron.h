#ifndef Manta_Model_QuadFacedHexahedron_h
#define Manta_Model_QuadFacedHexahedron_h

#include <Model/Primitives/PrimitiveCommon.h>

namespace Manta{
  class Material;

  class QuadFacedHexahedron : public PrimitiveCommon {
  public:
    QuadFacedHexahedron(Material *matl,
                        const Vector& v0, const Vector& v1, const Vector& v2, const Vector& v3,
                        const Vector& v4, const Vector& v5, const Vector& v6, const Vector& v7,
                        bool coplanar_test = false);

    void computeBounds(const PreprocessContext& context,
		       BBox& bbox_) const;

    void intersect(const RenderContext& context,
		   RayPacket& rays) const;

    void computeNormal(const RenderContext& context,
		       RayPacket& rays) const;

  private:
    static Vector intersectThreePlanes(const Vector& p1, const Vector& N1,
                                       const Vector& p2, const Vector& N2,
                                       const Vector& p3, const Vector& N3);

    static bool coplanar(const Vector& p, const Vector& N,
                         const Vector& v0, const Vector& v1, const Vector& v2, const Vector& v3);

    enum Face {
      Left = 0,
      Right,
      Bottom,
      Top,
      Front,
      Back
    };

    Vector corner[2], normal[6];
  };
}
#endif
