#ifndef MANTA_CORE_COLOR_COLORSPACE_FANCY_H_
#define MANTA_CORE_COLOR_COLORSPACE_FANCY_H_

#include <Core/Color/ColorSpace.h>
#include <string>
#include <sstream>

namespace Manta {

  template<typename Traits>
  std::string ColorSpace<Traits>::toString() const {
    std::ostringstream out;
    out << "ColorSpace("<<NumComponents<<")(";
    // If you do math with an enum, you should do a cast to in
    // integral type.
    for(int i=0;i<static_cast<int>(NumComponents)-1;i++)
      out << data[i] << ", ";
    if (NumComponents > 0)
      out << data[static_cast<int>(NumComponents)-1] << ")";
    return out.str();
  }

} // end namespace Manta

#endif // MANTA_CORE_COLOR_COLORSPACE_FANCY_H_
