#ifndef Manta_Model_MovingKSTriangle_h
#define Manta_Model_MovingKSTriangle_h

#include <Model/Primitives/MeshTriangle.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/Vector.h>
#include <Core/Geometry/Ray.h>
#include <Core/Geometry/BBox.h>
#include <Model/Groups/Mesh.h>

namespace Manta
{
  class MovingKSTriangle : public MeshTriangle, public TexCoordMapper
  {
  public:

    // Scratch pad structure for KenslerShirley Triangle.
    enum {
      SCRATCH_U = 0,
      SCRATCH_V,
      SCRATCH_LAST // Dependent classes may use this identifier to
                   // avoid conflicts.
    };

    MovingKSTriangle() { }
    MovingKSTriangle(Mesh *mesh, unsigned int id);

    virtual MovingKSTriangle* clone(CloneDepth depth, Clonable* incoming=NULL);
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

    void computeNormal(const RenderContext& context, RayPacket &rays) const;
  };
}

#endif
