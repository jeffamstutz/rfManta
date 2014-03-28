#ifndef Manta_Model_Intersections_Sphere__H
#define Manta_Model_Intersections_Sphere__H

namespace Manta {

    // This is the single ray version.
    inline bool intersectSphere(const Vector &center,//sphere center
                                const Real radius,   //sphere radius
                                const Ray &ray,      // Input Ray.
                                Real &tmin,          // Output min t.
                                Real &tmax           // Output max t.
                                )
    {
      // Rays of constant origin for not normalized directions
      Vector O(ray.origin()-center);
      Real C = Dot(O, O) - radius*radius;
      Real A = Dot(ray.direction(), ray.direction());
      Real B = Dot(O, ray.direction());
      Real disc = B*B-A*C;
      if(disc >= 0){
        Real r = Sqrt(disc);
        tmin = -(r+B)/A;
        tmax = (r-B)/A;
        if (tmax < tmin) {
          Real temp = tmax;
          tmax = tmin;
          tmin = temp;
        }
        return true;
      }
      return false;
    }

  // This is the single ray version for normalized directions.
  inline bool intersectSphere_NormalizedRay (const Vector &center,//sphere center
                                             const Real radius,   //sphere radius
                                             const Ray &ray,      // Input Ray.
                                             Real &tmin,          // Output min t.
                                             Real &tmax           // Output max t.
                                             )
  {
    // Rays of constant origin for not normalized directions
    Vector O(ray.origin()-center);
    //const Real A = 1;//Dot(D, D);
    Real B = Dot(O, ray.direction());

    const Real t = -B; // start ray between sphere hit points
    O = O+t*ray.direction();
    B = Dot(O, ray.direction());
    const Real C = Dot(O, O) - radius*radius;

    const Real disc = B*B-C;

    if(disc >= 0) {
      Real r = Sqrt(disc);
      tmin = t - (r+B);
      tmax = t + (r-B);
      if (tmax < tmin) {
        Real temp = tmax;
        tmax = tmin;
        tmin = temp;
      }
      return true;
    }
    return false;
  }

}

#endif
