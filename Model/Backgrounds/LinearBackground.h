
#ifndef Manta_Model_LinearBackground_h
#define Manta_Model_LinearBackground_h

#include <Interface/Background.h>
#include <Core/Color/Color.h>
#include <Core/Geometry/Vector.h>
#include <Core/Persistent/MantaRTTI.h>

namespace Manta {

  class LinearBackground : public Background {
  public:
    LinearBackground();
    LinearBackground(const Color& cup, const Color& cdown, const Vector& up);
    virtual ~LinearBackground();

    virtual void preprocess(const PreprocessContext& context);
    virtual void shade(const RenderContext& context, RayPacket& rays) const;

    void readwrite(ArchiveElement* archive);
  private:
    Color cup;
    Color cdown;
    Vector up;
  };
}

#endif
