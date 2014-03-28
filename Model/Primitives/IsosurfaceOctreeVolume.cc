/*
    Aaron Knoll
    Single-ray, non-SSE octree isosurface intersector
*/

#include <Model/Primitives/IsosurfaceOctreeVolume.h>
#include <Interface/RayPacket.h>
#include <Model/Intersections/IsosurfaceImplicit.h>

#ifdef MANTA_SSE
#include <Core/Math/SSEDefs.h>
#endif

#define USE_OCTREE_DATA

#define MIN4(a,b,c,d) min(min(a,b), min(c,d));
#define MAX4(a,b,c,d) max(max(a,b), max(c,d));

//requires local variables:
// NODE (either node or cap... anything that has .values[])
// offset = Vec3i(0,0,1)
// every other variable must be either locally or globall declared. See caller examples below.
#define octvol_fill_cell(NODE, cw) \
        if (target_child & 1) \
                this_rho = octdata->lookup_neighbor<0,0,1>(child_cell, offset, stop_depth, prev_depth, index_trace); \
        else \
                this_rho = NODE.values[target_child | 1]; \
        min_rho = MIN(min_rho, this_rho); \
        max_rho = MAX(max_rho, this_rho); \
        rho[0][0][1] = static_cast<float>(this_rho); \
        offset.data[1] = cw; \
        if (target_child & 3) \
                this_rho = octdata->lookup_neighbor<0,1,1>(child_cell, offset, stop_depth, prev_depth, index_trace); \
        else \
                this_rho = NODE.values[target_child | 3]; \
        min_rho = MIN(min_rho, this_rho); \
        max_rho = MAX(max_rho, this_rho); \
        rho[0][1][1] = static_cast<float>(this_rho); \
        offset.data[0] = cw; \
        if (target_child & 7) \
                this_rho = octdata->lookup_neighbor<1,1,1>(child_cell, offset, stop_depth, prev_depth, index_trace); \
        else \
                this_rho = NODE.values[target_child | 7]; \
        min_rho = MIN(min_rho, this_rho); \
        max_rho = MAX(max_rho, this_rho); \
        rho[1][1][1] = static_cast<float>(this_rho); \
        offset.data[2] = 0; \
        if (target_child & 6) \
                this_rho = octdata->lookup_neighbor<1,1,0>(child_cell, offset, stop_depth, prev_depth, index_trace); \
        else \
                this_rho = NODE.values[target_child | 6]; \
        min_rho = MIN(min_rho, this_rho); \
        max_rho = MAX(max_rho, this_rho); \
        rho[1][1][0] = static_cast<float>(this_rho); \
        offset.data[1] = 0; \
        if (target_child & 4) \
                this_rho = octdata->lookup_neighbor<1,0,0>(child_cell, offset, stop_depth, prev_depth, index_trace); \
        else \
                this_rho = NODE.values[target_child | 4]; \
        min_rho = MIN(min_rho, this_rho); \
        max_rho = MAX(max_rho, this_rho); \
        rho[1][0][0] = static_cast<float>(this_rho); \
        offset.data[2] = cw; \
        if (target_child & 5) \
                this_rho = octdata->lookup_neighbor<1,0,1>(child_cell, offset, stop_depth, prev_depth, index_trace); \
        else \
                this_rho = NODE.values[target_child | 5]; \
        min_rho = MIN(min_rho, this_rho); \
        max_rho = MAX(max_rho, this_rho); \
        rho[1][0][1] = static_cast<float>(this_rho); \
        offset.data[0] = 0; \
        offset.data[1] = cw; \
        offset.data[2] = 0; \
        if (target_child & 2) \
                this_rho = octdata->lookup_neighbor<0,1,0>(child_cell, offset, stop_depth, prev_depth, index_trace); \
        else \
                this_rho = NODE.values[target_child | 2]; \
        min_rho = MIN(min_rho, this_rho); \
        max_rho = MAX(max_rho, this_rho); \
        rho[0][1][0] = static_cast<float>(this_rho); \


static const int axis_table[] = {4, 2, 1};

using namespace Manta;

IsosurfaceOctreeVolume::IsosurfaceOctreeVolume(OctreeVolume* _octdata, Material* _matl)
: PrimitiveCommon(_matl), octdata(_octdata)
{
}

IsosurfaceOctreeVolume::~IsosurfaceOctreeVolume()
{
}

void IsosurfaceOctreeVolume::preprocess( PreprocessContext const &context )
{
    PrimitiveCommon::preprocess(context);
}

void IsosurfaceOctreeVolume::computeBounds( PreprocessContext const &context, BBox &box ) const
{
    box = octdata->get_bounds();
}

void IsosurfaceOctreeVolume::computeNormal(const RenderContext& context, RayPacket& rays) const
{
}

BBox IsosurfaceOctreeVolume::getBounds() const
{
    return octdata->get_bounds();
}

void IsosurfaceOctreeVolume::intersect(RenderContext const &context, RayPacket &packet) const
{
//#ifdef MANTA_SSE
#if 0
        packet_intersect_sse(packet);
#else
    for ( int i = packet.rayBegin; i < packet.rayEnd; i++ )
        single_intersect(packet, i);
#endif
}

void IsosurfaceOctreeVolume::single_intersect(RayPacket& rays, int which_one) const
{
    Vector t0;
    Vector t1;
    Vector t1p;

    Vector orig = rays.getOrigin(which_one);
    Vector dir = rays.getDirection(which_one);
    //Vector inv_dir = rays.getInverseDirection(which_one);
    Vector inv_dir = dir.inverse();

    MANTA_UNROLL(3);
    for(int axis=0; axis<3; axis++)
    {
        t0.data[axis] = -orig.data[axis] * inv_dir.data[axis];
        t1.data[axis] = (octdata->dims[axis] - orig.data[axis]) * inv_dir.data[axis];
        t1p.data[axis] = (octdata->padded_dims[axis] - orig.data[axis]) * inv_dir.data[axis];
    }
    float tenter, texit, tenter_padded, texit_padded;
    if (dir.data[0] > 0.f)
    {
        tenter = tenter_padded = t0.data[0];
        texit = t1.data[0];
        texit_padded = t1p.data[0];
    }
    else
    {
        tenter = t1.data[0];
        tenter_padded = t1p.data[0];
        texit = texit_padded = t0.data[0];
    }

    MANTA_UNROLL(2);
    for(int axis=1; axis<3; axis++)
    {
        float ft0, ft1, ft0p, ft1p;
        if (dir.data[axis] > 0.f)
        {
            ft0 = ft0p = t0.data[axis];
            ft1 = t1.data[axis];
            ft1p = t1p.data[axis];
        }
        else
        {
            ft0p = t1p.data[axis];
            ft0 = t1.data[axis];
            ft1 = ft1p = t0.data[axis];
        }

        tenter = MAX(tenter, ft0);
        texit = MIN(texit, ft1);
        tenter_padded = MAX(tenter_padded, ft0p);
        texit_padded = MIN(texit_padded, ft1p);

        if (tenter > texit)
            return;
    }

    if (texit < 0.f)
        return;

    //tenter_padded = MAX(0.f, tenter_padded);

    unsigned int* index_trace = MANTA_STACK_ALLOC(unsigned int, octdata->get_max_depth()+1);

    int stop_depth = octdata->get_cap_depth() - 0;

    Vec3i cell(0,0,0);
    single_traverse_node(rays, which_one, orig, dir, inv_dir, stop_depth,
                        0, 0, index_trace, cell, tenter_padded, texit_padded);
}

