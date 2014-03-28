#ifndef MANTA_INTERFACE_SAMPLE_GENERATOR_H_
#define MANTA_INTERFACE_SAMPLE_GENERATOR_H_

// NOTE(boulos): Is there a good way to forward declare Packet<float>
// and Packet<double> even though they have to be MANTA_ALIGNed?
#include <Interface/Packet.h>

namespace Manta {
  class RayPacket;
  class RenderContext;
  class SetupContext;

  class SampleGenerator {
  public:
    virtual ~SampleGenerator() {}

    // If there's some sort of precomputation that needs to be done at
    // the beginning of the setup phase you can do that here.
    virtual void setupBegin(const SetupContext&, int numChannels) = 0;

    // Whenever the display changes (e.g. a resolution change) allow
    // the SampleGenerator to perform some action.
    virtual void setupDisplayChannel(SetupContext& context) = 0;

    // If for some reason you want to do something each frame, you can
    // do so.
    virtual void setupFrame(const RenderContext& context) = 0;

    // Before rays are shot through the scene, the PixelSampler should
    // allow the SampleGenerator to setup whatever state it needs
    // (e.g. sample_id within a Hammersley pattern)
    virtual void setupPacket(const RenderContext& context, RayPacket& rays) = 0;
    // Whenever a new RayPacket is created as a child of a parent
    // RayPacket, let the SampleGenerator take a shot at setting up
    // the child. For SampleGenerators that care to mess with
    // sample_depth, etc, this is a good opportunity to do so.
    virtual void setupChildPacket(const RenderContext& context, RayPacket& parent, RayPacket& child) = 0;

    // In some cases (notably Dielectric.cc as of 11-Dec-2007), the
    // parent-child relationship is not a direct 1-to-1 mapping. This
    // function asks the SampleGenerator to make child ray j a child
    // of parent ray i. For now, however, do NOT setup a child packet
    // from multiple parents as we are assuming that the region
    // constant flag can be determined from the parent alone.
    virtual void setupChildRay(const RenderContext& context, RayPacket& parent, RayPacket& child, int i, int j) = 0;

    // Grab a set of random seeds (float and double versions)
    virtual void nextSeeds(const RenderContext& context, Packet<float>& results, RayPacket& rays) = 0;
    virtual void nextSeeds(const RenderContext& context, Packet<double>& results, RayPacket& rays) = 0;
  };
} // end namespace Manta

#endif // MANTA_INTERFACE_SAMPLE_GENERATOR_H_
