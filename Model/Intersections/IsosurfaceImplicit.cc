
#include <Model/Intersections/IsosurfaceImplicit.h>
#include <Core/Math/CubicSolver.h>
#include <iostream>

using namespace Manta;
using namespace std;

#define NEUBAUER_ITERATIONS 3

//From Steven Parker's 1997 RTRT isosurface intersection
bool IsosurfaceImplicit::single_intersect_schwarze(const Vector& orig, const Vector& dir, 
                      const Vector& pmin, const Vector& pmax, float rho[2][2][2], 
                      float iso, float tmin, float tmax, float& t)
{
    double c[4];
    double s[3];
    double ua[2];
    double ub[2];
    double va[2];
    double vb[2];
    double wa[2];
    double wb[2];
    int i,j,k;

    ua[1] = (orig.x() - pmin.x()) / (pmax.x() - pmin.x());
    ua[0] = 1 - ua[1];
    ub[1] = dir.x() / (pmax.x() - pmin.x());
    ub[0] = - ub[1];

    va[1] = (orig.y() - pmin.y()) / (pmax.y() - pmin.y());
    va[0] = 1 - va[1];
    vb[1] = dir.y() / (pmax.y() - pmin.y());
    vb[0] = - vb[1];

    wa[1] = (orig.z() - pmin.z()) / (pmax.z() - pmin.z());
    wa[0] = 1 - wa[1];
    wb[1] = dir.z()  / (pmax.z() - pmin.z());
    wb[0] = - wb[1];


    c[3] = c[2] = c[1] = c[0] = 0;
    for (i=0; i < 2; i++)
    for (j=0; j < 2; j++)
        for (k=0; k < 2; k++) {

            // cubic term
            c[3] += ub[i]*vb[j]*wb[k] * rho[i][j][k]; 

            // square term
            c[2] += (ua[i]*vb[j]*wb[k] + ub[i]*va[j]*wb[k] + ub[i]*vb[j]*wa[k]) * rho[i][j][k]; 

            // linear term
            c[1] += (ub[i]*va[j]*wa[k] + ua[i]*vb[j]*wa[k] + ua[i]*va[j]*wb[k]) * rho[i][j][k]; 

            // constant term
            c[0] +=  ua[i]*va[j]*wa[k] * rho[i][j][k]; 
        }
    c[0] -= iso;

    //gives us the roots of the cubic. s are the roots, n are the number of roots.
    int n = SolveCubic( c,  s);


    t = tmax;
    for (i = 0; i < n; i++) 
    {
        if (s[i] >= tmin && s[i] < t) 
            t = s[i];
    }
    
    return (t < tmax);

}


bool IsosurfaceImplicit::single_intersect_neubauer(const Vector& orig, const Vector& dir, 
                      const Vector& pmin, const Vector& pmax, float rho[2][2][2], 
                      float iso, float tenter, float texit, float& hit_t)
{
        const Vector p0 = (orig + dir*tenter) - pmin;
        const Vector p1 = (orig + dir*texit) - pmin;

        CubicPoly poly;
        poly.generate(p0, p1, rho, iso);
        
        float t0 = 0.f;
        float t1 = 1.f;
        float D0 = poly.d;
        float D1 = poly.a+poly.b+poly.c+poly.d;
        
        if (D0 * D1 >= 0)
            return false;

        for (int i=0;i<NEUBAUER_ITERATIONS;i++)
        {
            //compute linear interpolation
            const float denom = 1./(D0 - D1);
            float t = t0 + D0*denom*(t1-t0);

            //re-evaluate
            float D = poly.eval(t);
    
            if (D0*D < 0)
            {
                t1 = t;
                D1 = D;
            }
            else
            {
                t0 = t;
                D0 = D;
            }
        }
        
        const float denom = 1./(D0-D1);
        const float t = t0 + D0*denom*(t1-t0);
        hit_t = tenter + t*(texit-tenter);
        
        return (hit_t < texit);
}


