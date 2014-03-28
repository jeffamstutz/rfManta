
#ifndef Manta_Interface_TexCoordMapper_h
#define Manta_Interface_TexCoordMapper_h

#include <Interface/Interpolable.h>

namespace Manta {
  class ArchiveElement;
  class RayPacket;
  class RenderContext;

  class TexCoordMapper : virtual public Interpolable {
  public:
    TexCoordMapper();
    virtual ~TexCoordMapper();

    virtual void computeTexCoords2(const RenderContext& context,
				   RayPacket& rays) const = 0;
    virtual void computeTexCoords3(const RenderContext& context,
				   RayPacket& rays) const = 0;

    void readwrite(ArchiveElement* archive);
  private:
    TexCoordMapper(const TexCoordMapper&);
    TexCoordMapper& operator=(const TexCoordMapper&);
  };
}

#endif
