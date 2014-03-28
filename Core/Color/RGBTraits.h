
#ifndef Manta_Core_RGBTraits_h
#define Manta_Core_RGBTraits_h

#include <Core/Color/RGBColor.h>
#include <Core/Color/Conversion.h>
#include <Core/Persistent/MantaRTTI.h>
#include <Core/Math/Expon.h>

namespace Manta {
  class ArchiveElement;
  class RGBTraits {
  public:
    typedef RGBColor ColorType;
    typedef float ComponentType;
    enum {NumComponents = 3};

    // RGB
    static void convertFrom(ComponentType data[3], const RGBColor& rgb) {
      data[0] = rgb.r();
      data[1] = rgb.g();
      data[2] = rgb.b();
    }

    template <class C> static void convertFrom(ComponentType data[3], const C& color) {
      RGBColor rgb;
      convertColor(rgb, color);
      data[0] = rgb.r();
      data[1] = rgb.g();
      data[2] = rgb.b();
    }

    // Everything else
    static void convertTo(RGBColor& rgb, const ComponentType data[3]) {
      rgb = RGBColor(data[0], data[1], data[2]);
    }

    template <class C> static void convertTo(C& color, const ComponentType data[3]) {
      RGBColor rgb(data[0], data[1], data[2]);
      convertColor(color, rgb);
    }

    static ComponentType luminance(const ComponentType data[3]) {
      return data[0] * ComponentType(0.3) + data[1] * ComponentType(0.59) + data[2] * ComponentType(0.11);
    }

    // Convert from SRGB to RGB
    static ComponentType linearize(const ComponentType val) {
#if 1
      if (val > 0.04045) {
        return Pow((val + .055)/(1.055), 2.4);
      } else {
        return val / 12.92;
      }
#else
      return Pow(val, 2.2f);
#endif
    }

    static void readwrite(ArchiveElement* archive, ComponentType data[3]);
    // Uncomment when you actually fill this in
    //static bool parseColor(RGBColor& result, const std::string& str);
  };
}

#endif
