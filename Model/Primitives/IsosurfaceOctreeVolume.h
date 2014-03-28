
#ifndef _MANTA_ISOSURFACE_OCTREEVOLUME_HXX_
#define _MANTA_ISOSURFACE_OCTREEVOLUME_HXX_

#include <Model/Primitives/PrimitiveCommon.h>
#include <Core/Geometry/Vector.h>
#include <Core/Geometry/BBox.h>
#include <Core/Color/Color.h>
#include <Core/Util/Preprocessor.h>
#include <Interface/Texture.h>
#include <Model/Primitives/OctreeVolume.h>
#include <Interface/RayPacket.h>

#include <MantaSSE.h>

#ifdef MANTA_SSE
#include <Core/Math/SSEDefs.h>
#include <Interface/SSERayPacket.h>
#endif

//define this to show box cells, not the cap-value cells (and the implicit)
//#define IOV_BOX_CELLS


namespace Manta
{
    class IsosurfaceOctreeVolume : public PrimitiveCommon
    {
        const OctreeVolume* octdata;

        public:
            IsosurfaceOctreeVolume(OctreeVolume* _octdata, Material* _matl);
            ~IsosurfaceOctreeVolume();

            void preprocess( PreprocessContext const &context );
            void computeBounds( PreprocessContext const &context, BBox &box ) const;
            void intersect( RenderContext const &context, RayPacket &rays ) const;
            void computeNormal(const RenderContext& context, RayPacket& rays) const;

            BBox getBounds() const;

        private:

            void single_intersect(RayPacket& rays, int which_one) const;

            bool single_traverse_node(RayPacket& rays, int which_one,
                          const Vector& orig, const Vector& dir, const Vector& inv_dir, int res,
                          int depth, unsigned int node_index, unsigned int index_trace[], Vec3i& cell,
                          const float tenter, const float texit) const;

            bool single_traverse_leaf(RayPacket& rays, int which_one,
                        const Vector& orig, const Vector& dir, const Vector& inv_dir, int res,
                        int depth, int leaf_depth, ST scalar, Vec3i& leaf_base_cell,
                        unsigned int index_trace[], Vec3i& cell, const float tenter, const float texit) const;

            bool single_traverse_cap(RayPacket& rays, int which_one,
                          const Vector& orig, const Vector& dir, const Vector& inv_dir, int res,
                          int depth, unsigned int cap_index, unsigned int index_trace[], Vec3i& cell, const float tenter,
                          const float texit) const;

#ifdef MANTA_SSE
            struct FrustumInterval
            {
                //NOTE: these are all in the order umin, vmin, umax, vmax.
                //  But specifically, that order is defined by orig and dir.
                sse_t uvminmax_orig;
                sse_t uvminmax_dir;
                sse_t uvminmax_invdir;

                sse_t kminmax_orig;  //kmin_xforig, kmin_xforig, kmax_xforig, kmax_xforig
                sse_t kminmax_dir;
                sse_t kminmax_invdir;  //kmin_invdir, kmin_invdir, kmax_invdir, kmax_invdir
            };

            void packet_intersect_sse(RayPacket& rays) const;

            template<char K, char U, char V, char DK>
            void sse_traverse(SSERayPacket& srp, char first, char last,
                char DU, char DV) const;

            template<char K, char U, char V, char DK>
            void sse_traverse_node(SSERayPacket& srp,
                char first, char last, char DU, char DV,
                const FrustumInterval& fi,
                Vec3i& cell, char stop_depth, char depth, unsigned int index,
                unsigned int index_trace[]) const;

            template<char K, char U, char V, char DK>
            void sse_traverse_leaf(SSERayPacket& srp, char first, char last, char DU, char DV,
                const FrustumInterval& fi, const Vec3i& cell, int stop_depth, int depth,
                int leaf_depth, ST leaf_value, const Vec3i& leaf_base_cell,
                unsigned int index_trace[]) const;

            template<char K, char U, char V, char DK>
            void sse_traverse_cap(SSERayPacket& srp,
                char first, char last, char DU, char DV, const FrustumInterval& fi,
                Vec3i& cell, char stop_depth, char depth, unsigned int index,
                unsigned int index_trace[]) const;

