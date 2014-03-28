
#ifndef Manta_Engine_JitterSampler_h
#define Manta_Engine_JitterSampler_h

#include <Interface/PixelSampler.h>
#include <Core/Math/MT_RNG.h>
#include <Core/Geometry/vecdefs.h>

#include <string>
#include <vector>


namespace Manta {
  using namespace std;
  class RayPacket;
  
  class JitterSampler : public PixelSampler {
  public:
    JitterSampler(const vector<string>& args);
    JitterSampler( const int nx_, const int ny_, bool cheap_ = false ) :
      num_samples( nx_ * ny_ ), nx( nx_ ), ny( ny_ ), use_cheaprng( cheap_ ) { }

    virtual ~JitterSampler();
    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);
    virtual void renderFragment(const RenderContext& context,
				Fragment& fragment);

    static PixelSampler* create(const vector<string>& args);
  private:
    JitterSampler(const JitterSampler&);
    JitterSampler& operator=(const JitterSampler&);

    void computeAverages(Fragment& fragment, const RayPacket& rays,
                         int& samples_collected, int& current_fragment);
    
    int num_samples;
    // nx*ny == num_samples where nx~=ny (or as close as you can get it).
    int nx, ny;
    bool use_cheaprng;
    
    struct ChannelInfo {
      Real xscale;
      Real xoffset;
      Real yscale;
      Real yoffset;
      int xres, yres;
    };

    MT_RNG *random;
    vector<ChannelInfo> channelInfo;
  };
}

#endif
