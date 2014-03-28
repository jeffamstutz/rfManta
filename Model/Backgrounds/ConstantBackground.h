
#ifndef Manta_Model_ConstantBackground_h
#define Manta_Model_ConstantBackground_h

#include <Interface/Background.h>
#include <Core/Color/Color.h>
#include <Core/Persistent/MantaRTTI.h>

namespace Manta{
  class ConstantBackground : public Background {
  public:
    ConstantBackground();
    ConstantBackground(const Color& color);
    virtual ~ConstantBackground();

    virtual void preprocess(const PreprocessContext& context);
    virtual void shade(const RenderContext& context, RayPacket& rays) const;

    Color getValue() const;
    void setValue(Color new_color);

    void readwrite(ArchiveElement* archive);
  private:
    Color bgcolor;
  };
}

#endif
