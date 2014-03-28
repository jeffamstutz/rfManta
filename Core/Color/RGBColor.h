
#ifndef Manta_Core_RGBColor_h
#define Manta_Core_RGBColor_h

#include <string>

namespace Manta {
  class RGBColor {
  public:
    RGBColor()
    {
    }

    RGBColor(float r, float g, float b)
    {
      data[0] = r;
      data[1] = g;
      data[2] = b;
    }
    
    RGBColor(const RGBColor& copy)
    {
      for(int i=0;i<3;i++)
	data[i]=copy.data[i];
    }

    ~RGBColor() {
    }

    float r() const {
      return data[0];
    }
    float g() const {
      return data[1];
    }
    float b() const {
      return data[2];
    }

    void setR(float r) {
      data[0] = r;
    }

    void setG(float g) {
      data[1] = g;
    }

    void setB(float b) {
      data[2] = b;
    }

    float getComponent(int i) const {
      return data[i];
    }

    std::string toString() const;
    std::string writePersistentString() const;
    bool readPersistentString(const std::string& str);
    
  protected:
    float data[3];
  };

  // Shorthand for RGBColor
  typedef RGBColor RGB;
}

#endif
