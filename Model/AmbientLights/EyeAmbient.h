
#ifndef Manta_Model_EyeAmbient_h
#define Manta_Model_EyeAmbient_h

#include <Interface/AmbientLight.h>
#include <Core/Color/Color.h>
#include <string>

// Places a virtual light at the ray source (camera usually) so that we can
// still get some quick useful shading for things that are not lit.

namespace Manta{
  class EyeAmbient : public AmbientLight {
  public:
    EyeAmbient();
    EyeAmbient(const Color& color);
    virtual ~EyeAmbient();

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
