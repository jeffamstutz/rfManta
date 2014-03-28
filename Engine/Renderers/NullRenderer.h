
#ifndef Manta_Engine_NullRenderer_h
#define Manta_Engine_NullRenderer_h

#include <Interface/Renderer.h>
#include <Core/Color/Color.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;
  class NullRenderer : public Renderer {
  public:
    NullRenderer(const vector<string>& args);
    virtual ~NullRenderer();
    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);

    virtual void traceEyeRays(const RenderContext&, RayPacket& rays);
    virtual void traceRays(const RenderContext&, RayPacket& rays);

    static Renderer* create(const vector<string>& args);
  private:
    NullRenderer(const NullRenderer&);
    NullRenderer& operator=(const NullRenderer&);

    Color color;
  };
}

#endif
