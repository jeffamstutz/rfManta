
#ifndef Manta_Engine_Raytracer_h
#define Manta_Engine_Raytracer_h

#include <Interface/Renderer.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;
  class ShadowAlgorithm;

  class Raytracer : public Renderer {
  public:
    Raytracer() {}
    Raytracer(const vector<string>& args);
    virtual ~Raytracer();
    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);

    virtual void traceEyeRays(const RenderContext&, RayPacket& rays);
    virtual void traceRays(const RenderContext&, RayPacket& rays);
    virtual void traceRays(const RenderContext&, RayPacket& rays, Real cutoff);

    static Renderer* create(const vector<string>& args);
  private:
    Raytracer(const Raytracer&);
    Raytracer& operator=(const Raytracer&);
  };
}

#endif
