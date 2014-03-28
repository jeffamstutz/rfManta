
#ifndef Manta_Interface_Primitive_h
#define Manta_Interface_Primitive_h

#include <Interface/Object.h>
#include <Interface/Packet.h>
#include <MantaTypes.h>

namespace Manta {
  class TexCoordMapper;
  class Vector;
  class Primitive : public Object {
  public:
    Primitive();
    virtual ~Primitive();

    virtual void preprocess(const PreprocessContext& context) {};
    virtual void intersect(const RenderContext& context,
                           RayPacket& rays) const = 0;
    virtual void computeNormal(const RenderContext& context,
                               RayPacket& rays) const = 0;
    virtual void computeGeometricNormal(const RenderContext& context,
                                        RayPacket& rays) const;

    // Compute dPdu and dPdv (aka tangent vectors) - these are not
    // normalized. Cross(dPdu, dPdv) = k*N where k is some scale
    // factor. By default, we'll just construct a coordinate frame
    // around the Normal so that the invariant is satisfied.
    virtual void computeSurfaceDerivatives(const RenderContext& context,
                                           RayPacket& rays) const;

    virtual void setTexCoordMapper(const TexCoordMapper* new_tex) = 0;

    virtual void getRandomPoints(Packet<Vector>& points,
                                 Packet<Vector>& normals,
                                 Packet<Real>& pdfs,
                                 const RenderContext& context,
                                 RayPacket& rays) const;
  private:
    Primitive(const Primitive&);
    Primitive& operator=(const Primitive&);
  };
}

#endif
