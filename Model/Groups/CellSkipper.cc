#include <Model/Groups/CellSkipper.h>
#include <Interface/Context.h>
#include <Core/Thread/Time.h>
#include <Model/Intersections/TriangleBBoxOverlap.h>
#include <Model/Primitives/MeshTriangle.h>

#include <stdio.h>
#include <iostream>

using namespace std;
using namespace Manta;

const unsigned char CellSkipper::INF = 255; //max skip value

CellSkipper::CellSkipper(Group *group) : lists(NULL), currGroup(group)
{
}

CellSkipper::~CellSkipper()
{
  clearGrid();
}


void CellSkipper::clearGrid() {
  cells.resize(0,0,0,0);
  delete[] lists;
}

void CellSkipper::transformToLattice(const Vector& v,
                                      int& x, int& y, int& z) const
{
  const Vector s = (v-min)*inv_cellsize;
  x = Floor(s.x()); y = Floor(s.y()); z = Floor(s.z());
}

void CellSkipper::transformToLattice(const BBox& box,
                                      int& sx, int& sy, int& sz,
                                      int& ex, int& ey, int& ez) const
{
  Vector s = (box.getMin()-min)*inv_cellsize-Vector(1.e-6,1.e-6,1.e-6);
  sx = Floor(s.x()); sy = Floor(s.y()); sz = Floor(s.z());
  Vector e = (box.getMax()-min)*inv_cellsize+Vector(1.e-6,1.e-6,1.e-6);
  ex = Ceil(e.x()); ey = Ceil(e.y()); ez = Ceil(e.z());
}


void CellSkipper::preprocess( const PreprocessContext &context )
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

// #if defined (COLLECT_STATS) || defined (PARAMETER_SEARCH)
// if (context.proc == 0 && context.isInitialized())
//   context.manta_interface->registerSerialPreRenderCallback(
//     Callback::create(this, &CellSkipper::update));
// #endif
}

void CellSkipper::setGroup(Group* new_group)
{
  currGroup = new_group;
}

Group* CellSkipper::getGroup() const
{
  return currGroup;
}

void CellSkipper::rebuild(int proc, int numProcs)
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
  const double start = Time::currentSeconds();

  build(context, currGroup->getVectorOfObjects(), bounds);

  const double dt = Time::currentSeconds()-start;
  printf("Cell Skipper Grid built in %f seconds for a %d object scene\n", dt, (int)currGroup->size());
//   printf("There are %ld, %ld, %ld, %ld grids per level for a total of %ld cells\n",
//          nGrids[0], nGrids[1], nGrids[2], nGrids[3], nCells);
  printf("%d x %d x %d\n", cells.getNx(), cells.getNy(), cells.getNz());
}

void CellSkipper::build(const PreprocessContext &context,
                          const vector<Object*> &objs, const BBox &bbox)
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
  int totalObjects = objs.size();
  float exponent = 1.0/3.0;

    //7.0/27; //optimal for single level grids (use scale of 350).
    //4/3.0 / 3.0; //optimal for single level grid of long skinny triangles.

  //Found 8^(1/3) to work well for this codebase. It's possible this
  //could change with updated code.
  //float scale = 8;
  float scale = 100; //for cell skipping need finer res grid

  double m = powf( scale * totalObjects, exponent);
  //note: m^3 = M = total cells for that specfic grid level

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

  GridArray3<vector<Object*> > cell_objects;
  cell_objects.resize(nx, ny, nz,1);

  Mesh *mesh = dynamic_cast<Mesh*>(currGroup);

//   cerr << "1. Initializing grid\n";
  cells.initialize(CellData());

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
//             nTriRefs++;
            cells(x, y, z).listIndex++;
            cell_objects(x, y, z).push_back(obj);
          }
        }
      }
    }
  }

//   cerr << "3. Count cells\n";
  int sum = 0;
  int nUsedCells = 0;
  for(int i=0;i<M;i++){
    int nobj = cell_objects[i].size();
    cells[i].listIndex = sum;
    sum+=nobj;
    if (nobj > 0)
      nUsedCells++;
  }
  cells[M].listIndex = sum;

//   cerr << "4. Allocating " << sum << " list entries\n";
  lists = new const Object*[sum];

  GridArray3<int> offset(nx, ny, nz);
  offset.initialize(0);

//   cerr << "5. Filling in lists\n";
  for (int x=0; x<nx; ++x) {
    for (int y=0; y<ny; ++y) {
      for (int z=0; z<nz; ++z) {
        int start = cells(x, y, z).listIndex;
        for (unsigned int i=0; i < cell_objects(x,y,z).size(); ++i) {
          lists[start+i] = cell_objects(x,y,z)[i];
        }
      }
    }
  }

  initializeCellSkipping();
}