bool IsosurfaceOctreeVolume::single_traverse_node(RayPacket& rays, int which_one,
                        const Vector& orig, const Vector& dir, const Vector& inv_dir,
                        int stop_depth, int depth, unsigned int node_index,
                        unsigned int index_trace[], Vec3i& cell, const float tenter,
                        const float texit) const
{
    //cerr << "single_traverse_node, depth=" << depth << ", node_index=" << node_index << ", cell=" << cell.data[0] << "," << cell.data[1] << "," << cell.data[2] << endl;
    OctNode& node = octdata->get_node(depth, node_index);

    index_trace[depth] = node_index;

    int child_bit = octdata->get_child_bit_depth(depth);
    Vector center(static_cast<float>(cell.data[0] | child_bit),
                static_cast<float>(cell.data[1] | child_bit), static_cast<float>(cell.data[2] | child_bit));
    Vector tcenter = inv_dir * (center - orig);

    Vector penter = orig + (dir*tenter);

    Vec3i child_cell = cell;
    Vec3i tc( penter.x() >= center.x(), penter.y() >= center.y(), penter.z() >= center.z() );
    int target_child = (tc.data[0] << 2) | (tc.data[1] << 1) | tc.data[2];
    child_cell.data[0] |= tc.data[0] ? child_bit : 0;
    child_cell.data[1] |= tc.data[1] ? child_bit : 0;
    child_cell.data[2] |= tc.data[2] ? child_bit : 0;

    Vec3i axis_isects;
    if (tcenter.data[0] < tcenter.data[1] && tcenter.data[0] < tcenter.data[2]){
        axis_isects.data[0] = 0;
        if (tcenter.data[1] < tcenter.data[2]){
            axis_isects.data[1] = 1;
            axis_isects.data[2] = 2;
        }
        else{
            axis_isects.data[1] = 2;
            axis_isects.data[2] = 1;
        }
    }
    else if (tcenter.data[1] < tcenter.data[2]){
        axis_isects.data[0] = 1;
        if (tcenter.data[0] < tcenter.data[2]){
            axis_isects.data[1] = 0;
            axis_isects.data[2] = 2;
        }
        else{
            axis_isects.data[1] = 2;
            axis_isects.data[2] = 0;
        }
    }
    else{
        axis_isects.data[0] = 2;
        if (tcenter.data[0] < tcenter.data[1]){
            axis_isects.data[1] = 0;
            axis_isects.data[2] = 1;
        }
        else{
            axis_isects.data[1] = 1;
            axis_isects.data[2] = 0;
        }
    }

    float child_tenter = tenter;
    float child_texit;

    int next_depth = depth+1;

    for(int i=0;; i++)
    {
        int axis = -1;
        if (i>=3)
            child_texit = texit;
        else
        {
            axis = axis_isects.data[i];
            child_texit = tcenter.data[axis];
            if (child_texit < tenter)
                continue;
            if (child_texit >= texit){
                child_texit = texit;
                axis = -1;
            }
        }

        if (octdata->get_isovalue() >= node.mins[target_child] && octdata->get_isovalue() <= node.maxs[target_child])
        {

#ifdef OCTVOL_DYNAMIC_MULTIRES
                        if (depth == stop_depth)
                        {
                                Vector cmin(child_cell.data[0], child_cell.data[1], child_cell.data[2]);
                                Vector cmax(child_cell.data[0] + child_bit, child_cell.data[1] + child_bit, child_cell.data[2] + child_bit);

                                float rho[2][2][2];
                                ST min_rho, max_rho, this_rho;
                                min_rho = max_rho = this_rho = node.values[target_child];
                                rho[0][0][0] = static_cast<float>(this_rho);
                                int prev_depth = depth-1;
                                Vec3i offset(0,0,child_bit);
                                octvol_fill_cell(node, child_bit);

                                if (octdata->get_isovalue() >= min_rho && octdata->get_isovalue() <= max_rho)
                                {
                                        float hit_t;
                                        if (IsosurfaceImplicit::single_intersect(orig, dir, cmin, cmax, rho,
                                                octdata->get_isovalue(), child_tenter, child_texit, hit_t))
                                        {
                                                if (rays.hit(which_one, hit_t, PrimitiveCommon::getMaterial(), this, 0))
                                                {
                                                        Vector normal;
                                                        Vector phit = orig + dir*hit_t;
                                                        IsosurfaceImplicit::single_normal(normal, cmin, cmax, phit, rho);
                                                        normal.normalize();
                                                        rays.setNormal(which_one, normal);
                                                        return true;
                                                }
                                        }
                                }
                        }
                        else
#endif
            if (node.offsets[target_child]==-1)
            {
                if (single_traverse_leaf(rays, which_one, orig, dir, inv_dir, stop_depth,
                    next_depth, depth, node.values[target_child],
                    child_cell, index_trace, child_cell, child_tenter, child_texit))
                    return true;
            }
            else
            {
                unsigned int child_idx = node.children_start + node.offsets[target_child];
                if (depth == octdata->get_pre_cap_depth())      //cap
                {
                    if (single_traverse_cap(rays, which_one, orig, dir, inv_dir, stop_depth, next_depth, child_idx,
                        index_trace, child_cell, child_tenter, child_texit))
                        return true;
                }
                else
                {
                    if (single_traverse_node(rays, which_one, orig, dir, inv_dir, stop_depth, next_depth, child_idx,
                        index_trace, child_cell, child_tenter, child_texit))
                        return true;
                }
            }
        }

        if (axis==-1)
            return false;

        //move to the next target_child, update tenter and texit
        child_tenter = child_texit;
        int trueaxisbit = axis_table[axis];
        if (target_child & trueaxisbit)         //going from true to false
        {
            target_child &= ~trueaxisbit;
            child_cell.data[axis] &= ~child_bit;
        }
        else                                                            //going from false to true
        {
            target_child |= trueaxisbit;
            child_cell.data[axis] |= child_bit;
        }
    }
    return false;
}

