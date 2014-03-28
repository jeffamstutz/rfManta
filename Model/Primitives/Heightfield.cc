
#include <Model/Primitives/Heightfield.h>
#include <Interface/RayPacket.h>
#include <Core/Exceptions/InternalError.h>
#include <Core/Geometry/BBox.h>
#include <Core/Geometry/Vector.h>
#include <Core/Util/Assert.h>
#include <Core/Util/Preprocessor.h>

#include <limits>
#include <fstream>
#include <iostream>

using namespace Manta;
using namespace std;


Heightfield::Heightfield(Material *material, const string& filename,
                         const Vector &minBound, const Vector &maxBound// ,
//                          Real scale
                         )
  : PrimitiveCommon(material), barrier("heightfield barrier"),
    mutex("heightfield mutex")
{
  cout << "\n\nbounds are: " << minBound << " " <<maxBound<<endl;
  ifstream in(filename.c_str());
  if(!in){
    throw InternalError("Error opening " + filename);
  }
  Real minz, maxz;
  in >> nx >> ny >> minz >> maxz;
  if(!in){
    throw InternalError("Error reading header from " + filename);
  }
  in.get();
  data = new float*[nx+1];
  float* p = new float[(nx+1)*(ny+1)];
  for(int i=0;i<=nx;i++)
    data[i] = p+i*(ny+1);
  in.read(reinterpret_cast<char*>(data[0]), sizeof(float)*(nx+1)*(ny+1));
  if(!in){
    throw InternalError("Error reading data from " + filename);
  }
  Real dz = maxz-minz;
  if(dz < 1.e-3)
    dz = 1.e-3;
  m_Box = BBox( Vector(minBound.x(), minBound.y(), minz-dz*1.e-4),
                Vector(maxBound.x(), maxBound.y(), maxz+dz*1.e-4));
  // Step 1
  diag = m_Box.diagonal();
  cellsize = diag/Vector(nx, ny, 1);
  inv_cellsize = cellsize.inverse();

//   readHeightfieldFile(fileName, &m_Nx, &m_Ny, &m_Data);
//   rescaleDataHeight(scale);
}

Heightfield::~Heightfield()
{
  for (int i=0; i<=nx; ++i)
    delete data[i];
  delete[] data;
}

Heightfield* Heightfield::clone(Clonable::CloneDepth depth, Clonable *incoming)
{
  Heightfield *h;
  if (incoming)
    h = dynamic_cast<Heightfield*>(incoming);
  else
    h = new Heightfield();

  PrimitiveCommon::clone(depth, h);

  h->m_Box = m_Box;
  h->nx = nx;
  h->ny = ny;
  h->diag = diag;
  h->cellsize = cellsize;
  h->inv_cellsize = inv_cellsize;

  h->data = new float*[nx+1];
  float* p = new float[(nx+1)*(ny+1)];
  for(int i=0;i<=nx;i++)
    h->data[i] = p+i*(ny+1);

  return h;
}

Interpolable::InterpErr 
Heightfield::serialInterpolate(const std::vector<keyframe_t> &keyframes)
{
  return parallelInterpolate(keyframes, 0, 1);
}

Interpolable::InterpErr 
Heightfield::parallelInterpolate(const std::vector<keyframe_t> &keyframes,
                                 int proc, int numProc)
{
  //TODO: Parallelize this
  ASSERT(!keyframes.empty());

  Heightfield **heightfields = MANTA_STACK_ALLOC(Heightfield*, keyframes.size());

  for (unsigned int frame=0; frame < keyframes.size(); ++frame) {
    Heightfield *h = dynamic_cast<Heightfield*>(keyframes[frame].keyframe);
    if (h == NULL)
      return notInterpolable;
    heightfields[frame] = h;

    if (frame>0) {
      ASSERT(h->nx == heightfields[0]->nx);
      ASSERT(h->ny == heightfields[0]->ny);
    }
  }


  float minz = std::numeric_limits<float>::max(),
        maxz = -std::numeric_limits<float>::max();

  if (proc == 0) {
    PrimitiveCommon::interpolate(keyframes);
    m_Box = BBox( Vector(m_Box.getMin().x(), m_Box.getMin().y(), minz),
                  Vector(m_Box.getMax().x(), m_Box.getMax().y(), maxz));

    if (nx != heightfields[0]->nx || ny != heightfields[0]->ny) {
      for (int i=0; i<=nx; ++i)
        delete data[i];
      delete[] data;
    
      nx = heightfields[0]->nx;
      ny = heightfields[0]->ny;

      data = new float*[nx+1];
      float* p = new float[(nx+1)*(ny+1)];
      for(int i=0;i<=nx;i++)
        data[i] = p+i*(ny+1);
    }
  }

  barrier.wait(numProc);  

  //being tricky here and using the fact that data is contiguously allocated
  int start = proc*( (nx+1)*(ny+1) )/numProc;
  int end = (proc+1)*( (nx+1)*(ny+1) )/numProc;
  for (int i = start; i < end; ++i) {
    float val = 0;
    //lets hope this loop gets unrolled!
    for (unsigned int frame=0; frame < keyframes.size(); ++frame) {
      val += keyframes[frame].t * heightfields[frame]->data[0][i]; 
    }
    data[0][i] = val;
    minz = min(data[0][i], minz);
    maxz = max(data[0][i], maxz);
  }

  float dz = maxz-minz;
  if(dz < 1.e-3)
    dz = 1.e-3;

  mutex.lock();
  m_Box.extendByBox(BBox( Vector(m_Box.getMin().x(), m_Box.getMin().y(), minz-dz*1.e-4),
                          Vector(m_Box.getMax().x(), m_Box.getMax().y(), maxz+dz*1.e-4)));
  mutex.unlock();

  barrier.wait(numProc);

  if (proc == 0) {
    // Step 1
    diag = m_Box.diagonal();
    cellsize = diag/Vector(nx, ny, 1);
    inv_cellsize = cellsize.inverse();
  }

  return success;
}


