
#ifndef Manta_Core_Spectrum_h
#define Manta_Core_Spectrum_h

#include <Core/Color/RGBColor.h>
#include <string>
#include <vector>

namespace Manta {
  class Spectrum {
  public:
    Spectrum()
    {
    }

    Spectrum(const std::vector<float>& samples)
      : samples(samples)
    {
    }
    
    Spectrum(const Spectrum& copy)
      : samples(copy.samples)
    {
    }

    ~Spectrum() {
    }

    std::string toString() const;

    RGBColor getRGB() const;
    std::string writePersistentString() const;
    bool readPersistentString(const std::string& str);
    
  protected:
    std::vector<float> samples;
  };
}

#endif
