
#ifndef Manta_Interface_Renderer_h
#define Manta_Interface_Renderer_h

#include <MantaTypes.h>

namespace Manta {
  class SetupContext;
  class RenderContext;
  class RayPacket;
  class Renderer {
  public:
    virtual ~Renderer();
    virtual void setupBegin(const SetupContext&, int numChannels) = 0;
    virtual void setupDisplayChannel(SetupContext&) = 0;
    virtual void setupFrame(const RenderContext& context) = 0;

    virtual void traceEyeRays(const RenderContext&, RayPacket& rays) = 0;
    virtual void traceRays(const RenderContext&, RayPacket& rays) = 0;
    virtual void traceRays(const RenderContext&, RayPacket& rays, Real cutoff);
  protected:
    Renderer();
  private:
    Renderer(const Renderer&);
    Renderer& operator=(const Renderer&);
  };
}

#endif
