
#ifndef Manta_Engine_Moire_h
#define Manta_Engine_Moire_h

#include <Interface/Renderer.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;
  class Moire : public Renderer {
  public:
    Moire(const vector<string>& args);
    virtual ~Moire();
    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);

    virtual void traceEyeRays(const RenderContext&, RayPacket& rays);
    virtual void traceRays(const RenderContext&, RayPacket& rays);

    static Renderer* create(const vector<string>& args);
  private:
    Moire(const Moire&);
    Moire& operator=(const Moire&);

    double rate;
    double cycles;
  };
}

#endif
