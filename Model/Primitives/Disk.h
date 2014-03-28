
#ifndef Manta_Model_Disk_h
#define Manta_Model_Disk_h

#include <Model/Primitives/PrimitiveCommon.h>
#include <Interface/TexCoordMapper.h>
#include <Core/Geometry/Vector.h>

namespace Manta {
  class Disk : public PrimitiveCommon, public TexCoordMapper {
  public:
    Disk(Material* mat, const Vector& center, const Vector& n,
         Real radius, const Vector& axis);
    Disk(Material* mat, const Vector& center, const Vector& n,
         Real radius, const Vector& axis, Real minTheta, Real maxTheta);
    virtual ~Disk();
    
    virtual void computeBounds(const PreprocessContext& context, BBox& bbox) const;
    virtual void intersect(const RenderContext& context, RayPacket& rays) const ;
    virtual void computeNormal(const RenderContext& context, RayPacket& rays) const;
    
    virtual void computeTexCoords2(const RenderContext& context, RayPacket& rays) const;
    virtual void computeTexCoords3(const RenderContext& context, RayPacket& rays) const;
    
  private:
    Vector _c;
    Vector _n, _u, _v;
    Real _d, _r, _minTheta, _maxTheta;
    bool _partial;

    bool checkBounds(const Vector& p) const;
    void setupAxes(const Vector& axis);
    void getTexCoords(Vector& p) const;
  };
}

#endif
