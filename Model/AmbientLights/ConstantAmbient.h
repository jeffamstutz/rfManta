
#ifndef Manta_Model_ConstantAmbient_h
#define Manta_Model_ConstantAmbient_h

#include <Interface/AmbientLight.h>
#include <Core/Color/Color.h>
#include <string>

namespace Manta{
  class ConstantAmbient : public AmbientLight {
  public:
    ConstantAmbient();
    ConstantAmbient(const Color& color);
    virtual ~ConstantAmbient();

    virtual void preprocess(const PreprocessContext&);
    virtual void computeAmbient(const RenderContext& context, RayPacket& rays, ColorArray ambient) const;

    virtual std::string toString() const;
    virtual void readwrite(ArchiveElement*);
    Color getColor() const {
      return color;
    }
    void setColor(const Color& newcolor) {
      color = newcolor;
    }
  private:
    Color color;
  };
}

#endif
