
#include <Core/Color/RegularColorMap.h>
#include <Core/Math/MiscMath.h>
#include <Core/Math/Trig.h>
#include <Core/Math/Expon.h>
#include <Interface/Light.h>
#include <Interface/LightSet.h>
#include <Interface/Primitive.h>
#include <Interface/Packet.h>
#include <Interface/RayPacket.h>
#include <Interface/AmbientLight.h>
#include <Interface/Context.h>
#include <Interface/ShadowAlgorithm.h>
#include <Model/Primitives/GridSpheres.h>
#include <Model/Textures/Constant.h>
#include <Core/Containers/Array1.h>
#include <Core/Math/MinMax.h>
#include <Core/Util/Timer.h>

#include <iostream>
using std::cerr;

#include <float.h>
#include <cstdlib>

using namespace Manta;

GridSpheres::GridSpheres(float* spheres, int nspheres, int nvars, int ncells,
                         int depth, Real radius, int ridx, RegularColorMap* cmap,
                         int cidx) :
  spheres(spheres), nspheres(nspheres), nvars(nvars),
  radius(radius), ridx(ridx), ncells(ncells), depth(depth),
  cmap(cmap), cidx(cidx)
{
  cerr<<"Initializing GridSpheres\n";

  if (radius <= 0) {
    if (ridx <= 0)
      cerr<<"Resetting default radius to 1\n";
    radius=1;
  }

  // Compute inverses
  inv_radius=1/static_cast<Real>(radius);
  inv_ncells=1/static_cast<Real>(ncells);

  cerr<<"Recomputing min/max for GridSpheres\n";

  // Initialize min/max arrays
  min=new float[nvars];
  max=new float[nvars];

  for (int j=0; j<nvars; ++j) {
    min[j]=FLT_MAX;
    max[j]=-FLT_MAX;
  }

  // Find min/max values
  float* data=spheres;
  for (int i=0; i<nspheres; ++i) {
    for (int j=0; j<nvars; ++j) {
      min[j]=Min(min[j], data[j]);
      max[j]=Max(max[j], data[j]);
    }

    data += nvars;
  }

  counts=0;
  cells=0;

#ifdef USE_FUNCTION_POINER
  intersectSphere=0;
#endif
}

GridSpheres::~GridSpheres()
{
  delete [] min;
  delete [] max;

  if (counts)
    delete[] counts;

  if (cells)
    delete[] cells;

  if (macrocells)
    delete [] macrocells;
}

