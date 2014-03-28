#include <Model/Primitives/WaldTriangle.h>
#include <Interface/Context.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/BBox.h>
#include <Core/Util/Preprocessor.h>
#include <iostream>

using namespace Manta;
using namespace std;

WaldTriangle::WaldTriangle(Mesh *mesh, unsigned int id):
  MeshTriangle(mesh, id)
{
  update();
}

void WaldTriangle::update()
{
  const unsigned int index = myID*3;
  const Vector p1 = mesh->vertices[mesh->vertex_indices[index+0]];
  const Vector p2 = mesh->vertices[mesh->vertex_indices[index+1]];
  const Vector p3 = mesh->vertices[mesh->vertex_indices[index+2]];

  setPoints(p1, p2, p3);
}

WaldTriangle* WaldTriangle::clone(CloneDepth depth, Clonable* incoming)
{
  WaldTriangle *copy;
  if (incoming)
    copy = dynamic_cast<WaldTriangle*>(incoming);
  else
    copy = new WaldTriangle();

  copy->n_u = n_u;
  copy->n_v = n_v;
  copy->n_d = n_d;
  copy->k = k;
  copy->b_nu = b_nu;
  copy->b_nv = b_nv;
  copy->b_d = b_d;
  copy->n_k = n_k;
  copy->c_nu = c_nu;
  copy->c_nv = c_nv;
  copy->c_d = c_d;
  copy->myID = myID;
  copy->mesh = mesh;

  return copy;
}

Interpolable::InterpErr WaldTriangle::serialInterpolate(const std::vector<keyframe_t> &keyframes)
{
  //We assume that the mesh has already been interpolated.
  update();
  return success;
}

void WaldTriangle::preprocess(const PreprocessContext& context)
{
  update();
}

void WaldTriangle::setPoints(const Vector& _p1, const Vector& _p2, const Vector& _p3)
{
    Vector normal = Cross(_p2-_p1, _p3-_p1);
    normal.normalize();

    unsigned int n, u, v;

    n = 0;
    if ( fabsf(normal[1]) > fabsf(normal[n])) n = 1;
    if ( fabsf(normal[2]) > fabsf(normal[n])) n = 2;

    switch ( n )
    {
    case 0: u = 1; v = 2; break;
    case 1: u = 2; v = 0; break;
    default: u = 0; v = 1; break;
    };

    // copy to struct
    k = n;
    Real inv_normalk = 1 / normal[k];
    n_u = normal[u] * inv_normalk;
    n_v = normal[v] * inv_normalk;
    n_d = _p1[u] * n_u + _p1[v] * n_v + _p1[k];

    Real s;

    c_nu = + _p2[v] - _p1[v];
    c_nv = - _p2[u] + _p1[u];
    c_d  = - (_p1[u] * c_nu + _p1[v] * c_nv);

    s = 1.f / ( _p3[u] * c_nu + _p3[v] * c_nv + c_d );

    c_nu *= s;
    c_nv *= s;
    c_d  *= s;

    b_nu = + _p3[v] - _p1[v];
    b_nv = - _p3[u] + _p1[u];
    b_d  = - (_p1[u] * b_nu + _p1[v] * b_nv);

    s = 1.f / ( _p2[u] * b_nu + _p2[v] * b_nv + b_d );

    b_nu *= s;
    b_nv *= s;
    b_d  *= s;

    n_k = normal[k];
#if 0
    std::cerr << "n_u: " << n_u << std::endl
              << "n_v: " << n_v << std::endl
              << "n_d: " << n_d << std::endl
              << "k:   " << k << std::endl

              << "b_nu: " << b_nu << std::endl
              << "b_nv: " << b_nv << std::endl
              << "b_d:  " << b_d << std::endl
              << "n_k:  " << n_k << std::endl

              << "c_nu: " << c_nu << std::endl
              << "c_nv: " << c_nv << std::endl
              << "c_d:  " << c_d  << std::endl;
#endif
}



