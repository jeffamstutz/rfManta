#ifndef MANTA_ENGINE_TIMEVIEWSAMPLER_H
#define MANTA_ENGINE_TIMEVIEWSAMPLER_H

#include <MantaTypes.h>
#include <Interface/PixelSampler.h>
#include <Model/Textures/HeightColorMap.h>

#include <string>
#include <vector>

namespace Manta {

  class MantaInterface;
  
  class TimeViewSampler : public PixelSampler {
  public:
    TimeViewSampler(const std::vector<std::string>& args);
    virtual ~TimeViewSampler();

    // From PixelSampler
    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);
    virtual void renderFragment(const RenderContext& context,
                                Fragment& fragment);

    // For the factory interface, but it shouldn't really be called
    // from the factory anyway, since there isn't a mechanism in the
    // factory for child classes.
    static PixelSampler* create(const std::vector<std::string>& args);

    // New methods
    void setPixelSampler(PixelSampler* new_child);
    PixelSampler* getPixelSampler() { return child; }
    
    void setScale(ColorComponent new_scale);
    ColorComponent getScale() { return scale; }

    void setActive(bool activeOn);
    bool getActive() { return active; }

    //////////////////////////////////////
    // User interface helpers

    // This will toggle the timeView display from off, through the
    //different time views, and back to off on the manta_interface.
    //Both parameters must be non-NULL.  If non-NULL the function
    //returns the TimeViewSampler associated with the manta_interface,
    //otherwise it returns timeView.
    static TimeViewSampler* toggleTimeView(MantaInterface* manta_interface,
                                           TimeViewSampler* timeView);

    // Multiplies the scale by the factor.
    void changeScale(ColorComponent factor) { scale *= factor; }
  private:
    TimeViewSampler(const TimeViewSampler&);
    TimeViewSampler& operator=(const TimeViewSampler&);

    // Prints out the usage
    void usage();
    
    PixelSampler* child;
    ColorComponent scale;

    int state;
    static const int NUM_STATES = 2;
    bool active;
  };
}


#endif // #ifndef MANTA_ENGINE_TIMEVIEWSAMPLER_H
