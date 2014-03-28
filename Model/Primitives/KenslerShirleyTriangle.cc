#include <Model/Primitives/KenslerShirleyTriangle.h>
#include <Interface/Context.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/BBox.h>
#include <Core/Util/Preprocessor.h>
#include <Core/Util/Assert.h>
#include <iostream>

using namespace Manta;
using namespace std;

KenslerShirleyTriangle::KenslerShirleyTriangle(Mesh *mesh, unsigned int id):
  MeshTriangle(mesh, id)
{
}

KenslerShirleyTriangle* KenslerShirleyTriangle::clone(CloneDepth depth, Clonable* incoming)
{
  KenslerShirleyTriangle *copy;
  if (incoming)
    copy = dynamic_cast<KenslerShirleyTriangle*>(incoming);
  else
    return new KenslerShirleyTriangle(mesh, myID);

  copy->myID = myID;
  copy->mesh = mesh;

  return copy;
}

Interpolable::InterpErr KenslerShirleyTriangle::serialInterpolate(const std::vector<keyframe_t> &keyframes)
{
  //We assume that the mesh has already been interpolated.
  return success;
}

void KenslerShirleyTriangle::preprocess(const PreprocessContext& context)
{
  /* Empty */
}

void KenslerShirleyTriangle::computeSurfaceDerivatives(const RenderContext& context, RayPacket& rays) const
{
  const int which = myID*3;
  const unsigned int binormal0_idx = mesh->binormal_indices.size() ?
    mesh->binormal_indices[which] : Mesh::kNoBinormalIndex;

  // No binormals means use the default implementation
  if (binormal0_idx == Mesh::kNoBinormalIndex) {
    MeshTriangle::computeSurfaceDerivatives(context, rays);
    return;
  }

  const unsigned int binormal1_idx = mesh->binormal_indices[which+1];
  const unsigned int binormal2_idx = mesh->binormal_indices[which+2];

  const unsigned int tangent0_idx = mesh->tangent_indices[which+0];
  const unsigned int tangent1_idx = mesh->tangent_indices[which+1];
  const unsigned int tangent2_idx = mesh->tangent_indices[which+2];

  const Vector binormal0 = mesh->vertexBinormals[binormal0_idx];
  const Vector binormal1 = mesh->vertexBinormals[binormal1_idx];
  const Vector binormal2 = mesh->vertexBinormals[binormal2_idx];

  const Vector tangent0 = mesh->vertexTangents[tangent0_idx];
  const Vector tangent1 = mesh->vertexTangents[tangent1_idx];
  const Vector tangent2 = mesh->vertexTangents[tangent2_idx];

  for(int i = rays.begin(); i != rays.end(); ++i) {

    Real a = rays.getScratchpad<Real>(SCRATCH_U)[i];
    Real b = rays.getScratchpad<Real>(SCRATCH_V)[i];
    Real c = (1 - a - b);

    Vector dPdu = a * tangent1 + b * tangent2 + c * tangent0;
    Vector dPdv = a * binormal1 + b * binormal2 + c * binormal0;

    rays.setSurfaceDerivativeU(i, dPdu);
    rays.setSurfaceDerivativeV(i, dPdv);
  }

  rays.setFlag(RayPacket::HaveSurfaceDerivatives);
}

void KenslerShirleyTriangle::computeTexCoords2(const RenderContext&, RayPacket& rays) const
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

#define cross4(xout, yout, zout, x1, y1, z1, x2, y2, z2) \
  {                                                      \
    xout = sub4(mul4(y1, z2), mul4(z1, y2));             \
    yout = sub4(mul4(z1, x2), mul4(x1, z2));             \
    zout = sub4(mul4(x1, y2), mul4(y1, x2));             \
  }

