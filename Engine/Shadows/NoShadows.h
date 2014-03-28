
#ifndef Manta_Engine_NoShadows_h
#define Manta_Engine_NoShadows_h

#include <Interface/ShadowAlgorithm.h>
#include <vector>
#include <string>

namespace Manta {
  using namespace std;
  class NoShadows : public ShadowAlgorithm {
  public:
    NoShadows();
    NoShadows(const vector<string>& args);
    virtual ~NoShadows();

#ifndef SWIG
    virtual void computeShadows(const RenderContext& context, StateBuffer& stateBuffer,
                                const LightSet* lights, RayPacket& source, RayPacket& shadowRays);
#endif
    static ShadowAlgorithm* create(const vector<string>& args);

    virtual string getName() const;
    virtual string getSpecs() const;

  private:
    NoShadows(const NoShadows&);
    NoShadows& operator=(const NoShadows&);
  };
}

#endif
