
#ifndef Manta_Engine_SingleSampler_h
#define Manta_Engine_SingleSampler_h

#include <MantaTypes.h>
#include <Interface/PixelSampler.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;
  class SingleSampler : public PixelSampler {
  public:
    SingleSampler(const vector<string>& args);
    virtual ~SingleSampler();
    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);
    virtual void renderFragment(const RenderContext& context,
                                Fragment& fragment);

    static PixelSampler* create(const vector<string>& args);
  private:
    SingleSampler(const SingleSampler&);
    SingleSampler& operator=(const SingleSampler&);

    struct ChannelInfo {
      Real xscale;
      Real xoffset;
      Real yscale;
      Real yoffset;
      int xres, yres;
    };
    vector<ChannelInfo> channelInfo;
  };
}

#endif