void KenslerShirleyTriangle::intersect( const RenderContext& context, RayPacket& rays ) const {

  RayPacketData *data = rays.data;
  const int ray_begin = rays.begin();
  const int ray_end   = rays.end();
  const int sse_begin = ( ray_begin + 3 ) & ( ~3 );
  const int sse_end   = ( ray_end ) & ( ~3 );

  if (sse_begin >= sse_end) { //no sse section exists.
    const unsigned int index = myID*3;
    const Vector p0 = mesh->vertices[mesh->vertex_indices[index+0]];
    const Vector p1 = mesh->vertices[mesh->vertex_indices[index+1]];
    const Vector p2 = mesh->vertices[mesh->vertex_indices[index+2]];

    const Vector edge0 = p1 - p0;
    const Vector edge1 = p0 - p2;
    const Vector normal = Cross( edge0, edge1 );
    for ( int ray = ray_begin; ray < ray_end; ++ray ) {
      const Vector o = Vector( data->origin[ 0 ][ ray ],
                               data->origin[ 1 ][ ray ],
                               data->origin[ 2 ][ ray ] );
      const float oldt = data->minT[ ray ];
      const Vector d = Vector( data->direction[ 0 ][ ray ],
                               data->direction[ 1 ][ ray ],
                               data->direction[ 2 ][ ray ] );
      const float rcp = 1.0f / Dot( normal, d );
      const Vector edge2 = p0 - o;
      const float toverd = Dot( normal, edge2 ) * rcp;
      if ( toverd > oldt - T_EPSILON ||
           toverd < T_EPSILON )
        continue;
      const Vector interm = Cross( edge2, d );
      const float uoverd = Dot( interm, edge1 ) * rcp;
      if ( uoverd < 0.0f )
        continue;
      const float voverd = Dot( interm, edge0 ) * rcp;
      if ( uoverd + voverd > 1.0f || voverd < 0.0f )
        continue;
      if ( rays.hit( ray, toverd,
                     mesh->materials[ mesh->face_material[ myID ] ],
                     this, this ) ) {
        rays.getScratchpad< float >( SCRATCH_U )[ ray ] = uoverd;
        rays.getScratchpad< float >( SCRATCH_V )[ ray ] = voverd;
      }
    }
    return;
  }

  const sse_t eps4 = set4( T_EPSILON );

  const Vector p0 = mesh->getVertex(myID, 0);
  const Vector p1 = mesh->getVertex(myID, 1);
  const Vector p2 = mesh->getVertex(myID, 2);

  const sse_t p1x = set4( p1[ 0 ] );
  const sse_t p1y = set4( p1[ 1 ] );
  const sse_t p1z = set4( p1[ 2 ] );

  const sse_t p0x = set4( p0[ 0 ] );
  const sse_t p0y = set4( p0[ 1 ] );
  const sse_t p0z = set4( p0[ 2 ] );

  const sse_t edge0x = sub4( p1x, p0x );
  const sse_t edge0y = sub4( p1y, p0y );
  const sse_t edge0z = sub4( p1z, p0z );

  const sse_t p2x = set4( p2[ 0 ] );
  const sse_t p2y = set4( p2[ 1 ] );
  const sse_t p2z = set4( p2[ 2 ] );

  const sse_t edge1x = sub4( p0x, p2x );
  const sse_t edge1y = sub4( p0y, p2y );
  const sse_t edge1z = sub4( p0z, p2z );

  sse_t normalx, normaly, normalz;
  cross4( normalx, normaly, normalz, edge0x, edge0y, edge0z, edge1x, edge1y, edge1z );

  for ( int ray = sse_begin; ray < sse_end; ray += 4 ) {

    const sse_t ox = load44( &data->origin[ 0 ][ ray ] );
    const sse_t oy = load44( &data->origin[ 1 ][ ray ] );
    const sse_t oz = load44( &data->origin[ 2 ][ ray ] );

    const sse_t oldt = load44( &data->minT[ ray ] );

    const sse_t dx = load44( &data->direction[ 0 ][ ray ] );
    const sse_t dy = load44( &data->direction[ 1 ][ ray ] );
    const sse_t dz = load44( &data->direction[ 2 ][ ray ] );

    const sse_t det = dot4( normalx, normaly, normalz, dx, dy, dz );
    const sse_t rcp = oneOver( det );

    const sse_t edge2x = sub4( p0x, ox );
    const sse_t edge2y = sub4( p0y, oy );
    const sse_t edge2z = sub4( p0z, oz );

    const sse_t t = dot4( normalx, normaly, normalz, edge2x, edge2y, edge2z );
    const sse_t toverd = mul4( rcp, t );
    const sse_t tmaskb = cmp4_lt( toverd, sub4( oldt, eps4 ) );
    const sse_t tmaska = cmp4_gt( toverd, eps4 );
    sse_t mask = and4( tmaska, tmaskb );
    if ( getmask4( mask ) == 0x0 )
      continue;

    sse_t intermx, intermy, intermz;
    cross4( intermx, intermy, intermz, edge2x, edge2y, edge2z, dx, dy, dz );

    const sse_t u = dot4( intermx, intermy, intermz, edge1x, edge1y, edge1z );
    const sse_t uoverd = mul4( rcp, u );
    const sse_t umask = cmp4_ge( uoverd, _mm_zero );
    mask = and4( mask, umask );
    if ( getmask4( mask ) == 0x0 )
      continue;

    const sse_t v = dot4( intermx, intermy, intermz, edge0x, edge0y, edge0z );
    const sse_t uplusv = add4( u, v );
    const sse_t uvmask = cmp4_le( mul4( uplusv, det ), mul4( det, det ) );
    const sse_t voverd = mul4( rcp, v );
    const sse_t vmask = cmp4_ge( voverd, _mm_zero );

    mask = and4( mask, uvmask );
    mask = and4( mask, vmask );
    if ( getmask4( mask ) == 0x0 )
      continue;

    rays.hitWithoutTminCheck( ray, mask, toverd,
                              mesh->materials[ mesh->face_material[ myID ] ],
                              this, this );
    float *udata = &rays.getScratchpad< float >( SCRATCH_U )[ ray ];
    float *vdata = &rays.getScratchpad< float >( SCRATCH_V )[ ray ];
    store44( udata, mask4( mask, uoverd, load44( udata ) ) );
    store44( vdata, mask4( mask, voverd, load44( vdata ) ) );

  }

  if ( ray_begin < sse_begin || ray_end > sse_end )
  {
    const Vector edge0 = p1 - p0;
    const Vector edge1 = p0 - p2;
    const Vector normal = Cross( edge0, edge1 );
    for ( int ray = ray_begin; ray < sse_begin; ++ray ) {
      const Vector o = Vector( data->origin[ 0 ][ ray ],
                               data->origin[ 1 ][ ray ],
                               data->origin[ 2 ][ ray ] );
      const float oldt = data->minT[ ray ];
      const Vector d = Vector( data->direction[ 0 ][ ray ],
                               data->direction[ 1 ][ ray ],
                               data->direction[ 2 ][ ray ] );
      const float rcp = 1.0f / Dot( normal, d );
      const Vector edge2 = p0 - o;
      const float toverd = Dot( normal, edge2 ) * rcp;
      if ( toverd > oldt - T_EPSILON ||
           toverd < T_EPSILON )
        continue;
      const Vector interm = Cross( edge2, d );
      const float uoverd = Dot( interm, edge1 ) * rcp;
      if ( uoverd < 0.0f )
        continue;
      const float voverd = Dot( interm, edge0 ) * rcp;
      if ( uoverd + voverd > 1.0f || voverd < 0.0f )
        continue;
      if ( rays.hit( ray, toverd,
                     mesh->materials[ mesh->face_material[ myID ] ],
                     this, this ) ) {
        rays.getScratchpad< float >( SCRATCH_U )[ ray ] = uoverd;
        rays.getScratchpad< float >( SCRATCH_V )[ ray ] = voverd;
      }
    }
    for ( int ray = sse_end; ray < ray_end; ++ray ) {
      const Vector o = Vector( data->origin[ 0 ][ ray ],
                               data->origin[ 1 ][ ray ],
                               data->origin[ 2 ][ ray ] );
      const float oldt = data->minT[ ray ];
      const Vector d = Vector( data->direction[ 0 ][ ray ],
                               data->direction[ 1 ][ ray ],
                               data->direction[ 2 ][ ray ] );
      const float rcp = 1.0f / Dot( normal, d );
      const Vector edge2 = p0 - o;
      const float toverd = Dot( normal, edge2 ) * rcp;
      if ( toverd > oldt - T_EPSILON ||
           toverd < T_EPSILON )
        continue;
      const Vector interm = Cross( edge2, d );
      const float uoverd = Dot( interm, edge1 ) * rcp;
      if ( uoverd < 0.0f )
        continue;
      const float voverd = Dot( interm, edge0 ) * rcp;
      if ( uoverd + voverd > 1.0f || voverd < 0.0f )
        continue;
      if ( rays.hit( ray, toverd,
                     mesh->materials[ mesh->face_material[ myID ] ],
                     this, this ) ) {
        rays.getScratchpad< float >( SCRATCH_U )[ ray ] = uoverd;
        rays.getScratchpad< float >( SCRATCH_V )[ ray ] = voverd;
      }
    }
  }

}