void Heightfield::computeBounds(const PreprocessContext& /*context*/,
                                BBox & bbox) const
{
  bbox.extendByBox(m_Box);
}


// --------------------------------------------------------------------------------------
// --- Test whether the ray intersects the Heightfield or not
// --------------------------------------------------------------------------------------
void Heightfield::intersect(const RenderContext& /*context*/,
                            RayPacket& rays) const
{
  rays.normalizeDirections();
  rays.computeInverseDirections();

  for(int rayIndex=rays.begin(); rayIndex<rays.end(); rayIndex++) {
    Ray ray = rays.getRay(rayIndex);

    // Step 2
    Vector idir = rays.getInverseDirection(rayIndex);
    Vector v1 = (m_Box.getMin()-ray.origin())*idir;
    Vector v2 = (m_Box.getMax()-ray.origin())*idir;
    Vector vmin = Min(v1, v2);
    Vector vmax = Max(v1, v2);
    Real tnear = vmin.maxComponent();
    Real tfar = vmax.minComponent();
    if(tnear >= tfar)
      continue;
    if(tfar < 1.e-6)
      continue;
    if(tnear < 0)
      tnear = 0;

    // Step 3
    Vector p = ray.origin() + tnear * ray.direction();
    int Lx = (int)((p.x()-m_Box.getMin().x())*inv_cellsize.x());
    Lx = Lx<0?0:Lx>=nx?nx-1:Lx;
    int Ly = (int)((p.y()-m_Box.getMin().y())*inv_cellsize.y());
    if(Ly < 0) Ly = 0;
    else if(Ly >= ny) Ly = ny-1;

    // Step 4
    Real signx = diag.x()*ray.direction().x();
    Real signy = diag.y()*ray.direction().y();
    int dix = signx>0?1:-1;
    int stopx = signx>0?nx:-1;
    int diy = signy>0?1:-1;
    int stopy = signy>0?ny:-1;

    // Step 5
    Real dtdx = Abs(cellsize.x()/ray.direction().x());
    Real dtdy = Abs(cellsize.y()/ray.direction().y());

    // Step 6
    Real far_x;
    if(signx>0)
      far_x = (Lx+1)*cellsize.x() + m_Box.getMin().x();
    else
      far_x = Lx*cellsize.x() + m_Box.getMin().x();
    Real far_y;
    if(signy>0)
      far_y = (Ly+1)*cellsize.y() + m_Box.getMin().y();
    else
      far_y = Ly*cellsize.y() + m_Box.getMin().y();

    // Step 7
    Real tnext_x = (far_x - ray.origin().x())/ray.direction().x();
    Real tnext_y = (far_y - ray.origin().y())/ray.direction().y();

    // Step 8
    Real zenter = ray.origin().z() + tnear * ray.direction().z();
    tfar = Min((Real)tfar, rays.getMinT(rayIndex));
    while(tnear < tfar) {
      Real texit = Min(tnext_x, tnext_y);
      Real zexit = ray.origin().z() + texit * ray.direction().z();

      // Step 9
      float datamin = Min(Min(data[Lx][Ly], data[Lx+1][Ly]), 
                                  Min(data[Lx][Ly+1], data[Lx+1][Ly+1]));
      float datamax = Max(Max(data[Lx][Ly], data[Lx+1][Ly]),
                                  Max(data[Lx][Ly+1], data[Lx+1][Ly+1]));
      float zmin = Min(zenter, zexit);
      float zmax = Max(zenter, zexit);
      if(zmin < datamax && zmax > datamin){
        // Step 10
        Vector C = m_Box.getMin() + Vector(Lx, Ly, 0)*cellsize;
        Vector EC = ray.origin()+tnear*ray.direction()-C;
        Real Ex = EC.x()*inv_cellsize.x();
        Real Ey = EC.y()*inv_cellsize.y();
        Real Ez = ray.origin().z()+tnear*ray.direction().z();
        Real Vx = ray.direction().x()*inv_cellsize.x();
        Real Vy = ray.direction().y()*inv_cellsize.y();
        Real Vz = ray.direction().z();
        float za = data[Lx][Ly];
        float zb = data[Lx+1][Ly]-za;
        float zc = data[Lx][Ly+1]-za;
        float zd = data[Lx+1][Ly+1]-zb-zc-za;
        Real a = Vx*Vy*zd;
        Real b = -Vz + Vx*zb + Vy*zc + (Ex*Vy + Ey*Vx)*zd;
        Real c = -Ez + za + Ex*zb + Ey*zc + Ex*Ey*zd;
        if(Abs(a) < 1.e-6){
          // Linear
          Real tcell = -c/b;
          if(tcell > 0 && tnear+tcell < texit){
            if(rays.hit(rayIndex, tnear+tcell, getMaterial(), this, getTexCoordMapper())) {
              rays.scratchpad<Vector>(rayIndex) = Vector(Lx, Ly, 0);
              break;
            }
          }
        } else {
          // Solve quadratic
          Real disc = b*b-4*a*c;
          if(disc > 0){
            Real root = sqrt(disc);
            Real tcell1 = (-b + root)/(2*a);
            Real tcell2 = (-b - root)/(2*a);
            if(tcell1 >= 0 && tnear+tcell1 <= texit){
              if(rays.hit(rayIndex, tnear+tcell1, getMaterial(), this, getTexCoordMapper())) {
                if(tcell2 >= 0)
                  // Check the other root in case it is closer.
                  // No need for an additional if() because it will still
                  // be in the same cell
                  rays.hit(rayIndex, tnear+tcell2, getMaterial(), this, getTexCoordMapper());
                rays.scratchpad<Vector>(rayIndex) = Vector(Lx, Ly, 0);
                break;
              }
            } else if(tcell2 >= 0 && tnear+tcell2 <= texit){
              if(rays.hit(rayIndex, tnear+tcell2, getMaterial(), this, getTexCoordMapper())){ 
                rays.scratchpad<Vector>(rayIndex) = Vector(Lx, Ly, 0);
                break;
              }
            }
          }
        }
      }
      // Step 11
      zenter = zexit;
      if(tnext_x < tnext_y){
        Lx += dix;
        if(Lx == stopx)
          break;
        tnear = tnext_x;
        tnext_x += dtdx;
      } else {
        Ly += diy;
        if(Ly == stopy)
          break;
        tnear = tnext_y;
        tnext_y += dtdy;
      }
    }

  }
}


