
#ifndef Manta_Engine_NullSampler_h
#define Manta_Engine_NullSampler_h

#include <Interface/PixelSampler.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;
  class NullSampler : public PixelSampler {
  public:
    NullSampler(const vector<string>& args);
    virtual ~NullSampler();
    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);
    virtual void renderFragment(const RenderContext& context, Fragment& fragment);

    static PixelSampler* create(const vector<string>& args);
  private:
    NullSampler(const NullSampler&);
    NullSampler& operator=(const NullSampler&);
  };
}

#endif