            inline char first_intersects(SSERayPacket& srp, char first, char last,
                const Vector& min, const Vector& max) const
            {
                sse_t boxmin[3];
                sse_t boxmax[3];
                MANTA_UNROLL(3);
                for(int axis=0; axis<3; axis++)
                {
                    boxmin[axis] = set4(min[axis]);
                    boxmax[axis] = set4(max[axis]);
                }

                for(int smd=first; smd<=last; smd++)
                {
                    sse_t t0 = zero4();
                    sse_t t1 = srp.minT[smd];

                    sse_t signs = cmp4_ge(srp.dir[0][smd],zero4());
                    const sse_t b0_x = mask4(signs, boxmin[0], boxmax[0]);
                    const sse_t b1_x = mask4(signs, boxmax[0], boxmin[0]);
                    signs = cmp4_ge(srp.dir[1][smd],zero4());
                    const sse_t b0_y = mask4(signs, boxmin[1], boxmax[1]);
                    const sse_t b1_y = mask4(signs, boxmax[1], boxmin[1]);
                    signs = cmp4_ge(srp.dir[2][smd],zero4());
                    const sse_t b0_z = mask4(signs, boxmin[2], boxmax[2]);
                    const sse_t b1_z = mask4(signs, boxmax[2], boxmin[2]);

                    const sse_t tBoxNearX = mul4(sub4(b0_x, srp.orig[0][smd]), srp.inv_dir[0][smd]);
                    const sse_t tBoxNearY = mul4(sub4(b0_y, srp.orig[1][smd]), srp.inv_dir[1][smd]);
                    const sse_t tBoxNearZ = mul4(sub4(b0_z, srp.orig[2][smd]), srp.inv_dir[2][smd]);

                    t0 = max4(t0,tBoxNearX);
                    t0 = max4(t0,tBoxNearY);
                    t0 = max4(t0,tBoxNearZ);

                    const sse_t tBoxFarX = mul4(sub4(b1_x, srp.orig[0][smd]), srp.inv_dir[0][smd]);
                    const sse_t tBoxFarY = mul4(sub4(b1_y, srp.orig[1][smd]), srp.inv_dir[1][smd]);
                    const sse_t tBoxFarZ = mul4(sub4(b1_z, srp.orig[2][smd]), srp.inv_dir[2][smd]);

                    t1 = min4(t1,tBoxFarX);
                    t1 = min4(t1,tBoxFarY);
                    t1 = min4(t1,tBoxFarZ);

                    if (_mm_movemask_ps(cmp4_le(t0,t1)))    //if any hit
                        return smd;
                }
                return last+1;
            }

            inline char last_intersects(SSERayPacket& srp, char first, char last,
                const Vector& min, const Vector& max) const
            {
                sse_t boxmin[3];
                sse_t boxmax[3];
                MANTA_UNROLL(3);
                for(int axis=0; axis<3; axis++)
                {
                    boxmin[axis] = set4(min[axis]);
                    boxmax[axis] = set4(max[axis]);
                }

                for(int smd=last; smd>=first; smd--)
                {
                    sse_t t0 = zero4();
                    sse_t t1 = srp.minT[smd];

                    sse_t signs = cmp4_ge(srp.dir[0][smd],zero4());
                    const sse_t b0_x = mask4(signs, boxmin[0], boxmax[0]);
                    const sse_t b1_x = mask4(signs, boxmax[0], boxmin[0]);
                    signs = cmp4_ge(srp.dir[1][smd],zero4());
                    const sse_t b0_y = mask4(signs, boxmin[1], boxmax[1]);
                    const sse_t b1_y = mask4(signs, boxmax[1], boxmin[1]);
                    signs = cmp4_ge(srp.dir[2][smd],zero4());
                    const sse_t b0_z = mask4(signs, boxmin[2], boxmax[2]);
                    const sse_t b1_z = mask4(signs, boxmax[2], boxmin[2]);

                    const sse_t tBoxNearX = mul4(sub4(b0_x, srp.orig[0][smd]), srp.inv_dir[0][smd]);
                    const sse_t tBoxNearY = mul4(sub4(b0_y, srp.orig[1][smd]), srp.inv_dir[1][smd]);
                    const sse_t tBoxNearZ = mul4(sub4(b0_z, srp.orig[2][smd]), srp.inv_dir[2][smd]);

                    t0 = max4(t0,tBoxNearX);
                    t0 = max4(t0,tBoxNearY);
                    t0 = max4(t0,tBoxNearZ);

                    const sse_t tBoxFarX = mul4(sub4(b1_x, srp.orig[0][smd]), srp.inv_dir[0][smd]);
                    const sse_t tBoxFarY = mul4(sub4(b1_y, srp.orig[1][smd]), srp.inv_dir[1][smd]);
                    const sse_t tBoxFarZ = mul4(sub4(b1_z, srp.orig[2][smd]), srp.inv_dir[2][smd]);

                    t1 = min4(t1,tBoxFarX);
                    t1 = min4(t1,tBoxFarY);
                    t1 = min4(t1,tBoxFarZ);

                    if (_mm_movemask_ps(cmp4_le(t0,t1)))    //if any hit
                        return smd;
                }
                return -1;
            }

