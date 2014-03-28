
#ifndef Manta_Core_GrayColor_h
#define Manta_Core_GrayColor_h

#include <string>

namespace Manta {
  class GrayColor {
  public:
    GrayColor()
    {
    }

    GrayColor(float value)
      : value(value)
    {
    }
    
    GrayColor(const GrayColor& copy)
    {
      value = copy.value;
    }

    ~GrayColor() {
    }

    float grayValue() const
    {
      return value;
    }

    std::string toString() const;
    
  protected:
    float value;
  };
}

#endif