bool IsosurfaceOctreeVolume::single_traverse_leaf(RayPacket& rays, int which_one,
                        const Vector& orig, const Vector& dir, const Vector& inv_dir, int stop_depth,
                        int depth, int leaf_depth, ST scalar, Vec3i& leaf_base_cell,
                        unsigned int index_trace[], Vec3i& cell, const float tenter, const float texit) const
{
    //using pretty much the same algorithm, find which "implicit" cell (-->voxel) we're in at octdata->get_max_depth()

    int child_bit = octdata->get_child_bit_depth(depth);
    int unsafe_zone = octdata->get_child_bit_depth(depth-1) - octdata->get_child_bit_depth(octdata->get_cap_depth());

    Vector center(static_cast<float>(cell.data[0] | child_bit), static_cast<float>(cell.data[1] | child_bit), static_cast<float>(cell.data[2] | child_bit));
    Vector tcenter = inv_dir * (center - orig);
    Vector penter = orig + (dir*tenter);

    Vec3i child_cell = cell;
    Vec3i tc( penter.x() >= center.x(), penter.y() >= center.y(), penter.z() >= center.z() );
    int target_child = (tc.data[0] << 2) | (tc.data[1] << 1) | tc.data[2];
    child_cell.data[0] |= tc.data[0] ? child_bit : 0;
    child_cell.data[1] |= tc.data[1] ? child_bit : 0;
    child_cell.data[2] |= tc.data[2] ? child_bit : 0;

    Vec3i axis_isects;
    if (tcenter.data[0] < tcenter.data[1] && tcenter.data[0] < tcenter.data[2]){
        axis_isects.data[0] = 0;
        if (tcenter.data[1] < tcenter.data[2]){
            axis_isects.data[1] = 1;
            axis_isects.data[2] = 2;
        }
        else{
            axis_isects.data[1] = 2;
            axis_isects.data[2] = 1;
        }
    }
    else if (tcenter.data[1] < tcenter.data[2]){
        axis_isects.data[0] = 1;
        if (tcenter.data[0] < tcenter.data[2]){
            axis_isects.data[1] = 0;
            axis_isects.data[2] = 2;
        }
        else{
            axis_isects.data[1] = 2;
            axis_isects.data[2] = 0;
        }
    }
    else{
        axis_isects.data[0] = 2;
        if (tcenter.data[0] < tcenter.data[1]){
            axis_isects.data[1] = 0;
            axis_isects.data[2] = 1;
        }
        else{
            axis_isects.data[1] = 1;
            axis_isects.data[2] = 0;
        }
    }

    float child_tenter = tenter;
    float child_texit;

    int next_depth = depth+1;

    for(int i=0;; i++)
    {
        int axis = -1;
        if (i>=3)
            child_texit = texit;
        else
        {
            axis = axis_isects.data[i];
            child_texit = tcenter.data[axis];
            if (child_texit < tenter)
                continue;
            if (child_texit >= texit){
                child_texit = texit;
                axis = -1;
            }
        }

        Vec3i local_child_cell = child_cell - leaf_base_cell;
        if (local_child_cell.data[0] & unsafe_zone || local_child_cell.data[1] & unsafe_zone || local_child_cell.data[2] & unsafe_zone)
        {
            if (depth == stop_depth)
            {
                //try isosurface intersection
                Vector cmin(child_cell.data[0], child_cell.data[1], child_cell.data[2]);
                                Vector cmax(child_cell.data[0] + child_bit, child_cell.data[1] + child_bit, child_cell.data[2] + child_bit);

#ifdef USE_OCTREE_DATA
                //use octree data
                float rho[2][2][2];
                ST min_rho, max_rho, this_rho;
                min_rho = max_rho = this_rho = scalar;
                rho[0][0][0] = static_cast<float>(this_rho);
                Vec3i offset(0,0,child_bit);

                //0,0,1
                if (target_child & 1)
                {
                    this_rho = octdata->lookup_neighbor<0,0,1>(child_cell, offset, stop_depth, leaf_depth, index_trace);
                    min_rho = MIN(min_rho, this_rho);
                    max_rho = MAX(max_rho, this_rho);
                }
                else
                    this_rho = scalar;
                rho[0][0][1] = static_cast<float>(this_rho);

                //0,1,1
                offset.data[1] = child_bit;
                if (target_child & 3)
                {
                    this_rho = octdata->lookup_neighbor<0,1,1>(child_cell, offset, stop_depth, leaf_depth, index_trace);
                    min_rho = MIN(min_rho, this_rho);
                    max_rho = MAX(max_rho, this_rho);
                }
                else
                    this_rho = scalar;
                rho[0][1][1] = static_cast<float>(this_rho);

                //1,1,1
                offset.data[0] = child_bit;
                if (target_child & 7)
                {
                    this_rho = octdata->lookup_neighbor<1,1,1>(child_cell, offset, stop_depth, leaf_depth, index_trace);
                    min_rho = MIN(min_rho, this_rho);
                    max_rho = MAX(max_rho, this_rho);
                }
                else
                    this_rho = scalar;
                rho[1][1][1] = static_cast<float>(this_rho);

                //1,1,0
                offset.data[2] = 0;
                if (target_child & 6)
                {
                    this_rho = octdata->lookup_neighbor<1,1,0>(child_cell, offset, stop_depth, leaf_depth, index_trace);
                    min_rho = MIN(min_rho, this_rho);
                    max_rho = MAX(max_rho, this_rho);
                }
                else
                    this_rho = scalar;
                rho[1][1][0] = static_cast<float>(this_rho);

                //1,0,0
                offset.data[1] = 0;
                if (target_child & 4)
                {
                    this_rho = octdata->lookup_neighbor<1,0,0>(child_cell, offset, stop_depth, leaf_depth, index_trace);
                    min_rho = MIN(min_rho, this_rho);
                    max_rho = MAX(max_rho, this_rho);
                }
                else
                    this_rho = scalar;
                rho[1][0][0] = static_cast<float>(this_rho);

                //1,0,1
                offset.data[2] = child_bit;
                if (target_child & 5)
                {
                    this_rho = octdata->lookup_neighbor<1,0,1>(child_cell, offset, stop_depth, leaf_depth, index_trace);
                    min_rho = MIN(min_rho, this_rho);
                    max_rho = MAX(max_rho, this_rho);
                }
                else
                    this_rho = scalar;
                rho[1][0][1] = static_cast<float>(this_rho);

                //0,1,0
                offset.data[0] = 0;
                offset.data[1] = child_bit;
                offset.data[2] = 0;
                if (target_child & 2)
                {
                    this_rho = octdata->lookup_neighbor<0,1,0>(child_cell, offset, stop_depth, leaf_depth, index_trace);
                    min_rho = MIN(min_rho, this_rho);
                    max_rho = MAX(max_rho, this_rho);
                }
                else
                    this_rho = scalar;
                rho[0][1][0] = static_cast<float>(this_rho);
#else
                //use original grid data
                float rho[2][2][2];
                ST min_rho, max_rho;
#define MYDATA octdata->indata  //toggle this to octdata if you want to test pure point location (no neighbor finding)
                min_rho = max_rho = lookup_safe(MYDATA, child_cell.data[0], child_cell.data[1], child_cell.data[2]);
                rho[0][0][0] = static_cast<float>(min_rho);
                for(int c=1; c<8; c++)
                {
                    Vec3i offset((c&4)!=0, (c&2)!=0, c&1);
                    Vec3i neighboridx = child_cell + offset;
                    ST this_rho = lookup_safe(MYDATA, neighboridx.data[0], neighboridx.data[1], neighboridx.data[2]);
                    rho[offset.data[0]][offset.data[1]][offset.data[2]] = static_cast<float>(this_rho);
                    min_rho = MIN(this_rho, min_rho);
                    max_rho = MAX(this_rho, max_rho);
                }
#endif
                if (octdata->get_isovalue() >= min_rho && octdata->get_isovalue() <= max_rho)
                {
                    float hit_t;
                    if (IsosurfaceImplicit::single_intersect_neubauer(orig, dir, cmin, cmax, rho,
                        octdata->get_isovalue(), child_tenter, child_texit, hit_t))
                    {
                        if (rays.hit(which_one, hit_t, PrimitiveCommon::getMaterial(), this, 0))
                        {
                            Vector normal;
                            Vector phit = orig + dir*hit_t;
                            IsosurfaceImplicit::single_normal(normal, cmin, cmax, phit, rho);
                            normal.normalize();
                            rays.setNormal(which_one, normal);
                            return true;
                        }
                    }
                }
            }
            else //not at cap-level depth
            {
                if (single_traverse_leaf(rays, which_one, orig, dir, inv_dir, stop_depth,
                    next_depth, leaf_depth, scalar, leaf_base_cell,
                    index_trace, child_cell, child_tenter, child_texit))
                    return true;
            }
        }

        if (axis==-1)
            return false;

        //move to the next target_child, update tenter and texit
        child_tenter = child_texit;
        int trueaxisbit = axis_table[axis];
        if (target_child & trueaxisbit)         //going from true to false
        {
            target_child &= ~trueaxisbit;
            child_cell.data[axis] &= ~child_bit;
        }
        else                                                            //going from false to true
        {
            target_child |= trueaxisbit;
            child_cell.data[axis] |= child_bit;
        }
    }
    return false;
}

bool IsosurfaceOctreeVolume::single_traverse_cap(RayPacket& rays, int which_one,
                        const Vector& orig, const Vector& dir, const Vector& inv_dir, int stop_depth,
                        int depth, unsigned int cap_index, unsigned int index_trace[], Vec3i& cell,
                        const float tenter, const float texit) const
{
    //cerr << "single_traverse_cap, depth=" << depth << ", cap_index=" << cap_index << ", cell=" << cell[0] << "," << cell[1] << "," << cell[2] << endl;

    OctCap& cap = octdata->get_cap(cap_index);
    index_trace[depth] = cap_index;

    Vector penter = orig + (dir*tenter);
    Vector center(static_cast<float>(cell.data[0] | 1), static_cast<float>(cell.data[1] | 1), static_cast<float>(cell.data[2] | 1));
    Vector tcenter = inv_dir * (center - orig);

    Vec3i child_cell = cell;
    Vec3i tc( penter.x() >= center.x(), penter.y() >= center.y(), penter.z() >= center.z() );
    int target_child = (tc.data[0] << 2) | (tc.data[1] << 1) | tc.data[2];
    child_cell.data[0] |= tc.data[0];
    child_cell.data[1] |= tc.data[1];
    child_cell.data[2] |= tc.data[2];

    Vec3i axis_isects;
    if (tcenter.data[0] < tcenter.data[1] && tcenter.data[0] < tcenter.data[2]){
        axis_isects.data[0] = 0;
        if (tcenter.data[1] < tcenter.data[2]){
            axis_isects.data[1] = 1;
            axis_isects.data[2] = 2;
        }
        else{
            axis_isects.data[1] = 2;
            axis_isects.data[2] = 1;
        }
    }
    else if (tcenter.data[1] < tcenter.data[2]){
        axis_isects.data[0] = 1;
        if (tcenter.data[0] < tcenter.data[2]){
            axis_isects.data[1] = 0;
            axis_isects.data[2] = 2;
        }
        else{
            axis_isects.data[1] = 2;
            axis_isects.data[2] = 0;
        }
    }
    else{
        axis_isects.data[0] = 2;
        if (tcenter.data[0] < tcenter.data[1]){
            axis_isects.data[1] = 0;
            axis_isects.data[2] = 1;
        }
        else{
            axis_isects.data[1] = 1;
            axis_isects.data[2] = 0;
        }
    }

    float child_tenter = tenter;
    float child_texit;

    for(int i=0;; i++)
    {
        int axis = -1;
        if (i>=3)
            child_texit = texit;
        else
        {
            axis = axis_isects.data[i];
            child_texit = tcenter.data[axis];
            if (child_texit < tenter)
                continue;
            if (child_texit >= texit){
                child_texit = texit;
                axis = -1;
            }
        }

        //try isosurface intersection in this node
        Vector cmin(child_cell.data[0], child_cell.data[1], child_cell.data[2]);
                Vector cmax(child_cell.data[0] + 1, child_cell.data[1] + 1, child_cell.data[2] + 1);

#ifdef USE_OCTREE_DATA
        float rho[2][2][2];
        ST min_rho, max_rho, this_rho;
        min_rho = max_rho = this_rho = cap.values[target_child];
        rho[0][0][0] = static_cast<float>(this_rho);
        int prev_depth = depth-1;
        Vec3i offset(0,0,1);
                octvol_fill_cell(cap, 1);
#else
        //use original grid data
        float rho[2][2][2];
        ST min_rho, max_rho;
#define MYDATA octdata->indata
        min_rho = max_rho = lookup_safe(MYDATA, child_cell.data[0], child_cell.data[1], child_cell.data[2]);
        rho[0][0][0] = static_cast<float>(min_rho);
        for(int c=1; c<8; c++)
        {
            Vec3i offset((c&4)!=0, (c&2)!=0, c&1);
            Vec3i neighboridx = child_cell + offset;
            ST this_rho = lookup_safe(MYDATA, neighboridx.data[0], neighboridx.data[1], neighboridx.data[2]);
            rho[offset.data[0]][offset.data[1]][offset.data[2]] = static_cast<float>(this_rho);
            min_rho = MIN(this_rho, min_rho);
            max_rho = MAX(this_rho, max_rho);
        }
#endif
        if (octdata->get_isovalue() >= min_rho && octdata->get_isovalue() <= max_rho)
        {
            float hit_t;
            if (IsosurfaceImplicit::single_intersect_neubauer(orig, dir, cmin, cmax, rho,
                octdata->get_isovalue(), child_tenter, child_texit, hit_t))
            {
                if (rays.hit(which_one, hit_t, PrimitiveCommon::getMaterial(), this, 0))
                {
                    Vector normal;
                    Vector phit = orig + dir*hit_t;
                    IsosurfaceImplicit::single_normal(normal, cmin, cmax, phit, rho);
                    normal.normalize();
                    rays.setNormal(which_one, normal);
                    return true;
                }
            }
        }

        if (axis==-1)
            return false;

        //move to the next target_child, update tenter and texit
        child_tenter = child_texit;
        int trueaxisbit = axis_table[axis];
        if (target_child & trueaxisbit)         //going from true to false
        {
            target_child &= ~trueaxisbit;
            child_cell.data[axis] &= ~1;
        }
        else                                                            //going from false to true
        {
            target_child |= trueaxisbit;
            child_cell.data[axis] |= 1;
        }
    }
    return false;
}

