#ifndef _MANTA_ISOSURFACEINTERSECTOR_H_
#define _MANTA_ISOSURFACEINTERSECTOR_H_

#include <Interface/RayPacket.h>
#include <Core/Geometry/Vector.h>
#include <Interface/Material.h>
#include <Interface/Primitive.h>

#include <MantaSSE.h>

#ifdef MANTA_SSE
#include <Core/Math/SSEDefs.h>
#include <Interface/SSERayPacket.h>
#endif

namespace Manta
{
    struct IsosurfaceImplicit
    {
    
        static bool single_intersect_schwarze(const Vector& orig, const Vector& dir, 
                      const Vector& pmin, const Vector& pmax, float rho[2][2][2], 
                      float iso, float tmin, float tmax, float& t);
                      
        static bool single_intersect_neubauer(const Vector& orig, const Vector& dir, 
              const Vector& pmin, const Vector& pmax, float rho[2][2][2], 
              float iso, float tenter, float texit, float& hit_t);
                                
        static void single_normal(Vector& outNormal, const Vector& pmin, 
                      const Vector& pmax, const Vector& p, float rho[2][2][2]);
                      
                                            
        //TODO - non-SSE packet intersection              
        
        //From Marmitt, Wald et al. 04 - for Neubauer intersection
        struct CubicPoly
        {
            float a,b,c,d;
            
            void generate(const Vector& p0,
                          const Vector& p1,
                          const float voxel[2][2][2], 
                          const float isovalue)
            {
                const Vector e0 = Vector(1,1,1) - p0;
                const Vector e1 = p0;
                const Vector d1 = p1 - p0;
                float interimROO = d1[1] * d1[2];
                
                const float interimRRR = d1[0] * interimROO;
                const float interimNRR = e1[0] * interimROO;
                const float interimORR = e0[0] * interimROO;
                interimROO = e1[1] * d1[2];
                const float interimRNR = d1[0] * interimROO;
                const float interimNNR = e1[0] * interimROO;
                const float interimONR = e0[0] * interimROO;
                interimROO = d1[1] * e1[2];
                const float interimRRN = d1[0] * interimROO;
                const float interimNRN = e1[0] * interimROO;
                const float interimORN = e0[0] * interimROO;
                interimROO = d1[1] * e0[2];
                const float interimRRO = d1[0] * interimROO;
                const float interimNRO = e1[0] * interimROO;
                const float interimORO = e0[0] * interimROO;
                interimROO = e0[1] * d1[2];
                const float interimROR = d1[0] * interimROO;
                const float interimNOR = e1[0] * interimROO;
                const float interimOOR = e0[0] * interimROO;
                interimROO = d1[0] * e1[1];
                const float interimRNN = interimROO * e1[2];
                const float interimRNO = interimROO * e0[2];
                interimROO = d1[0] * e0[1];
                const float interimRON = interimROO * e1[2];
                interimROO = interimROO * e0[2];
                
                a
                = interimRRR * (  voxel[1][1][1]
                   - voxel[1][1][0]
                   - voxel[1][0][1]
                   + voxel[1][0][0]
                   - voxel[0][1][1]
                   + voxel[0][1][0]
                   + voxel[0][0][1]
                   - voxel[0][0][0]);
                b
                = voxel[1][1][1] * (interimNRR + interimRNR + interimRRN)
                - voxel[1][1][0] * (interimNRR + interimRNR - interimRRO)
                - voxel[1][0][1] * (interimNRR - interimROR + interimRRN)
                + voxel[1][0][0] * (interimNRR - interimROR - interimRRO)
                + voxel[0][1][1] * (interimORR - interimRNR - interimRRN)
                - voxel[0][1][0] * (interimORR - interimRNR + interimRRO)
                - voxel[0][0][1] * (interimORR + interimROR - interimRRN)
                + voxel[0][0][0] * (interimORR + interimROR + interimRRO);
                
                c
                = voxel[1][1][1] * (interimRNN + interimNRN + interimNNR)
                + voxel[1][1][0] * (interimRNO + interimNRO - interimNNR)
                + voxel[1][0][1] * (interimRON - interimNRN + interimNOR)
                + voxel[1][0][0] * (interimROO - interimNRO - interimNOR)
                + voxel[0][1][1] * (interimORN + interimONR - interimRNN)
                - voxel[0][1][0] * (interimRNO - interimORO + interimONR)
                - voxel[0][0][1] * (interimRON + interimORN - interimOOR)
                - voxel[0][0][0] * (interimROO + interimORO + interimOOR);

                d 
                =
                e1[0] * (e1[1] * (e1[2] * voxel[1][1][1] +
                     e0[2] * voxel[1][1][0]) +
                 e0[1] * (e1[2] * voxel[1][0][1] +
                     e0[2] * voxel[1][0][0])) +
                e0[0] * (e1[1] * (e1[2] * voxel[0][1][1] +
                     e0[2] * voxel[0][1][0]) +
                 e0[1] * (e1[2] * voxel[0][0][1] +
                     e0[2] * voxel[0][0][0]));
                     
                 d -= isovalue;
            }
            