void WaldTriangle::computeTexCoords2(const RenderContext&, RayPacket& rays) const
{
  const int which = myID*3;

  const unsigned int index0 = mesh->texture_indices.size() ?
                              mesh->texture_indices[which] : Mesh::kNoTextureIndex;

  if (index0 == Mesh::kNoTextureIndex) {
    // NOTE(boulos): Assume that if the first index is invalid, they
    // all are.  In this case, set the texture coordinates to be the
    // barycentric coordinates of the triangle.
    for(int ray=rays.begin(); ray<rays.end(); ray++) {
      Real a = rays.getScratchpad<Real>(SCRATCH_U)[ray];
      Real b = rays.getScratchpad<Real>(SCRATCH_V)[ray];
      Real c = (1.0 - a - b);

      rays.setTexCoords(ray, Vector(a, b, c));
    }
    rays.setFlag( RayPacket::HaveTexture2|RayPacket::HaveTexture3 );
    return;
  }

  const unsigned int index1 = mesh->texture_indices[which+1];
  const unsigned int index2 = mesh->texture_indices[which+2];


  const Vector &tex0 = mesh->texCoords[index0];
  const Vector &tex1 = mesh->texCoords[index1];
  const Vector &tex2 = mesh->texCoords[index2];

  for(int ray=rays.begin(); ray<rays.end(); ray++) {
    Real a = rays.getScratchpad<Real>(SCRATCH_U)[ray];
    Real b = rays.getScratchpad<Real>(SCRATCH_V)[ray];
    Real c = (1.0 - a - b);

    rays.setTexCoords(ray, (tex1 * a) + (tex2 * b) + (tex0 * c));
  }
  rays.setFlag( RayPacket::HaveTexture2|RayPacket::HaveTexture3 );
}


#ifdef MANTA_SSE

//These should be defined in SSEDefs.h, but aren't...
#define none4(mask) (getmask4( (mask) ) == 0x0)
#define all4(mask) (getmask4( (mask) ) == 0xf)
#define any4(mask) (getmask4( (mask) ) != 0x0)