/*
        Begin packet intersection code, for SSE packets only.
*/

//AARONBAD - This SSE octree traverser turned out to be very slow. Not recommended.
#if 0

//#ifdef MANTA_SSE

void IsosurfaceOctreeVolume::packet_intersect_sse(RayPacket& rays) const
{
    rays.computeInverseDirections();
    rays.computeSigns();
    rays.resetHits();

    RayPacketData* data = rays.data;
    SSERayPacket srp;

    //intersect the global bounding box: find first, last
    //  this will require a special-case AABB intersection
    MANTA_UNROLL(3);
    for(int axis=0; axis<3; axis++)
    {
        srp.orig[axis] = (sse_t*)(data->origin[axis]);
        srp.dir[axis] = (sse_t*)(data->direction[axis]);
        srp.inv_dir[axis] = (sse_t*)(data->inverseDirection[axis]);
        srp.signs[axis] = (sse_t*)(data->signs[axis]);
        srp.normal[axis] = (sse_t*)(data->normal[axis]);
    }
    srp.rp = &rays;
    srp.minT = (sse_t*)(data->minT);
    srp.activeRays = 0;
    const int sse_begin = rays.begin() >> 2; // equivalent to Floor(rays.begin()/4)
    const int sse_end = ((rays.end()-1+3) >> 2); // Ceil(rays.end()-1/4)

    char first = RayPacket::SSE_MaxSize;
    char last = -1;
    sse_t octdims[3];
    for(int axis=0; axis<3; axis++)
        octdims[axis] = set4(octdata->dims[axis]);

    for(int smd=sse_begin; smd<sse_end; smd++)
    {
        sse_t t0 = zero4();
        sse_t t1 = srp.minT[smd];

        sse_t signs = cmp4_ge(srp.dir[0][smd],zero4());
        const sse_t b0_x = mask4(signs, zero4(), octdims[0]);
        const sse_t b1_x = mask4(signs, octdims[0], zero4());
        signs = cmp4_ge(srp.dir[1][smd],zero4());
        const sse_t b0_y = mask4(signs, zero4(), octdims[1]);
        const sse_t b1_y = mask4(signs, octdims[1], zero4());
        signs = cmp4_ge(srp.dir[2][smd],zero4());
        const sse_t b0_z = mask4(signs, zero4(), octdims[2]);
        const sse_t b1_z = mask4(signs, octdims[2], zero4());

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

        srp.activeMask[smd] = cmp4_le(t0,t1);
        if (_mm_movemask_ps(srp.activeMask[smd]))    //if any hit
        {
            first = MIN(first, smd);
            last = smd;
            srp.activeRays += count_nonzeros(srp.activeMask[smd]);
        }
    }

    if (first > last)
        return;

    Vector direction = rays.getDirection(first<<2);
    Vector dir2 = direction * direction;
    if (dir2[0] > dir2[1] && dir2[0] > dir2[2])
    {
        if (direction[0] > 0)
            sse_traverse<0,1,2,1>(srp, first, last, (direction[1] > 0.f ? 1 : -1), (direction[2] > 0.f ? 1 : -1));
        else
            sse_traverse<0,1,2,0>(srp, first, last, (direction[1] > 0.f ? 1 : -1), (direction[2] > 0.f ? 1 : -1));
    }
    else if (dir2[1] < dir2[2])
    {
        if (direction[1] > 0)
            sse_traverse<1,0,2,1>(srp, first, last, (direction[0] > 0.f ? 1 : -1), (direction[2] > 0.f ? 1 : -1));
        else
            sse_traverse<1,0,2,0>(srp, first, last, (direction[0] > 0.f ? 1 : -1), (direction[2] > 0.f ? 1 : -1));
    }
    else
    {
        if (direction[2] > 0)
            sse_traverse<2,0,1,1>(srp, first, last, (direction[0] > 0.f ? 1 : -1), (direction[1] > 0.f ? 1 : -1));
        else
            sse_traverse<2,0,1,0>(srp, first, last, (direction[0] > 0.f ? 1 : -1), (direction[1] > 0.f ? 1 : -1));
    }
}

#define DBGP 0

template<char K, char U, char V, char DK>
void IsosurfaceOctreeVolume::sse_traverse(SSERayPacket& srp, char first, char last, char DU, char DV) const
{
    //find the bounding frustum of the rays, as (umin, vmin, umax, vmax) coordinates
    sse_t smin_orig[3];
    sse_t smin_dir[3];
    sse_t smax_orig[3];
    sse_t smax_dir[3];

    MANTA_UNROLL(3);
    for(int axis=0; axis<3; axis++)
    {
        smin_orig[axis] = _mm_infty;
        smin_dir[axis] = _mm_infty;
        smax_orig[axis] = _mm_minus_infty;
        smax_dir[axis] = _mm_minus_infty;
    }
    for (int smd=first; smd<=last; smd++)
    {
      MANTA_UNROLL(3);
      for(int axis=0; axis<3; axis++)
        {
          smin_orig[axis] = min4(smin_orig[axis], srp.orig[axis][smd]);
          smax_orig[axis] = max4(smax_orig[axis], srp.orig[axis][smd]);
          smin_dir[axis] = min4(smin_dir[axis], srp.dir[axis][smd]);
          smax_dir[axis] = max4(smax_dir[axis], srp.dir[axis][smd]);
        }
    }

    FrustumInterval fi;
    fi.uvminmax_dir = set44(min4f(smin_dir[U]), min4f(smin_dir[V]), max4f(smax_dir[U]), max4f(smax_dir[V]));
    fi.uvminmax_invdir = oneOver(fi.uvminmax_dir);
    fi.uvminmax_orig = set44(min4f(smin_orig[U]), min4f(smin_orig[V]), max4f(smax_orig[U]), max4f(smax_orig[V]));

    #if DBGP
    cerr << "fi.uvminmax_orig "; simd_cerr(fi.uvminmax_orig);
    cerr << "fi.uvminmax_dir "; simd_cerr(fi.uvminmax_dir);
    #endif

    float komin = min4f(smin_orig[K]);
    float komax = max4f(smax_orig[K]);
    float kdmin = min4f(smin_dir[K]);
    float kdmax = max4f(smax_dir[K]);
    fi.kminmax_orig = set44(komin, komin, komax, komax);
    fi.kminmax_dir = set44(kdmin, kdmin, kdmax, kdmax);
    fi.kminmax_invdir = oneOver(fi.kminmax_dir);

    #if DBGP
    cerr << "fi.kminmax_orig "; simd_cerr(fi.kminmax_orig);
    cerr << "fi.kminmax_dir "; simd_cerr(fi.kminmax_dir);
    #endif

    unsigned int index_trace[octdata->get_max_depth() + 1];
    Vec3i cell(0,0,0);
    sse_traverse_node<K,U,V,DK>(srp, first, last, DU, DV, fi, cell, octdata->get_cap_depth(), 0, 0, index_trace);
}



