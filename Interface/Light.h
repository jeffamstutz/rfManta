
#ifndef Manta_Interface_Light_h
#define Manta_Interface_Light_h

#include <Core/Color/Color.h>
#include <Core/Geometry/Vector.h>
#include <Interface/RayPacket.h>

namespace Manta {

  class Archive;
  class PreprocessContext;
  class RenderContext;

  class Light {
  public:
    Light();
    virtual ~Light();

    virtual void preprocess(const PreprocessContext& context ) {};

    // This method is called on the light by the shadow algorithm. The
    // color and direction produced by the light may change for each
    // ray in the packet, and may change based on the render context.
    //
    // This function is in charge of filling in:
    // 1. The color:     RayPacket::setColor()
    // 2. The direction: RayPacket::setDirection()
    // 3. The distance:  RayPacket::overrideMinT() (in terms of the
    //                                              direction length).
    virtual void computeLight(RayPacket& destRays,
                              const RenderContext &context,
                              RayPacket& sourceRays) const = 0;

    void readwrite(Archive* archive);
  private:
    // Lights may not be copied.
    Light( const Light & );
    Light& operator = ( const Light & );
  };
}

#endif