void WaldTriangle::intersect(const RenderContext& context, RayPacket& rays) const {
  bool debugFlag = rays.getFlag(RayPacket::DebugPacket);
  if (debugFlag) {
    cerr << MANTA_FUNC << " called\n";
    cerr << "Rays are: \n" << rays << endl;
  }

    const int axis = k;
    const int ku = (k==2)?0:k+1;
    const int kv = (k==0)?2:k-1;

    RayPacketData* data = rays.data;

    const bool HasCommonOrigin = rays.getFlag(RayPacket::ConstantOrigin);
    const bool HasCornerRays = rays.getFlag(RayPacket::HaveCornerRays);
    const int ray_begin = rays.begin();
    const int ray_end   = rays.end();
    const int sse_begin = (ray_begin + 3) & (~3);
    const int sse_end   = (ray_end) & (~3);

    if (sse_begin >= sse_end) { //no sse section exists.

      const Real* const dir_k  = data->direction[axis];
      const Real* const dir_ku = data->direction[ku];
      const Real* const dir_kv = data->direction[kv];

      Real org_k = 0,
        org_ku = 0,
        org_kv = 0,
        f0 = 0;

      if (HasCommonOrigin) {
        org_k  = data->origin[axis][rays.begin()];
        org_ku = data->origin[ku][rays.begin()];
        org_kv = data->origin[kv][rays.begin()];
        f0 = n_d - (org_k + n_u * org_ku + n_v * org_kv);
      }

      for (int i = rays.begin(); i < rays.end(); i++ ) {
        const Real nd0 = n_u * dir_ku[i] + n_v * dir_kv[i] + dir_k[i];
        const Real nd  = 1.f/nd0;

        if (!HasCommonOrigin) {
          org_k  = data->origin[axis][i];
          org_ku = data->origin[ku][i];
          org_kv = data->origin[kv][i];

          f0 = n_d - (org_k + n_u * org_ku + n_v * org_kv);
        }

        const Real f = f0 * nd;
        // plane test
        if ( f < T_EPSILON || f > data->minT[i] )
          continue;

        const Real hu = org_ku + f*dir_ku[i];
        const Real hv = org_kv + f*dir_kv[i];
        const Real lambda = b_d + hu*b_nu + hv * b_nv;

        // barycentric test
        if ( lambda < 0.f )
          continue;

        const Real mue = c_d + hu * c_nu + hv * c_nv;
        if ( mue < 0.f || mue + lambda > 1.f )
          continue;

        const bool hit = rays.hit(i, f, mesh->materials[mesh->face_material[myID]], this, this);
        if (hit) {
          Real *u = &rays.getScratchpad<Real>(SCRATCH_U)[i];
          Real *v = &rays.getScratchpad<Real>(SCRATCH_V)[i];
          *u = lambda;
          *v = mue;
        }
      }
      return;
    }

    const sse_t sse_n_u  = set4(n_u);
    const sse_t sse_n_v  = set4(n_v);
    const sse_t sse_n_d  = set4(n_d);

    const sse_t sse_b_nu = set4(b_nu);
    const sse_t sse_b_nv = set4(b_nv);
    const sse_t sse_b_d  = set4(b_d);

    const sse_t sse_c_nu = set4(c_nu);
    const sse_t sse_c_nv = set4(c_nv);
    const sse_t sse_c_d  = set4(c_d);

    // Initialized to quiet warnings
    sse_t org_k  = zero4();
    sse_t org_ku = zero4();
    sse_t org_kv = zero4();
    sse_t f0     = zero4();
    if (HasCommonOrigin) {
      org_k  = set4(data->origin[axis][ray_begin]);
      org_ku = set4(data->origin[ku][ray_begin]);
      org_kv = set4(data->origin[kv][ray_begin]);
      f0 = sub4(sse_n_d,
                add4(org_k,
                     add4(mul4(sse_n_u, org_ku),
                          mul4(sse_n_v, org_kv))));

      if (HasCornerRays) {
        const sse_t nd0 = add4(add4(mul4(sse_n_u,load44(data->corner_dir[ku])),
                                    mul4(sse_n_v,load44(data->corner_dir[kv]))),
                               load44(data->corner_dir[k]));
        const sse_t nd = oneOver(nd0);
        const sse_t f  = mul4(f0,nd);
        sse_t mask = cmp4_gt(f,_mm_eps);
        if (none4(mask))
          return;

        const sse_t hu = add4(org_ku,mul4(f,load44(data->corner_dir[ku])));
        const sse_t hv = add4(org_kv,mul4(f,load44(data->corner_dir[kv])));
        const sse_t lambda = add4(sse_b_d,
                                  add4(mul4(hu,sse_b_nu),
                                       mul4(hv,sse_b_nv)));
        if (all4(lambda))
          return;

        const sse_t mue = add4(sse_c_d,add4(mul4(hu,sse_c_nu),
                                            mul4(hv,sse_c_nv)));
        if (all4(mue))
          return;

        mask = cmp4_gt(add4(lambda,mue),_mm_one);
        if (all4(mask))
          return;
      }
    }

    if (ray_begin < sse_begin) {
      //TODO: the trailing code is just copied over from the non-sse
      //intersection method.  There is some redundant work and plenty
      //that could still be optimized with sse.
      const Real* const dir_k  = data->direction[axis];
      const Real* const dir_ku = data->direction[ku];
      const Real* const dir_kv = data->direction[kv];

      Real org_k  = 0;
      Real org_ku = 0;
      Real org_kv = 0;
      Real f0     = 0;

      const bool RaysConstantOrigin = rays.getAllFlags() & RayPacket::ConstantOrigin;

      if (RaysConstantOrigin)
      {
        org_k  = data->origin[axis][rays.begin()];
        org_ku = data->origin[ku][rays.begin()];
        org_kv = data->origin[kv][rays.begin()];
        f0 = n_d - (org_k + n_u * org_ku + n_v * org_kv);
      }

      for (int i = ray_begin; i < sse_begin; i++ )
      {
        const Real nd0 = n_u * dir_ku[i] + n_v * dir_kv[i] + dir_k[i];
        const Real nd  = 1.f/nd0;

        if (!RaysConstantOrigin)
        {
          org_k  = data->origin[axis][i];
          org_ku = data->origin[ku][i];
          org_kv = data->origin[kv][i];

          f0 = n_d - (org_k + n_u * org_ku + n_v * org_kv);
        }

        const Real f = f0 * nd;
        // plane test
        if ( f < T_EPSILON || f > data->minT[i] )
          continue;

        const Real hu = org_ku + f*dir_ku[i];
        const Real hv = org_kv + f*dir_kv[i];
        const Real lambda = b_d + hu*b_nu + hv * b_nv;

        // barycentric test
        if ( lambda < 0.f )
          continue;

        const Real mue = c_d + hu * c_nu + hv * c_nv;
        if ( mue < 0.f || mue + lambda > 1.f )
          continue;

        const bool hit = rays.hit(i, f, mesh->materials[mesh->face_material[myID]], this, this);
        if (hit) {
          Real *u = &rays.getScratchpad<Real>(SCRATCH_U)[i];
          Real *v = &rays.getScratchpad<Real>(SCRATCH_V)[i];
          *u = lambda;
          *v = mue;
        }
      }
    }

    for (int ray = sse_begin; ray < sse_end; ray += 4) {
      const sse_t nd0 = add4(add4(mul4(sse_n_u, load44(&data->direction[ku][ray])),
                                  mul4(sse_n_v, load44(&data->direction[kv][ray]))),
                             load44(&data->direction[k][ray]));
      const sse_t nd  = oneOver(nd0);

      if (!HasCommonOrigin) {
        org_k  = load44(&data->origin[axis][ray]);
        org_ku = load44(&data->origin[ku][ray]);
        org_kv = load44(&data->origin[kv][ray]);
        f0     = sub4(set4(n_d),
                      add4(org_k, add4(mul4(sse_n_u,
                                            org_ku),
                                       mul4(sse_n_v,
                                            org_kv))));
      }

      const sse_t f = mul4(f0, nd); // maybe these would be faster as swizzle after load44
      // plane test
      sse_t mask_test = and4( _mm_cmpnle_ps(f, set4(T_EPSILON)),
                              _mm_cmpnle_ps(load44(&data->minT[ray]), f));
      if (getmask4(mask_test) == 0x0) continue;

      const sse_t hu = add4( mul4(f, load44(&data->direction[ku][ray])), org_ku);
      const sse_t hv = add4( mul4(f, load44(&data->direction[kv][ray])), org_kv);
      const sse_t lambda = add4( sse_b_d,
                                 add4( mul4(hu, sse_b_nu),
                                       mul4(hv, sse_b_nv)));
      mask_test = and4(mask_test, _mm_cmpnlt_ps(lambda, zero4()));
      if (getmask4(mask_test) == 0x0) continue;

      const sse_t mue = add4( sse_c_d,
                              add4( mul4(hu, sse_c_nu),
                                    mul4(hv, sse_c_nv)));

      mask_test = and4(mask_test, and4( _mm_cmpnlt_ps(mue, zero4()),
                                        _mm_cmpnlt_ps(set4(1.f), add4(mue, lambda))));

      rays.hitWithoutTminCheck(ray, mask_test, f, mesh->materials[mesh->face_material[myID]], this, this);

      Real *u = &rays.getScratchpad<Real>(SCRATCH_U)[ray];
      Real *v = &rays.getScratchpad<Real>(SCRATCH_V)[ray];
//       int *which = rays.getScratchpad<int*>(2)[ray];

      store44(u, mask4(mask_test, lambda, load44(u)));
      store44(v, mask4(mask_test, mue, load44(v)));
//       store44i(which, mask4i((sse_t)mask_test, set4i(myID), load44i(which)));
    }

    if (sse_end < ray_end) {
      const Real* const dir_k  = data->direction[axis];
      const Real* const dir_ku = data->direction[ku];
      const Real* const dir_kv = data->direction[kv];

      // Initialized to quiet warnings
      Real org_k  = 0;
      Real org_ku = 0;
      Real org_kv = 0;
      Real f0     = 0;

      const bool RaysConstantOrigin = rays.getAllFlags() & RayPacket::ConstantOrigin;

      if (RaysConstantOrigin)
      {
        org_k  = data->origin[axis][rays.begin()];
        org_ku = data->origin[ku][rays.begin()];
        org_kv = data->origin[kv][rays.begin()];
        f0 = n_d - (org_k + n_u * org_ku + n_v * org_kv);
      }

      for (int i = sse_end; i < ray_end; i++ )
      {
        const Real nd0 = n_u * dir_ku[i] + n_v * dir_kv[i] + dir_k[i];
        const Real nd  = 1.f/nd0;

        if (!RaysConstantOrigin)
        {
          org_k  = data->origin[axis][i];
          org_ku = data->origin[ku][i];
          org_kv = data->origin[kv][i];

          f0 = n_d - (org_k + n_u * org_ku + n_v * org_kv);
        }

        const Real f = f0 * nd;
        // plane test
        if ( f < T_EPSILON || f > data->minT[i] )
          continue;

        const Real hu = org_ku + f*dir_ku[i];
        const Real hv = org_kv + f*dir_kv[i];
        const Real lambda = b_d + hu*b_nu + hv * b_nv;

        // barycentric test
        if ( lambda < 0.f )
          continue;

        const Real mue = c_d + hu * c_nu + hv * c_nv;
        if ( mue < 0.f || mue + lambda > 1.f )
          continue;

        const bool hit = rays.hit(i, f, mesh->materials[mesh->face_material[myID]], this, this);
        if (hit) {
          Real *u = &rays.getScratchpad<Real>(SCRATCH_U)[i];
          Real *v = &rays.getScratchpad<Real>(SCRATCH_V)[i];
          *u = lambda;
          *v = mue;
        }
      }
    }
}
#else
void WaldTriangle::intersect(const RenderContext& context, RayPacket& rays) const
{
   const int axis = k;
   const int ku = (k==2)?0:k+1;
   const int kv = (k==0)?2:k-1;

   // what qualifiers go here?
   RayPacketData* data = rays.data;

   const Real* const dir_k  = data->direction[axis];
   const Real* const dir_ku = data->direction[ku];
   const Real* const dir_kv = data->direction[kv];

   Real org_k = 0,
     org_ku = 0,
     org_kv = 0,
     f0 = 0;

   const bool RaysConstantOrigin = rays.getAllFlags() & RayPacket::ConstantOrigin;

   if (RaysConstantOrigin) {
     org_k  = data->origin[axis][rays.begin()];
     org_ku = data->origin[ku][rays.begin()];
     org_kv = data->origin[kv][rays.begin()];
     f0 = n_d - (org_k + n_u * org_ku + n_v * org_kv);
   }

   for (int i = rays.begin(); i < rays.end(); i++ )
   {
      const Real nd0 = n_u * dir_ku[i] + n_v * dir_kv[i] + dir_k[i];
      const Real nd  = 1.f/nd0;

      if (!RaysConstantOrigin)
      {
         org_k  = data->origin[axis][i];
         org_ku = data->origin[ku][i];
         org_kv = data->origin[kv][i];

         f0 = n_d - (org_k + n_u * org_ku + n_v * org_kv);
      }

      const Real f = f0 * nd;
      // plane test
      if ( f < T_EPSILON || f > data->minT[i] )
         continue;

      const Real hu = org_ku + f*dir_ku[i];
      const Real hv = org_kv + f*dir_kv[i];
      const Real lambda = b_d + hu*b_nu + hv * b_nv;

      // barycentric test
      if ( lambda < 0.f )
         continue;

      const Real mue = c_d + hu * c_nu + hv * c_nv;
      if ( mue < 0.f || mue + lambda > 1.f )
         continue;

      const bool hit = rays.hit(i, f, mesh->materials[mesh->face_material[myID]], this, this);
      if (hit) {
        Real *u = &rays.getScratchpad<Real>(SCRATCH_U)[i];
        Real *v = &rays.getScratchpad<Real>(SCRATCH_V)[i];
        *u = lambda;
        *v = mue;
      }
   }
}
#endif // MANTA_SSE

