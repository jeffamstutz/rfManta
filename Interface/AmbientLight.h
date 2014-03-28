
#ifndef Manta_Interface_AmbientLight_h
#define Manta_Interface_AmbientLight_h

#include <MantaTypes.h>
#include <string>
#include <Interface/RayPacket.h>

namespace Manta {
  class PreprocessContext;
  class RenderContext;
  class RayPacket;
  class AmbientLight {
  public:
    AmbientLight();
    virtual ~AmbientLight();

    virtual void preprocess(const PreprocessContext& context) {};
    virtual void computeAmbient(const RenderContext& context, RayPacket& rays, ColorArray ambient) const = 0;

    virtual std::string toString() const = 0;
  private:
    AmbientLight(const AmbientLight&);
    AmbientLight& operator=(const AmbientLight&);
  };
}

#endif
