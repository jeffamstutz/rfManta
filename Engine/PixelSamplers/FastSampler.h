
#ifndef Manta_Engine_FastSampler_h
#define Manta_Engine_FastSampler_h

#include <MantaTypes.h>
#include <Interface/PixelSampler.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;
  class FastSampler : public PixelSampler {
  public:
    FastSampler(const vector<string>& args);
    virtual ~FastSampler();
    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);
    virtual void renderFragment(const RenderContext& context,
                                Fragment& fragment);

    static PixelSampler* create(const vector<string>& args);
  private:
    FastSampler(const FastSampler&);
    FastSampler& operator=(const FastSampler&);

    struct ChannelInfo {
      Real xscale;
      Real xoffset;
      Real yscale;
      Real yoffset;
    };
    vector<ChannelInfo> channelInfo;
  };
}

#endif
