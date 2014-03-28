
#ifndef Manta_Engine_Noise_h
#define Manta_Engine_Noise_h

#include <Interface/Renderer.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;
  class Noise : public Renderer {
  public:
    Noise(const vector<string>& args);
    virtual ~Noise();
    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);

    virtual void traceEyeRays(const RenderContext&, RayPacket& rays);
    virtual void traceRays(const RenderContext&, RayPacket& rays);

    static Renderer* create(const vector<string>& args);
  private:
    Noise(const Noise&);
    Noise& operator=(const Noise&);

    double rate;
    bool sse;
  };
}

#endif
