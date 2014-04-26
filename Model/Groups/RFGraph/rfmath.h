/* *****************************************************************************
 *
 * Copyright (c) 2007-2013 Alexis Naveros.
 * Portions developed under contract to the SURVICE Engineering Company.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 *
 * *****************************************************************************
 */

#ifndef RFMATH_H
#define RFMATH_H


#define rfMathVectorZero(x) ({(x)[0]=0.0;(x)[1]=0.0;(x)[2]=0.0;})
#define rfMathVectorMagnitude(x) (sqrt((x)[0]*(x)[0]+(x)[1]*(x)[1]+(x)[2]*(x)[2]))
#define rfMathVectorCopy(x,y) ({(x)[0]=(y)[0];(x)[1]=(y)[1];(x)[2]=(y)[2];})
#define rfMathVectorAdd(x,y) (x)[0]+=(y)[0];(x)[1]+=(y)[1];(x)[2]+=(y)[2]
#define rfMathVectorAddStore(x,y,z) (x)[0]=(y)[0]+(z)[0];(x)[1]=(y)[1]+(z)[1];(x)[2]=(y)[2]+(z)[2]
#define rfMathVectorAddMulScalar(x,y,z) (x)[0]+=(y)[0]*(z);(x)[1]+=(y)[1]*(z);(x)[2]+=(y)[2]*(z)
#define rfMathVectorAddMul(x,y,z) (x)[0]+=(y)[0]*(z)[0];(x)[1]+=(y)[1]*(z)[1];(x)[2]+=(y)[2]*(z)[2]
#define rfMathVectorSub(x,y) (x)[0]-=(y)[0];(x)[1]-=(y)[1];(x)[2]-=(y)[2]
#define rfMathVectorSubStore(x,y,z) (x)[0]=(y)[0]-(z)[0];(x)[1]=(y)[1]-(z)[1];(x)[2]=(y)[2]-(z)[2]
#define rfMathVectorSubMulScalar(x,y,z) (x)[0]-=(y)[0]*(z);(x)[1]-=(y)[1]*(z);(x)[2]-=(y)[2]*(z)
#define rfMathVectorSubMul(x,y,z) (x)[0]-=(y)[0]*(z)[0];(x)[1]-=(y)[1]*(z)[1];(x)[2]-=(y)[2]*(z)[2]
#define rfMathVectorMul(x,y) (x)[0]*=(y)[0];(x)[1]*=(y)[1];(x)[2]*=(y)[2]
#define rfMathVectorMulStore(x,y,z) (x)[0]=(y)[0]*(z)[0];(x)[1]=(y)[1]*(z)[1];(x)[2]=(y)[2]*(z)[2]
#define rfMathVectorMulScalar(x,y) (x)[0]*=(y);(x)[1]*=(y);(x)[2]*=(y)
#define rfMathVectorMulScalarStore(x,y,z) (x)[0]=(y)[0]*(z);(x)[1]=(y)[1]*(z);(x)[2]=(y)[2]*(z)
#define rfMathVectorDiv(x,y) (x)[0]/=(y)[0];(x)[1]/=(y)[1];(x)[2]/=(y)[2]
#define rfMathVectorDivStore(x,y,z) (x)[0]=(y)[0]/(z)[0];(x)[1]=(y)[1]/(z)[1];(x)[2]=(y)[2]/(z)[2]
#define rfMathVectorDotProduct(x,y) ((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define rfMathPlanePoint(x,y) ((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2]+(x)[3])
#define rfMathVectorNormalize(x) ({typeof(*(x)) _f;_f=1.0/sqrt((x)[0]*(x)[0]+(x)[1]*(x)[1]+(x)[2]*(x)[2]);(x)[0]*=_f;(x)[1]*=_f;(x)[2]*=_f;})

#define rfMathVectorCrossProduct(x,y,z) ({(x)[0]=((y)[1]*(z)[2])-((y)[2]*(z)[1]);(x)[1]=((y)[2]*(z)[0])-((y)[0]*(z)[2]);(x)[2]=((y)[0]*(z)[1])-((y)[1]*(z)[0]);})
#define rfMathVectorAddCrossProduct(x,y,z) ({(x)[0]+=((y)[1]*(z)[2])-((y)[2]*(z)[1]);(x)[1]+=((y)[2]*(z)[0])-((y)[0]*(z)[2]);(x)[2]+=((y)[0]*(z)[1])-((y)[1]*(z)[0]);})
#define rfMathVectorSubCrossProduct(x,y,z) ({(x)[0]-=((y)[1]*(z)[2])-((y)[2]*(z)[1]);(x)[1]-=((y)[2]*(z)[0])-((y)[0]*(z)[2]);(x)[2]-=((y)[0]*(z)[1])-((y)[1]*(z)[0]);})

