
#ifndef Manta_Model_LitMaterial_h
#define Manta_Model_LitMaterial_h

#include <Model/Materials/OpaqueShadower.h>
#include <Core/Persistent/MantaRTTI.h>

namespace Manta {
  class LightSet;

  class LitMaterial : public OpaqueShadower {
  public:
    LitMaterial();
    virtual ~LitMaterial();

    virtual void preprocess(const PreprocessContext&);

    // Pure virtual methods.
    virtual void shade(const RenderContext& context, RayPacket& rays) const = 0;

    void readwrite(ArchiveElement* archive);
  protected:
    const LightSet* activeLights;
    LightSet* localLights;
    bool localLightsOverrideGlobal;
  };

  MANTA_DECLARE_RTTI_DERIVEDCLASS(LitMaterial, OpaqueShadower, AbstractClass, readwriteMethod);
}

#endif
