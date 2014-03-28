
#include <Model/Groups/RecursiveGrid.h>
#include <Model/Intersections/TriangleBBoxOverlap.h>
#include <Model/Primitives/MeshTriangle.h>
#include <Interface/MantaInterface.h>
#include <Interface/RayPacket.h>
#include <Interface/Context.h>
#include <Core/Exceptions/IllegalArgument.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Geometry/BBox.h>
#include <Core/Util/Args.h>
#include <Core/Thread/Time.h>
#include <Core/Thread/Mutex.h>
#include <iostream>
#include <stdio.h>

using namespace std;
using namespace Manta;


long RecursiveGrid::nIntersects=0;
long RecursiveGrid::nCellTraversals=0;
long RecursiveGrid::nGridTraversals=0;
long RecursiveGrid::nCells=0;
long RecursiveGrid::nTotalRays=0;
long RecursiveGrid::nGrids[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
long RecursiveGrid::nFilledCells=0;
long RecursiveGrid::nTriRefs=0;

RecursiveGrid::RecursiveGrid(int numLevels, Group *group) :
  lists(NULL), currGroup(group), numLevels(numLevels)
{
}

RecursiveGrid::~RecursiveGrid()
{
  clearGrid();
}

void RecursiveGrid::clearGrid() {
  //delete sub-grids
  const int totalCells = cells.getNx()*cells.getNy()*cells.getNz();
  const int listSize = totalCells>0? cells[totalCells]: 0;
  for (int i=0; i < listSize; ++i) {
    if (dynamic_cast<const RecursiveGrid*>(lists[i]))
      delete lists[i];
  }

  delete[] lists;
  cells.resize(0,0,0,0);
  subGridList.resize(0,0,0,0);
}

void RecursiveGrid::transformToLattice(const Vector& v,
                                      int& x, int& y, int& z) const
{
  const Vector s = (v-min)*inv_cellsize;
  x = Floor(s.x()); y = Floor(s.y()); z = Floor(s.z());
}

void RecursiveGrid::transformToLattice(const BBox& box,
                                      int& sx, int& sy, int& sz,
                                      int& ex, int& ey, int& ez) const
{
  Vector s = (box.getMin()-min)*inv_cellsize-Vector(1.e-6,1.e-6,1.e-6);
  sx = Floor(s.x()); sy = Floor(s.y()); sz = Floor(s.z());
  Vector e = (box.getMax()-min)*inv_cellsize+Vector(1.e-6,1.e-6,1.e-6);
  ex = Ceil(e.x()); ey = Ceil(e.y()); ez = Ceil(e.z());
}

void RecursiveGrid::preprocess( const PreprocessContext &context )
{
  if (currGroup) {
    currGroup->preprocess(context);

    BBox bounds;
    currGroup->computeBounds(context, bounds);

    if (context.proc == 0) {
      min = bounds[0];
      max = bounds[1];
    }
    if (context.isInitialized())
      rebuild(context.proc, context.numProcs);
  }

#if defined (COLLECT_STATS) || defined (PARAMETER_SEARCH)
  if (context.proc == 0 && context.isInitialized())
    context.manta_interface->registerSerialPreRenderCallback(
      Callback::create(this, &RecursiveGrid::update));
#endif
}

void RecursiveGrid::setGroup(Group* new_group)
{
  currGroup = new_group;
}

Group* RecursiveGrid::getGroup() const
{
  return currGroup;
}

void RecursiveGrid::rebuild(int proc, int numProcs)
{
  //We only support serial builds right now.
  if (proc != 0) {
    return;
  }

  //Something's wrong, let's bail. Since we reset the bounds right
  //above, this grid should never get intersected.
  if (!currGroup)
    return;

  PreprocessContext context;

  BBox bounds;
  currGroup->computeBounds(context, bounds);

  clearGrid();
  for (int i=0; i<10; ++i)
    nGrids[i]=0;
  nCells = 0;
  nTriRefs = nFilledCells = 0;
  const double start = Time::currentSeconds();

  build(context, currGroup->getVectorOfObjects(), bounds, 0, currGroup->size());

  const double dt = Time::currentSeconds()-start;
  printf("Recursive Grid built in %f seconds for a %d object scene\n", dt, (int)currGroup->size());
  printf("There are %ld, %ld, %ld, %ld grids per level for a total of %ld cells\n",
         nGrids[0], nGrids[1], nGrids[2], nGrids[3], nCells);
  printf("%d x %d x %d\n", cells.getNx(), cells.getNy(), cells.getNz());
}


// This is picking the grid resolution according to: "Grid Creation Strategies
// for Efficient Ray Tracing" by Thiago Ize, Peter Shirley, and Steve Parker.
// The build has not been optimized at all for build speed!
void RecursiveGrid::build(const PreprocessContext &context,
                          const vector<Object*> &objs, const BBox &bbox,
                          int depth, int totalObjects)
{
  min = bbox.getMin();
  max = bbox.getMax();
  Vector diag;
  diag = bbox.diagonal();

  //Need to expand the grid by a small amount. For numerical precision
  //reasons, need to find a safe lower bound for this epsilon.
  Vector expandBy = Max(Max(Max(bbox[0], -bbox[0]),
                            Max(bbox[1], -bbox[1])),
                        Vector(1,1,1)*diag.length()) * 1e-6;

  diag = Max(expandBy, diag);
  min -= expandBy;
  max += expandBy;
  diag = max-min;

  double volume = diag.x()*diag.y()*diag.z();
  double vol3 = Cbrt(volume);
  const float initial_exp_d = (numLevels)*2+1;
  totalObjects = objs.size();
  float exponent =
    1.0 / (initial_exp_d-depth*2);
    //7.0/27; //optimal for single level grids (use scale of 350).
    //4/3.0 / 3.0; //optimal for single level grid of long skinny triangles.

  //Found 8^(1/3) to work well for this codebase. It's possible this
  //could change with updated code.
  float scale = 8;

  double m = powf( scale * totalObjects, exponent);
  //note: m^3 = M = total cells for that specfic grid level
  bool build_N_Grid = false;
 buildgrid:
  if (build_N_Grid)
    m = Cbrt((double)objs.size()); //TODO: figure out if a scalar is needed

  int nx = 0, ny = 0, nz = 0;
  {
    int count = 0; //in case we get in some ugly infinite loop.
    Vector _diag = diag;
    while (nx*ny*nz < 8) {
      nx = static_cast<int>((_diag.x() * m) / vol3 +.5);
      ny = static_cast<int>((_diag.y() * m) / vol3 +.5);
      nz = static_cast<int>((_diag.z() * m) / vol3 +.5);

      //Since we must have 1 or more voxels in each dimension, if we
      //have zero, then we need to increase that to 1 and decrease the
      //other dimensions so that we maintain the same number of total
      //grid cells.
      if (nx < 1) {
        float nxf = (_diag.x() * m) / vol3;
        float nxf3 = Sqrt(nxf);
        _diag[0] /= nxf;
        _diag[1] *= nxf3;
        _diag[2] *= nxf3;
      }

      if (ny < 1) {
        float nyf = (_diag.y() * m) / vol3;
        float nyf3 = Sqrt(nyf);
        _diag[1] /= nyf;
        _diag[0] *= nyf3;
        _diag[2] *= nyf3;
      }

      if (nz < 1) {
        float nzf = (_diag.z() * m) / vol3;
        float nzf3 = Sqrt(nzf);
        _diag[2] /= nzf;
        _diag[0] *= nzf3;
        _diag[1] *= nzf3;
      }

      if (count++ > 4) {
        nx = 2; ny = 2; nz = 2;
        cout<<"possible infinite loop choosing dimensions for grid: " << diag
            << "with " << objs.size() <<" triangles"<< endl;
        cout <<bbox<<endl
             <<min<<"\t    "<<max<<endl;
        //printf("m > 1e20.  %lf", vol3 );
        break;//exit(1);
      }
    }
  }
  int M = nx*ny*nz;

  cellsize = diag/Vector(nx, ny, nz);
  inv_cellsize = cellsize.inverse();
  cells.resize(nx, ny, nz, 1);
  subGridList.resize(nx, ny, nz, 1);

  GridArray3<vector<Object*> > cell_objects;
  cell_objects.resize(nx, ny, nz,1);

  Mesh *mesh = dynamic_cast<Mesh*>(currGroup);

//   cerr << "1. Initializing grid\n";
  cells.initialize(0);
  subGridList.initialize(false);

//   cerr << "2. Count objects\n";
  for(int i=0;i<static_cast<int>(objs.size());i++){
    Object* obj = objs[i];

    BBox objbox;
    obj->computeBounds(context,objbox);
    objbox.intersection(bbox);
    int sx, sy, sz, ex, ey, ez;
    transformToLattice(objbox, sx, sy, sz, ex, ey, ez);
    sx = Max(0, sx);
    sy = Max(0, sy);
    sz = Max(0, sz);
    ex = Min(nx, ex);
    ey = Min(ny, ey);
    ez = Min(nz, ez);

    for(int x=sx;x<ex;x++){
      for(int y=sy;y<ey;y++){
        for(int z=sz;z<ez;z++){
          Vector xyz(x,y,z);
          BBox cellBounds(min + xyz*cellsize,
                          min + (xyz+Vector(1,1,1)) * cellsize);
          if (!mesh ||
              triBoxOverlap(static_cast<MeshTriangle*>(obj),
                            cellBounds)) {
            nTriRefs++;
            cells(x, y, z)++;
            cell_objects(x, y, z).push_back(obj);
          }
        }
      }
    }
  }

  unsigned int MAX_OBJS = 4;
  if (depth >= numLevels-1)
    MAX_OBJS = 100000000;

//   cerr << "3. Count cells\n";
  int sum = 0;
  int nUsedCells = 0;
  for(int i=0;i<M;i++){
    int nobj = cell_objects[i].size();
    nobj = nobj> (int)MAX_OBJS ? 1: nobj;
    cells[i] = sum;
    sum+=nobj;
    if (nobj > 0)
      nUsedCells++;
  }
  cells[M] = sum;

  nFilledCells += nUsedCells; //for debugging/testing

  const float USED_CELL_THREASHOLD = 1.9f;
  if (nUsedCells > M*USED_CELL_THREASHOLD &&
      !build_N_Grid) {
    //redo with O(N) cells
    build_N_Grid = true;
    goto buildgrid;
  }

  nGrids[depth]++;
  nCells+=M;

//   cerr << "4. Allocating " << sum << " list entries\n";
  lists = new const Object*[sum];

  GridArray3<int> offset(nx, ny, nz);
  offset.initialize(0);

//   cerr << "5. Filling in lists\n";
  for (int x=0; x<nx; ++x) {
    for (int y=0; y<ny; ++y) {
      for (int z=0; z<nz; ++z) {
        if (cell_objects(x,y,z).size() > MAX_OBJS) {
          Vector xyz(x,y,z);
          BBox cellBounds(min + xyz*cellsize,
                          min + (xyz+Vector(1,1,1)) * cellsize);

          //create recursive grid with tight bounding box
          BBox objBounds;
          for (unsigned int i=0; i < cell_objects(x,y,z).size(); ++i) {
            cell_objects(x,y,z)[i]->computeBounds(context, objBounds);
          }
          cellBounds.intersection(objBounds);

          RecursiveGrid *gg = new RecursiveGrid(numLevels, mesh);
          gg->build(context, cell_objects(x,y,z), cellBounds, depth+1, totalObjects);
          int start = cells(x, y, z);
          lists[start] = gg;
          subGridList(x,y,z)=true;
        }
        else { //just add individual objects to cell
          int start = cells(x, y, z);
          for (unsigned int i=0; i < cell_objects(x,y,z).size(); ++i) {
            lists[start+i] = cell_objects(x,y,z)[i];
          }
        }
      }
    }
  }
}

void RecursiveGrid::intersect( const RenderContext &context, RayPacket &rays ) const
{
  rays.computeInverseDirections();
  for(int i=rays.begin(); i<rays.end(); i++) {

#ifdef COLLECT_STATS
  nTotalRays++;
#endif //COLLECT_STATS

    RayPacket subpacket(rays, i, i+1);

    const Vector dir = rays.getDirection(i);
    if (dir[0] >= 0) {
      if (dir[1] >= 0) {
        if (dir[2] >= 0) {
          intersectRay<true, true, true> (context, subpacket);
        }
        else {
          intersectRay<true, true, false> (context, subpacket);
        }
      }
      else {
        if (dir[2] >= 0) {
          intersectRay<true, false, true> (context, subpacket);
        }
        else {
          intersectRay<true, false, false> (context, subpacket);
        }
      }
    }
    else {
      if (dir[1] >= 0) {
        if (dir[2] >= 0) {
          intersectRay<false, true, true> (context, subpacket);
        }
        else {
          intersectRay<false, true, false> (context, subpacket);
        }
      }
      else {
        if (dir[2] >= 0) {
          intersectRay<false, false, true> (context, subpacket);
        }
        else {
          intersectRay<false, false, false> (context, subpacket);
        }
      }
    }
  }
}

template<const bool posX, const bool posY, const bool posZ>
void RecursiveGrid::intersectRay( const RenderContext &context, RayPacket &rays ) const
{
  const int i = rays.begin();
  const Vector origin(rays.getOrigin(i));
  const Vector direction(rays.getDirection(i));
  const Vector inv_direction(rays.getInverseDirection(i));

  // Step 2
  const Vector v1 = (min-origin)*inv_direction;
  const Vector v2 = (max-origin)*inv_direction;
  const Vector vmin = Min(v1, v2);
  const Vector vmax = Max(v1, v2);
  Real tnear = vmin.maxComponent();
  Real tfar = vmax.minComponent();
  if(tnear >= tfar)
    return;
  if(tfar < (Real)1.e-6)
    return;
  if(tnear < 0)
    tnear = 0;

  // Step 3
  const Vector p = origin + tnear * direction;
  const Vector L = (p-min)*inv_cellsize;
  int Lx = Clamp(static_cast<int>(L.x()), 0, cells.getNx()-1);
  int Ly = Clamp(static_cast<int>(L.y()), 0, cells.getNy()-1);
  int Lz = Clamp(static_cast<int>(L.z()), 0, cells.getNz()-1);

  // Step 4
  const int stopx = posX ? cells.getNx() : -1;
  const int stopy = posY ? cells.getNy() : -1;
  const int stopz = posZ ? cells.getNz() : -1;

  // Step 5
  const Real dtdx = posX? cellsize.x()*inv_direction.x() :
                         -cellsize.x()*inv_direction.x();
  const Real dtdy = posY? cellsize.y()*inv_direction.y() :
                         -cellsize.y()*inv_direction.y();
  const Real dtdz = posZ? cellsize.z()*inv_direction.z() :
                         -cellsize.z()*inv_direction.z();

  // Step 6
  const Real far_x = posX ? Lx*cellsize.x() + min.x() + cellsize.x():
                            Lx*cellsize.x() + min.x();
  const Real far_y = posY ? Ly*cellsize.y() + min.y() + cellsize.y():
                            Ly*cellsize.y() + min.y();
  const Real far_z = posZ ? Lz*cellsize.z() + min.z() + cellsize.z():
                            Lz*cellsize.z() + min.z();

  // Step 7
  Real tnext_x = (far_x - origin.x())*inv_direction.x();
  Real tnext_y = (far_y - origin.y())*inv_direction.y();
  Real tnext_z = (far_z - origin.z())*inv_direction.z();

  // Step 8
  while(tnear < rays.getMinT(i)){

#ifdef COLLECT_STATS
    nCellTraversals++;
#endif //COLLECT_STATS

    const int idx = cells.getIndex(Lx, Ly, Lz);
    int l = cells[idx];
    const int e = cells[idx+1];
    for(;l < e; l++){
      if (subGridList[idx]) {
        static_cast<const RecursiveGrid*>
          (lists[cells[idx]])->intersectRay<posX, posY, posZ>(context, rays);
#ifdef COLLECT_STATS
        nGridTraversals++;
#endif //COLLECT_STATS
      }
      else {
        const Object* obj = lists[l];
        obj->intersect(context, rays);
#ifdef COLLECT_STATS
        nIntersects++;
#endif //COLLECT_STATS
      }
    }
    // Step 11
    if(tnext_x < tnext_y && tnext_x < tnext_z){
      Lx += posX? 1 : -1;
      if(Lx == stopx)
        break;
      tnear = tnext_x;
      tnext_x += dtdx;
    } else if(tnext_y < tnext_z){
      Ly += posY? 1 : -1;
      if(Ly == stopy)
        break;
      tnear = tnext_y;
      tnext_y += dtdy;
    } else {
      Lz += posZ? 1 : -1;
      if(Lz == stopz)
        break;
      tnear = tnext_z;
      tnext_z += dtdz;
    }
  }
}

void RecursiveGrid::update(int proc, int numProcs)
{
#ifdef COLLECT_STATS
  printf("primitive intersections per ray: %f\n", (float)nIntersects/nTotalRays);
  printf("cell traversals per ray:         %f\n", (float)nCellTraversals/nTotalRays);
  printf("grid traversals per ray:         %f\n", (float)nGridTraversals/nTotalRays);
  printf("number of rays:                  %ld\n", nTotalRays);
  nIntersects = 0;
  nCellTraversals = 0;
  nGridTraversals = 0;
  nTotalRays  = 0;
#endif
}
