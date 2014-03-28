
#ifndef Manta_Interface_Texture_h
#define Manta_Interface_Texture_h

#include <Interface/Interpolable.h>
#include <Interface/Packet.h>

namespace Manta {
  class RayPacket;
  class RenderContext;
  template<typename ValueType>
  class Texture : virtual public Interpolable {
  public:
    Texture() {}
    virtual ~Texture() {}

    virtual void mapValues(Packet<ValueType>& results,
                           const RenderContext&,
                           RayPacket& rays) const = 0;
  private:
    Texture(const Texture&);
    Texture& operator=(const Texture&);
  };
}

#endif