#else // MANTA_SSE

void KenslerShirleyTriangle::intersect( const RenderContext& context, RayPacket& rays ) const {
  RayPacketData *data = rays.data;
  const int ray_begin = rays.begin();
  const int ray_end   = rays.end();

  const unsigned int index = myID*3;
  const Vector p0 = mesh->vertices[mesh->vertex_indices[index+0]];
  const Vector p1 = mesh->vertices[mesh->vertex_indices[index+1]];
  const Vector p2 = mesh->vertices[mesh->vertex_indices[index+2]];

  const Vector edge0 = p1 - p0;
  const Vector edge1 = p0 - p2;
  const Vector normal = Cross( edge0, edge1 );
  for ( int ray = ray_begin; ray < ray_end; ++ray ) {
    const Vector o = Vector( data->origin[ 0 ][ ray ],
                             data->origin[ 1 ][ ray ],
                             data->origin[ 2 ][ ray ] );
    const Real oldt = data->minT[ ray ];
    const Vector d = Vector( data->direction[ 0 ][ ray ],
                             data->direction[ 1 ][ ray ],
                             data->direction[ 2 ][ ray ] );
    const Real rcp = 1.0f / Dot( normal, d );
    const Vector edge2 = p0 - o;
    const Real toverd = Dot( normal, edge2 ) * rcp;
    if ( toverd > oldt - T_EPSILON ||
         toverd < T_EPSILON )
      continue;
    const Vector interm = Cross( edge2, d );
    const Real uoverd = Dot( interm, edge1 ) * rcp;
    if ( uoverd < 0.0f )
      continue;
    const Real voverd = Dot( interm, edge0 ) * rcp;
    if ( uoverd + voverd > 1.0f || voverd < 0.0f )
      continue;
    if ( rays.hit( ray, toverd,
                   mesh->materials[ mesh->face_material[ myID ] ],
                   this, this ) ) {
      rays.getScratchpad< Real >( SCRATCH_U )[ ray ] = uoverd;
      rays.getScratchpad< Real >( SCRATCH_V )[ ray ] = voverd;
    }
  }
}

