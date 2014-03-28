
#ifndef Manta_Engine_HardShadows_h
#define Manta_Engine_HardShadows_h

#include <Interface/ShadowAlgorithm.h>
#include <vector>
#include <string>

namespace Manta {
  using namespace std;
  class HardShadows : public ShadowAlgorithm {
  public:
    HardShadows() {}
    HardShadows(const vector<string>& args);
    HardShadows(const bool attenuateShadows_ ) :
      attenuateShadows( attenuateShadows_ ) { }
    
    virtual ~HardShadows();
#ifndef SWIG
    virtual void computeShadows(const RenderContext& context, StateBuffer& stateBuffer,
                                const LightSet* lights, RayPacket& source, RayPacket& shadowRays);
#endif
    static ShadowAlgorithm* create(const vector<string>& args);

    virtual string getName() const;
    virtual string getSpecs() const;

    // If true it will compute attenuated shadows
    bool attenuateShadows;
  private:
    HardShadows(const HardShadows&);
    HardShadows& operator=(const HardShadows&);
  };
}

#endif
