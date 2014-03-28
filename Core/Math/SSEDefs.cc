
#include <Core/Math/SSEDefs.h>

#ifdef MANTA_SSE
#include <iostream>
#include <xmmintrin.h>

namespace std {
  std::ostream& operator<<(std::ostream& os, __m128 t)
  {
    // If you have problems getting out the correct value look at the
    // __m128i version for an alternative implementation.
    MANTA_ALIGN(16) float f[4];
    _mm_store_ps(f,t);
    os << f[0] << ", " << f[1] << ", " << f[2] << ", " << f[3];
    return os;
  }

#if !defined(__INTEL_COMPILER)
  // On intel compilers, it appears that the __m128 and __m128i are
  // the same type in terms of this operator, so we should only make a
  // single version.
  std::ostream& operator<<(std::ostream& os, __m128i t)
  {
    // This code caused bad values to be printed out.  Don't do it this way.
    //
    // MANTA_ALIGN(16) int i[4];
    // _mm_store_si128((__m128i*)&i[0],t);
    // os << "__m128i = " << i[0] << ", " << i[1] << ", " << i[2] << ", " << i[3];
    os << "__m128i = ";
    for (int i = 0; i < 4; i++) {
      os << ((int*)&t)[i];
      if (i != 3)
        os << ", ";
    }
    return os;
  }
#endif
}

#endif // MANTA_SSE
