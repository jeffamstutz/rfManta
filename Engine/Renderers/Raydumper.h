
#ifndef Manta_Engine_Raydumper_h
#define Manta_Engine_Raydumper_h

#include <Interface/MantaInterface.h>
#include <Interface/RayPacket.h>
#include <Interface/Renderer.h>
#include <Interface/ShadowAlgorithm.h>
#include <pabst.h>
#include <sgi_stl_warnings_off.h>
#include <string>
#include <vector>
#include <sgi_stl_warnings_on.h>

namespace Manta {
  using namespace std;

  class Raydumper : public Renderer, public ShadowAlgorithm {
  public:
    Raydumper(const vector<string>& args);
    virtual ~Raydumper();

    // Renderer
    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);

    virtual void traceEyeRays(const RenderContext&, RayPacket& rays);
    virtual void traceRays(const RenderContext&, RayPacket& rays);

    static Renderer* create(const vector<string>& args);

    // Shadow algorithm
    virtual bool computeShadows(const RenderContext& context,
                                const LightSet* lights, RayPacket& source,
                                RayPacket& shadowRays, bool firstTime,
                                StateBuffer& stateBuffer);
    
    string getName() const;
    string getSpecs() const;

  private:
    Raydumper(const Raydumper&);
    Raydumper& operator=(const Raydumper&);

    void logRays(RayPacket& rays, RayInfo::RayType type);
    void quit(int, int);

    string renderer_string;
    string shadow_string;

    MantaInterface* rtrt;
    Renderer* renderer;
    ShadowAlgorithm* shadows;

    RayForest forest;
    unsigned long long ray_id;
    FILE* fp;
  };
}

#endif
