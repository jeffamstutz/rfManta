
#ifndef Manta_Core_GrayTraits_h
#define Manta_Core_GrayTraits_h

#include <Core/Color/GrayColor.h>
#include <Core/Color/Conversion.h>

namespace Manta {
  class GrayTraits {
  public:
    typedef float ComponentType;
    enum {NumComponents = 1};

    // Gray
    static void convertFrom(ComponentType data, const GrayColor& gray) {
      data = gray.grayValue();
    }

    template <class C> static void convertFrom(ComponentType data,
                                               const C& color) {
      GrayColor gray;
      convertColor(gray, color);
      data = gray.grayValue();
    }

    // Everything else
    static void convertTo(GrayColor& gray, const ComponentType data) {
      gray = GrayColor(data);
    }

    template <class C> static void convertTo(C& color, const ComponentType data) {
      GrayColor gray(data);
      convertColor(color, gray);
    }

    static ComponentType luminance(const ComponentType data) {
      return data;
    }

    static ComponentType linearize(const ComponentType val) {
      return val;
    }
  };
}

#endif
