#ifndef MANTA_ENGINE_SAMPLEGENERATORS_UNIFORM_RANDOM_GENERATOR_H_
#define MANTA_ENGINE_SAMPLEGENERATORS_UNIFORM_RANDOM_GENERATOR_H_

#include <Interface/SampleGenerator.h>

namespace Manta {
  class UniformRandomGenerator : public SampleGenerator {
  public:
    UniformRandomGenerator();
    ~UniformRandomGenerator();

    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext& context);
    virtual void setupFrame(const RenderContext& context);

    virtual void setupPacket(const RenderContext& context, RayPacket& rays);
    virtual void setupChildPacket(const RenderContext& context, RayPacket& parent, RayPacket& child);
    virtual void setupChildRay(const RenderContext& context, RayPacket& parent, RayPacket& child, int i, int j);

    virtual void nextSeeds(const RenderContext& context, Packet<float>& results, RayPacket& rays);
    virtual void nextSeeds(const RenderContext& context, Packet<double>& results, RayPacket& rays);
  };
} // end namespace Manta

#endif // MANTA_ENGINE_SAMPLEGENERATORS_UNIFORM_RANDOM_GENERATOR_H_
