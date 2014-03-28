
#ifndef Manta_Engine_ClusterSampler_h
#define Manta_Engine_ClusterSampler_h

#include <Interface/PixelSampler.h>
#include <Core/Math/MT_RNG.h>
#include <Core/Geometry/vecdefs.h>

#include <string>
#include <vector>


namespace Manta {
  using namespace std;
  class RayPacket;
  class SampleGenerator;

  class ClusterSampler : public PixelSampler {
  public:
    ClusterSampler(const vector<string>& args);

    virtual ~ClusterSampler();
    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);
    virtual void renderFragment(const RenderContext& context,
                                Fragment& fragment);

    static PixelSampler* create(const vector<string>& args);
  private:
    ClusterSampler(const ClusterSampler&);
    ClusterSampler& operator=(const ClusterSampler&);

    void computeAverages(Fragment& fragment, const RayPacket& rays,
                         int& samples_collected, int& current_fragment);

    int num_samples;
    int min_shading_samples;
    // nx*ny == num_samples where nx~=ny (or as close as you can get it).
    int nx, ny;
    int maxSqrt;

    struct ChannelInfo {
      Real xscale;
      Real xoffset;
      Real yscale;
      Real yoffset;
      int xres, yres;
    };

    SampleGenerator* antialiasGenerator;
    SampleGenerator** sampleGenerators; // An array of sample generators for each refinement level
    vector<ChannelInfo> channelInfo;
  };
}

#endif
