
#ifndef Manta_Model_Checker_h
#define Manta_Model_Checker_h

#include <Interface/Material.h>
#include <Core/Geometry/Vector.h>

namespace Manta{
  class LightSet;

  class Checker : public Material {
  public:
    Checker(Material* m1, Material* m2, const Vector& v1, const Vector& v2);
    virtual ~Checker();

    virtual void preprocess(const PreprocessContext&);
    virtual void shade(const RenderContext& context, RayPacket& rays) const;
    virtual void attenuateShadows(const RenderContext& context,
                                  RayPacket& shadowRays) const;
  private:
    Material* m1;
    Material* m2;
    Vector v1, v2;
    bool need_w;
  };
}

#endif
