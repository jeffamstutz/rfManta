#include <Interface/MantaInterface.h>
#include <Core/Util/Callback.h>

#include <Engine/Factory/Factory.h>
#include <Engine/Factory/RegisterKnownComponents.h>

#include <string>

namespace Manta {
  using namespace std;

  class PipelineChanger {
    MantaInterface* rtrt_interface;
    Factory factory;
  public:
    PipelineChanger(MantaInterface* rtrt_interface):
      rtrt_interface(rtrt_interface),
      factory( rtrt_interface )
    {
      registerKnownComponents( &factory );
    }
    
    void changePixelSamplerCallback(int, int,
                                    string spec)
    {
      factory.selectPixelSampler(spec);
    }

    void changeResolutionCallBack(int, int,
                                  int channel, int new_xres, int new_yres)
    {
      rtrt_interface->changeResolution(channel, new_xres, new_yres, true);
    }
    
    void changePixelSampler(string spec) {
      rtrt_interface->addOneShotCallback(MantaInterface::Relative, 0,
            Callback::create(this, &PipelineChanger::changePixelSamplerCallback,
                             spec));
    }

    void changeResolution(int channel, int new_xres, int new_yres) {
      rtrt_interface->addOneShotCallback(MantaInterface::Relative, 0,
            Callback::create(this, &PipelineChanger::changeResolutionCallBack,
                             channel, new_xres, new_yres));
    }
  };

} // end namespace Manta