void WaldTriangle::computeNormal(const RenderContext& context, RayPacket &rays) const
{
  if (mesh->hasVertexNormals()) {
    for(int ray=rays.begin(); ray<rays.end(); ray++) {

      const Real a = rays.getScratchpad<Real>(SCRATCH_U)[ray];
      const Real b = rays.getScratchpad<Real>(SCRATCH_V)[ray];
      const Real c = (1.0 - a - b);

      const Vector &n0 = mesh->vertexNormals[mesh->normal_indices[myID*3+0]];
      const Vector &n1 = mesh->vertexNormals[mesh->normal_indices[myID*3+1]];
      const Vector &n2 = mesh->vertexNormals[mesh->normal_indices[myID*3+2]];

      const Vector normal = (n1*a) + (n2*b) + (n0*c);
      rays.setNormal(ray, normal);
    }
  }
  else {
    const int axis = k;
    const int ku = (k==2)?0:k+1;
    const int kv = (k==0)?2:k-1;

    const Real nu = n_k*n_u;
    const Real nv = n_k*n_v;
    RayPacketData* data = rays.data;
    for ( int ray = rays.begin(); ray < rays.end(); ray++ ) {
      data->normal[axis][ray] = n_k;
      data->normal[ku][ray]   = nu;
      data->normal[kv][ray]   = nv;
    }
    rays.setFlag(RayPacket::HaveUnitNormals);
  }
}

void WaldTriangle::computeGeometricNormal(const RenderContext& context,
                                          RayPacket& rays) const
{
  const int axis = k;
  const int ku = (k==2)?0:k+1;
  const int kv = (k==0)?2:k-1;

  const Real nu = n_k*n_u;
  const Real nv = n_k*n_v;
  RayPacketData* data = rays.data;
  for ( int ray = rays.begin(); ray < rays.end(); ray++ ) {
    data->geometricNormal[axis][ray] = n_k;
    data->geometricNormal[ku][ray]   = nu;
    data->geometricNormal[kv][ray]   = nv;
  }
  rays.setFlag( RayPacket::HaveGeometricNormals |
                RayPacket::HaveUnitGeometricNormals );
}
