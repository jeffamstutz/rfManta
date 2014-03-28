//vecdefs: common typedefs for the vector class


#ifndef Manta_Core_vecdefs_h
#define Manta_Core_vecdefs_h

#include <MantaTypes.h>
#include <Core/Geometry/VectorT.h>

namespace Manta {
  typedef VectorT<Real, 2> Vector2D;
  
  typedef VectorT<float, 2> Vec2f;
  typedef VectorT<float, 3> Vec3f;
  typedef VectorT<float, 4> Vec4f;
  
  typedef VectorT<int, 2> Vec2i;
  typedef VectorT<int, 3> Vec3i;
  typedef VectorT<int, 4> Vec4i;

} // end namespace Manta

#endif


























