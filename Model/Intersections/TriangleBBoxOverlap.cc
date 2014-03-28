#include <Model/Intersections/TriangleBBoxOverlap.h>

/* This is a direct copy of the Moller Triangle-Box Overlap Test.
   It could probably use some optimizing...
*/

#define CROSS(dest,v1,v2) \
          dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
          dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
          dest[2]=v1[0]*v2[1]-v1[1]*v2[0];

#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])

#define SUB(dest,v1,v2) \
          dest[0]=v1[0]-v2[0]; \
          dest[1]=v1[1]-v2[1]; \
          dest[2]=v1[2]-v2[2];

#define FINDMINMAX(x0,x1,x2,min,max) \
  min = max = x0;   \
  if(x1<min) min=x1;\
  if(x1>max) max=x1;\
  if(x2<min) min=x2;\
  if(x2>max) max=x2;

int planeBoxOverlap(float normal[3], float vert[3], float maxbox[3])
{
  int q;
  float vmin[3],vmax[3],v;
  for(q=0;q<=2;q++)
  {
    v=vert[q];                  // -NJMP-
    if(normal[q]>0.0f)
    {
      vmin[q]=-maxbox[q] - v;   // -NJMP-
      vmax[q]= maxbox[q] - v;   // -NJMP-
    }
    else
    {
      vmin[q]= maxbox[q] - v;   // -NJMP-
      vmax[q]=-maxbox[q] - v;   // -NJMP-
    }
  }
  if(DOT(normal,vmin)>0.0f) return false;   // -NJMP-
  if(DOT(normal,vmax)>=0.0f) return true;   // -NJMP-

  return false;
}

/*======================== X-tests ========================*/
#define AXISTEST_X01(a, b, fa, fb)                     \
  p0 = a*v0[1] - b*v0[2];                              \
  p2 = a*v2[1] - b*v2[2];                              \
  if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;}   \
  rad = fa * boxhalfsize[1] + fb * boxhalfsize[2];     \
  if(min>rad || max<-rad) return false;

#define AXISTEST_X2(a, b, fa, fb)                      \
  p0 = a*v0[1] - b*v0[2];                              \
  p1 = a*v1[1] - b*v1[2];                              \
  if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;}   \
  rad = fa * boxhalfsize[1] + fb * boxhalfsize[2];     \
  if(min>rad || max<-rad) return false;

/*======================== Y-tests ========================*/
#define AXISTEST_Y02(a, b, fa, fb)                     \
  p0 = -a*v0[0] + b*v0[2];                             \
  p2 = -a*v2[0] + b*v2[2];                             \
  if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;}   \
  rad = fa * boxhalfsize[0] + fb * boxhalfsize[2];     \
  if(min>rad || max<-rad) return false;

#define AXISTEST_Y1(a, b, fa, fb)                      \
  p0 = -a*v0[0] + b*v0[2];                             \
  p1 = -a*v1[0] + b*v1[2];                             \
  if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;}   \
  rad = fa * boxhalfsize[0] + fb * boxhalfsize[2];     \
  if(min>rad || max<-rad) return false;

/*======================== Z-tests ========================*/

#define AXISTEST_Z12(a, b, fa, fb)                     \
  p1 = a*v1[0] - b*v1[1];                              \
  p2 = a*v2[0] - b*v2[1];                              \
  if(p2<p1) {min=p2; max=p1;} else {min=p1; max=p2;}   \
  rad = fa * boxhalfsize[0] + fb * boxhalfsize[1];     \
  if(min>rad || max<-rad) return false;

#define AXISTEST_Z0(a, b, fa, fb)                          \
  p0 = a*v0[0] - b*v0[1];                                  \
  p1 = a*v1[0] - b*v1[1];                                  \
  if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;}       \
  rad = fa * boxhalfsize[0] + fb * boxhalfsize[1];         \
  if(min>rad || max<-rad) return false;