void GridSpheres::preprocess(const PreprocessContext& context)
{
  // Preprocess material
  LitMaterial::preprocess(context);

  if (context.proc != 0) {
    context.done();
    return;
  }
  // Build grid
  cerr<<"Building GridSpheres\n";

  WallClockTimer timer;
  timer.start();

  cerr<<"  min:  ("<<min[0]<<", "<<min[1]<<", "<<min[2]<<")\n";
  cerr<<"  max:  ("<<max[0]<<", "<<max[1]<<", "<<max[2]<<")\n";

  // Determine the maximum radius
  float max_radius;
  if (ridx>0) {
    cerr<<"  GridSpheres::preprocess - ridx="<<ridx<<'\n';
    max_radius=max[ridx];

    if (max_radius <= 0) {
      cerr<<"  max_radius ("<<max_radius<<") <= 0, setting to default radius ("
          <<radius<<")\n";
      max_radius=radius;
    }
  } else
    max_radius=radius;

  // Bound the spheres
  computeBounds(context, bounds);
  diagonal=bounds.diagonal();
  inv_diagonal=Vector(1/diagonal.x(), 1/diagonal.y(), 1/diagonal.z());

  // Determine grid size
  totalcells=1;
  for (int i=0; i<=depth; ++i)
    totalcells *= ncells;

  int totalsize=totalcells*totalcells*totalcells;
  cerr<<"  Computing "<<totalcells<<'x'<<totalcells<<'x'<<totalcells
      <<" grid ("<<totalsize<<" cells total)\n";

  // Clear grid data
  if (counts)
    delete[] counts;

  if (cells)
    delete[] cells;

  // Allocate and initialize counts
  counts=new int[2*totalsize];
  memset(counts, 0, 2*totalsize*sizeof(int));

  cerr<<"    0/6:  Allocation took "<<timer.time()<<" seconds\n";

  // Generate map
  int* map=new int[totalsize];
  int idx=0;
  for (int x=0; x<totalcells; ++x) {
    for (int y=0; y<totalcells; ++y) {
      for (int z=0; z<totalcells; ++z) {
        map[idx++]=mapIdx(x, y, z, depth);
      }
    }
  }

  Real stime=timer.time();
  cerr<<"    1/6:  Generating map took "<<stime<<" seconds\n";

  // Compute cell counts
  float* data=spheres;
  int tc2=totalcells*totalcells;
  for (int i=0; i<nspheres; ++i) {
    Real ctime=timer.time();
    if (ctime - stime>5.0) {
      cerr<<i<<"/"<<nspheres<<'\n';
      stime=ctime;
    }

    // Compute cell overlap
    Vector current_radius;
    if (ridx>0) {
      if (data[ridx] <= 0)
        continue;

      current_radius=Vector(data[ridx], data[ridx], data[ridx]);
    } else {
      current_radius=Vector(radius, radius, radius);
    }

    Vector center(data[0], data[1], data[2]);
    BBox box(center - current_radius, center + current_radius);
    int sx, sy, sz, ex, ey, ez;
    transformToLattice(box, sx, sy, sz, ex, ey, ez);

    int idx_x=sx*tc2;
    for (int x=sx; x<=ex; ++x) {
      int idx_y=idx_x + sy*totalcells;
      idx_x += tc2;
      for (int y=sy; y<=ey; ++y) {
        int idx=idx_y + sz;
        idx_y += totalcells;
        for (int z=sz; z<=ez; ++z) {
          int aidx=map[idx++];
          counts[2*aidx + 1]++;
        }
      }
    }

    data += nvars;
  }

  cerr<<"    2/6:  Counting cells took "<<timer.time()<<" seconds\n";

  int total=0;
  for (int i=0; i<totalsize; ++i) {
    int count=counts[2*i + 1];
    counts[2*i]=total;
    total += count;
  }

  // Allocate cells
  cerr<<"  Allocating "<<total<<" grid cells ("
      <<static_cast<Real>(total)/nspheres<<" per object, "
      <<static_cast<Real>(total)/totalsize<<" per cell)\n";

  cells=new int[total];
  for (int i=0; i<total; ++i)
    cells[i]=-1;

  stime=timer.time();
  cerr<<"    3/6:  Calculating offsets took "<<stime<<" seconds\n";

  // Populate the grid
  Array1<int> current(totalsize);
  current.initialize(0);
  data=spheres;
  for (int i=0; i<nspheres; ++i) {
    Real ctime=timer.time();
    if (ctime - stime>5.0) {
      cerr<<i<<"/"<<nspheres<<'\n';
      stime=ctime;
    }

    // Compute cell overlap
    Vector current_radius;
    if (ridx>0) {
      if (data[ridx] <= 0)
        continue;

      current_radius=Vector(data[ridx], data[ridx], data[ridx]);
    } else {
      current_radius=Vector(radius, radius, radius);
    }

    Vector center(data[0], data[1], data[2]);
    BBox box(center - current_radius, center + current_radius);
    int sx, sy, sz, ex, ey, ez;
    transformToLattice(box, sx, sy, sz, ex, ey, ez);

    for (int x=sx; x<=ex; ++x) {
      for (int y=sy; y<=ey; ++y) {
        int idx=totalcells*(totalcells*x + y) + sz;
        for (int z=sz; z<=ez; ++z) {
          int aidx=map[idx++];
          int cur=current[aidx]++;
          int pos=counts[2*aidx] + cur;
          cells[pos]=nvars*i;
        }
      }
    }

    data += nvars;
  }

  delete [] map;

  cerr<<"    4/6:  Filling grid took "<<timer.time()<<" seconds\n";

  // Verify the grid
  for (int i=0; i<totalsize; ++i) {
    if (current[i] != counts[2*i + 1]) {
      cerr<<"Fatal error:  current="<<current[i]<<", but counts="
          <<counts[2*i + 1]<<'\n';
      exit(1);
    }
  }

  for (int i=0; i<total; ++i) {
    if (cells[i]==-1) {
      cerr<<"Fatal error:  cells["<<i<<"] == -1\n";
      exit(1);
    }
  }

  cerr<<"    5/6:  Verifying grid took "<<timer.time()<<" seconds\n";

  // Build the macrocells
  if (depth > 0) {
    macrocells=new MCell*[depth + 1];
    macrocells[0]=0;

    int size=ncells*ncells*ncells;
    for (int d=depth; d>=1; d--) {
      MCell* mcell=new MCell[size];
      macrocells[d]=mcell;

      float* mm=new float[2*nvars*size];
      for (int i=0; i<size; ++i) {
        // Minimum
        mcell->min=mm;
        mm += nvars;

        // Maximum
        mcell->max=mm;
        mm += nvars;

        mcell++;
      }

      size *= ncells*ncells*ncells;
    }

    MCell top;
    fillMCell(top, depth, 0);
    if (top.nspheres != total) {
      cerr<<"Fatal error:  macrocell construction went wrong\n";
      cerr<<"  Top macrocell:   "<<top.nspheres<<'\n';
      cerr<<"  Total nspheres:  "<<total<<'\n';
      exit(1);
    }

    cerr<<"    6/6:  Calculating macrocells took "<<timer.time()<<" seconds\n";
  } else {
    macrocells=0;
    cerr<<"    6/6:  Macrocell hierarchy not built (depth == 0)\n";
  }

  cerr<<"Done building GridSpheres\n";
  context.done();
}

