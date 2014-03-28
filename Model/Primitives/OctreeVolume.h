
/*
*  OctreeVolume.h : A multiresolution octree data
*   wrapper for scalar volumes, such as Array3.
*
*  Written by:
*   Aaron M Knoll
*   Department of Computer Science
*   University of Utah
*   October 2005
*
*  Copyright (C) 2005 SCI Group
*/

#ifndef _MANTA_OCTREEVOLUME_H_
#define _MANTA_OCTREEVOLUME_H_

#define lookup_safe(data, x, y, z) \
(x > 0 && x < data.dim1() && y > 0 \
&& y < data.dim2() && z > 0 && z < data.dim3()) ? data(x,y,z) : 0 \
\


//AARONBAD - this is already defined in Abe's evil.h. 
//   But it's not that evil, really...
//   maybe should be in Core/Math?
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

//enable this ONLY when testing octree data and original data side by side
//#define TEST_INDATA

//enable dynamic multires via the "stop_depth"
//#define OCTVOL_DYNAMIC_MULTIRES

#include <Core/Geometry/BBox.h>
#include <Core/Containers/Array3.h>
#include <Core/Geometry/vecdefs.h>
#include <Core/Geometry/Vector.h>
#include <stdlib.h>
#include <fstream>
#include <fcntl.h>
#include <math.h>

using namespace std;

namespace Manta{

    typedef unsigned char ST;

    union SO
    {
        ST scalar;
        unsigned char offset;
    };

    //XXX: OctCap isn't really a leaf.
    // rather, it's a "bottom node" which by nature CANNOT have children; it MUST have scalar values
    struct OctCap
    {
        ST values[8];
    };
	
    struct OctNode
    {
        char offsets[8];	//-1 if leaf; 0-7 if child exists
	ST mins[8];
	ST maxs[8];
	unsigned int children_start;
	ST values[8];
    };

    //node used in build
    struct BuildNode
    {
        char offsets[8];
	ST values[8];
	unsigned int myStart;
        unsigned int children_start;
	ST expected_value;
	ST min, max;
	unsigned char myIndex;
        bool keepMe;
        bool isLeaf;
    };

    //class ST = the scalar value type (e.g. unsigned char)

    class OctreeVolume
    {

    public:
		// depth of the caps minus 1; hence always == max_depth-2
		// traversals often check this value
		int pre_cap_depth;
	
        //the maximum depth of the octree, finest resolution
        int max_depth;
		
	//the depth of caps; always == max_depth-2
	int cap_depth;
        
        //the width of the kernel that is allegedly reading the octree data.
        //  This determines the ghosting overlap, if any, of the min/max values
        //  Ex: for isosurface rendering with forward differences, use width = 2
        //  For full central differences, use width=3
        int kernel_width;
        
        //number of multiresolution levels. 
        int mres_levels;
        
        //total number of timesteps in the data
        int num_timesteps;
        
        int* child_bit_depth;
        float* inv_child_bit_depth;
        int max_depth_bit;
        float inv_max_depth_bit;
        
        ST datamin, datamax;
        
        struct Timestep
        {
            OctNode** nodes;	//interior nodes, e.g. from depth = 0 to max_depth-2
            OctCap* caps;		//caps *would* be leaves in a full octree. 
            unsigned int* num_nodes;
            unsigned int num_caps;
        };
        
        Timestep* steps;
        
        BBox bounds;                //=={pmin,pmax}
        Vector dims;                // = the volume dimensions in float (same as diag)
        Vector padded_dims;         //the rounded-up highest power of 2 of any dimension
        Vec3i int_dims;             //the actual dims, padded to the lowest power of 2 that contains the largest one.
        
        /* THESE VARS SHOULD BE IN OCTISOVOLUME */
        int multires_level;
        int current_timestep;
        ST isoval;
        bool use_adaptive_res;		
        /* END VARS SHOULD BE IN OCTISOVOLUME */
		
#ifdef TEST_INDATA   
        Array3<ST> indata;
#endif    
        
        inline float getMaxTime() const
        {
            return num_timesteps - 1;
        }
        
        inline bool setTime(const int t)
        {
            if (t >= num_timesteps || t < 0)
                return false;
            
            current_timestep = t;
            return true;
        }
        
        inline void increment_timestep()
        {
            if (current_timestep < num_timesteps - 1)
                current_timestep++;
        }
        
        inline int get_timestep() const
        {
            return current_timestep;
        }
        
        inline void decrement_isovalue()
        {
            if (isoval > datamin+1)
                isoval--;
        }
        inline void increment_isovalue()
        {
            if (isoval < datamax-1)
                isoval++;
        }
        
        inline void set_isovalue(double isovalue)
        {
            isoval = (ST)isovalue;
            isoval = MAX(datamin, isoval);
            isoval = MIN(datamax, isoval);
        }
        
        inline int get_isovalue() const
        {
            return isoval;
        }
        
