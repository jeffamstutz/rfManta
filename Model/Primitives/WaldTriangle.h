
#ifndef Manta_Model_WaldTriangle_h
#define Manta_Model_WaldTriangle_h

#include <Model/Primitives/MeshTriangle.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/Vector.h>
#include <Core/Geometry/Ray.h>
#include <Core/Geometry/BBox.h>
#include <Model/Groups/Mesh.h>

namespace Manta
{
  class WaldTriangle : public MeshTriangle, public TexCoordMapper
  {
  public:

    // Scratch pad structure for Wald Triangle.
    enum {
      SCRATCH_U = 0,
      SCRATCH_V,
      SCRATCH_LAST // Dependent classes may use this identifier to
                   // avoid conflicts.
    };

    WaldTriangle(){ };
    WaldTriangle(Mesh *mesh, unsigned int id);

    void update();

    virtual WaldTriangle* clone(CloneDepth depth, Clonable* incoming=NULL);
    virtual InterpErr serialInterpolate(const std::vector<keyframe_t> &keyframes);

    virtual void preprocess(const PreprocessContext& context);

    virtual void setTexCoordMapper(const TexCoordMapper* new_tex) {
      //we always use ourselves as the TexCoordMapper
    }
    virtual void computeTexCoords2(const RenderContext&, RayPacket& rays) const;
    virtual void computeTexCoords3(const RenderContext& context, RayPacket& rays) const {
      computeTexCoords2(context, rays);
    }

    void intersect(const RenderContext& context, RayPacket& rays) const;

    virtual void computeNormal(const RenderContext& context, RayPacket &rays) const;
    virtual void computeGeometricNormal(const RenderContext& context,
                                        RayPacket& rays) const;

  protected:
    void setPoints(const Vector& p1, const Vector& p2, const Vector& p3);

    Real n_u;
    Real n_v;
    Real n_d;
    unsigned int k;

    Real b_nu;
    Real b_nv;
    Real b_d;
    Real n_k;

    Real c_nu;
    Real c_nv;
    Real c_d;

  };
}

#endif
