#ifndef Manta_Engine_NPREdges_h
#define Manta_Engine_NPREdges_h

#include <Interface/Renderer.h>

#include <string>
#include <vector>

using namespace Manta;

namespace Manta {
  class Primitive;
  class Raytracer;
  class RayPacketData;

  using namespace std;

  class NPREdges : public Renderer {
  public:
    NPREdges() {}
    NPREdges(const vector<string>& args);
    virtual ~NPREdges();
    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);

    virtual void traceEyeRays(const RenderContext&, RayPacket& rays);
    virtual void traceRays(const RenderContext&, RayPacket& rays);
    virtual void traceRays(const RenderContext&, RayPacket& rays, Real cutoff);

    static Renderer* create(const vector<string>& args);

    void setNormalThreshold(Real val);

  protected:
    void computeCircleStencil(RayPacket *stencil, RayPacketData *stencil_data, const RayPacket& rays) const;

  protected:
    typedef enum {
      Background,
      Shade,
      Intersection
    } PixelType ;

    PixelType type(const RayPacket& sample, const RayPacket *stencil, int i) const;
    PixelType type(const RayPacket& sample, const RayPacket *stencil, int i, int& count) const;

    static bool creasable(const Primitive *prim);

    Raytracer *raytracer;

    Real imageX, imageY;
    Real lineWidth, normalThreshold;
    bool computeCreases, noshade;

    int N, M, stencil_size;
    int last_ring;

  private:
    NPREdges(const NPREdges&);
    NPREdges& operator=(const NPREdges&);
  };
}

#endif