void GridSpheres::computeBounds(const PreprocessContext& context,
                                BBox& bbox) const
{
  // Determine the maximum radius
  float max_radius;
  if (ridx>0) {
    max_radius=max[ridx];

    if (max_radius <= 0) {
      cerr<<"  max_radius ("<<max_radius<<") <= 0, setting to default radius ("
          <<radius<<")\n";
      max_radius=radius;
    }
  } else
    max_radius=radius;

  // Bound the spheres
  Vector mr(max_radius, max_radius, max_radius);
  bbox.reset(Vector(min[0], min[1], min[2]) - mr,
             Vector(max[0], max[1], max[2]) + mr);

  const Vector eps3(1.e-3, 1.e-3, 1.e-3);
  bbox.extendByPoint(bbox.getMin() - eps3);
  bbox.extendByPoint(bbox.getMax() + eps3);

  Vector diag(bbox.diagonal());
  bbox.extendByPoint(bbox.getMin() - diag*eps3);
  bbox.extendByPoint(bbox.getMax() + diag*eps3);
}

void GridSpheres::intersect(const RenderContext& context, RayPacket& rays) const
{
  Vector bmin=bounds.getMin();
  Vector bmax=bounds.getMax();

  rays.computeInverseDirections();
  rays.computeSigns();

#ifdef USE_OPTIMIZED_FCNS
  // Determine appropriate sphere intersection function for this ray packet
  SphereIntersectFcn intersectSphere = &GridSpheres::intersectSphereDefault;

  switch (rays.getAllFlags() & (RayPacket::ConstantOrigin |
                                RayPacket::NormalizedDirections)) {
  case RayPacket::ConstantOrigin|RayPacket::NormalizedDirections:
    // Rays of constant origin and normalized directions
    intersectSphere=&GridSpheres::intersectSphereCOND;
    break;
  case RayPacket::ConstantOrigin:
    // Rays of constant origin for not normalized directions
    intersectSphere=&GridSpheres::intersectSphereCO;
    break;
  case RayPacket::NormalizedDirections:
    // Rays of non-constant origin and normalized directions
    intersectSphere=&GridSpheres::intersectSphereND;
    break;
  case 0:
    // Rays of non-constant origin and non-normalized directions
    intersectSphere=&GridSpheres::intersectSphereDefault;
    break;
  }
#endif // USE_OPTIMIZED_FCNS

  for (int i=rays.begin(); i<rays.end(); ++i) {
    const Vector origin(rays.getOrigin(i));
    const Vector direction(rays.getDirection(i));
    const Vector inv_direction(rays.getInverseDirection(i));

    // Intersect ray with bounding box
    Real tnear, tfar;
    int di_dx;
    int ddx;
    int didx_dx;
    int stop_x;
    if (direction.x()>0) {
      tnear=(bmin.x() - origin.x())*inv_direction.x();
      tfar=(bmax.x() - origin.x())*inv_direction.x();
      di_dx=1;
      ddx=1;
      didx_dx=ncells*ncells;
      stop_x=ncells;
    } else {
      tnear=(bmax.x() - origin.x())*inv_direction.x();
      tfar=(bmin.x() - origin.x())*inv_direction.x();
      di_dx=-1;
      ddx=0;
      didx_dx=-ncells*ncells;
      stop_x=-1;
    }

    Real y0, y1;
    int di_dy;
    int ddy;
    int didx_dy;
    int stop_y;
    if (direction.y()>0) {
      y0=(bmin.y() - origin.y())*inv_direction.y();
      y1=(bmax.y() - origin.y())*inv_direction.y();
      di_dy=1;
      ddy=1;
      didx_dy=ncells;
      stop_y=ncells;
    } else {
      y0=(bmax.y() - origin.y())*inv_direction.y();
      y1=(bmin.y() - origin.y())*inv_direction.y();
      di_dy=-1;
      ddy=0;
      didx_dy=-ncells;
      stop_y=-1;
    }

    if (y0>tnear)
      tnear=y0;
    if (y1<tfar)
      tfar=y1;
    if (tfar<tnear)
      continue;

    Real z0, z1;
    int di_dz;
    int ddz;
    int didx_dz;
    int stop_z;
    if (direction.z()>0) {
      z0=(bmin.z() - origin.z())*inv_direction.z();
      z1=(bmax.z() - origin.z())*inv_direction.z();
      di_dz=1;
      ddz=1;
      didx_dz=1;
      stop_z=ncells;
    } else {
      z0=(bmax.z() - origin.z())*inv_direction.z();
      z1=(bmin.z() - origin.z())*inv_direction.z();
      di_dz=-1;
      ddz=0;
      didx_dz=-1;
      stop_z=-1;
    }

    if (z0>tnear)
      tnear=z0;
    if (z1<tfar)
      tfar=z1;
    if (tfar<tnear)
      continue;
    if (tfar<static_cast<Real>(1.e-6))
      continue;
    if (tnear < 0)
      tnear=0;

    // Compute lattice coordinates
    Vector p=origin + tnear*direction;
    Vector lattice=(p - bmin)*inv_diagonal;
    int ix=Clamp(static_cast<int>(lattice.x()*ncells), 0, ncells - 1);
    int iy=Clamp(static_cast<int>(lattice.y()*ncells), 0, ncells - 1);
    int iz=Clamp(static_cast<int>(lattice.z()*ncells), 0, ncells - 1);

    // Compute cell index
    int idx=(ix*ncells + iy)*ncells + iz;

    // Compute delta t in each direction
    Real dt_dx=di_dx*diagonal.x()*inv_ncells*inv_direction.x();
    Real dt_dy=di_dy*diagonal.y()*inv_ncells*inv_direction.y();
    Real dt_dz=di_dz*diagonal.z()*inv_ncells*inv_direction.z();

    // Compute far edges of the cell
    Vector next(ix + ddx, iy + ddy, iz + ddz);
    Vector far=diagonal*next*inv_ncells + bmin;

    // Compute t values at far edges of the cell
    Vector tnext=(far - origin)*inv_direction;

    // Compute cell corner and direction
    Vector factor(ncells*inv_diagonal);
    Vector cellcorner((origin - bmin)*factor);
    Vector celldir(direction*factor);

    // Traverse the hierarchy
    traverse(i, rays, depth, tnear, ix, iy, iz, idx, dt_dx, dt_dy, dt_dz,
             tnext.x(), tnext.y(), tnext.z(),
             cellcorner, celldir,
             di_dx, di_dy, di_dz, didx_dx, didx_dy, didx_dz,
#ifdef USE_OPTIMIZED_FCNS
             stop_x, stop_y, stop_z, intersectSphere);
#else
             stop_x, stop_y, stop_z);
#endif // USE_OPTIMIZED_FCNS
  }
}

