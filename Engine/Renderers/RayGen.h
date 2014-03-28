
#ifndef Manta_Engine_RayGen_h
#define Manta_Engine_RayGen_h

#include <Interface/Renderer.h>
#include <Core/Color/Color.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;
  class RayGen : public Renderer {
  public:
    RayGen(const vector<string>& args);
    virtual ~RayGen();
    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);

    virtual void traceEyeRays(const RenderContext&, RayPacket& rays);
    virtual void traceRays(const RenderContext&, RayPacket& rays);

    static Renderer* create(const vector<string>& args);
  private:
    RayGen(const RayGen&);
    RayGen& operator=(const RayGen&);

    Color color;
  };
}

#endif
