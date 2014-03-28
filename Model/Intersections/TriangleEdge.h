
/*
 For more information, please see: http://software.sci.utah.edu
 
 The MIT License
 
 Copyright (c) 2005
 Silicon Graphics Inc. Mountain View California.
 
 License for the specific language governing rights and limitations under
 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.
 */

#ifndef Manta_Model_Intersections_TriangleEdge
#define Manta_Model_Intersections_TriangleEdge

#include <MantaTypes.h>
#include <Core/Geometry/Ray.h>
#include <Core/Geometry/Vector.h>
#include <Interface/RayPacket.h>

namespace Manta {


    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////
    // Triangle Intersection routines.
    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////


    ///////////////////////////////////////////////////////////////////////////////
    // C-style ray triangle edge intersection test. Slightly faster than C++
    // version included below.
    
#define EPSILON 1e-6f

#define CROSS(dest,v1,v2) \
(dest)[0]=(v1)[1]*(v2)[2]-(v1)[2]*(v2)[1]; \
(dest)[1]=(v1)[2]*(v2)[0]-(v1)[0]*(v2)[2]; \
(dest)[2]=(v1)[0]*(v2)[1]-(v1)[1]*(v2)[0];
#define DOT(v1,v2) ((v1)[0]*(v2)[0]+(v1)[1]*(v2)[1]+(v1)[2]*(v2)[2])
#define SUB(dest,v1,v2) \
(dest)[0]=(v1)[0]-(v2)[0]; \
(dest)[1]=(v1)[1]-(v2)[1]; \
(dest)[2]=(v1)[2]-(v2)[2]; 

  inline int intersect_triangle3_edge(const float orig[3], const float dir[3],
                                      const float vert0[3],
                                      float *t, float *u, float *v,
                                      const float *edge1, const float *edge2 )
  {
      // float edge1[3], edge2[3];
      float tvec[3], pvec[3], qvec[3];
      float det,inv_det;
	
      /* find vectors for two edges sharing vert0 */
      // SUB(edge1, vert1, vert0);
      // SUB(edge2, vert2, vert0);
	
      /* begin calculating determinant - also used to calculate U parameter */
      CROSS(pvec, dir, edge2);
	
      /* if determinant is near zero, ray lies in plane of triangle */
      det = DOT(edge1, pvec);
	
      /* calculate distance from vert0 to ray origin */
      SUB(tvec, orig, vert0);
	
      CROSS(qvec, tvec, edge1);
	
      /* calculate U parameter */
      float uu = DOT(tvec, pvec);
      float vv;
	
      if (det > EPSILON)
        {
          if (uu < 0.0f || uu > det)
            return 0;
		
          /* calculate V parameter and test bounds */
          vv = DOT(dir, qvec);
          if (vv < 0.0f || uu + vv > det)
            return 0;
		
        }
      else if(det < -EPSILON)
        {
          if (uu > 0.0f || uu < det)
            return 0;
		
          /* calculate V parameter and test bounds */
          vv = DOT(dir, qvec) ;
          if (vv > 0.0f || uu + vv < det)
            return 0;
        }
      else return 0;  /* ray is parallell to the plane of the triangle */
	
      inv_det = 1.0f/ det;

      *t = DOT(edge2, qvec) * inv_det;
      (*u) = uu*inv_det;
      (*v) = vv*inv_det;
	
      return 1;
    }
    
    
    
		inline bool intersectTriangleEdge(Real& t, Real& u, Real& v,
                                      const Ray& ray, 
                                      const Vector& edge1,
                                      const Vector& edge2,
                                      const Vector& p0 )
		{
			
			Vector tvec, pvec, qvec;
			
			/* begin calculating determinant - also used to calculate U parameter */
			pvec = Cross( ray.direction(), edge2 );
			
			// CROSS(pvec, dir, edge2);
			
			/* if determinant is near zero, ray lies in plane of triangle */
			// det = DOT(edge1, pvec);
			Real det = Dot( edge1, pvec );
			
			/* calculate distance from vert0 to ray origin */
			// SUB(tvec, orig, vert0);
			tvec = ray.origin() - p0;
			
			qvec = Cross( tvec, edge1 );
			// CROSS(qvec, tvec, edge1);
			
			/* calculate U parameter */
			// float uu = DOT(tvec, pvec);
			// float vv;
			Real uu = Dot( tvec, pvec );
			Real vv;
			
			if (det > (Real)1e-5) {
				if (uu < 0 || uu > det)
					return false;
				
				/* calculate V parameter and test bounds */
				vv = Dot( ray.direction(), qvec );
				
				if (vv < 0 || uu + vv > det)
					return false;
				
			}
			else if(det < (Real)-1e-5) {
				if (uu > 0 || uu < det)
					return false;
				
				/* calculate V parameter and test bounds */
				vv = Dot(ray.direction(), qvec) ;
				if (vv > 0 || uu + vv < det)
					return false;
			}
			else {
				return false;  /* ray is parallell to the plane of the triangle */
			}
			
			Real inv_det = 1 / det;
			
			t = (Dot( edge2, qvec ) * inv_det);
			u = (uu * inv_det);
			v = (vv * inv_det);
			
			return true;
		}

};

#endif