//From Steven Parker's 1997 RTRT isosurface intersection
void IsosurfaceImplicit::single_normal(Vector& outNormal, const Vector& pmin, const Vector& pmax, const Vector& p, float rho[2][2][2])
{
    float x = p.x();
    float y = p.y();
    float z = p.z();
    
    float x_0 = pmin.x();
    float y_0 = pmin.y();
    float z_0 = pmin.z();
    
    float x_1 = pmax.x();
    float y_1 = pmax.y();
    float z_1 = pmax.z();
    
    outNormal.data[0]  =   - (y_1-y)*(z_1-z)*rho[0][0][0] 
            + (y_1-y)*(z_1-z)*rho[1][0][0]
            - (y-y_0)*(z_1-z)*rho[0][1][0]
            - (y_1-y)*(z-z_0)*rho[0][0][1]
            + (y-y_0)*(z_1-z)*rho[1][1][0] 
            + (y_1-y)*(z-z_0)*rho[1][0][1]
            - (y-y_0)*(z-z_0)*rho[0][1][1]
            + (y-y_0)*(z-z_0)*rho[1][1][1];
    
    outNormal.data[1]  =   - (x_1-x)*(z_1-z)*rho[0][0][0]
            - (x-x_0)*(z_1-z)*rho[1][0][0]
            + (x_1-x)*(z_1-z)*rho[0][1][0]
            - (x_1-x)*(z-z_0)*rho[0][0][1]
            + (x-x_0)*(z_1-z)*rho[1][1][0]
            - (x-x_0)*(z-z_0)*rho[1][0][1]
            + (x_1-x)*(z-z_0)*rho[0][1][1]
            + (x-x_0)*(z-z_0)*rho[1][1][1];
    
    outNormal.data[2] =    - (x_1-x)*(y_1-y)*rho[0][0][0]
            - (x-x_0)*(y_1-y)*rho[1][0][0]
            - (x_1-x)*(y-y_0)*rho[0][1][0]
            + (x_1-x)*(y_1-y)*rho[0][0][1]
            - (x-x_0)*(y-y_0)*rho[1][1][0]
            + (x-x_0)*(y_1-y)*rho[1][0][1]
            + (x_1-x)*(y-y_0)*rho[0][1][1]
            + (x-x_0)*(y-y_0)*rho[1][1][1];
}


#ifdef MANTA_SSE
//SSE packet implementation
//Based on Marmitt et al. 04, Wald 05 SSE intersections (OpenRT)
//  as well as Knoll DynRT-vol implementation
void IsosurfaceImplicit::sse_intersect(SSERayPacket& srp, 
            char first, char last, const Vector& pmin, const Vector& pmax, float rho[2][2][2], 
            float isovalue, sse_t tenter[], sse_t texit[], sse_t hitmask[],
            const Manta::Primitive* prim, const Manta::Material* matl)
{
    //cerr << "sse_intersect: first=" << (int)first << ",last=" << (int)last << endl;
    for(int smd=first; smd<=last; smd++)
    {
        sse_t sse_thisvoxelmask = hitmask[smd];
        int int_thisvoxelmask = _mm_movemask_ps(sse_thisvoxelmask);
    
        if (int_thisvoxelmask==0)
        {
            continue;
        }
    
        //compute p0, p1
        sse_t p0[3];
        sse_t p1[3];
        
        for(int axis=0; axis<3; axis++)
        {			
            p0[axis] = sub4(add4(srp.orig[axis][smd], mul4(srp.dir[axis][smd], tenter[smd])), set4(pmin[axis]));
            p1[axis] = sub4(add4(srp.orig[axis][smd], mul4(srp.dir[axis][smd], texit[smd])), set4(pmin[axis]));
        }
        
        CubicPoly4 poly;
        poly.generate(p0, p1, rho, isovalue);

        sse_t t0 = zero4();                                        
        sse_t t1 = _mm_one;
        sse_t D0 = poly.d;
        sse_t D1 = add4(add4(poly.a,poly.b), add4(poly.c,poly.d));
        
        //AARONBAD - we'd want something like this to avoid extra work
        //sse_t sse_thisvoxelmask = and4(hitmask[smd], cmp4_lt(tenter[smd], srp.minT[smd]));

        //find which rays have differing signs for D0, D1. Only retain the ones that have same signs?
        sse_t differingSigns = cmp4_lt(mul4(D0,D1), zero4());
        sse_thisvoxelmask = and4(sse_thisvoxelmask, differingSigns);
        int_thisvoxelmask = _mm_movemask_ps(sse_thisvoxelmask);
        
        if (int_thisvoxelmask == 0)	//if none of them hit, don't bother iterating any more
            continue;

        for (int i=0;i<NEUBAUER_ITERATIONS;i++)
        {
            //compute linear interpolation
            const sse_t denom = accurateReciprocal(sub4(D0,D1));
            sse_t t = add4(t0,mul4(mul4(D0,denom), sub4(t1,t0)));
    
            //re-evaluate
            sse_t D = poly.eval(t);
    
            //conditionally store
            const sse_t frontHalf = _mm_cmplt_ps(mul4(D0,D), zero4());
            t1 = or4(_mm_and_ps(frontHalf,t), _mm_andnot_ps(frontHalf,t1));
            t0 = or4(_mm_and_ps(frontHalf,t0), _mm_andnot_ps(frontHalf,t));
            D1 = or4(_mm_and_ps(frontHalf,D), _mm_andnot_ps(frontHalf,D1));
            D0 = or4(_mm_and_ps(frontHalf,D0), _mm_andnot_ps(frontHalf,D));
        }

        //compute hit distance
        const sse_t denom = accurateReciprocal(sub4(D0,D1));
        sse_t t = add4(t0, mul4(mul4(D0,denom), sub4(t1,t0)));
        sse_t hit_t = add4(tenter[smd], mul4(t, sub4(texit[smd], tenter[smd])));
        
        //the mask should only include rays that are active
        sse_thisvoxelmask = and4(sse_thisvoxelmask, cmp4_ge(hit_t, zero4()));
        sse_thisvoxelmask = and4(sse_thisvoxelmask, srp.activeMask[smd]);
        sse_thisvoxelmask = and4(sse_thisvoxelmask, cmp4_lt(hit_t,srp.minT[smd]));
        srp.minT[smd] = mask4(sse_thisvoxelmask, hit_t, srp.minT[smd]);
        int_thisvoxelmask = _mm_movemask_ps(sse_thisvoxelmask);
        if (int_thisvoxelmask)
        {
            sse_t normal[3];
            sse_normal(srp, smd, normal, pmin, pmax, rho);
            for(int axis=0; axis<3; axis++)
                srp.normal[axis][smd] = mask4(sse_thisvoxelmask, normal[axis], srp.normal[axis][smd]);
                

            int sse_ray = smd << 2;
            for(int ray=0; ray<4; ray++)
            {
                if (int_thisvoxelmask & (1<<ray))
                {
                    int realray=sse_ray+ray;
                    srp.rp->data->hitMatl[realray] = matl;
                    srp.rp->data->hitPrim[realray] = prim;
                }
            }
            
            //int nonzeros = count_nonzeros(sse_thisvoxelmask);
            //cerr << "nonzeros=" << nonzeros << endl;
            
            //this line of code should implement early termination, but this is broken.
            srp.activeRays -= count_nonzeros(sse_thisvoxelmask);
            
            //active rays in this smd are ones that were active before, and did NOT intersect.
            srp.activeMask[smd] = andnot4(sse_thisvoxelmask, srp.activeMask[smd]);
        }
    }
}
            