void GridSpheres::computeNormal(const RenderContext& context,
                                RayPacket& rays) const
{
  rays.computeHitPositions();
  for (int i=rays.begin(); i<rays.end(); ++i) {
    float* data=spheres + rays.scratchpad<int>(i);
    Vector n=rays.getHitPosition(i) - Vector(data[0], data[1], data[2]);

    if (ridx>0) {
      if (data[ridx] <= 0)
        n *= inv_radius;
      else
        n /= data[ridx];
    } else {
      n *= inv_radius;
    }

    rays.setNormal(i, n);
  }

  rays.setFlag(RayPacket::HaveUnitNormals);
}

void GridSpheres::shade(const RenderContext& context, RayPacket& rays) const
{
  // Compute ambient light
  ColorArray ambient;
  activeLights->getAmbientLight()->computeAmbient(context, rays, ambient);

  // Shade a bunch of rays that have intersected the same particle
  lambertianShade(context, rays, ambient);
}

void GridSpheres::lambertianShade(const RenderContext& context, RayPacket& rays,
                                  ColorArray& totalLight) const
{
  // Compute normals
  rays.computeNormals<true>(context);

  // Compute colors
  Packet<Color> diffuse;
  mapDiffuseColors(diffuse, rays);

  // Normalize directions for proper dot product computation
  rays.normalizeDirections();

  ShadowAlgorithm::StateBuffer shadowState;
  do {
    RayPacketData shadowData;
    RayPacket shadowRays(shadowData, RayPacket::UnknownShape, 0, 0,
                         rays.getDepth(), 0);

    // Call the shadow algorithm (SA) to generate shadow rays.  We may not be
    // able to compute all of them, so we pass along a buffer for the SA
    // object to store it's state.  The firstTime flag tells the SA to fill
    // in the state rather than using anything in the state buffer.  Most
    // SAs will only need to store an int or two in the statebuffer.
    context.shadowAlgorithm->computeShadows(context, shadowState, activeLights,
                                            rays, shadowRays);

    // Normalize directions for proper dot product computation
    shadowRays.normalizeDirections();

    for (int i=shadowRays.begin(); i < shadowRays.end(); ++i) {
      if (!shadowRays.wasHit(i)) {
        // Not in shadow, so compute the direct and specular contributions
        Vector normal=rays.getNormal(i);
        Vector shadowdir=shadowRays.getDirection(i);
        ColorComponent cos_theta=Dot(shadowdir, normal);
        Color light=shadowRays.getColor(i);
        for (int j=0; j < Color::NumComponents; ++j)
          totalLight[j][i] += light[j]*cos_theta;
      }
    }
  } while(!shadowState.done());

  // Sum up diffuse/specular contributions
  for (int i=rays.begin(); i < rays.end(); ++i) {
    Color result;
    for (int j=0;j<Color::NumComponents; ++j)
      result[j]=totalLight[j][i]*diffuse.colordata[j][i];
    rays.setColor(i, result);
  }
}

