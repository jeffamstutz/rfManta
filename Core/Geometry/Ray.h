
#ifndef Manta_Interface_Ray_h
#define Manta_Interface_Ray_h

#include <Core/Geometry/Vector.h>

namespace Manta {
  class Ray {
  public:
    Ray()
    {
    }
    Ray(const Vector& origin, const Vector& direction)
      : orig(origin), dir(direction)
    {
    }

#ifndef SWIG
    Ray& operator=(const Ray& copy)
    {
      orig=copy.orig;
      dir=copy.dir;
      return *this;
    }
#endif

    void set(const Vector& origin, const Vector& direction)
    {
      orig = origin;
      dir = direction;
    }
    void setOrigin(const Vector& origin)
    {
      orig = origin;
    }
    void setDirection(const Vector& direction)
    {
      dir = direction;
    }

    const Vector& origin() const {
      return orig;
    }
    const Vector& direction() const {
      return dir;
    }
    Real normalizeDirection() {
      return dir.normalize();
    }
  private:
    Vector orig;
    Vector dir;
  };
}

#endif

