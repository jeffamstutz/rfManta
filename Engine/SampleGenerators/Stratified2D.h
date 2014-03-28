#ifndef MANTA_ENGINE_SAMPLEGENERATORS_STRATIFIED_2D_H_
#define MANTA_ENGINE_SAMPLEGENERATORS_STRATIFIED_2D_H_

#include <Interface/SampleGenerator.h>

namespace Manta {
  class RandomNumberGenerator;

  class Stratified2D : public SampleGenerator {
  public:
    Stratified2D(unsigned int spp);
    ~Stratified2D();

    void allocateSamples(unsigned int num_threads, unsigned int max_depth);

    void buildSamplesAllDepths(RandomNumberGenerator* rng,
                               unsigned int thread_id,
                               unsigned int region_id);

    void buildSamples(RandomNumberGenerator* rng,
                      unsigned int thread_id,
                      unsigned int region_id,
                      unsigned int depth);

    void build2DSamples(RandomNumberGenerator* rng,
                        float* x_samples,
                        float* y_samples);

    void build1DSamples(RandomNumberGenerator* rng,
                        float* sample_buf);

    unsigned int computeRegion(const RenderContext& context, RayPacket& rays, int i);

    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext& context);
    virtual void setupFrame(const RenderContext& context);

    virtual void setupPacket(const RenderContext& context, RayPacket& rays);
    virtual void setupChildPacket(const RenderContext& context, RayPacket& parent, RayPacket& child);
    virtual void setupChildRay(const RenderContext& context, RayPacket& parent, RayPacket& child, int i, int j);

    virtual void nextSeeds(const RenderContext& context, Packet<float>& results, RayPacket& rays);
    virtual void nextSeeds(const RenderContext& context, Packet<double>& results, RayPacket& rays);

    unsigned int spp;
    float inv_spp;
    unsigned int sqrt_spp;
    float inv_sqrt_spp;
    float*** thread_samples;
    unsigned int max_depth;
    unsigned int max_regions;
    unsigned int num_threads;
    unsigned int* current_region_id; // num_threads wide
    unsigned int* current_raw_region_id; // num_threads wide
    int** reached_depth; // num_threads by num_regions

    unsigned int xres;
    unsigned int yres;
  };
} // end namespace Manta

#endif // MANTA_ENGINE_SAMPLEGENERATORS_UNIFORM_RANDOM_GENERATOR_H_