void GridSpheres::computeTexCoords2(const RenderContext& context,
                                    RayPacket& rays) const
{
  rays.computeHitPositions();
  rays.computeNormals<true>(context);
  for(int i=rays.begin();i<rays.end();i++){
    Vector n=rays.getNormal(i);
    Real angle=Clamp(n.z(), (Real)-1, (Real)1);
    Real theta=Acos(angle);
    Real phi=Atan2(n.y(), n.x());
    Real x=phi*(Real)(0.5*M_1_PI);
    if (x < 0)
      x += 1;
    Real y=theta*(Real)M_1_PI;
    rays.setTexCoords(i, Vector(x, y, 0));
  }

  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}

void GridSpheres::computeTexCoords3(const RenderContext& context,
                                    RayPacket& rays) const
{
  rays.computeHitPositions();
  rays.computeNormals<true>(context);
  for(int i=rays.begin();i<rays.end();i++){
    Vector n=rays.getNormal(i);
    Real angle=Clamp(n.z(), (Real)-1, (Real)1);
    Real theta=Acos(angle);
    Real phi=Atan2(n.y(), n.x());
    Real x=phi*(Real)(0.5*M_1_PI);
    if (x < 0)
      x += 1;
    Real y=theta*(Real)M_1_PI;
    rays.setTexCoords(i, Vector(x, y, 0));
  }

  rays.setFlag(RayPacket::HaveTexture2|RayPacket::HaveTexture3);
}

void GridSpheres::traverse(int ray_idx, RayPacket& rays, int depth, Real tnear,
                           int ix, int iy, int iz, int idx,
                           Real dt_dx, Real dt_dy, Real dt_dz,
                           Real tnext_x, Real tnext_y, Real tnext_z,
                           const Vector& cellcorner, const Vector& celldir,
                           int di_dx, int di_dy, int di_dz,
                           int didx_dx, int didx_dy, int didx_dz,
#ifdef USE_OPTIMIZED_FCNS
                           int stop_x, int stop_y, int stop_z,
                           SphereIntersectFcn intersectSphere) const
