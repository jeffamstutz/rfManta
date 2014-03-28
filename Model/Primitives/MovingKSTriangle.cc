#include <Model/Primitives/MovingKSTriangle.h>
#include <Interface/Context.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/BBox.h>
#include <Core/Util/Preprocessor.h>
#include <Core/Util/Assert.h>
#include <iostream>

using namespace Manta;
using namespace std;

MovingKSTriangle::MovingKSTriangle(Mesh *mesh, unsigned int id):
  MeshTriangle(mesh, id)
{
}

MovingKSTriangle* MovingKSTriangle::clone(CloneDepth depth, Clonable* incoming)
{
  MovingKSTriangle *copy;
  if (incoming)
    copy = dynamic_cast<MovingKSTriangle*>(incoming);
  else
    return new MovingKSTriangle(mesh, myID);

  copy->myID = myID;
  copy->mesh = mesh;

  return copy;
}

Interpolable::InterpErr MovingKSTriangle::serialInterpolate(const std::vector<keyframe_t> &keyframes)
{
  //We assume that the mesh has already been interpolated.
  return success;
}

void MovingKSTriangle::preprocess(const PreprocessContext& context)
{
  if (context.proc != 0) { context.done(); return; }
  //TODO: this materials->preprocess might end up getting called lots
  //of times (imagine all the triangles share the same
  //material). Would be nice to have the preprocess for this in the
  //mesh class and just iterate over the materials. Not sure how to do
  //that, so we will do extra work for now.
  mesh->materials[mesh->face_material[myID]]->preprocess(context);
  context.done();
}

