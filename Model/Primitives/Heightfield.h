
#ifndef Manta_Model_Heightfield_h
#define Manta_Model_Heightfield_h

#include <Core/Geometry/Vector.h>
#include <Core/Geometry/BBox.h>
#include <Core/Thread/Barrier.h>
#include <Interface/Clonable.h>
#include <Model/MiscObjects/KeyFrameAnimation.h>
#include <Model/Primitives/PrimitiveCommon.h>
#include <Core/Thread/Mutex.h>

namespace Manta {

  class Heightfield : public PrimitiveCommon
  {

  public:
    Heightfield(Material *material, const std::string& filename, 
                const Vector &minBound, const Vector &maxBound //, Real scale = 0.95);
                );

    virtual ~Heightfield();

    virtual Heightfield* clone(Clonable::CloneDepth depth, Clonable *incoming);

    virtual void computeBounds(const PreprocessContext & context, BBox & bbox) const;
    virtual void intersect(const RenderContext& context, RayPacket& rays) const;
    virtual void computeNormal(const RenderContext & context, RayPacket & rays) const;

//     virtual void rescaleDataHeight(Real scale);

    virtual bool isParallel() const { return true; }
    Interpolable::InterpErr serialInterpolate(const std::vector<keyframe_t> &keyframes);
    Interpolable::InterpErr parallelInterpolate(const std::vector<keyframe_t> &keyframes,
                                                int proc, int numproc);

  private:
    BBox m_Box;
    float** data;
    int nx, ny;
    Vector diag;
    Vector cellsize;
    Vector inv_cellsize;

    Barrier barrier;
    Mutex mutex;

    Heightfield() : barrier("heightfield barrier"), mutex("heightfield mutex"){ };

  };
}

#endif
