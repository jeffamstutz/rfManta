#ifndef DIRECTIONAL_LIGHT_H_
#define DIRECTIONAL_LIGHT_H_

#include <Interface/Light.h>
#include <Core/Geometry/Vector.h>
#include <Core/Color/Color.h>

namespace Manta {
  class Archive;

  class DirectionalLight : public Light {
  public:
    DirectionalLight();
    DirectionalLight(const Vector& direction, const Color& color);
    virtual ~DirectionalLight();

    virtual void preprocess(const PreprocessContext&);

    virtual void computeLight(RayPacket& rays, const RenderContext &context,
                              RayPacket& source) const;

    // Accessors
    Vector getDirection() const { return direction; }
    void setDirection(Vector new_dir) { direction = new_dir; }

    Color getColor() const { return color; }
    void setColor(Color new_c) { color = new_c; }

    void readwrite(ArchiveElement* archive);
  private:
    Vector direction;
    Color color;
  };
}



#endif // DIRECTIONAL_LIGHT_H_
