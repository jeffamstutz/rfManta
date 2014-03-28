
#ifndef Manta_Model_HeadLight_h
#define Manta_Model_HeadLight_h

#include <Interface/Light.h>
#include <Core/Geometry/Vector.h>
#include <Core/Color/Color.h>

namespace Manta {
  class HeadLight : public Light {
  public:
    HeadLight(const Real offset_, const Color &color_)
      : offset( offset_ ), color( color_ )
    {  };

    virtual void preprocess(const PreprocessContext&) { /* Does Nothing. */ };
    virtual void computeLight(RayPacket& destRays,
                              const RenderContext &context,
                              RayPacket& sourceRays) const;

    Real getOffset() const { return offset; };
    void setOffset( Real offset_ ) { offset = offset_; };

    Color getColor() const { return color; }
    void setColor(const Color& new_c) { color = new_c; }
    
  private:
    Real  offset;
    Color color;
  };
}

#endif
