#ifndef Manta_Interface_Colormap_h
#define Manta_Interface_Colormap_h

namespace Manta{
  template<typename C, typename T>
  class Colormap {
  public:
    typedef C ColorType;
    typedef T ValueType;

  public:
    virtual ~Colormap() {}

    virtual C color(const T& value) const = 0;
  };
}

#endif