#endif // MANTA_SSE

void KenslerShirleyTriangle::computeNormal(const RenderContext& context, RayPacket &rays) const
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
  } else {
    const unsigned int index = myID*3;
    const Vector p0 = mesh->vertices[mesh->vertex_indices[index+0]];
    const Vector p1 = mesh->vertices[mesh->vertex_indices[index+1]];
    const Vector p2 = mesh->vertices[mesh->vertex_indices[index+2]];

    const Vector edge0 = p1 - p0;
    const Vector edge1 = p2 - p0;
    const Vector normal = Cross(edge0, edge1);
    for(int ray=rays.begin(); ray<rays.end(); ray++)
      rays.setNormal(ray, normal);
  }
}

void KenslerShirleyTriangle::computeGeometricNormal(const RenderContext& context, RayPacket& rays) const
{
  const unsigned int index = myID*3;
  const Vector p0 = mesh->vertices[mesh->vertex_indices[index+0]];
  const Vector p1 = mesh->vertices[mesh->vertex_indices[index+1]];
  const Vector p2 = mesh->vertices[mesh->vertex_indices[index+2]];

  const Vector edge0 = p1 - p0;
  const Vector edge1 = p2 - p0;
  const Vector normal = Cross(edge0, edge1);
  for(int ray=rays.begin(); ray<rays.end(); ray++)
    rays.setGeometricNormal(ray, normal);
}

namespace Manta {
  MANTA_DECLARE_RTTI_DERIVEDCLASS(KenslerShirleyTriangle, MeshTriangle, ConcreteClass, readwriteMethod);
  MANTA_REGISTER_CLASS(KenslerShirleyTriangle);
}

void KenslerShirleyTriangle::readwrite(ArchiveElement* archive)
{
  MantaRTTI<MeshTriangle>::readwrite(archive, *this);
}