void MovingKSTriangle::computeTexCoords2(const RenderContext&, RayPacket& rays) const
{
  const int which = myID*3;

  const unsigned int index0 = mesh->texture_indices.size() ?
                              mesh->texture_indices[which] : Mesh::kNoTextureIndex;

  if (index0 == Mesh::kNoTextureIndex) {
    // NOTE(boulos): Assume that if the first index is invalid, they
    // all are.  In this case, set the texture coordinates to be the
    // barycentric coordinates of the triangle.
    for(int ray=rays.begin(); ray<rays.end(); ray++) {
      float a = rays.getScratchpad<float>(SCRATCH_U)[ray];
      float b = rays.getScratchpad<float>(SCRATCH_V)[ray];
      float c = (1.0 - a - b);

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
    float a = rays.getScratchpad<float>(SCRATCH_U)[ray];
    float b = rays.getScratchpad<float>(SCRATCH_V)[ray];
    float c = (1.0 - a - b);

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

void MovingKSTriangle::intersect( const RenderContext& context, RayPacket& rays ) const {

  RayPacketData *data = rays.data;
  const int ray_begin = rays.begin();
  const int ray_end   = rays.end();
  const int sse_begin = ( ray_begin + 3 ) & ( ~3 );
  const int sse_end   = ( ray_end ) & ( ~3 );

  const sse_t eps4 = set4( T_EPSILON );

  for ( int ray = sse_begin; ray < sse_end; ray += 4 ) {

    Vector p0_vec[4];
    Vector p1_vec[4];
    Vector p2_vec[4];
    for (int i = 0; i < 4; i++) {
      const Vector p0 = mesh->getVertex(myID, 0, rays.getTime(ray + i));
      const Vector p1 = mesh->getVertex(myID, 1, rays.getTime(ray + i));
      const Vector p2 = mesh->getVertex(myID, 2, rays.getTime(ray + i));
      p0_vec[i] = p0;
      p1_vec[i] = p1;
      p2_vec[i] = p2;
    }

    const sse_t p1x = set44( p1_vec[0][0], p1_vec[1][0], p1_vec[2][0], p1_vec[3][0] );
    const sse_t p1y = set44( p1_vec[0][1], p1_vec[1][1], p1_vec[2][1], p1_vec[3][1] );
    const sse_t p1z = set44( p1_vec[0][2], p1_vec[0][2], p1_vec[0][2], p1_vec[3][2] );

    const sse_t p0x = set44( p0_vec[0][0], p0_vec[1][0], p0_vec[2][0], p0_vec[3][0] );
    const sse_t p0y = set44( p0_vec[0][1], p0_vec[1][1], p0_vec[2][1], p0_vec[3][1] );
    const sse_t p0z = set44( p0_vec[0][2], p0_vec[0][2], p0_vec[0][2], p0_vec[3][2] );

    const sse_t edge0x = sub4( p1x, p0x );
    const sse_t edge0y = sub4( p1y, p0y );
    const sse_t edge0z = sub4( p1z, p0z );

    const sse_t p2x = set44( p2_vec[0][0], p2_vec[1][0], p2_vec[2][0], p2_vec[3][0] );
    const sse_t p2y = set44( p2_vec[0][1], p2_vec[1][1], p2_vec[2][1], p2_vec[3][1] );
    const sse_t p2z = set44( p2_vec[0][2], p2_vec[0][2], p2_vec[0][2], p2_vec[3][2] );


    const sse_t edge1x = sub4( p0x, p2x );
    const sse_t edge1y = sub4( p0y, p2y );
    const sse_t edge1z = sub4( p0z, p2z );

    sse_t normalx, normaly, normalz;
    cross4( normalx, normaly, normalz, edge0x, edge0y, edge0z, edge1x, edge1y, edge1z );

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
    for ( int ray = ray_begin; ray < sse_begin; ++ray ) {
      const Vector p0 = mesh->getVertex(myID, 0, rays.getTime(ray));
      const Vector p1 = mesh->getVertex(myID, 1, rays.getTime(ray));
      const Vector p2 = mesh->getVertex(myID, 2, rays.getTime(ray));

      const Vector edge0 = p1 - p0;
      const Vector edge1 = p0 - p2;
      const Vector normal = Cross( edge0, edge1 );

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
      const Vector p0 = mesh->getVertex(myID, 0, rays.getTime(ray));
      const Vector p1 = mesh->getVertex(myID, 1, rays.getTime(ray));
      const Vector p2 = mesh->getVertex(myID, 2, rays.getTime(ray));

      const Vector edge0 = p1 - p0;
      const Vector edge1 = p0 - p2;
      const Vector normal = Cross( edge0, edge1 );

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

void MovingKSTriangle::intersect( const RenderContext& context, RayPacket& rays ) const {
  RayPacketData *data = rays.data;
  const int ray_begin = rays.begin();
  const int ray_end   = rays.end();

  for ( int ray = ray_begin; ray < ray_end; ++ray ) {
    const Vector p0 = mesh->getVertex(myID, 0, rays.getTime(ray));
    const Vector p1 = mesh->getVertex(myID, 1, rays.getTime(ray));
    const Vector p2 = mesh->getVertex(myID, 2, rays.getTime(ray));

    const Vector edge0 = p1 - p0;
    const Vector edge1 = p0 - p2;
    const Vector normal = Cross( edge0, edge1 );

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

#endif // MANTA_SSE

void MovingKSTriangle::computeNormal(const RenderContext& context, RayPacket &rays) const
{
  if (mesh->hasVertexNormals()) {
    for(int ray=rays.begin(); ray<rays.end(); ray++) {
      const float a = rays.getScratchpad<float>(SCRATCH_U)[ray];
      const float b = rays.getScratchpad<float>(SCRATCH_V)[ray];
      const float c = (1.0 - a - b);
      const Vector &n0 = mesh->vertexNormals[mesh->normal_indices[myID*3+0]];
      const Vector &n1 = mesh->vertexNormals[mesh->normal_indices[myID*3+1]];
      const Vector &n2 = mesh->vertexNormals[mesh->normal_indices[myID*3+2]];
      const Vector normal = (n1*a) + (n2*b) + (n0*c);
      rays.setNormal(ray, normal);
    }
  } else {
    for(int ray=rays.begin(); ray<rays.end(); ray++) {
      const Vector p0 = mesh->getVertex(myID, 0, rays.getTime(ray));
      const Vector p1 = mesh->getVertex(myID, 1, rays.getTime(ray));
      const Vector p2 = mesh->getVertex(myID, 2, rays.getTime(ray));

      const Vector edge0 = p1 - p0;
      const Vector edge1 = p2 - p0;
      const Vector normal = Cross(edge0, edge1);
      rays.setNormal(ray, normal);
    }
  }
}
