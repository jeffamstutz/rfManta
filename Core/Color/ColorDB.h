
#ifndef Manta_Core_ColorDB_h
#define Manta_Core_ColorDB_h

#include <Core/Color/Color.h>
#include <string>

namespace Manta {
  class RGBColor;
  class ColorDB {
  public:
    static Color getNamedColor(const std::string& name);
    static bool getNamedColor(RGBColor& result, const std::string& name);
    static bool  getNamedColor(unsigned char result[3], const std::string &name);

    // Tests to make sure the namedColors array is sorted properly and
    // that you can find all the entries.  Return true if everything
    // is in order, false otherwise with some errors going to cerr.
    static bool  test();
  private:
    // Prevent instantiation
    ColorDB(const ColorDB&);
  };
}

#endif