            inline float eval(const float t) const
            {
                return ((((a)*t+b)*t+c)*t+d);
            }
        };
        
                      
#ifdef MANTA_SSE
        static void sse_intersect(SSERayPacket& srp, 
                    char first, char last, const Vector& pmin, const Vector& pmax, float rho[2][2][2], 
                    float isovalue, sse_t tenter[], sse_t texit[], sse_t hitmask[],
                    const Manta::Primitive* prim, const Manta::Material* matl);
                    
        static void sse_normal(SSERayPacket& srp, int smd, 
                    sse_t normal[], const Vector& pmin, const Vector& pmax,
                    const float rho[2][2][2]);
                                    
        struct CubicPoly4
        {
            MANTA_ALIGN(16) sse_t a, b, c, d;		
        
            inline void generate(sse_t p0[], sse_t p1[], const float voxels_cell[2][2][2], float isovalue)
            {
                sse_t e0[3];
                sse_t e1[3];
                sse_t d1[3];
                
                for(int axis=0; axis<3; axis++)
                {
                    e0[axis] = sub4(_mm_one, p0[axis]);
                    e1[axis] = p0[axis];
                    d1[axis] = sub4(p1[axis], p0[axis]);
                }

                sse_t interimROO = mul4(d1[1], d1[2]);
                const sse_t interimRRR = mul4(d1[0], interimROO);
                const sse_t interimNRR = mul4(e1[0], interimROO);
                const sse_t interimORR = mul4(e0[0], interimROO);
                
                interimROO = mul4(e1[1], d1[2]);
                const sse_t interimRNR = mul4(d1[0], interimROO);
                const sse_t interimNNR = mul4(e1[0], interimROO);
                const sse_t interimONR = mul4(e0[0], interimROO);
                
                interimROO = mul4(d1[1], e1[2]);
                const sse_t interimRRN = mul4(d1[0], interimROO);
                const sse_t interimNRN = mul4(e1[0], interimROO);
                const sse_t interimORN = mul4(e0[0], interimROO);
                
                interimROO = mul4(d1[1], e0[2]);
                const sse_t interimRRO = mul4(d1[0], interimROO);
                const sse_t interimNRO = mul4(e1[0], interimROO);
                const sse_t interimORO = mul4(e0[0], interimROO);
                
                interimROO = mul4(e0[1], d1[2]);
                const sse_t interimROR = mul4(d1[0], interimROO);
                const sse_t interimNOR = mul4(e1[0], interimROO);
                const sse_t interimOOR = mul4(e0[0], interimROO);
                
                interimROO = mul4(d1[0], e1[1]);
                const sse_t interimRNN = mul4(interimROO, e1[2]);
                const sse_t interimRNO = mul4(interimROO, e0[2]);
                
                interimROO = mul4(d1[0], e0[1]);
                const sse_t interimRON = mul4(interimROO, e1[2]);
                interimROO = mul4(interimROO, e0[2]);
                
                a = mul4(interimRRR, _mm_set_ps1(+ voxels_cell[1][1][1]
                        - voxels_cell[0][1][1]
                                - voxels_cell[1][0][1]
                                + voxels_cell[0][0][1]
                                - voxels_cell[1][1][0]
                                + voxels_cell[0][1][0]
                                + voxels_cell[1][0][0]
                                - voxels_cell[0][0][0]));
                
                b = mul4(_mm_set_ps1(voxels_cell[1][1][1]),add4(add4(interimNRR,interimRNR),interimRRN));
                b = sub4(b,mul4(_mm_set_ps1(voxels_cell[1][1][0]),sub4(add4(interimNRR,interimRNR),interimRRO)));
                b = sub4(b,mul4(_mm_set_ps1(voxels_cell[1][0][1]),add4(sub4(interimNRR,interimROR),interimRRN)));
                b = add4(b,mul4(_mm_set_ps1(voxels_cell[1][0][0]),sub4(sub4(interimNRR,interimROR),interimRRO)));
                b = add4(b,mul4(_mm_set_ps1(voxels_cell[0][1][1]),sub4(sub4(interimORR,interimRNR),interimRRN)));
                b = sub4(b,mul4(_mm_set_ps1(voxels_cell[0][1][0]),add4(sub4(interimORR,interimRNR),interimRRO)));
                b = sub4(b,mul4(_mm_set_ps1(voxels_cell[0][0][1]),sub4(add4(interimORR,interimROR),interimRRN)));
                b = add4(b,mul4(_mm_set_ps1(voxels_cell[0][0][0]),add4(add4(interimORR,interimROR),interimRRO)));
                
                c = mul4(_mm_set_ps1(voxels_cell[1][1][1]),add4(add4(interimRNN,interimNRN),interimNNR));
                c = add4(c,mul4(_mm_set_ps1(voxels_cell[1][1][0]),sub4(add4(interimRNO,interimNRO),interimNNR)));
                c = add4(c,mul4(_mm_set_ps1(voxels_cell[1][0][1]),add4(sub4(interimRON,interimNRN),interimNOR)));
                c = add4(c,mul4(_mm_set_ps1(voxels_cell[1][0][0]),sub4(sub4(interimROO,interimNRO),interimNOR)));
                c = add4(c,mul4(_mm_set_ps1(voxels_cell[0][1][1]),sub4(add4(interimORN,interimONR),interimRNN)));
                c = sub4(c,mul4(_mm_set_ps1(voxels_cell[0][1][0]),add4(sub4(interimRNO,interimORO),interimONR)));
                c = sub4(c,mul4(_mm_set_ps1(voxels_cell[0][0][1]),sub4(add4(interimRON,interimORN),interimOOR)));
                c = sub4(c,mul4(_mm_set_ps1(voxels_cell[0][0][0]),add4(add4(interimROO,interimORO),interimOOR)));

                d = add4(mul4(e1[0], add4(mul4(e1[1], add4(mul4(e1[2], 
                               set4(voxels_cell[1][1][1])),
                               mul4(e0[2], set4(voxels_cell[1][1][0])))),
                               mul4(e0[1], add4(mul4(e1[2], set4(voxels_cell[1][0][1])),
                                   mul4(e0[2], set4(voxels_cell[1][0][0])))))),
                mul4(e0[0], add4(mul4(e1[1], add4(mul4(e1[2], 
                    set4(voxels_cell[0][1][1])),
                    mul4(e0[2], set4(voxels_cell[0][1][0])))),
                    mul4(e0[1], add4(mul4(e1[2], set4(voxels_cell[0][0][1])),
                        mul4(e0[2], set4(voxels_cell[0][0][0])))))));		
                
                d = sub4(d, set4(isovalue));
            }
        
            inline sse_t eval(const sse_t &t) const
            {
                return add4(mul4(add4(mul4(add4(mul4(a,t),b),t),c),t),d);
            }
        };	
            
                    
#endif                      
    };
};

#endif

