
#ifndef Manta_Core_Conversion_h
#define Manta_Core_Conversion_h

#include <Core/Color/GrayColor.h>
#include <Core/Color/RGBColor.h>
#include <Core/Color/Spectrum.h>

namespace Manta {
  inline void convertColor(RGBColor& rgb, const GrayColor& gray)
  {
    float g = gray.grayValue();
    rgb = RGBColor(g, g, g);
  }
  inline void convertColor(RGBColor& rgb, const Spectrum& spectrum)
  {
    rgb = spectrum.getRGB();
  }
}

#endif