void IsosurfaceImplicit::sse_normal(SSERayPacket& srp, int smd, 
            sse_t normal[], const Vector& pmin, const Vector& pmax,
            const float rho[2][2][2])
{
    sse_t phit[3];
    for(int axis=0; axis<3; axis++)
        phit[axis] = add4(srp.orig[axis][smd], mul4(srp.dir[axis][smd], srp.minT[smd]));

    int axis, U, V;	
    
    //axis=0
    axis=0;
    U=1;
    V=2;
    sse_t max_m_hit_U = sub4(set4(pmax[U]), phit[U]);
    sse_t max_m_hit_V = sub4(set4(pmax[V]), phit[V]);
    sse_t min_m_hit_U = sub4(set4(pmin[U]), phit[U]);
    sse_t min_m_hit_V = sub4(set4(pmin[V]), phit[V]);
    normal[axis] = mul4(mul4(max_m_hit_U, max_m_hit_V), set4(rho[1][0][0] - rho[0][0][0]));
    normal[axis] = add4(normal[axis], mul4(mul4(min_m_hit_U, max_m_hit_V), set4(rho[0][1][0] - rho[1][1][0])));
    normal[axis] = add4(normal[axis], mul4(mul4(max_m_hit_U, min_m_hit_V), set4(rho[0][0][1] - rho[1][0][1])));
    normal[axis] = add4(normal[axis], mul4(mul4(min_m_hit_U, min_m_hit_V), set4(rho[1][1][1] - rho[0][1][1])));
    
    //axis=1
    axis=1;
    U=0;
    V=2;
    max_m_hit_U = sub4(set4(pmax[U]), phit[U]);
    max_m_hit_V = sub4(set4(pmax[V]), phit[V]);
    min_m_hit_U = sub4(set4(pmin[U]), phit[U]);
    min_m_hit_V = sub4(set4(pmin[V]), phit[V]);
    normal[axis] = mul4(mul4(max_m_hit_U, max_m_hit_V), set4(rho[0][1][0] - rho[0][0][0]));
    normal[axis] = add4(normal[axis], mul4(mul4(min_m_hit_U, max_m_hit_V), set4(rho[1][0][0] - rho[1][1][0])));
    normal[axis] = add4(normal[axis], mul4(mul4(max_m_hit_U, min_m_hit_V), set4(rho[0][0][1] - rho[0][1][1])));
    normal[axis] = add4(normal[axis], mul4(mul4(min_m_hit_U, min_m_hit_V), set4(rho[1][1][1] - rho[1][0][1])));
    
    //axis=2
    axis=2;
    U=0;
    V=1;
    max_m_hit_U = sub4(set4(pmax[U]), phit[U]);
    max_m_hit_V = sub4(set4(pmax[V]), phit[V]);
    min_m_hit_U = sub4(set4(pmin[U]), phit[U]);
    min_m_hit_V = sub4(set4(pmin[V]), phit[V]);
    normal[axis] = mul4(mul4(max_m_hit_U, max_m_hit_V), set4(rho[0][0][1] - rho[0][0][0]));
    normal[axis] = add4(normal[axis], mul4(mul4(min_m_hit_U, max_m_hit_V), set4(rho[1][0][0] - rho[1][0][1])));
    normal[axis] = add4(normal[axis], mul4(mul4(max_m_hit_U, min_m_hit_V), set4(rho[0][1][0] - rho[0][1][1])));
    normal[axis] = add4(normal[axis], mul4(mul4(min_m_hit_U, min_m_hit_V), set4(rho[1][1][1] - rho[1][1][0])));

}

#endif 