void CellSkipper::intersect( const RenderContext &context, RayPacket &rays ) const
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

#if 0
//standard single ray traversal (useful for benchmarking/debugging)
template<const bool posX, const bool posY, const bool posZ>
void CellSkipper::intersectRay( const RenderContext &context, RayPacket &rays ) const
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
    int l = cells[idx].listIndex;
    const int e = cells[idx+1].listIndex;
    for(;l < e; l++){
      const Object* obj = lists[l];
      obj->intersect(context, rays);
#ifdef COLLECT_STATS
      nIntersects++;
#endif //COLLECT_STATS
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

#else

template<const bool posX, const bool posY, const bool posZ>
void CellSkipper::intersectRay(const RenderContext &context,
                               RayPacket &rays) const
{
#define MIN_SKIP_SIZE 2

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

  //get principal direction
  int principal_dir;
  const Vector dirMagnitudes(posX? direction[0] : -direction[0],
                             posY? direction[1] : -direction[1],
                             posZ? direction[2] : -direction[2]);

  if (dirMagnitudes[0] > dirMagnitudes[1]) {
    if (dirMagnitudes[0] > dirMagnitudes[2])
      principal_dir = posX? 0 : 1;
    else
      principal_dir = posZ? 4 : 5;
  }
  else if (dirMagnitudes[1] > dirMagnitudes[2])
    principal_dir = posY? 2 :3;
  else
    principal_dir = posZ? 4 :5;

  const float L1_distance_inv = 1.0f / (dirMagnitudes[0] + dirMagnitudes[1] + dirMagnitudes[2]);

  // Step 8
  while(tnear < rays.getMinT(i)){
#ifdef COLLECT_STATS
    nCellTraversals++;
#endif //COLLECT_STATS

    const int idx = cells.getIndex(Lx, Ly, Lz);
    int l = cells[idx].listIndex;
    const int e = cells[idx+1].listIndex;
    for(;l < e; l++){
      const Object* obj = lists[l];
      obj->intersect(context, rays);
#ifdef COLLECT_STATS
      nIntersects++;
#endif //COLLECT_STATS
    }

    int cell_skips_i = cells[idx].distances[principal_dir];

    if (cell_skips_i > MIN_SKIP_SIZE) {
      //steps+=cell_skips_i;
//         cell_skip_steps++;
      if (cell_skips_i == INF && true) {
        return;
      }
      else{
        const float cell_skips = (float) cell_skips_i;

        //cell_skips -= 1; //we need to go one less (do we????)

        const Vector startPoint = origin + direction*tnear;
        const Vector endPoint = startPoint + ((cell_skips*cellsize) * L1_distance_inv) * direction;

        const Vector end_cell_idxs = endPoint - min;

        Lx = static_cast<int>(end_cell_idxs[0] * inv_cellsize[0]);
        if ( (posX && Lx >= cells.getNx()) || //did we leave the grid?
             (!posX && Lx < 0))
          return;

        Ly = static_cast<int>(end_cell_idxs[1] * inv_cellsize[1]);
        if ( (posY && Ly >= cells.getNy()) ||
             (!posY && Ly < 0))
          return;

        Lz = static_cast<int>(end_cell_idxs[2] * inv_cellsize[2]);
        if ( (posZ && Lz >= cells.getNz()) ||
             (!posZ && Lz < 0))
          return;

        const Vector idxs(Lx, Ly, Lz);
        const Vector cellMin = min + idxs*cellsize;

        float txmin;
        if (posX) {
          txmin   = (cellMin[0] - origin[0]) * inv_direction[0];
          tnext_x = txmin + dtdx;
        }
        else {
          tnext_x = (cellMin[0] - origin[0]) * inv_direction[0];
          txmin   = tnext_x - dtdx;
        }

        float tymin;
        if (posY) {
          tymin   = (cellMin[1] - origin[1]) * inv_direction[1];
          tnext_y = tymin + dtdy;
        }
        else {
          tnext_y = (cellMin[1] - origin[1]) * inv_direction[1];
          tymin   = tnext_y - dtdy;
        }

        float tzmin;
        if (posZ) {
          tzmin   = (cellMin[2] - origin[2]) * inv_direction[2];
          tnext_z = tzmin + dtdz;
        }
        else {
          tnext_z = (cellMin[2] - origin[2]) * inv_direction[2];
          tzmin   = tnext_z - dtdz;
        }

        tnear = std::max(txmin, std::max(tymin, tzmin));
      }
    }
    else {

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
}

#endif

//Note: this build code is very slow and can likely be optimized a whole lot...
inline bool CellSkipper::hasDirectNeighbor(int i, int j, int k, int direction) const {

  return( (direction != 0 && i > 0               && cellHasObjects(i-1,j,k)) ||
          (direction != 1 && i < cells.getNx()-1 && cellHasObjects(i+1,j,k)) ||
          (direction != 2 && j > 0               && cellHasObjects(i,j-1,k)) ||
          (direction != 3 && j < cells.getNy()-1 && cellHasObjects(i,j+1,k)) ||
          (direction != 4 && k > 0               && cellHasObjects(i,j,k-1)) ||
          (direction != 5 && k < cells.getNz()-1 && cellHasObjects(i,j,k+1)) );
}

bool CellSkipper::hasDiagonalNeighbor(int i, int j, int k, int direction) const{
  switch (direction) {
  case 0: case 1: //X axis
    return( (j>0 && k>0 &&               cellHasObjects(i,j-1,k-1)) ||
            (j<cells.getNy()-1 && k>0 && cellHasObjects(i,j+1,k-1)) ||
            (j>0 && k<cells.getNz()-1 && cellHasObjects(i,j-1,k+1)) ||
            (j<cells.getNy()-1 && k<cells.getNz()-1 && cellHasObjects(i,j+1,k+1)) );
  case 2: case 3: // Y axis
    return( (i>0 && k>0 &&              cellHasObjects(i-1,j,k-1)) ||
            (i<cells.getNx()-1 && k>0 && cellHasObjects(i+1,j,k-1)) ||
            (i>0 && k<cells.getNz()-1 && cellHasObjects(i-1,j,k+1)) ||
            (i<cells.getNx()-1 && k<cells.getNz()-1 && cellHasObjects(i+1,j,k+1)) );
  case 4: case 5: // Z axis
    return( (i>0 && j>0 &&              cellHasObjects(i-1,j-1,k)) ||
            (i<cells.getNx()-1 && j>0 && cellHasObjects(i+1,j-1,k)) ||
            (i>0 && j<cells.getNy()-1 && cellHasObjects(i-1,j+1,k)) ||
            (i<cells.getNx()-1 && j<cells.getNy()-1 && cellHasObjects(i+1,j+1,k)) );
  default: return false;  //should never occur.
  }
}


inline unsigned char CellSkipper::getDirectCount(int i, int j, int k,
                                                 int direction) const {
  switch (direction) {
  case 0: return cells(i+1,j,k).distances[0];
  case 1: return cells(i-1,j,k).distances[1];
  case 2: return cells(i,j+1,k).distances[2];
  case 3: return cells(i,j-1,k).distances[3];
  case 4: return cells(i,j,k+1).distances[4];
  case 5: return cells(i,j,k-1).distances[5];
  default: return 0; //should never occur.
  }
}

inline void CellSkipper::getDiagonalsCount(unsigned char diagonals[8], int i,
                                           int j, int k, int direction)  const{
  switch (direction) {
  case 0: i+=2;
  case 1: i--;
    diagonals[0] = j == 0?               INF: cells(i,j-1,k).distances[direction];
    diagonals[1] = j == cells.getNy()-1? INF: cells(i,j+1,k).distances[direction];
    diagonals[2] = k == 0?               INF: cells(i,j,k-1).distances[direction];
    diagonals[3] = k == cells.getNz()-1? INF: cells(i,j,k+1).distances[direction];
    diagonals[4] = (j == 0 || k == 0)?               INF: cells(i,j-1,k-1).distances[direction];
    diagonals[5] = (j == cells.getNy()-1 || k == 0)? INF: cells(i,j+1,k-1).distances[direction];
    diagonals[6] = (j == 0 || k == cells.getNz()-1)? INF: cells(i,j-1,k+1).distances[direction];
    diagonals[7] = (j == cells.getNy()-1 || k == cells.getNz()-1)? INF: cells(i,j+1,k+1).distances[direction];
    break;
  case 2: j+=2;
  case 3: j--;
    diagonals[0] = i == 0?               INF: cells(i-1,j,k).distances[direction];
    diagonals[1] = i == cells.getNx()-1? INF: cells(i+1,j,k).distances[direction];
    diagonals[2] = k == 0?               INF: cells(i,j,k-1).distances[direction];
    diagonals[3] = k == cells.getNz()-1? INF: cells(i,j,k+1).distances[direction];
    diagonals[4] = (i == 0 || k == 0)?               INF: cells(i-1,j,k-1).distances[direction];
    diagonals[5] = (i == cells.getNx()-1 || k == 0)? INF: cells(i+1,j,k-1).distances[direction];
    diagonals[6] = (i == 0 || k == cells.getNz()-1)? INF: cells(i-1,j,k+1).distances[direction];
    diagonals[7] = (i == cells.getNx()-1 || k == cells.getNz()-1)? INF: cells(i+1,j,k+1).distances[direction];
    break;
  case 4: k+=2;
  case 5: k--;
    diagonals[0] = i == 0?               INF: cells(i-1,j,k).distances[direction];
    diagonals[1] = i == cells.getNx()-1? INF: cells(i+1,j,k).distances[direction];
    diagonals[2] = j == 0?               INF: cells(i,j-1,k).distances[direction];
    diagonals[3] = j == cells.getNy()-1? INF: cells(i,j+1,k).distances[direction];
    diagonals[4] = (i == 0 || j == 0)?               INF: cells(i-1,j-1,k).distances[direction];
    diagonals[5] = (i == cells.getNx()-1 || j == 0)? INF: cells(i+1,j-1,k).distances[direction];
    diagonals[6] = (i == 0 || j == cells.getNy()-1)? INF: cells(i-1,j+1,k).distances[direction];
    diagonals[7] = (i == cells.getNx()-1 || j == cells.getNy()-1)? INF: cells(i+1,j+1,k).distances[direction];
    break;
  }
}

inline void CellSkipper::setInitialDistances(int i, int j, int k, int direction) {
  //check for triangles to the sides
  if (hasDirectNeighbor(i,j,k, direction))
    cells(i,j,k).distances[direction] = 1;
  else if (hasDiagonalNeighbor(i,j,k, direction))
    cells(i,j,k).distances[direction] = 2;
  else
    cells(i,j,k).distances[direction] = INF;
}

void CellSkipper::setDistances(int i, int j, int k, int direction) {

  //check for triangles to the sides
  if (hasDirectNeighbor(i,j,k, direction)) {
    cells(i,j,k).distances[direction] = 1;
  }

  else if (hasDiagonalNeighbor(i,j,k, direction)) {
    cells(i,j,k).distances[direction] = 2;
  }
  else {

    unsigned char direct = getDirectCount(i, j, k, direction);
    unsigned char diagonals[8];
    getDiagonalsCount(diagonals, i, j, k, direction);

    unsigned char min_direct;

    switch (direct) {
    case INF-1: min_direct = INF-1; break;
    case INF: min_direct = INF; break;
    default: min_direct = direct+1;
    }

    unsigned char min_diagonal = diagonals[0];
    for (int d=1; d < 4; d++) {
      min_diagonal = std::min(min_diagonal, diagonals[d]);
    }
    for (int d=4; d < 8; d++) {
      if (diagonals[d] < INF-1)
        min_diagonal = std::min(min_diagonal, static_cast<unsigned char>(diagonals[d]+1));
      else
        min_diagonal = std::min(min_diagonal, diagonals[d]);
    }

    if (min_diagonal < INF-1)
      min_diagonal++;
    if (min_diagonal < INF-1)
      min_diagonal++;

    cells(i,j,k).distances[direction] = std::min(min_direct, min_diagonal);
  }
}

void CellSkipper::initializeCellSkipping() {

  for (int i = 0; i < cells.getNy(); i++)
    for (int j = 0; j < cells.getNz(); j++) {
      setInitialDistances(cells.getNx()-1, i, j, 0);
      setInitialDistances(0,              i, j, 1);
    }

  for (int i = 0; i < cells.getNx(); i++)
    for (int j = 0; j < cells.getNz(); j++) {
      setInitialDistances(i, cells.getNy()-1, j, 2);
      setInitialDistances(i, 0,              j, 3);
    }

  for (int i = 0; i < cells.getNx(); i++)
    for (int j = 0; j < cells.getNy(); j++) {
      setInitialDistances(i, j, cells.getNz()-1, 4);
      setInitialDistances(i, j, 0,              5);
    }

  //fill in insides

  for (int i = cells.getNx()-2; i>=0; i--)
    for (int j = 0; j < cells.getNy() ; j++)
      for (int k = 0; k < cells.getNz(); k++)
        setDistances(i, j, k, 0);

  for (int i = 1; i < cells.getNx(); i++)
    for (int j = 0; j < cells.getNy() ; j++)
      for (int k = 0; k < cells.getNz(); k++)
        setDistances(i, j, k, 1);

  for (int j = cells.getNy()-2; j >= 0; j--)
    for (int i = 0; i < cells.getNx(); i++)
      for (int k = 0; k < cells.getNz(); k++)
        setDistances(i, j, k, 2);

  for (int j = 1; j < cells.getNy() ; j++)
    for (int i = 0; i < cells.getNx(); i++)
      for (int k = 0; k < cells.getNz(); k++)
        setDistances(i, j, k, 3);

  for (int k = cells.getNz()-2; k >=0; k--)
    for (int i = 0; i < cells.getNx(); i++)
      for (int j = 0; j < cells.getNy() ; j++)
        setDistances(i, j, k, 4);

  for (int k = 1; k < cells.getNz(); k++)
    for (int i = 0; i < cells.getNx(); i++)
      for (int j = 0; j < cells.getNy() ; j++)
        setDistances(i, j, k, 5);
}
