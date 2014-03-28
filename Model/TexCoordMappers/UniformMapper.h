
#ifndef Manta_Model_UniformMapper_h
#define Manta_Model_UniformMapper_h

#include <Interface/TexCoordMapper.h>

namespace Manta {
  class ArchiveElement;

  class UniformMapper : public TexCoordMapper {
  public:
    UniformMapper();
    virtual ~UniformMapper();

    virtual void computeTexCoords2(const RenderContext& context,
			   RayPacket& rays) const;
    virtual void computeTexCoords3(const RenderContext& context,
			    RayPacket& rays) const;

    void readwrite(ArchiveElement* archive);
  private:
    UniformMapper(const UniformMapper&);
    UniformMapper& operator=(const UniformMapper&);
  };
}

#endif