#else
                           int stop_x, int stop_y, int stop_z) const
#endif // USE_OPTIMIZED_FCNS
{
  if (depth>0) {
    // Traverse the macrocell layers
    MCell* mcells=macrocells[depth];
    while (tnear < rays.getMinT(ray_idx)) {
      MCell& mcell=mcells[idx];
      if (mcell.nspheres > 0) {
        // XXX:  Range checking would go here...  Ignore for now

        // Compute lattice coordinates
        int new_ix=Clamp(static_cast<int>((cellcorner.x() + tnear*celldir.x() - ix)*ncells), 0, ncells - 1);
        int new_iy=Clamp(static_cast<int>((cellcorner.y() + tnear*celldir.y() - iy)*ncells), 0, ncells - 1);
        int new_iz=Clamp(static_cast<int>((cellcorner.z() + tnear*celldir.z() - iz)*ncells), 0, ncells - 1);

        // Compute new cell index
        int new_idx=((idx*ncells + new_ix)*ncells + new_iy)*ncells + new_iz;

        // Compute new delta t in each direction
        Real new_dt_dx=dt_dx*inv_ncells;
        Real new_dt_dy=dt_dy*inv_ncells;
        Real new_dt_dz=dt_dz*inv_ncells;

        // Compute new t values at far edges of the cell
        Vector signs=rays.getSigns(ray_idx);
        Real new_tnext_x=tnext_x + (1 - 2*signs.x())*new_ix*new_dt_dx +
          (1 - signs.x())*(new_dt_dx - dt_dx);
        Real new_tnext_y=tnext_y + (1 - 2*signs.y())*new_iy*new_dt_dy +
          (1 - signs.y())*(new_dt_dy - dt_dy);
        Real new_tnext_z=tnext_z + (1 - 2*signs.z())*new_iz*new_dt_dz +
          (1 - signs.z())*(new_dt_dz - dt_dz);

        // Compute new cell corner and direction
        Vector new_cellcorner=(cellcorner - Vector(ix, iy, iz))*ncells;
        Vector new_celldir=celldir*ncells;

        // Descend to next depth in the hierarchy
        traverse(ray_idx, rays, depth - 1, tnear,
                 new_ix, new_iy, new_iz,
                 new_idx,
                 new_dt_dx, new_dt_dy, new_dt_dz,
                 new_tnext_x, new_tnext_y, new_tnext_z,
                 new_cellcorner, new_celldir,
                 di_dx, di_dy, di_dz,
                 didx_dx, didx_dy, didx_dz,
#ifdef USE_OPTIMIZED_FCNS
                 stop_x, stop_y, stop_z, intersectSphere);
#else
                 stop_x, stop_y, stop_z);
#endif // USE_OPTIMIZED_FCNS
      }

      // March to next macrocell at the current depth
      if (tnext_x<tnext_y && tnext_x<tnext_z) {
        ix += di_dx;
        if (ix == stop_x)
          break;
        tnear=tnext_x;
        tnext_x += dt_dx;
        idx += didx_dx;
      } else if (tnext_y<tnext_z) {
        iy += di_dy;
        if (iy == stop_y)
          break;
        tnear=tnext_y;
        tnext_y += dt_dy;
        idx += didx_dy;
      } else {
        iz += di_dz;
        if (iz == stop_z)
          break;
        tnear=tnext_z;
        tnext_z += dt_dz;
        idx += didx_dz;
      }
    }
  } else {
    // Traverse cells, intersecting spheres along the way as necessary
    while (tnear < rays.getMinT(ray_idx)) {
      int start=counts[2*idx];
      int nsph=counts[2*idx + 1];
      for (int j=0; j<nsph; ++j) {
        float* data=spheres + cells[start + j];

        // XXX:  Range checking would go here...  Ignore for now

        // Sphere is in range, determine it's radius (squared)
        float radius2;
        if (ridx>0) {
          float current_radius=data[ridx];
          if (current_radius <= 0)
            continue;

          radius2=current_radius*current_radius;
        } else {
          radius2=radius*radius;
        }

#ifdef USE_OPTIMIZED_FCNS
        // Intersect the sphere using the appropriately optimized function
        (*this.*intersectSphere)(rays, ray_idx, start + j,
                                 Vector(data[0], data[1], data[2]), radius2);
#else
        intersectSphereDefault(rays, ray_idx, start + j,
                               Vector(data[0], data[1], data[2]), radius2);
#endif
      }

      // March to the next cell
      if (tnext_x < tnext_y && tnext_x < tnext_z) {
        ix += di_dx;
        if (ix == stop_x)
          break;
        tnear=tnext_x;
        tnext_x += dt_dx;
        idx += didx_dx;
      } else if (tnext_y < tnext_z){
        iy += di_dy;
        if (iy == stop_y)
          break;
        tnear=tnext_y;
        tnext_y += dt_dy;
        idx += didx_dy;
      } else {
        iz += di_dz;
        if (iz == stop_z)
          break;
        tnear=tnext_z;
        tnext_z += dt_dz;
        idx += didx_dz;
      }
    }
  }
}