#define rfMathMatrixMulVector(d,p,m) ({ \
(d)[0]=(p)[0]*(m)[0*4+0]+(p)[1]*(m)[1*4+0]+(p)[2]*(m)[2*4+0]; \
(d)[1]=(p)[0]*(m)[0*4+1]+(p)[1]*(m)[1*4+1]+(p)[2]*(m)[2*4+1]; \
(d)[2]=(p)[0]*(m)[0*4+2]+(p)[1]*(m)[1*4+2]+(p)[2]*(m)[2*4+2]; })

#define rfMathMatrixTransMulVector(d,p,m) ({ \
(d)[0]=(p)[0]*(m)[0*4+0]+(p)[1]*(m)[0*4+1]+(p)[2]*(m)[0*4+2]; \
(d)[1]=(p)[0]*(m)[1*4+0]+(p)[1]*(m)[1*4+1]+(p)[2]*(m)[1*4+2]; \
(d)[2]=(p)[0]*(m)[2*4+0]+(p)[1]*(m)[2*4+1]+(p)[2]*(m)[2*4+2]; })

#define rfMathMatrixMulVectorSelf(p,m) ({ \
typeof(*(p)) _n[3] = { p[0], p[1], p[2] }; \
(p)[0]=(_n)[0]*(m)[0*4+0]+(_n)[1]*(m)[1*4+0]+(_n)[2]*(m)[2*4+0]; \
(p)[1]=(_n)[0]*(m)[0*4+1]+(_n)[1]*(m)[1*4+1]+(_n)[2]*(m)[2*4+1]; \
(p)[2]=(_n)[0]*(m)[0*4+2]+(_n)[1]*(m)[1*4+2]+(_n)[2]*(m)[2*4+2]; })

#define rfMathMatrixMulPoint(d,p,m) ({ \
(d)[0]=(p)[0]*(m)[0*4+0]+(p)[1]*(m)[1*4+0]+(p)[2]*(m)[2*4+0]+(m)[3*4+0]; \
(d)[1]=(p)[0]*(m)[0*4+1]+(p)[1]*(m)[1*4+1]+(p)[2]*(m)[2*4+1]+(m)[3*4+1]; \
(d)[2]=(p)[0]*(m)[0*4+2]+(p)[1]*(m)[1*4+2]+(p)[2]*(m)[2*4+2]+(m)[3*4+2]; })

#define rfMathMatrixMulPointSelf(p,m) ({ \
typeof(*(p)) _n[3] = { p[0], p[1], p[2] }; \
(p)[0]=(_n)[0]*(m)[0*4+0]+(_n)[1]*(m)[1*4+0]+(_n)[2]*(m)[2*4+0]+(m)[3*4+0]; \
(p)[1]=(_n)[0]*(m)[0*4+1]+(_n)[1]*(m)[1*4+1]+(_n)[2]*(m)[2*4+1]+(m)[3*4+1]; \
(p)[2]=(_n)[0]*(m)[0*4+2]+(_n)[1]*(m)[1*4+2]+(_n)[2]*(m)[2*4+2]+(m)[3*4+2]; })


static inline int rfMathVectorNormalizeCheckf( float *v )
{
  float f;
  f = sqrt( rfMathVectorDotProduct( v, v ) );
  if( !( f ) )
    return 0;
  f = 1.0 / f;
  rfMathVectorMulScalar( v, f );
  return 1;
}

static inline int rfMathVectorNormalizeCheckd( double *v )
{
  double f;
  f = sqrt( rfMathVectorDotProduct( v, v ) );
  if( !( f ) )
    return 0;
  f = 1.0 / f;
  rfMathVectorMulScalar( v, f );
  return 1;
}


void rfMathMatrixIdentityf( float *m );
void rfMathMatrixIdentityd( double *m );

void rfMathMatrixInvertf( float *mdst, float *m );
void rfMathMatrixInvertd( double *mdst, double *m );

void rfMathPlaneIntersect3f( float *pt, float *p0, float *p1, float *p2 );
void rfMathPlaneIntersect3d( double *pt, double *p0, double *p1, double *p2 );


#endif /* RFMATH_H */

