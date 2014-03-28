
#ifndef Manta_Core_RegularColorMap_h
#define Manta_Core_RegularColorMap_h

#include <Core/Geometry/ScalarTransform1D.h>
#include <Core/Containers/Array1.h>
#include <MantaTypes.h>
#include <Core/Color/ColorSpace.h>

namespace Manta
{
  class RegularColorMap
  {
  public:
    RegularColorMap(const char* filename, unsigned int size=256);
    RegularColorMap(Array1<Color>& colors, unsigned int size=256);
    RegularColorMap(unsigned int type=RegularColorMap::InvRainbowIso,
                    unsigned int size=256);

    virtual ~RegularColorMap(void) { }

    static unsigned int parseType(const char* type);
    
    enum {
      InvRainbowIso=0,
      InvRainbow,
      Rainbow,
      InvGreyScale,
      InvBlackBody,
      BlackBody,
      GreyScale,      
      Unknown
    };

#ifndef SWIG
    ScalarTransform1D<float, Color> lookup;
    Array1<Color> blended;
#endif


    
    // Normalized lookup.
    const Color &normalized( const Real &normalized ) const {

      const int ncolors = blended.size() - 1;
      const int idx = Clamp(static_cast<int>(normalized*ncolors), 0, ncolors);
      return blended[idx];
    }

  protected:
    bool fillColor(const char* filename, Array1<Color>& colors);
    void fillColor(unsigned int type, Array1<Color>& colors);
    void fillBlendedColors(Array1<Color>& blended);

    Array1<Color> colors;
    Array1<Color> new_blended;
  };
}

#endif // Manta_Core_RegularColorMap_h