template<char K, char U, char V, char DK>
void IsosurfaceOctreeVolume::sse_traverse_node(SSERayPacket& srp, char first, char last,
        char DU, char DV, const FrustumInterval& fi,
        Vec3i& cell, char stop_depth, char depth, unsigned int index, unsigned int index_trace[]) const
{
#if DBGP
    cerr << "octnode, depth " << (int)depth << ", index " << (int)index << ", first=" << (int)first << ",last=" << (int)last << "cell " << cell[0] << ", " << cell[1] << ", " << cell[2] << endl;
    cerr << "(with K=" << (int)(K) << ", U=" << (int)(U) << ", V=" << (int)(V) << ", DK=" << (int)(DK) << endl;
#endif

    OctNode& node = octdata->get_node(depth, index);
    int child_bit = octdata->get_child_bit_depth(depth);
    Vec3i child_cell;
    index_trace[depth] = index;
    Vector pcenter( static_cast<float>(cell[0] | child_bit),
                    static_cast<float>(cell[1] | child_bit),
                    static_cast<float>(cell[2] | child_bit));

    MANTA_UNROLL(2);
    for(int k=0; k<2; k++)
        {
        sse_t child_tkenter;
        sse_t child_tkexit;
        int tc_K;

                if (k)  //AFTER THE K MIDPLANE
                {
                        if (DK)
                        {
                child_tkenter = mul4(sub4(set4(pcenter[K]), fi.kminmax_orig), fi.kminmax_invdir);
                child_tkexit = mul4(sub4(set4(cell[K]+(child_bit<<1)), fi.kminmax_orig), fi.kminmax_invdir);
                                tc_K = axis_table[K];
                                child_cell[K] = cell[K] | child_bit;
                #if DBGP
                cerr << "kenter = pcenter[K] =" << pcenter[K] << endl;
                cerr << "kexit = cell[K] + depth_bit =" << cell[K]+(child_bit<<1) << endl;
                #endif
                        }
                        else
                        {
                child_tkenter = mul4(sub4(set4(pcenter[K]), fi.kminmax_orig), fi.kminmax_invdir);
                child_tkexit = mul4(sub4(set4(cell[K]), fi.kminmax_orig), fi.kminmax_invdir);
                                tc_K = 0;
                                child_cell[K] = cell[K];
                #if DBGP
                cerr << "kenter = pcenter[K] =" << pcenter[K] << endl;
                cerr << "kexit = cell[K] =" << cell[K] << endl;
                #endif
                        }

                }
                else    //BEFORE THE K MIDPLANE
                {
                        if (DK)
                        {
                child_tkenter = mul4(sub4(set4(cell[K]), fi.kminmax_orig), fi.kminmax_invdir);
                child_tkexit = mul4(sub4(set4(pcenter[K]), fi.kminmax_orig), fi.kminmax_invdir);
                                tc_K = 0;
                                child_cell[K] = cell[K];
                #if DBGP
                cerr << "kenter = cell[K] =" << cell[K] << endl;
                cerr << "kexit = pcenter[K] =" << pcenter[K] << endl;
                #endif
                        }
                        else
                        {
                child_tkenter = mul4(sub4(set4(cell[K]+(child_bit<<1)), fi.kminmax_orig), fi.kminmax_invdir);
                child_tkexit = mul4(sub4(set4(pcenter[K]), fi.kminmax_orig), fi.kminmax_invdir);
                                tc_K = axis_table[K];
                                child_cell[K] = cell[K] | child_bit;
                #if DBGP
                cerr << "kenter = cell[K]+depth_bit =" << cell[K]+(child_bit<<1) << endl;
                cerr << "kexit = pcenter[K] =" << pcenter[K] << endl;
                #endif
                        }
                }

        #if DBGP
        cerr << "child_tkenter "; simd_cerr(child_tkenter);
        cerr << "child_tkexit "; simd_cerr(child_tkexit);
        cerr << "child_pkenter[K] = "; simd_cerr( mul4(oneOver(fi.kminmax_invdir), add4(fi.kminmax_orig, child_tkenter)) );
        cerr << "child_pkexit[K] = "; simd_cerr( mul4(oneOver(fi.kminmax_invdir), add4(fi.kminmax_orig, child_tkexit)) );
        #endif

        //we have child_tkenter, child_tkexit.
        if (_mm_movemask_ps(cmp4_ge(child_tkexit, zero4()))==0)
        {
            #if DBGP
            cerr << "texit was negative; continuing." << endl;
            #endif
                        continue;
        }

        //find pkenter_uvminmax, pkexit[u], pkexit[v]
        //child_pkenter = dir*(orig/dir) + dir*t = dir(orig+t)
        const sse_t child_pkenter_uvminmax = add4(fi.uvminmax_orig, mul4(fi.uvminmax_dir, child_tkenter));
        const sse_t child_pkexit_uvminmax = add4(fi.uvminmax_orig, mul4(fi.uvminmax_dir, child_tkexit));

        #if DBGP
        cerr << "child_pkenter_uvminmax "; simd_cerr(child_pkenter_uvminmax);
        cerr << "child_pkexit_uvminmax "; simd_cerr(child_pkexit_uvminmax);
        #endif

        sse_union tmp_min, tmp_max;
        tmp_min.sse = min4(child_pkenter_uvminmax, child_pkexit_uvminmax);
        tmp_max.sse = max4(child_pkenter_uvminmax, child_pkexit_uvminmax);

        #if DBGP
        cerr << "tmp_min "; simd_cerr(tmp_min.sse);
        cerr << "tmp_max "; simd_cerr(tmp_max.sse);
        #endif

        const float umin = MIN(tmp_min.f[3], tmp_min.f[1]);
        const float vmin = MIN(tmp_min.f[2], tmp_min.f[0]);
        const float umax = MAX(tmp_max.f[3], tmp_max.f[1]);
        const float vmax = MAX(tmp_max.f[2], tmp_max.f[0]);
        sse_t sse_fuvminmax = set44(umin, vmin, umax, vmax);

        #if DBGP
        cerr << "sse_fuvminmax (before clamp) = "; simd_cerr(sse_fuvminmax);
        #endif

                sse_fuvminmax = sub4(sse_fuvminmax, set44(cell[U], cell[V], cell[U], cell[V]));
                sse_fuvminmax = mul4(sse_fuvminmax, set4(octdata->get_inv_child_bit_depth(depth)));
                sse_fuvminmax = max4(sse_fuvminmax, set44(0.0f, 0.0f, -9.9e9999f, -9.9e9999f));
                sse_fuvminmax = min4(sse_fuvminmax, set44(9.9e9999f, 9.9e9999f, 1.0f, 1.0f));

        #if DBGP
        cerr << "sse_fuvminmax (after clamp) = "; simd_cerr(sse_fuvminmax);
        #endif

        sse_int_union iuvminmax;

                //convert to int
                iuvminmax.ssei = _mm_cvttps_epi32(sse_fuvminmax);

        #if DBGP
        cerr << "iuvminmax = " << iuvminmax.i[0] << ", " << iuvminmax.i[1] << ", " << iuvminmax.i[2] << ", " << iuvminmax.i[3];
        cerr << endl << endl;
        #endif

                for(int u= (DU==1 ? iuvminmax.i[3] : iuvminmax.i[1]); (DU==1 ? u <= iuvminmax.i[1] : u >= iuvminmax.i[3]); u += DU)
                {
                        int tc_U;
                        if (u)
                        {
                                tc_U = axis_table[U];
                                child_cell[U] = cell[U] | child_bit;
                        }
                        else
                        {
                                tc_U = 0;
                                child_cell[U] = cell[U];
                        }

                        for(int v= (DV==1 ? iuvminmax.i[2] : iuvminmax.i[0]); (DV==1 ? v <= iuvminmax.i[0] : v >= iuvminmax.i[2]); v += DV)
                        {
                                int tc_V;
                                if (v)
                                {
                                        tc_V = axis_table[V];
                                        child_cell[V] = cell[V] | child_bit;
                                }
                                else
                                {
                                        tc_V = 0;
                                        child_cell[V] = cell[V];
                                }

                                int target_child = tc_K | tc_U | tc_V;

                if (octdata->get_isovalue() >= node.mins[target_child] && octdata->get_isovalue() <= node.maxs[target_child])
                {
                    Vector cmin(child_cell.data[0], child_cell.data[1], child_cell.data[2]);
                    Vector cmax(child_cell.data[0]+child_bit, child_cell.data[1]+child_bit, child_cell.data[2]+child_bit);
                    char newfirst = first_intersects(srp, first, last, cmin, cmax);

                    //cerr << "newfirst=" << (int)newfirst << ", last=" << (int)last << endl;

                    if (first <= last)
                    {
                        if (node.offsets[target_child]==-1)
                        {
                            sse_traverse_leaf<K,U,V,DK>(srp, newfirst, last, DU, DV, fi,
                                child_cell, stop_depth, depth+1, depth, node.values[target_child], child_cell, index_trace);
                        }
                        else
                        {
                            unsigned int child_idx = node.children_start + node.offsets[target_child];
                            if (depth == octdata->get_pre_cap_depth())  //cap
                            {
                                sse_traverse_cap<K,U,V,DK>(srp, newfirst, last, DU, DV, fi, child_cell,
                                    stop_depth, depth+1, child_idx, index_trace);
                            }
                            else
                            {
                                sse_traverse_node<K,U,V,DK>(srp, newfirst, last, DU, DV, fi, child_cell,
                                    stop_depth, depth+1, child_idx, index_trace);
                            }
                        }
                        if (srp.activeRays<=0)
                            return;
                    }
                }
            }
        }
    }
}

