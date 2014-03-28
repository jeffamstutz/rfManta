
#ifndef Manta_Engine_KajiyaPathtracer_h
#define Manta_Engine_KajiyaPathtracer_h

#include <Interface/Renderer.h>
#include <Interface/Packet.h>
#include <string>
#include <vector>

namespace Manta {
  using namespace std;
  class ShadowAlgorithm;

  class KajiyaPathtracer : public Renderer {
  public:
    enum PreSortMode {
      PreSortNone, PreSortDirection, PreSortOrigin, PreSort5D
    };
    enum MatlSortMode {
      MatlSortBackgroundSweep, MatlSortSweep, MatlSortFull
    };
    enum RussianRouletteMode {
      RussianRouletteNone, RussianRouletteFull
    };

    KajiyaPathtracer() {}
    KajiyaPathtracer(const vector<string>& args);
    virtual ~KajiyaPathtracer();
    virtual void setupBegin(const SetupContext&, int numChannels);
    virtual void setupDisplayChannel(SetupContext&);
    virtual void setupFrame(const RenderContext& context);

    virtual void traceEyeRays(const RenderContext&, RayPacket& rays);
    virtual void traceRays(const RenderContext&, RayPacket& rays);
    virtual void traceRays(const RenderContext&, RayPacket& rays, Real cutoff);

    static Renderer* create(const vector<string>& args);
  private:
    KajiyaPathtracer(const KajiyaPathtracer&);
    KajiyaPathtracer& operator=(const KajiyaPathtracer&);

    std::vector<PreSortMode> presort_mode;
    std::vector<MatlSortMode> matlsort_mode;
    std::vector<RussianRouletteMode> russian_roulette_mode;
    ShadowAlgorithm* ambient_only_sa;
    int override_maxdepth;
    bool do_ambient_on_maxdepth;

    void presort_origin(const RenderContext& context, RayPacket& rays, Packet<Color>& result,
                        Packet<int>& permute);
    void presort_direction(const RenderContext& context, RayPacket& rays, Packet<Color>& result,
                           Packet<int>& permute);
    void presort_5d(const RenderContext& context, RayPacket& rays, Packet<Color>& result,
                    Packet<int>& permute);

    void matlsort_sweep(const RenderContext& context, RayPacket& rays, Packet<Color>& result,
                        Packet<Color>& reflectance, Packet<int>& permute);
    void matlsort_bgsweep(const RenderContext& context, RayPacket& rays,  Packet<Color>& result,
                          Packet<int>& permute);
    void matlsort_full(const RenderContext& context, RayPacket& rays, Packet<Color>& result,
                       Packet<int>& permute);

    void russian_roulette_full(const RenderContext& context, RayPacket& rays, Packet<Color>& result,
                               Packet<Color>& reflectance, Packet<int>& permute);
  };
}

#endif