#ifdef USE_OPTIMIZED_FCNS
void GridSpheres::intersectSphereCOND(RayPacket& rays, int ray_idx, int idx,
                                      const Vector& center, float radius2) const
{
  // Rays of constant origin and normalized directions
  Vector O(rays.getOrigin(ray_idx) - center);
  Real C=Dot(O, O) - radius2;
  Vector D(rays.getDirection(ray_idx));
  Real B=Dot(O, D);
  Real disc=B*B - C;
  if (disc >= 0) {
    Real r=Sqrt(disc);
    Real t0=-(r + B);
    if (t0 > T_EPSILON) {
      if (rays.hit(ray_idx, t0, this, this, this))
        rays.scratchpad<int>(ray_idx)=cells[idx];
    } else {
      Real t1=r - B;
      if (rays.hit(ray_idx, t1, this, this, this))
        rays.scratchpad<int>(ray_idx)=cells[idx];
    }
  }
}

void GridSpheres::intersectSphereCO(RayPacket& rays, int ray_idx, int idx,
                                    const Vector& center, float radius2) const
{
  // Rays of constant origin for not normalized directions
  Vector O(rays.getOrigin(ray_idx) - center);
  Real C=Dot(O, O) - radius2;
  Vector D(rays.getDirection(ray_idx));
  Real A=Dot(D, D);
  Real B=Dot(O, D);
  Real disc=B*B - A*C;
  if (disc >= 0) {
    Real r=Sqrt(disc);
    Real t0=-(r + B)/A;
    if (t0 > T_EPSILON) {
      if (rays.hit(ray_idx, t0, this, this, this))
        rays.scratchpad<int>(ray_idx)=cells[idx];
    } else {
      Real t1=(r - B)/A;
      if (rays.hit(ray_idx, t1, this, this, this))
        rays.scratchpad<int>(ray_idx)=cells[idx];
    }
  }
}

void GridSpheres::intersectSphereND(RayPacket& rays, int ray_idx, int idx,
                                    const Vector& center, float radius2) const
{
  // Rays of non-constant origin and normalized directions
  Vector O(rays.getOrigin(ray_idx) - center);
  Vector D(rays.getDirection(ray_idx));
  Real B=Dot(O, D);
  Real C=Dot(O, O) - radius2;
  Real disc=B*B - C;
  if (disc >= 0) {
    Real r=Sqrt(disc);
    Real t0=-(r + B);
    if (t0 > T_EPSILON) {
      if (rays.hit(ray_idx, t0, this, this, this))
        rays.scratchpad<int>(ray_idx)=cells[idx];
    } else {
      Real t1=r - B;
      if (rays.hit(ray_idx, t1, this, this, this))
        rays.scratchpad<int>(ray_idx)=cells[idx];
    }
  }
}
#endif // USE_OPTIMIZED_FCNS