        inline int get_multires_level() const
        {
            return multires_level;
        }
        
        inline void decrease_resolution()
        {
            if (multires_level < mres_levels)
                multires_level++;
        }
        inline void increase_resolution()
        {
            if (multires_level > 0)
                multires_level--;
        }
        
        inline const BBox& get_bounds() const
        {
            return bounds;
        }
                
        inline int get_child_bit_depth(int d) const
        {
            return child_bit_depth[d];
        }
        
        inline float get_inv_child_bit_depth(int d) const
        {
            return inv_child_bit_depth[d];
        }
        
        inline int get_max_depth() const
        {
            return max_depth;
        }
		
	inline int get_cap_depth() const
       	{
       	    return cap_depth;
       	}
		
	inline int get_pre_cap_depth() const
       	{
       	    return pre_cap_depth;
	}

        OctreeVolume();

        OctreeVolume(char* filebase);
        
        /*!
            Multiple timestep constructor
         */
        OctreeVolume(char* filebase, 
                    int startTimestep, 
                    int endTimestep,
                    double variance_threshold, 
                    int kernelWidth, 
                    ST isomin, 
                    ST isomax);

        ~OctreeVolume();

        inline OctNode& get_node(int depth, unsigned int index) const
        {
            return steps[current_timestep].nodes[depth][index];
        }
        
        inline OctCap& get_cap(unsigned int index) const
        {
            return steps[current_timestep].caps[index];
        }
        
        inline ST operator()(Vec3i& v) const
        {
            return lookup_node(v, cap_depth, 0, 0);
        }
        
        inline ST operator()(int d1, int d2, int d3) const
        {
            //do a lookup in octree data, on [0, 1 << max_depth]
            Vec3i p(d1, d2, d3);
            return lookup_node(p, cap_depth, 0, 0);
        }
        
        inline ST locate_trace(int d1, int d2, int d3, unsigned int* index_trace) const
        {
            //do a lookup in octree data
            Vec3i p(d1, d2, d3);
            return lookup_node_trace(p, cap_depth, 0, 0, index_trace);
        }
        
        inline ST lookup_node_trace(Vec3i& p, int stop_depth, int depth, unsigned int index, unsigned int* index_trace) const
        {
            for(;;)
            {
                OctNode& node = steps[current_timestep].nodes[depth][index];
                index_trace[depth] = index;
				                
                int child_bit = child_bit_depth[depth];
                int target_child = (((p.data[0] & child_bit)!=0) << 2) | (((p.data[1] & child_bit)!=0) << 1) | ((p.data[2] & child_bit)!=0);
            
				#ifdef OCTVOL_DYNAMIC_MULTIRES				
				if (depth == stop_depth)
					return node.values[target_child];
				#endif				

                if (node.offsets[target_child] == -1)
                    return node.values[target_child];
                if (depth == pre_cap_depth)
                {
                    index = node.children_start + node.offsets[target_child];
                    index_trace[depth+1] = index;
                    target_child = ((p.data[0] & 1) << 2) | ((p.data[1] & 1) << 1) | (p.data[2] & 1);
                    return steps[current_timestep].caps[index].values[target_child];
                }

                index = node.children_start + node.offsets[target_child];
                depth++;
            }
        }
        
        inline ST lookup_node(Vec3i& p, int stop_depth, int depth, unsigned int index) const
        {
            for(;;)
            {
                OctNode& node = steps[current_timestep].nodes[depth][index];
        
                int child_bit = child_bit_depth[depth];
                int target_child = (((p.data[0] & child_bit)!=0) << 2) | (((p.data[1] & child_bit)!=0) << 1) | ((p.data[2] & child_bit)!=0);

				#ifdef OCTVOL_DYNAMIC_MULTIRES				
				if (depth == stop_depth)
					return node.values[target_child];
				#endif				
                
                if (node.offsets[target_child] == -1)
                    return node.values[target_child];
                if (depth == pre_cap_depth)
                {
                    index = node.children_start + node.offsets[target_child];
                    int target_child = ((p.data[0] & 1) << 
                    2) | ((p.data[1] & 1) << 1) | (p.data[2] & 1);
                    return steps[current_timestep].caps[index].values[target_child];
                }
                
                index = node.children_start + node.offsets[target_child];
                depth++;
            }
        }
        
