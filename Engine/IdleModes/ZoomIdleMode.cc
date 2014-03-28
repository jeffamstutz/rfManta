
#include <Engine/IdleModes/ZoomIdleMode.h>
#include <Interface/Context.h>
#include <Interface/MantaInterface.h>
#include <Core/Math/MinMax.h>

using namespace Manta;

IdleMode* ZoomIdleMode::create(const vector<string>& args)
{
  return new ZoomIdleMode(args);
}

ZoomIdleMode::ZoomIdleMode(const vector<string>& /* args */)
{
  factor = 6;
}

ZoomIdleMode::~ZoomIdleMode()
{
}

void ZoomIdleMode::changeIdleMode(const SetupContext& context, bool changed,
                                  bool firstFrame, bool& setupNeeded)
{
  if(firstFrame)
    return;
  int oldSize = static_cast<int>(channeldata.size());
  channeldata.resize(context.numChannels);
  if(context.numChannels > oldSize){
    for(int i=oldSize; i < context.numChannels; i++)
      channeldata[i].xres = channeldata[i].yres = -1;
  }
  for(int channel=0;channel < context.numChannels; channel++){
    int xres, yres;
    bool stereo;
    context.rtrt_int->getResolution(channel, stereo, xres, yres);
    if(changed){
      // Save old resolution and shrink the image
      channeldata[channel].xres = xres;
      channeldata[channel].yres = yres;
      xres = Max((int)(xres/factor+(Real)0.5), 0);
      yres = Max((int)(yres/factor+(Real)0.5), 0);
    } else {
      if(channeldata[channel].xres == -1){
        // Grow it - we don't know what the old res was
        xres = Max((int)(xres*factor+(Real)0.5), 0);
        yres = Max((int)(yres*factor+(Real)0.5), 0);
      } else {
        // Restore old resolution
        xres = channeldata[channel].xres;
        yres = channeldata[channel].yres;
        channeldata[channel].xres = -1;
        channeldata[channel].yres = -1;
      }
    }
    context.rtrt_int->changeResolution(channel, xres, yres, false);
  }
  setupNeeded = true;
}
