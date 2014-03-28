#ifndef Manta_Model_AmbientOcclusion_h
#define Manta_Model_AmbientOcclusion_h

#include <Interface/AmbientLight.h>
#include <Core/Color/Color.h>
#include <Core/Geometry/Vector.h>
#include <MantaSSE.h>

namespace Manta
{
  class AmbientOcclusion : public AmbientLight
  {
  public:
    AmbientOcclusion(const Color& color, float cutoff_dist, int num_dirs, bool bounce = false);
    AmbientOcclusion() {  }
    ~AmbientOcclusion();

    void generateDirections(int num_directions);
    void setCutoffDistance(float dist) { cutoff = dist; }
    void setNumRays(int num_dirs) {
      num_directions = num_dirs;
      inv_num_directions = 1.0/num_dirs;
      generateDirections(num_dirs);
    }

    void gatherColor(bool enable) { bounce = enable; }

    // generate the directions
    void computeAmbient(const RenderContext& context, RayPacket& rays,
                        ColorArray ambient) const;

    virtual std::string toString() const;
  private:
    float cutoff;
    Color color;
#ifdef MANTA_SSE
    sse_t color4[Manta::Color::NumComponents];
#endif
    bool bounce;

    // increase the numPermutations past 1 to remove aliasing at the
    // expense of a slight performance loss.
    static const int numPermutations = 127;
    Real* directions[numPermutations][3];
    int num_directions;
    ColorComponent inv_num_directions;
  };
}

#endif
