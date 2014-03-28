

#ifndef Manta_Model_Intersections_Plane__H
#define Manta_Model_Intersections_Plane__H

namespace Manta {

    template< typename Scalar >
    inline bool intersectPlane( const Vector& point,
                                const Vector& normal,
                                Scalar& t,
                                const Ray& ray ) {

      Scalar dn = Dot( ray.direction(), normal );
      if (dn != 0) {
        Scalar ao = Dot( (point-ray.origin()), normal );
        t = ao/dn;
        return true;
      }
      return true;
    }


}; // end namespace Manta

#endif
