
// AxisAlignedBox Intersection algorithm based on the Amy Williams intersection
// test.
// Abe Stephens abe@sgi.com
// May 2005

#ifndef Manta_Model_Intersections_AxisAlignedBox__H
#define Manta_Model_Intersections_AxisAlignedBox__H

#include <limits>

namespace Manta {

    // This intersection method uses the Amy Williams ray/box intersection
    // technique.
    // See http://www.cs.utah.edu/~awilliam/box/box.pdf
    // The BOX type must define a [] operator for accessing the min/max
    // points in the bounds.
    // NOTE: The inverse direction and ray sign mask come from the raypacket
    
    // This is the single ray version.
    template< typename BOX, typename Scalar >
    inline bool intersectAaBox(const BOX &bounds,     // Object implementing []
                              Scalar &tmin,          // Output min t.
                              Scalar &tmax,          // Output max t.
                              const Ray &r,          // Input Ray.
                              const VectorT<int,3>&s,// Input Ray mask.
                              const Vector &d_inv,   // Input 1.0 / ray direction
                              Scalar t0 = 0,         // Input bounding interval for t
                              Scalar t1 = std::numeric_limits<Scalar>::max()
                               )
    {
      tmin  = (bounds[s[0]  ][0] - r.origin()[0]) * d_inv[0];
      tmax  = (bounds[1-s[0]][0] - r.origin()[0]) * d_inv[0];
      Scalar tymin = (bounds[s[1]  ][1] - r.origin()[1]) * d_inv[1];
      Scalar tymax = (bounds[1-s[1]][1] - r.origin()[1]) * d_inv[1];

      // If boxes are allowed to be inside out.
      // if (tmin > tmax) 
      // 	return false;
      
      if ( tmin > tymax || tymin > tmax )
        return false;
      if ( tymin > tmin )
        tmin = tymin;
      if ( tymax < tmax )
        tmax = tymax;

      Scalar tzmin = (bounds[s[2]  ][2] - r.origin()[2]) * d_inv[2];
      Scalar tzmax = (bounds[1-s[2]][2] - r.origin()[2]) * d_inv[2];
			
      if ( tmin > tzmax || tzmin > tmax )
        return false;
      if ( tzmin > tmin )
        tmin = tzmin;
      if ( tzmax < tmax )
        tmax = tzmax;
			
      return tmin < t1 && tmax > t0;
    } 
}

#endif