            inline void intersect_cap_octant(SSERayPacket& srp, char first, char last,
                    char& newfirst, char& newlast, const Vector& min, const Vector& max,
                    sse_t tenter[], sse_t texit[], sse_t hitmask[]) const
            {
                sse_t boxmin[3];
                sse_t boxmax[3];
                sse_t tnear[3];
                sse_t tfar[3];
                sse_t dgt0[3];

                MANTA_UNROLL(3);
                for(int axis=0; axis<3; axis++)
                {
                    boxmin[axis] = set4(min[axis]);
                    boxmax[axis] = set4(max[axis]);
                }

                newlast = first;
                newfirst = last+1;
                for(int smd=first; smd<=last; smd++)
                {
                    tenter[smd] = zero4();
                    texit[smd] = srp.minT[smd];

                    dgt0[0] = cmp4_ge(srp.dir[0][smd],zero4());
                    const sse_t b0_x = mask4(dgt0[0], boxmin[0], boxmax[0]);
                    const sse_t b1_x = mask4(dgt0[0], boxmax[0], boxmin[0]);
                    dgt0[1] = cmp4_ge(srp.dir[1][smd],zero4());
                    const sse_t b0_y = mask4(dgt0[1], boxmin[1], boxmax[1]);
                    const sse_t b1_y = mask4(dgt0[1], boxmax[1], boxmin[1]);
                    dgt0[2] = cmp4_ge(srp.dir[2][smd],zero4());
                    const sse_t b0_z = mask4(dgt0[2], boxmin[2], boxmax[2]);
                    const sse_t b1_z = mask4(dgt0[2], boxmax[2], boxmin[2]);

                    tnear[0] = mul4(sub4(b0_x, srp.orig[0][smd]), srp.inv_dir[0][smd]);
                    tnear[1] = mul4(sub4(b0_y, srp.orig[1][smd]), srp.inv_dir[1][smd]);
                    tnear[2] = mul4(sub4(b0_z, srp.orig[2][smd]), srp.inv_dir[2][smd]);

                    tenter[smd] = max4(tenter[smd],tnear[0]);
                    tenter[smd] = max4(tenter[smd],tnear[1]);
                    tenter[smd] = max4(tenter[smd],tnear[2]);

                    tfar[0] = mul4(sub4(b1_x, srp.orig[0][smd]), srp.inv_dir[0][smd]);
                    tfar[1] = mul4(sub4(b1_y, srp.orig[1][smd]), srp.inv_dir[1][smd]);
                    tfar[2] = mul4(sub4(b1_z, srp.orig[2][smd]), srp.inv_dir[2][smd]);

                    texit[smd] = min4(texit[smd],tfar[0]);
                    texit[smd] = min4(texit[smd],tfar[1]);
                    texit[smd] = min4(texit[smd],tfar[2]);

                    hitmask[smd] = cmp4_lt(tenter[smd], texit[smd]);
                    if (_mm_movemask_ps(hitmask[smd]))
                    {
                        newfirst = MIN(newfirst, smd);
                        newlast = smd;
                    }

#ifdef IOV_BOX_CELLS
                    sse_t hitmask2 = and4(hitmask[smd], cmp4_lt(tenter[smd], srp.minT[smd]));
                    srp.minT[smd] = mask4(hitmask2, tenter[smd], srp.minT[smd]);
                    if (_mm_movemask_ps(hitmask2))
                    {
                        sse_t normal[3];
                        for(int axis=0; axis<3; axis++)
                        {
                            normal[axis] = mask4(cmp4_eq(tenter[smd], tnear[axis]), mask4(dgt0[axis], set4(-1.0f), _mm_one), zero4());
                            srp.normal[axis][smd] = mask4(hitmask2, normal[axis], srp.normal[axis][smd]);
                        }

                        MANTA_UNROLL(3);
                        for(int axis=0; axis<3; axis++)
                            srp.normal[axis][smd] = mask4(hitmask2, normal[axis], srp.normal[axis][smd]);

                        int int_hitmask2 = _mm_movemask_ps(hitmask2);

                        MANTA_UNROLL(4);
                        for(int ray=0; ray<4; ray++)
                        {
                            if (int_hitmask2 & (1<<ray))
                            {
                                int realray=(smd<<2)+ray;
                                srp.rp->data->hitMatl[realray] = PrimitiveCommon::getMaterial();
                                srp.rp->data->hitPrim[realray] = this;
                            }
                        }
                    }

#endif
                }
            }

#endif   //MANTA_SSE
    };
};

#endif
