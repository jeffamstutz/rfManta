
#ifndef Manta_Model_PointLight_h
#define Manta_Model_PointLight_h

#include <Interface/Light.h>
#include <Core/Geometry/Vector.h>
#include <Core/Color/Color.h>

namespace Manta {
  class Archive;

  class PointLight : public Light {
  public:
    PointLight();
    PointLight(const Vector& position, const Color& color);
    virtual ~PointLight();

    virtual void preprocess(const PreprocessContext&);

    virtual void computeLight(RayPacket& rays, const RenderContext &context,
                              RayPacket& source) const;

    // Accessors
    Vector getPosition() const { return position; }
    void setPosition(Vector new_p) { position = new_p; }

    Color getColor() const { return color; }
    void setColor(Color new_c) { color = new_c; }

    void readwrite(ArchiveElement* archive);
  private:
    Vector position;
    Color color;
  };
}

#endif
