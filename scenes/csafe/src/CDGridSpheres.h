//! Filename: CDGridSpheres.h
/*!
  same as Gridspheres, but with small changes to incorporate csafe demo.  TODO: incorporate changes into main Gridspheres file
*/

#ifndef Manta_Model_CDGridSpheres_h
#define Manta_Model_CDGridSpheres_h

#include <Core/Color/Color.h>
#include <Core/Geometry/BBox.h>
#include <Interface/RayPacket.h>
#include <Interface/TexCoordMapper.h>
#include <Interface/Texture.h>
#include <Model/Primitives/PrimitiveCommon.h>
#include <Model/Materials/LitMaterial.h>
//#include <Model/Materials/AmbientOcclusion.h>
#include <Model/Materials/Volume.h>
#include <Core/Thread/Barrier.h>
#include <Core/Thread/Mutex.h>

#include <iostream>


#define USE_OPTIMIZED_FCNS

namespace Manta {
  class RegularColorMap;
	
  class CDGridSpheres : public PrimitiveCommon, public LitMaterial,
    public TexCoordMapper, public Texture<Color>
    {
    public:
      //! constructor
      /*! \param ncells affects the grenularity of cells, higher numbers mean
      // more memory
      // \param xindex is the index of the x values (y and z should be after)
      */
      CDGridSpheres(float* spheres, int nspheres, int nvars, int ncells, int depth,
                    Real radius, int ridx, RGBAColorMap* cmap, int cidx, int xindex = 0);
      ~CDGridSpheres(void);
		
      void preprocess(const PreprocessContext&);
		
      void computeBounds(const PreprocessContext& context,
                         BBox& bbox) const;
      void intersect(const RenderContext& context, RayPacket& rays) const;
      void computeNormal(const RenderContext& context, RayPacket& rays) const;
		
      virtual void shade(const RenderContext& context, RayPacket& rays) const;
		
      void computeTexCoords2(const RenderContext& context,
                             RayPacket& rays) const;
      void computeTexCoords3(const RenderContext& context,
                             RayPacket& rays) const;
      void setCMinMax(int index, float min, float max) { cmin[index] = min; cmax[index] = max; }
      void getCMinMax(int index, float& min, float& max) { min = cmin[index]; max = cmax[index]; }
      void computeHistogram(int index, int numBuckets, int* histValues); //index is data index to use.  histValues should be of size numBuckets
      void getMinMax(int index, float& dmin, float&dmax)
        {
          dmin = min[index];
          dmax = max[index];
        }
      void setCidx(int cidx) { this->cidx = cidx; }
      void setCMap(RGBAColorMap* map) { cmap = map; }
      bool clip(int s) const;
      void setClipMinMax(int index, float clipMin, float clipMax)
        {
          clipMins[index] = clipMin;
          clipMaxs[index] = clipMax;
        }
      void useAmbientOcclusion(bool st) { _useAmbientOcclusion = st; }
      void setAmbientOcclusionVariables(float cutoff, int numDirs) { 
//if (_ao) { _ao->setCutoffDistance(cutoff); _ao->setNumRays(numDirs); } 
}
        
    protected:
#ifdef USE_OPTIMIZED_FCNS
      typedef void (CDGridSpheres::*SphereIntersectFcn)(RayPacket&, int, int,
                                                        const Vector&, float) const;
#endif
		
      struct MCell {
        int nspheres;
        float* max;
        float* min;
      };
		
      void traverse(int i, RayPacket& rays, int depth,
                    Real tnear,
                    int ix, int iy, int iz,
                    int idx,
                    Real dt_dx, Real dt_dy, Real dt_dz,
                    Real tnext_x, Real tnext_y, Real tnext_z,
                    const Vector& corner, const Vector& celldir,
                    int di_dx, int di_dy, int di_dz,
                    int didx_dx, int didx_dy,
#ifdef USE_OPTIMIZED_FCNS
                    int didx_dz, int stop_x, int stop_y, int stop_z,
                    SphereIntersectFcn intersectSphere) const;
#else
      int didx_dz, int stop_x, int stop_y, int stop_z) const;
#endif // USE_OPTIMIZED_FCNS
  void transformToLattice(const BBox& box, int& sx, int& sy, int& sz,
                          int& ex, int& ey, int& ez) const;
  int mapIdx(int ix, int iy, int iz, int depth);
  void fillMCell(MCell& mcell, int depth, int startidx) const;
  void mapDiffuseColors(Packet<Color>& diffuse, RayPacket& rays) const;
		
#ifdef USE_OPTIMIZED_FCNS
  // Sphere intersection functions
  void intersectSphereCOND(RayPacket& rays, int ray_idx, int idx,
                           const Vector& center, float radius2) const;
  void intersectSphereCO(RayPacket& rays, int ray_idx, int idx,
                         const Vector& center, float radius2) const;
  void intersectSphereND(RayPacket& rays, int ray_idx, int idx,
                         const Vector& center, float radius2) const;
#endif // USE_OPTIMIZED_FCNS
  void intersectSphereDefault(RayPacket& rays, int ray_idx, int idx,
                              const Vector& center, float radius2) const;
		
  void lambertianShade(const RenderContext& context, RayPacket& rays,
                       ColorArray& totalLight) const;
  void mapValues(Packet<Color>& results,
                 const RenderContext&,
                 RayPacket& rays) const;

  void update(int, int);
  void updateStatic(int, int);

                             
 protected:
  mutable Barrier barrier;
  mutable Mutex mutex;
  float* spheres;
  int nspheres;
  int nvars;
  Real radius;
  Real inv_radius;
  int ridx;
		
  Vector diagonal;
  Vector inv_diagonal;
  int totalcells;
		
  int* counts;
  int* cells;
  float* min;
  float* max;
  float* cmin;  //specified min/max values to override actual ones(used for coloring)
  float* cmax;  
  float* clipMins;  //clip data by these values (array of nvars size)
  float* clipMaxs;
  BBox bounds;
		
  int ncells;
  int depth;
  Real inv_ncells;
		
  MCell** macrocells;
		
  RGBAColorMap* cmap;
  int cidx;
//  AmbientOcclusion* _ao;
  bool _useAmbientOcclusion;
  Material* _matl;
  int xindex;

  static CDGridSpheres** __grids;
  static int __numGrids;
  static bool* __processedGrids;

};
}

#endif
