
#ifndef _MANTA_ISOSURFACEGRIDVOLUME_H_
#define _MANTA_ISOSURFACEGRIDVOLUME_H_

#include <Model/Primitives/PrimitiveCommon.h>
#include <Core/Containers/Array3.h>
#include <Model/MiscObjects/BrickArray3.h>
#include <Core/Geometry/Vector.h>
#include <Core/Geometry/BBox.h>
#include <Core/Color/Color.h>
#include <Interface/Texture.h>
#include <Model/Primitives/OctreeVolume.h>

//generate the octree data
//#define TEST_OCTDATA

//test the octree data
//#define USE_OCTREE_DATA
//#define USE_OTD_NEIGHBOR_FIND	

typedef unsigned char ST;

namespace Manta
{

    class IsosurfaceGridVolume : public PrimitiveCommon
    {
    public:
        struct VMCell 
        {
          ST max;
          ST min;
        };

        BBox bounds;
        Vector datadiag;
        Vector hierdiag;
        Vector ihierdiag;
        Vector sdiag;
        int nx,ny,nz;
            
#ifdef TEST_OCTDATA
        OctreeVolume octdata;
#endif
            
        BrickArray3<ST> blockdata;
        ST datamin, datamax;
        int mc_depth;
        int* xsize;
        int* ysize;
        int* zsize;
        double* ixsize;
        double* iysize;
        double* izsize;
        ST isoval;

        BrickArray3<VMCell>* macrocells;
        char* filebase;

        IsosurfaceGridVolume(char* filebase, int _mc_depth, double isovalue, Material* _matl);
        ~IsosurfaceGridVolume();
        
        void preprocess( PreprocessContext const &context );
        void computeBounds( PreprocessContext const &context, BBox &box ) const;
        void intersect( RenderContext const &context, RayPacket &rays ) const;
        void computeNormal(const RenderContext& context, RayPacket& rays) const;
        
        BBox getBounds() const;

        inline double get_isovalue() const
        {
            return (double)isoval;
        }
            
        inline void set_isovalue(double val)
        {
            isoval = (ST)val;
        }
        
        inline void increase_isovalue()
        {
            if (isoval < std::numeric_limits<ST>::max())
                isoval++;
        }
        
        inline void decrease_isovalue()
        {
            if (isoval > std::numeric_limits<ST>::min())
                isoval--;
        }		
              
    private:

        inline void single_intersect(RayPacket& rays, int which_one) const;

        void isect(RayPacket& rays, int which_one, 
               const Vector& orig, const Vector& dir, const Vector& inv_dir, 
               int depth, float t,
               float dtdx, float dtdy, float dtdz,
               float next_x, float next_y, float next_z,
               int ix, int iy, int iz,
               int dix_dx, int diy_dy, int diz_dz,
               int startx, int starty, int startz,
               const Vector& cellcorner, const Vector& celldir) const;

        void calc_mcell(int depth, int startx, int starty, int startz, VMCell& mcell);
    };

};

#endif
