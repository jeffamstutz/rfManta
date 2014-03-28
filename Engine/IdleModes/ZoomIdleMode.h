
#ifndef Manta_Engine_ZoomIdleMode_h
#define Manta_Engine_ZoomIdleMode_h

#include <MantaTypes.h>
#include <Interface/IdleMode.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;

  class ZoomIdleMode : public IdleMode {
  public:
    ZoomIdleMode(const vector<string>& args);
    virtual ~ZoomIdleMode();

    virtual void changeIdleMode(const SetupContext&, bool changed,
                                bool firstFrame, bool& pipelineNeedsSetup);

    static IdleMode* create(const vector<string>& args);
  private:
    ZoomIdleMode(const ZoomIdleMode&);
    ZoomIdleMode& operator=(const ZoomIdleMode&);

    struct ChannelData {
      int xres, yres;
    };
    vector<ChannelData> channeldata;

    Real factor;
  };
}

#endif
