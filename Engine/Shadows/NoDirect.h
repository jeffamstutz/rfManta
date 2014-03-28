
#ifndef Manta_Engine_NoDirect_h
#define Manta_Engine_NoDirect_h

#include <Interface/ShadowAlgorithm.h>
#include <vector>
#include <string>

namespace Manta {
  using namespace std;
  class NoDirect : public ShadowAlgorithm {
  public:
    NoDirect();
    NoDirect(const vector<string>& args);
    virtual ~NoDirect();

#ifndef SWIG
    virtual void computeShadows(const RenderContext& context, StateBuffer& stateBuffer,
                                const LightSet* lights, RayPacket& source, RayPacket& shadowRays);
#endif
    static ShadowAlgorithm* create(const vector<string>& args);

    virtual string getName() const;
    virtual string getSpecs() const;

  private:
    NoDirect(const NoDirect&);
    NoDirect& operator=(const NoDirect&);
  };
}

#endif
