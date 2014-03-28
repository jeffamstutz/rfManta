#ifndef Manta_Model_AmbientOcclusionBackground_h
#define Manta_Model_AmbientOcclusionBackground_h

#include <Interface/Background.h>
#include <Interface/AmbientLight.h>
#include <Core/Color/Color.h>
#include <Core/Geometry/Vector.h>
#include <MantaSSE.h>

namespace Manta
{
  class AmbientOcclusionBackground : public AmbientLight
  {
    public:
      AmbientOcclusionBackground(const Color& color, float cutoff_dist, int num_dirs, bool bounce = false, Background* bg = NULL);
      AmbientOcclusionBackground();

      ~AmbientOcclusionBackground();

      void generateDirections(int num_directions);
      void setCutoffDistance(float dist) { cutoff = dist; }
      void setNumRays(int num_dirs) {
        num_directions = num_dirs;
        inv_num_directions = 1.0/num_dirs;
        generateDirections(num_dirs);
      }
      //setting background to non null will cause  any non-hitting rays to use bg colors
      // if bounce is enabled the scenes background is always used because this is faster...
      void setBackground(Background* bg) { background = bg; }
      //background sampling is multiplied by backgroundcolor
      void setBackgroundColor(const Color& c) { bcolor = c; }
      void setColor(const Color& c) { color = c; }
      //note that shortening the sampling angle results in a directed reflection like a glossy material
      //angle is in radians, default is a full 2pi
      void setSamplingAngle(float a) { samplingAngle = a; }

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
      Background* background;
      Color bcolor;
      float samplingAngle;
  };
}

#endif
