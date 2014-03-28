
#ifndef Manta_Model_AreaLight_h
#define Manta_Model_AreaLight_h

#include <Interface/Light.h>
#include <Core/Geometry/Vector.h>
#include <Core/Color/Color.h>

namespace Manta {
  class Primitive;
  class AreaLight : public Light {
  public:
    AreaLight();
    AreaLight(Primitive* primitive, const Color& color);
    virtual ~AreaLight();

    virtual void preprocess(const PreprocessContext&);

    virtual void computeLight(RayPacket& rays, const RenderContext &context,
                              RayPacket& source) const;

    Color getColor() const { return color; }
    void setColor(Color new_c) { color = new_c; }

    Primitive* getPrimitive() const { return primitive; }
    void setPrimitive(Primitive* new_prim) { primitive = new_prim; }

    void readwrite(ArchiveElement* archive);
  private:
    Primitive* primitive;
    Color color;
  };
}

#endif