void GridSpheres::intersectSphereDefault(RayPacket& rays, int ray_idx, int idx,
                                         const Vector& center,
                                         float radius2) const
{
  // Rays of non-constant origin and non-normalized directions
  Vector O(rays.getOrigin(ray_idx) - center);
  Vector D(rays.getDirection(ray_idx));
  Real A=Dot(D, D);
  Real B=Dot(O, D);
  Real C=Dot(O, O) - radius2;
  Real disc=B*B - A*C;
  if (disc >= 0) {
    Real r=Sqrt(disc);
    Real t0=-(r + B)/A;
    if (t0 > T_EPSILON) {
      if (rays.hit(ray_idx, t0, this, this, this))
        rays.scratchpad<int>(ray_idx)=cells[idx];
    } else {
      Real t1=(r - B)/A;
      if (rays.hit(ray_idx, t1, this, this, this));
      rays.scratchpad<int>(ray_idx)=cells[idx];
    }
  }
}

void GridSpheres::transformToLattice(const BBox& box, int& sx, int& sy, int& sz,
                                     int& ex, int& ey, int& ez) const
{
  Vector s=(box.getMin() - bounds.getMin())*inv_diagonal;
  sx=static_cast<int>(s.x()*totalcells);
  sy=static_cast<int>(s.y()*totalcells);
  sz=static_cast<int>(s.z()*totalcells);
  Clamp(sx, 0, totalcells - 1);
  Clamp(sy, 0, totalcells - 1);
  Clamp(sz, 0, totalcells - 1);

  Vector e=(box.getMax() - bounds.getMin())*inv_diagonal;
  ex=static_cast<int>(e.x()*totalcells);
  ey=static_cast<int>(e.y()*totalcells);
  ez=static_cast<int>(e.z()*totalcells);
  Clamp(ex, 0, totalcells - 1);
  Clamp(ey, 0, totalcells - 1);
  Clamp(ez, 0, totalcells - 1);
}

int GridSpheres::mapIdx(int ix, int iy, int iz, int depth)
{
  if (depth==0) {
    return (ix*ncells + iy)*ncells + iz;
  } else {
    int idx=mapIdx(static_cast<int>(ix*inv_ncells),
                   static_cast<int>(iy*inv_ncells),
                   static_cast<int>(iz*inv_ncells),
                   depth - 1);
    int nx=ix%ncells;
    int ny=iy%ncells;
    int nz=iz%ncells;

    return ((idx*ncells + nx)*ncells + ny)*ncells + nz;
  }
}

void GridSpheres::fillMCell(MCell& mcell, int depth, int startidx) const
{
  mcell.nspheres=0;
  mcell.min=new float[2*nvars];
  mcell.max=mcell.min + nvars;

  // Initialize min/max
  for (int i=0; i<nvars; ++i) {
    mcell.min[i]=FLT_MAX;
    mcell.max[i]=-FLT_MAX;
  }

  // Determine min/max
  int ncells3=ncells*ncells*ncells;
  if (depth>0) {
    MCell* mcells=macrocells[depth];
    for (int i=0; i<ncells3; ++i) {
      int idx=startidx + i;
      fillMCell(mcells[idx], depth - 1, idx*ncells*ncells*ncells);
      mcell.nspheres += mcells[idx].nspheres;
      for (int j=0; j<nvars; ++j) {
        if (mcells[idx].min[j]<mcell.min[j])
          mcell.min[j]=mcells[idx].min[j];
        if (mcells[idx].max[j]>mcell.max[j])
          mcell.max[j]=mcells[idx].max[j];
      }
    }
  } else {
    for (int i=0; i<ncells3; ++i) {
      int idx=startidx + i;
      int nsph=counts[2*idx + 1];
      mcell.nspheres += nsph;
      int s=counts[2*idx];
      for (int j=0; j<nsph; ++j) {
        float* data=spheres + cells[s + j];
        for (int k=0; k<nvars; ++k) {
          if (data[k]<mcell.min[k])
            mcell.min[k]=data[k];
          if (data[k]>mcell.max[k])
            mcell.max[k]=data[k];
        }
      }
    }
  }
}

void GridSpheres::mapDiffuseColors(Packet<Color>& diffuse, RayPacket& rays) const
{
  for (int i=rays.begin(); i<rays.end(); ++i) {
    int particle=rays.scratchpad<int>(i);
    float value=*(spheres + particle + cidx);
    float minimum=min[cidx];
    float normalized=(value - minimum)/(max[cidx] - minimum);
    int ncolors=cmap->blended.size() - 1;
    int idx=Clamp(static_cast<int>(ncolors*normalized), 0, ncolors);
    diffuse.set(i, cmap->blended[idx]);
  }
}