bool Manta::triBoxOverlap(const MeshTriangle *tri, const BBox& bbox)
{
  float triverts[3][3];
  //need to figure out what kind of triangle so we can get its vertices.
  for (size_t v=0; v < 3; ++v) {
    const Vector vertex = tri->getVertex(v);
      for (size_t i=0; i < 3; ++i)
        triverts[v][i] = vertex[i];
  }

  const Vector bbox_center = bbox.center();
  float boxcenter[3] = { bbox_center[0], bbox_center[1], bbox_center[2] };
  const Vector bbox_half_size = bbox.diagonal()*.5;
  float boxhalfsize[3] = { bbox_half_size[0],
                           bbox_half_size[1],
                           bbox_half_size[2] };

  /*    use separating axis theorem to test overlap between triangle and box */
  /*    need to test for overlap in these directions: */
  /*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
  /*       we do not even need to test these) */
  /*    2) normal of the triangle */
  /*    3) crossproduct(edge from tri, {x,y,z}-directin) */
  /*       this gives 3x3=9 more tests */
  float v0[3],v1[3],v2[3];
  //   float axis[3];
  float min,max,p0,p1,p2,rad,fex,fey,fez;       // -NJMP- "d" local variable removed
  float normal[3],e0[3],e1[3],e2[3];

  /* This is the fastest branch on Sun */
  /* move everything so that the boxcenter is in (0,0,0) */
  SUB(v0,triverts[0],boxcenter);
  SUB(v1,triverts[1],boxcenter);
  SUB(v2,triverts[2],boxcenter);

  /* compute triangle edges */
  SUB(e0,v1,v0);      /* tri edge 0 */
  SUB(e1,v2,v1);      /* tri edge 1 */
  SUB(e2,v0,v2);      /* tri edge 2 */

  /* Bullet 3:  */
  /*  test the 9 tests first (this was faster) */
  fex = fabsf(e0[0]);
  fey = fabsf(e0[1]);
  fez = fabsf(e0[2]);
  AXISTEST_X01(e0[2], e0[1], fez, fey);
  AXISTEST_Y02(e0[2], e0[0], fez, fex);
  AXISTEST_Z12(e0[1], e0[0], fey, fex);

  fex = fabsf(e1[0]);
  fey = fabsf(e1[1]);
  fez = fabsf(e1[2]);
  AXISTEST_X01(e1[2], e1[1], fez, fey);
  AXISTEST_Y02(e1[2], e1[0], fez, fex);
  AXISTEST_Z0(e1[1], e1[0], fey, fex);

  fex = fabsf(e2[0]);
  fey = fabsf(e2[1]);
  fez = fabsf(e2[2]);
  AXISTEST_X2(e2[2], e2[1], fez, fey);
  AXISTEST_Y1(e2[2], e2[0], fez, fex);
  AXISTEST_Z12(e2[1], e2[0], fey, fex);

  /* Bullet 1: */
  /*  first test overlap in the {x,y,z}-directions */
  /*  find min, max of the triangle each direction, and test for overlap in */
  /*  that direction -- this is equivalent to testing a minimal AABB around */
  /*  the triangle against the AABB */

   /* test in X-direction */
   FINDMINMAX(v0[0],v1[0],v2[0],min,max);
   if(min>boxhalfsize[0] || max<-boxhalfsize[0]) return false;

   /* test in Y-direction */
   FINDMINMAX(v0[1],v1[1],v2[1],min,max);
   if(min>boxhalfsize[1] || max<-boxhalfsize[1]) return false;

   /* test in Z-direction */
   FINDMINMAX(v0[2],v1[2],v2[2],min,max);
   if(min>boxhalfsize[2] || max<-boxhalfsize[2]) return false;

  /* Bullet 2: */
  /*  test if the box intersects the plane of the triangle */
  /*  compute plane equation of triangle: normal*x+d=0 */
  CROSS(normal,e0,e1);
  // -NJMP- (line removed here)
  if(!planeBoxOverlap(normal,v0,boxhalfsize)) return false; // -NJMP-

  return true;   /* box and triangle overlaps */
}
