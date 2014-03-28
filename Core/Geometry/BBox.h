
#ifndef Manta_Core_BBox_h
#define Manta_Core_BBox_h

#include <Core/Geometry/Vector.h>
#include <Core/Math/Expon.h>
#include <Core/Persistent/MantaRTTI.h>
#include <limits>
#include <iosfwd>

namespace Manta {
  class BBox {
  public:

    BBox(const Vector& min_, const Vector& max_ ) {
      bounds[0] = min_;
      bounds[1] = max_;
    }
    inline BBox() {
      // Need to initialize min and max.
      reset();
    }
    inline ~BBox() {
    }
    BBox(const BBox& copy)
    {
      bounds[0] = copy.bounds[0];
      bounds[1] = copy.bounds[1];
    }
#ifndef SWIG
    BBox& operator=(const BBox& copy)
    {
      bounds[0] = copy.bounds[0];
      bounds[1] = copy.bounds[1];
      return *this;
    }
#endif

    bool operator==(const BBox& bbox) const {
      return bounds[0] == bbox[0] && bounds[1] == bbox[1];
    }

    // This resets min and max to an uninitialized state that will
    // accept new bounds [MAX, -MAX].
    inline void reset() {
      Real max_val = std::numeric_limits<Real>::max();
      bounds[0] = Vector( max_val,  max_val,  max_val);
      bounds[1] = Vector(-max_val, -max_val, -max_val);
    }

    inline void reset( const Vector& min_, const Vector& max_ ) {
      bounds[0] = min_;
      bounds[1] = max_;
    }

    // Test whether the box is in the "uninitialized state"
    bool isDefault() const {
      Real max_val = std::numeric_limits<Real>::max();
      return ( bounds[0] == Vector( max_val,  max_val,  max_val) &&
               bounds[1] == Vector(-max_val, -max_val, -max_val) );
    }

    Vector diagonal() const {
      return bounds[1] - bounds[0];
    }
    Vector center() const {
      return Interpolate(bounds[0], bounds[1], (Real)0.5);
    }

    inline void extendByPoint(const Vector& p) {
      bounds[0] = Min(bounds[0], p);
      bounds[1] = Max(bounds[1], p);
    }

    void extendBySphere(const Vector& p, Real radius) {
      bounds[0] = Min(bounds[0], p-Vector(radius, radius, radius));
      bounds[1] = Max(bounds[1], p+Vector(radius, radius, radius));
    }
    // n really needs to be normalize, or you could get NaNs when
    // taking the square root of a negative number.
    void extendByDisc(const Vector& p, const Vector& n, Real radius) {
      Vector v(Sqrt(1-n.x()*n.x())*radius,
               Sqrt(1-n.y()*n.y())*radius,
               Sqrt(1-n.z()*n.z())*radius);
      bounds[0] = Min(bounds[0], p-v);
      bounds[1] = Max(bounds[1], p+v);
    }
    inline void extendByBox(const BBox& b) {
      bounds[0] = Min(bounds[0], b.bounds[0]);
      bounds[1] = Max(bounds[1], b.bounds[1]);
    }

    void intersection(const BBox& b) {
      bounds[0] = Max(bounds[0], b.bounds[0]);
      bounds[1] = Min(bounds[1], b.bounds[1]);
    }

    inline const Vector& getMin() const {
      return bounds[0];
    }
    inline const Vector& getMax() const {
      return bounds[1];
    }

    Vector getCorner(int i) const {
      return Vector(bounds[(i&4)>>2].x(),
                    bounds[(i&2)>>1].y(),
                    bounds[i&1].z());
    }

    double computeVolume() const
    {
      Vector d = bounds[1] - bounds[0];
      return ( d.x() * d.y() * d.z() );
    }

    inline double computeArea() const
    {
      Vector d = bounds[1] - bounds[0];
      return 2 * (d.x()*d.y() + d.y()*d.z() + d.x()*d.z());
    }

    int longestAxis() const
    {
      Vector d = bounds[1] - bounds[0];
      if ( d.x() > d.y() )
        return d.x() > d.z() ? 0 : 2;
      else
        return d.y() > d.z() ? 1 : 2;
    }

    bool contains(const Vector &p) const {
      return
        (p[0] >= bounds[0][0] && p[0] <= bounds[1][0]) &&
        (p[1] >= bounds[0][1] && p[1] <= bounds[1][1]) &&
        (p[2] >= bounds[0][2] && p[2] <= bounds[1][2]);
    }

#ifndef SWIG
    inline       Vector &operator[] (int i)       { return bounds[i]; }
    inline const Vector &operator[] (int i) const { return bounds[i]; }
#endif

    void readwrite(ArchiveElement*);

  private:
    Vector bounds[2];
  };

  // Outside the class but in Manta namespace
  std::ostream& operator<< (std::ostream& os, const BBox& v);

  MANTA_DECLARE_RTTI_BASECLASS(BBox, ConcreteClass, readwriteMethod);
}

#endif
