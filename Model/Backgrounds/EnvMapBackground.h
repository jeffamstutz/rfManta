
#ifndef Manta_Model_EnvMapBackground_h
#define Manta_Model_EnvMapBackground_h

#include <Core/Color/Color.h>
#include <Interface/Background.h>
#include <Interface/Texture.h>

namespace Manta {
  class EnvMapBackground : public Background {
  public:
    enum MappingType {
      LatLon,
      CylindricalEqualArea,
      DebevecSphere,
      OldBehavior
    };
    EnvMapBackground(Texture<Color>* image,
                     MappingType map_type,
                     const Vector& right=Vector(1, 0, 0),
                     const Vector& up=Vector(0, 0, 1));
    virtual ~EnvMapBackground(void);

    virtual void preprocess(const PreprocessContext& context);
    virtual void shade(const RenderContext& context, RayPacket& rays) const;

    void LatLonMapping(const RenderContext& context, RayPacket& rays) const;
    void CylindricalEqualAreaMapping(const RenderContext& context, RayPacket& rays) const;
    void OldBehaviorMapping(const RenderContext& context, RayPacket& rays) const;
    void DebevecMapping(const RenderContext& context, RayPacket& rays) const;

    void setRightUp(Vector right, Vector up);
  private:
    Texture<Color>* image;
    MappingType map_type;
    Vector U, V, W;
  };
} // end namespace Manta

#endif
