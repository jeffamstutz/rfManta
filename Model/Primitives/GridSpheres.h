
#ifndef Manta_Model_GridSpheres_h
#define Manta_Model_GridSpheres_h

#include <Core/Color/Color.h>
#include <Core/Geometry/BBox.h>
#include <Interface/RayPacket.h>
#include <Interface/TexCoordMapper.h>
#include <Interface/Texture.h>
#include <Model/Primitives/PrimitiveCommon.h>
#include <Model/Materials/LitMaterial.h>

#define USE_OPTIMIZED_FCNS

namespace Manta {
  class RegularColorMap;

  class GridSpheres : public PrimitiveCommon, public LitMaterial,
                      public TexCoordMapper
  {
  public:
    GridSpheres(float* spheres, int nspheres, int nvars, int ncells, int depth,
                Real radius, int ridx, RegularColorMap* cmap, int cidx);
    ~GridSpheres(void);

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

  protected:
#ifdef USE_OPTIMIZED_FCNS
    typedef void (GridSpheres::*SphereIntersectFcn)(RayPacket&, int, int,
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
    BBox bounds;

    int ncells;
    int depth;
    Real inv_ncells;

    MCell** macrocells;

    RegularColorMap* cmap;
    int cidx;
  };
}

#endif