template<char K, char U, char V, char DK>
void IsosurfaceOctreeVolume::sse_traverse_leaf(SSERayPacket& srp, char first, char last, char DU, char DV,
            const FrustumInterval& fi, const Vec3i& cell, int stop_depth, int depth,
            int leaf_depth, ST leaf_value, const Vec3i& leaf_base_cell,
            unsigned int index_trace[]) const
{
    int child_bit = octdata->get_child_bit_depth(depth);
    int unsafe_zone = (child_bit<<1) - octdata->get_child_bit_depth(octdata->get_cap_depth());
    Vec3i child_cell;
    Vector pcenter( static_cast<float>(cell[0] | child_bit),
                    static_cast<float>(cell[1] | child_bit),
                    static_cast<float>(cell[2] | child_bit));

    MANTA_UNROLL(2);
    for(int k=0; k<2; k++)
        {
        sse_t child_tkenter;
        sse_t child_tkexit;
        int tc_K;

                if (k)  //AFTER THE K MIDPLANE
                {
                        if (DK)
                        {
                child_tkenter = mul4(sub4(set4(pcenter[K]), fi.kminmax_orig), fi.kminmax_invdir);
                child_tkexit = mul4(sub4(set4(cell[K]+(child_bit<<1)), fi.kminmax_orig), fi.kminmax_invdir);
                                tc_K = axis_table[K];
                                child_cell[K] = cell[K] | child_bit;
                #if DBGP
                cerr << "kenter = pcenter[K] =" << pcenter[K] << endl;
                cerr << "kexit = cell[K] + depth_bit =" << cell[K]+(child_bit<<1) << endl;
                #endif
                        }
                        else
                        {
                child_tkenter = mul4(sub4(set4(pcenter[K]), fi.kminmax_orig), fi.kminmax_invdir);
                child_tkexit = mul4(sub4(set4(cell[K]), fi.kminmax_orig), fi.kminmax_invdir);
                                tc_K = 0;
                                child_cell[K] = cell[K];
                #if DBGP
                cerr << "kenter = pcenter[K] =" << pcenter[K] << endl;
                cerr << "kexit = cell[K] =" << cell[K] << endl;
                #endif
                        }

                }
                else    //BEFORE THE K MIDPLANE
                {
                        if (DK)
                        {
                child_tkenter = mul4(sub4(set4(cell[K]), fi.kminmax_orig), fi.kminmax_invdir);
                child_tkexit = mul4(sub4(set4(pcenter[K]), fi.kminmax_orig), fi.kminmax_invdir);
                                tc_K = 0;
                                child_cell[K] = cell[K];
                #if DBGP
                cerr << "kenter = cell[K] =" << cell[K] << endl;
                cerr << "kexit = pcenter[K] =" << pcenter[K] << endl;
                #endif
                        }
                        else
                        {
                child_tkenter = mul4(sub4(set4(cell[K]+(child_bit<<1)), fi.kminmax_orig), fi.kminmax_invdir);
                child_tkexit = mul4(sub4(set4(pcenter[K]), fi.kminmax_orig), fi.kminmax_invdir);
                                tc_K = axis_table[K];
                                child_cell[K] = cell[K] | child_bit;
                #if DBGP
                cerr << "kenter = cell[K]+depth_bit =" << cell[K]+(child_bit<<1) << endl;
                cerr << "kexit = pcenter[K] =" << pcenter[K] << endl;
                #endif
                        }
                }

        #if DBGP
        cerr << "child_tkenter "; simd_cerr(child_tkenter);
        cerr << "child_tkexit "; simd_cerr(child_tkexit);
        cerr << "child_pkenter[K] = "; simd_cerr( mul4(oneOver(fi.kminmax_invdir), add4(fi.kminmax_orig, child_tkenter)) );
        cerr << "child_pkexit[K] = "; simd_cerr( mul4(oneOver(fi.kminmax_invdir), add4(fi.kminmax_orig, child_tkexit)) );
        #endif

        //we have child_tkenter, child_tkexit.
        if (_mm_movemask_ps(cmp4_ge(child_tkexit, zero4()))==0)
        {
            #if DBGP
            cerr << "texit was negative; continuing." << endl;
            #endif
                        continue;
        }

        //find pkenter_uvminmax, pkexit[u], pkexit[v]
        //child_pkenter = dir*(orig/dir) + dir*t = dir(orig+t)
        const sse_t child_pkenter_uvminmax = add4(fi.uvminmax_orig, mul4(fi.uvminmax_dir, child_tkenter));
        const sse_t child_pkexit_uvminmax = add4(fi.uvminmax_orig, mul4(fi.uvminmax_dir, child_tkexit));

        #if DBGP
        cerr << "child_pkenter_uvminmax "; simd_cerr(child_pkenter_uvminmax);
        cerr << "child_pkexit_uvminmax "; simd_cerr(child_pkexit_uvminmax);
        #endif

        sse_union tmp_min, tmp_max;
        tmp_min.sse = min4(child_pkenter_uvminmax, child_pkexit_uvminmax);
        tmp_max.sse = max4(child_pkenter_uvminmax, child_pkexit_uvminmax);

        #if DBGP
        cerr << "tmp_min "; simd_cerr(tmp_min.sse);
        cerr << "tmp_max "; simd_cerr(tmp_max.sse);
        #endif

        const float umin = MIN(tmp_min.f[3], tmp_min.f[1]);
        const float vmin = MIN(tmp_min.f[2], tmp_min.f[0]);
        const float umax = MAX(tmp_max.f[3], tmp_max.f[1]);
        const float vmax = MAX(tmp_max.f[2], tmp_max.f[0]);
        sse_t sse_fuvminmax = set44(umin, vmin, umax, vmax);

        #if DBGP
        cerr << "sse_fuvminmax (before clamp) = "; simd_cerr(sse_fuvminmax);
        #endif

                sse_fuvminmax = sub4(sse_fuvminmax, set44(cell[U], cell[V], cell[U], cell[V]));
                sse_fuvminmax = mul4(sse_fuvminmax, set4(octdata->get_inv_child_bit_depth(depth)));
                sse_fuvminmax = max4(sse_fuvminmax, set44(0.0f, 0.0f, -9.9e9999f, -9.9e9999f));
                sse_fuvminmax = min4(sse_fuvminmax, set44(9.9e9999f, 9.9e9999f, 1.0f, 1.0f));

        #if DBGP
        cerr << "sse_fuvminmax (after clamp) = "; simd_cerr(sse_fuvminmax);
        #endif

        sse_int_union iuvminmax;

                //convert to int
                iuvminmax.ssei = _mm_cvttps_epi32(sse_fuvminmax);

        #if DBGP
        cerr << "iuvminmax = " << iuvminmax.i[0] << ", " << iuvminmax.i[1] << ", " << iuvminmax.i[2] << ", " << iuvminmax.i[3];
        cerr << endl << endl;
        #endif

                for(int u= (DU==1 ? iuvminmax.i[3] : iuvminmax.i[1]); (DU==1 ? u <= iuvminmax.i[1] : u >= iuvminmax.i[3]); u += DU)
                {
                        int tc_U;
                        if (u)
                        {
                                tc_U = axis_table[U];
                                child_cell[U] = cell[U] | child_bit;
                        }
                        else
                        {
                                tc_U = 0;
                                child_cell[U] = cell[U];
                        }

                        for(int v= (DV==1 ? iuvminmax.i[2] : iuvminmax.i[0]); (DV==1 ? v <= iuvminmax.i[0] : v >= iuvminmax.i[2]); v += DV)
                        {
                                int tc_V;
                                if (v)
                                {
                                        tc_V = axis_table[V];
                                        child_cell[V] = cell[V] | child_bit;
                                }
                                else
                                {
                                        tc_V = 0;
                                        child_cell[V] = cell[V];
                                }

                                int target_child = tc_K | tc_U | tc_V;

                Vec3i local_child_cell = child_cell - leaf_base_cell;
                if (local_child_cell.data[0] & unsafe_zone || local_child_cell.data[1] & unsafe_zone || local_child_cell.data[2] & unsafe_zone)
                {
                    if (depth == stop_depth)
                    {
                        sse_t child_tenter[RayPacket::SSE_MaxSize];
                        sse_t child_texit[RayPacket::SSE_MaxSize];
                        sse_t hitmask[RayPacket::SSE_MaxSize];
                        Vector cmin(child_cell.data[0], child_cell.data[1], child_cell.data[2]);
                        Vector cmax(child_cell.data[0] + child_bit, child_cell.data[1] + child_bit, child_cell.data[2] + child_bit);
                        char newfirst, newlast;
                        intersect_cap_octant(srp, first, last, newfirst, newlast, cmin, cmax, child_tenter, child_texit, hitmask);

                        if (newfirst > newlast)
                            continue;
#ifdef USE_OCTREE_DATA
                        //use octree data
                        float rho[2][2][2];
                        ST min_rho, max_rho, this_rho;
                        min_rho = max_rho = this_rho = leaf_value;
                        rho[0][0][0] = static_cast<float>(this_rho);
                        Vec3i offset(0,0,child_bit);

                        //0,0,1
                        if (target_child & 1)
                        {
                            this_rho = octdata->lookup_neighbor<0,0,1>(child_cell, offset, stop_depth, leaf_depth, index_trace);
                            min_rho = MIN(min_rho, this_rho);
                            max_rho = MAX(max_rho, this_rho);
                        }
                        else
                            this_rho = leaf_value;
                        rho[0][0][1] = static_cast<float>(this_rho);

                        //0,1,1
                        offset.data[1] = child_bit;
                        if (target_child & 3)
                        {
                            this_rho = octdata->lookup_neighbor<0,1,1>(child_cell, offset, stop_depth, leaf_depth, index_trace);
                            min_rho = MIN(min_rho, this_rho);
                            max_rho = MAX(max_rho, this_rho);
                        }
                        else
                            this_rho = leaf_value;
                        rho[0][1][1] = static_cast<float>(this_rho);

                        //1,1,1
                        offset.data[0] = child_bit;
                        if (target_child & 7)
                        {
                            this_rho = octdata->lookup_neighbor<1,1,1>(child_cell, offset, stop_depth, leaf_depth, index_trace);
                            min_rho = MIN(min_rho, this_rho);
                            max_rho = MAX(max_rho, this_rho);
                        }
                        else
                            this_rho = leaf_value;
                        rho[1][1][1] = static_cast<float>(this_rho);

                        //1,1,0
                        offset.data[2] = 0;
                        if (target_child & 6)
                        {
                            this_rho = octdata->lookup_neighbor<1,1,0>(child_cell, offset, stop_depth, leaf_depth, index_trace);
                            min_rho = MIN(min_rho, this_rho);
                            max_rho = MAX(max_rho, this_rho);
                        }
                        else
                            this_rho = leaf_value;
                        rho[1][1][0] = static_cast<float>(this_rho);

                        //1,0,0
                        offset.data[1] = 0;
                        if (target_child & 4)
                        {
                            this_rho = octdata->lookup_neighbor<1,0,0>(child_cell, offset, stop_depth, leaf_depth, index_trace);
                            min_rho = MIN(min_rho, this_rho);
                            max_rho = MAX(max_rho, this_rho);
                        }
                        else
                            this_rho = leaf_value;
                        rho[1][0][0] = static_cast<float>(this_rho);

                        //1,0,1
                        offset.data[2] = child_bit;
                        if (target_child & 5)
                        {
                            this_rho = octdata->lookup_neighbor<1,0,1>(child_cell, offset, stop_depth, leaf_depth, index_trace);
                            min_rho = MIN(min_rho, this_rho);
                            max_rho = MAX(max_rho, this_rho);
                        }
                        else
                            this_rho = leaf_value;
                        rho[1][0][1] = static_cast<float>(this_rho);

                        //0,1,0
                        offset.data[0] = 0;
                        offset.data[1] = child_bit;
                        offset.data[2] = 0;
                        if (target_child & 2)
                        {
                            this_rho = octdata->lookup_neighbor<0,1,0>(child_cell, offset, stop_depth, leaf_depth, index_trace);
                            min_rho = MIN(min_rho, this_rho);
                            max_rho = MAX(max_rho, this_rho);
                        }
                        else
                            this_rho = leaf_value;
                        rho[0][1][0] = static_cast<float>(this_rho);
#else
                        //use original grid data
                        float rho[2][2][2];
                        ST min_rho, max_rho;
#define MYDATA octdata->indata  //toggle this to octdata if you want to test pure point location (no neighbor finding)
                        min_rho = max_rho = lookup_safe(MYDATA, child_cell.data[0], child_cell.data[1], child_cell.data[2]);
                        rho[0][0][0] = static_cast<float>(min_rho);
                        for(int c=1; c<8; c++)
                        {
                            Vec3i offset((c&4)!=0, (c&2)!=0, c&1);
                            Vec3i neighboridx = child_cell + offset;
                            ST this_rho = lookup_safe(MYDATA, neighboridx.data[0], neighboridx.data[1], neighboridx.data[2]);
                            rho[offset.data[0]][offset.data[1]][offset.data[2]] = static_cast<float>(this_rho);
                            min_rho = MIN(this_rho, min_rho);
                            max_rho = MAX(this_rho, max_rho);
                        }
#endif

                        if (octdata->get_isovalue() >= min_rho && octdata->get_isovalue() <= max_rho)
                        {
                            IsosurfaceImplicit::sse_intersect(srp, newfirst, newlast, cmin, cmax, rho,
                                octdata->get_isovalue(), child_tenter, child_texit, hitmask, this, PrimitiveCommon::getMaterial());
                        }
                    }
                    else    //not at stop depth
                    {
                        Vector cmin(child_cell.data[0], child_cell.data[1], child_cell.data[2]);
                        Vector cmax(child_cell.data[0]+child_bit, child_cell.data[1]+child_bit, child_cell.data[2]+child_bit);
                        char newfirst = first_intersects(srp, first, last, cmin, cmax);

                        sse_traverse_leaf<K,U,V,DK>(srp, newfirst, last, DU, DV, fi,
                            child_cell, stop_depth, depth+1, leaf_depth, leaf_value, leaf_base_cell, index_trace);

                    }
                    if (srp.activeRays<=0)
                        return;
                }
            }
        }
    }
}

