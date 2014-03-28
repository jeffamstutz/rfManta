#include <Interface/RandomNumberGenerator.h>

namespace Manta {

  // Don't put these in the header, or you will get multiply defined
  // symbols.
  template<>
  float RandomNumberGenerator::next<float>()
  {
    return nextFloat();
  }

  template<>
  double RandomNumberGenerator::next<double>()
  {
    return nextDouble();
  }

} // end namespace Manta
