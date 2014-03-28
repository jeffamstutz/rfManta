
#ifndef Manta_Model_ArcAmbient_h
#define Manta_Model_ArcAmbient_h

#include <Interface/AmbientLight.h>
#include <Core/Color/Color.h>
#include <Core/Geometry/Vector.h>
#include <string>

namespace Manta {

  class ArcAmbient : public AmbientLight {
  public:
    ArcAmbient();
    ArcAmbient(const Color& cup, const Color& cdown,
	       const Vector& up);
    virtual ~ArcAmbient();

    virtual void preprocess(const PreprocessContext&);
    virtual void computeAmbient(const RenderContext& context, RayPacket& rays, ColorArray ambient) const;

    virtual std::string toString() const;
    virtual void readwrite(ArchiveElement*);
  private:
    Color cup;
    Color cdown;
    Vector up;
  };
}

#endif