        template<bool CHECK_X, bool CHECK_Y, bool CHECK_Z>
        inline ST lookup_neighbor(Vec3i& cell, Vec3i& offset, int stop_depth, int depth, unsigned int* index_trace) const
        {
            Vec3i target_neighbor = cell + offset;
            int child_bit;
            
            //recurse up the tree until we find a parent that contains both cell and target_neighbor
            for(int up = depth; up >= 0; up--)
            {	
                child_bit = child_bit_depth[up];
                
                if (CHECK_X && CHECK_Y && CHECK_Z)
                {
                    if ( ((target_neighbor.data[0] & child_bit) == (cell.data[0] & child_bit)) &&
                         ((target_neighbor.data[1] & child_bit) == (cell.data[1] & child_bit)) &&
                         ((target_neighbor.data[2] & child_bit) == (cell.data[2] & child_bit)) )
                        return lookup_node(target_neighbor, stop_depth, up, index_trace[up]);
                }
                else if (CHECK_X && CHECK_Y)
                {
                    if ( ((target_neighbor.data[0] & child_bit) == (cell.data[0] & child_bit)) &&
                        ((target_neighbor.data[1] & child_bit) == (cell.data[1] & child_bit)) )
                        return lookup_node(target_neighbor, stop_depth, up, index_trace[up]);				
                }
                else if (CHECK_X && CHECK_Z)
                {
                    if ( ((target_neighbor.data[0] & child_bit) == (cell.data[0] & child_bit)) &&
                        ((target_neighbor.data[2] & child_bit) == (cell.data[2] & child_bit)) )
                        return lookup_node(target_neighbor, stop_depth, up, index_trace[up]);				
                }
                else if (CHECK_Y && CHECK_Z)
                {
                    if ( ((target_neighbor.data[1] & child_bit) == (cell.data[1] & child_bit)) &&
                        ((target_neighbor.data[2] & child_bit) == (cell.data[2] & child_bit)) )
                        return lookup_node(target_neighbor, stop_depth, up, index_trace[up]);				
                }
                else if (CHECK_X)
                {
                    if ((target_neighbor.data[0] & child_bit) == (cell.data[0] & child_bit))
                        return lookup_node(target_neighbor, stop_depth, up, index_trace[up]);
                }
                else if (CHECK_Y)
                {
                    if ((target_neighbor.data[1] & child_bit) == (cell.data[1] & child_bit))
                        return lookup_node(target_neighbor, stop_depth, up, index_trace[up]);
                }
                else if (CHECK_Z)
                {
                    if ((target_neighbor.data[2] & child_bit) == (cell.data[2] & child_bit))
                        return lookup_node(target_neighbor, stop_depth, up, index_trace[up]);
                }
            }
            
            child_bit = max_depth_bit;
            if (CHECK_X && CHECK_Y && CHECK_Z)
            {
                if ( ((target_neighbor.data[0] & child_bit) == (cell.data[0] & child_bit)) &&
                    ((target_neighbor.data[1] & child_bit) == (cell.data[1] & child_bit)) &&
                    ((target_neighbor.data[2] & child_bit) == (cell.data[2] & child_bit)) )
                    return lookup_node(target_neighbor, stop_depth, 0, 0);
            }
            else if (CHECK_X && CHECK_Y)
            {
                if ( ((target_neighbor.data[0] & child_bit) == (cell.data[0] & child_bit)) &&
                    ((target_neighbor.data[1] & child_bit) == (cell.data[1] & child_bit)) )
                    return lookup_node(target_neighbor, stop_depth, 0, 0);			
            }
            else if (CHECK_X && CHECK_Z)
            {
                if ( ((target_neighbor.data[0] & child_bit) == (cell.data[0] & child_bit)) &&
                    ((target_neighbor.data[2] & child_bit) == (cell.data[2] & child_bit)) )
                    return lookup_node(target_neighbor, stop_depth, 0, 0);			
            }
            else if (CHECK_Y && CHECK_Z)
            {
                if ( ((target_neighbor.data[1] & child_bit) == (cell.data[1] & child_bit)) &&
                    ((target_neighbor.data[2] & child_bit) == (cell.data[2] & child_bit)) )
                    return lookup_node(target_neighbor, stop_depth, 0, 0);				
            }
            else if (CHECK_X)
            {
                if ((target_neighbor.data[0] & child_bit) == (cell.data[0] & child_bit))
                    return lookup_node(target_neighbor, stop_depth, 0, 0);
            }
            else if (CHECK_Y)
            {
                if ((target_neighbor.data[1] & child_bit) == (cell.data[1] & child_bit))
                    return lookup_node(target_neighbor, stop_depth, 0, 0);
            }
            else if (CHECK_Z)
            {
                if ((target_neighbor.data[2] & child_bit) == (cell.data[2] & child_bit))
                    return lookup_node(target_neighbor, stop_depth, 0, 0);
            }
                    
            return 0;
        }	
        
        /*!
            The octree data building routine. 
            AARONBAD - this is highly unoptimized. While the resulting data is fine, 
            the build process could be made faster by just unrolling loops. Parallelizing it would also be
            desirable.
        */
        void make_from_single_res_data(Array3<ST>& originalData, 
                                        int ts, 
                                        double variance_threshold, 
                                        int kernelWidth, 
                                        ST isomin = 0, 
                                        ST isomax = 0);

        bool read_file(char* filebase);
        bool write_file(char* filebase);
        	
    };
    
};

#endif