template<char K, char U, char V, char DK>
void IsosurfaceOctreeVolume::sse_traverse_cap(SSERayPacket& srp, char first, char last,
        char DU, char DV, const FrustumInterval& fi,
        Vec3i& cell, char stop_depth, char depth, unsigned int index, unsigned int index_trace[]) const
{
#if DBGP
    cerr << "octcap " << index << ", first=" << (int)first << ",last=" << (int)last << endl;
#endif
    OctCap& cap = octdata->get_cap(index);
    int child_bit = octdata->get_child_bit_depth(depth);
    Vec3i child_cell;
    index_trace[depth] = index;
    Vector pcenter( static_cast<float>(cell[0] | child_bit),
                    static_cast<float>(cell[1] | child_bit),
                    static_cast<float>(cell[2] | child_bit));

    MANTA_UNROLL(2);
    for(int k=0; k<2; k++)
        {
        sse_t child_tkenter;
        sse_t child_tkexit;
        int tc_K;

                if (k)  //AFTER THE K MIDPLANE
                {
                        if (DK)
                        {
                child_tkenter = mul4(sub4(set4(pcenter[K]), fi.kminmax_orig), fi.kminmax_invdir);
                child_tkexit = mul4(sub4(set4(cell[K]+(child_bit<<1)), fi.kminmax_orig), fi.kminmax_invdir);
                                tc_K = axis_table[K];
                                child_cell[K] = cell[K] | child_bit;
                #if DBGP
                cerr << "kenter = pcenter[K] =" << pcenter[K] << endl;
                cerr << "kexit = cell[K] + depth_bit =" << cell[K]+(child_bit<<1) << endl;
                #endif
                        }
                        else
                        {
                child_tkenter = mul4(sub4(set4(pcenter[K]), fi.kminmax_orig), fi.kminmax_invdir);
                child_tkexit = mul4(sub4(set4(cell[K]), fi.kminmax_orig), fi.kminmax_invdir);
                                tc_K = 0;
                                child_cell[K] = cell[K];
                #if DBGP
                cerr << "kenter = pcenter[K] =" << pcenter[K] << endl;
                cerr << "kexit = cell[K] =" << cell[K] << endl;
                #endif
                        }

                }
                else    //BEFORE THE K MIDPLANE
                {
                        if (DK)
                        {
                child_tkenter = mul4(sub4(set4(cell[K]), fi.kminmax_orig), fi.kminmax_invdir);
                child_tkexit = mul4(sub4(set4(pcenter[K]), fi.kminmax_orig), fi.kminmax_invdir);
                                tc_K = 0;
                                child_cell[K] = cell[K];
                #if DBGP
                cerr << "kenter = cell[K] =" << cell[K] << endl;
                cerr << "kexit = pcenter[K] =" << pcenter[K] << endl;
                #endif
                        }
                        else
                        {
                child_tkenter = mul4(sub4(set4(cell[K]+(child_bit<<1)), fi.kminmax_orig), fi.kminmax_invdir);
                child_tkexit = mul4(sub4(set4(pcenter[K]), fi.kminmax_orig), fi.kminmax_invdir);
                                tc_K = axis_table[K];
                                child_cell[K] = cell[K] | child_bit;
                #if DBGP
                cerr << "kenter = cell[K]+depth_bit =" << cell[K]+(child_bit<<1) << endl;
                cerr << "kexit = pcenter[K] =" << pcenter[K] << endl;
                #endif
                        }
                }

        #if DBGP
        cerr << "child_tkenter "; simd_cerr(child_tkenter);
        cerr << "child_tkexit "; simd_cerr(child_tkexit);
        cerr << "child_pkenter[K] = "; simd_cerr( mul4(oneOver(fi.kminmax_invdir), add4(fi.kminmax_orig, child_tkenter)) );
        cerr << "child_pkexit[K] = "; simd_cerr( mul4(oneOver(fi.kminmax_invdir), add4(fi.kminmax_orig, child_tkexit)) );
        #endif

        //we have child_tkenter, child_tkexit.
        if (_mm_movemask_ps(cmp4_ge(child_tkexit, zero4()))==0)
        {
            #if DBGP
            cerr << "texit was negative; continuing." << endl;
            #endif
                        continue;
        }

        //find pkenter_uvminmax, pkexit[u], pkexit[v]
        //child_pkenter = dir*(orig/dir) + dir*t = dir(orig+t)
        const sse_t child_pkenter_uvminmax = add4(fi.uvminmax_orig, mul4(fi.uvminmax_dir, child_tkenter));
        const sse_t child_pkexit_uvminmax = add4(fi.uvminmax_orig, mul4(fi.uvminmax_dir, child_tkexit));

        #if DBGP
        cerr << "child_pkenter_uvminmax "; simd_cerr(child_pkenter_uvminmax);
        cerr << "child_pkexit_uvminmax "; simd_cerr(child_pkexit_uvminmax);
        #endif

        sse_union tmp_min, tmp_max;
        tmp_min.sse = min4(child_pkenter_uvminmax, child_pkexit_uvminmax);
        tmp_max.sse = max4(child_pkenter_uvminmax, child_pkexit_uvminmax);

        #if DBGP
        cerr << "tmp_min "; simd_cerr(tmp_min.sse);
        cerr << "tmp_max "; simd_cerr(tmp_max.sse);
        #endif

        const float umin = MIN(tmp_min.f[3], tmp_min.f[1]);
        const float vmin = MIN(tmp_min.f[2], tmp_min.f[0]);
        const float umax = MAX(tmp_max.f[3], tmp_max.f[1]);
        const float vmax = MAX(tmp_max.f[2], tmp_max.f[0]);
        sse_t sse_fuvminmax = set44(umin, vmin, umax, vmax);

        #if DBGP
        cerr << "sse_fuvminmax (before clamp) = "; simd_cerr(sse_fuvminmax);
        #endif

                sse_fuvminmax = sub4(sse_fuvminmax, set44(cell[U], cell[V], cell[U], cell[V]));
                sse_fuvminmax = mul4(sse_fuvminmax, set4(octdata->get_inv_child_bit_depth(depth)));
                sse_fuvminmax = max4(sse_fuvminmax, set44(0.0f, 0.0f, -9.9e9999f, -9.9e9999f));
                sse_fuvminmax = min4(sse_fuvminmax, set44(9.9e9999f, 9.9e9999f, 1.0f, 1.0f));

        #if DBGP
        cerr << "sse_fuvminmax (after clamp) = "; simd_cerr(sse_fuvminmax);
        #endif

        sse_int_union iuvminmax;

                //convert to int
                iuvminmax.ssei = _mm_cvttps_epi32(sse_fuvminmax);

        #if DBGP
        cerr << "iuvminmax = " << iuvminmax.i[0] << ", " << iuvminmax.i[1] << ", " << iuvminmax.i[2] << ", " << iuvminmax.i[3];
        cerr << endl << endl;
        #endif

                for(int u= (DU==1 ? iuvminmax.i[3] : iuvminmax.i[1]); (DU==1 ? u <= iuvminmax.i[1] : u >= iuvminmax.i[3]); u += DU)
                {
                        int tc_U;
                        if (u)
                        {
                                tc_U = axis_table[U];
                                child_cell[U] = cell[U] | child_bit;
                        }
                        else
                        {
                                tc_U = 0;
                                child_cell[U] = cell[U];
                        }

                        for(int v= (DV==1 ? iuvminmax.i[2] : iuvminmax.i[0]); (DV==1 ? v <= iuvminmax.i[0] : v >= iuvminmax.i[2]); v += DV)
                        {
                                int tc_V;
                                if (v)
                                {
                                        tc_V = axis_table[V];
                                        child_cell[V] = cell[V] | child_bit;
                                }
                                else
                                {
                                        tc_V = 0;
                                        child_cell[V] = cell[V];
                                }

                                int target_child = tc_K | tc_U | tc_V;

                sse_t child_tenter[RayPacket::SSE_MaxSize];
                sse_t child_texit[RayPacket::SSE_MaxSize];
                sse_t hitmask[RayPacket::SSE_MaxSize];
                Vector cmin(child_cell.data[0], child_cell.data[1], child_cell.data[2]);
                Vector cmax(child_cell.data[0]+1, child_cell.data[1]+1, child_cell.data[2]+1);
                char newfirst, newlast;
                intersect_cap_octant(srp, first, last, newfirst, newlast, cmin, cmax, child_tenter, child_texit, hitmask);
#ifndef IOV_BOX_CELLS
                if (newfirst > newlast)
                    continue;

#ifdef USE_OCTREE_DATA
                float rho[2][2][2];
                ST min_rho, max_rho, this_rho;
                min_rho = max_rho = this_rho = cap.values[target_child];
                rho[0][0][0] = static_cast<float>(this_rho);
                int prev_depth = depth-1;
                Vec3i offset(0,0,1);
                octvol_fill_cell(cap, 1);
#else
                //use original grid data
                float rho[2][2][2];
                ST min_rho, max_rho;
#define MYDATA octdata->indata
                min_rho = max_rho = lookup_safe(MYDATA, child_cell.data[0], child_cell.data[1], child_cell.data[2]);
                rho[0][0][0] = static_cast<float>(min_rho);
                for(int c=1; c<8; c++)
                {
                    Vec3i offset((c&4)!=0, (c&2)!=0, c&1);
                    Vec3i neighboridx = child_cell + offset;
                    ST this_rho = lookup_safe(MYDATA, neighboridx.data[0], neighboridx.data[1], neighboridx.data[2]);
                    rho[offset.data[0]][offset.data[1]][offset.data[2]] = static_cast<float>(this_rho);
                    min_rho = MIN(this_rho, min_rho);
                    max_rho = MAX(this_rho, max_rho);
                }
#endif

                if (octdata->get_isovalue() >= min_rho && octdata->get_isovalue() <= max_rho)
                {
                    //cerr << "in cap " << (unsigned long)(&cap) << ", octant " << target_child << endl;
                    IsosurfaceImplicit::sse_intersect(srp, newfirst, newlast, cmin, cmax, rho,
                        octdata->get_isovalue(), child_tenter, child_texit, hitmask, this, PrimitiveCommon::getMaterial());
                    if (srp.activeRays<=0)
                        return;
                }
#endif
            }
        }
    }
}

#endif  //#ifdef MANTA_SSE