// --------------------------------------------------------------------------------------
// --- Set the normals
// --------------------------------------------------------------------------------------
void Heightfield::computeNormal(const RenderContext& /*context*/,
                                RayPacket& rays) const
{
  rays.computeHitPositions();
  
  for (int rayIndex=rays.begin(); rayIndex<rays.end(); rayIndex++) {
    int Lx = (int)rays.scratchpad<Vector>(rayIndex).x();
    int Ly = (int)rays.scratchpad<Vector>(rayIndex).y();
    Vector C = m_Box.getMin() + Vector(Lx, Ly, 0)*cellsize;
    Real u = (rays.getHitPosition(rayIndex).x()-C.x())*inv_cellsize.x();
    Real v = (rays.getHitPosition(rayIndex).y()-C.y())*inv_cellsize.y();
    Real dudx = inv_cellsize.x();
    Real dvdy = inv_cellsize.y();
    float za = data[Lx][Ly];
    float zb = data[Lx+1][Ly]-za;
    float zc = data[Lx][Ly+1]-za;
    float zd = data[Lx+1][Ly+1]-zb-zc-za;
    Real px = dudx*zb + dudx*v*zd;
    Real py = dvdy*zc + dvdy*u*zd;
    rays.setNormal( rayIndex, Vector(-px, -py, 1) );
  }
  rays.setFlag(RayPacket::HaveNormals);
}


// // --------------------------------------------------------------------------------------
// // --- Rescale the height of the data to fit the Box
// // --- according to the given percentage of the size of the box to which the data should be rescaled
// // --------------------------------------------------------------------------------------
// void Heightfield::rescaleDataHeight(Real scale)
// {
//   using Min;
//   using Max;

//   unsigned int i, j;
//   Real min, max, factor, margin;

//   min = m_Data[0][0];
//   max = min;
//   for(i=0; i<=m_Nx; i++)
//     for(j=0; j<=m_Ny; j++)
//     {
//       min = Min(min, m_Data[i][j]);
//       max = Max(max, m_Data[i][j]);
//     }

//   factor = m_Box.getMax().z() - m_Box.getMin().z();
//   margin = factor * (1 - scale) * (Real)0.5;
//   factor *= scale / (max - min);

//   for(i=0; i<=m_Nx; i++)
//     for(j=0; j<=m_Ny; j++)
//       m_Data[i][j] = ((m_Data[i][j] - min) * factor) + (m_Box.getMin().z() + margin);
// }
