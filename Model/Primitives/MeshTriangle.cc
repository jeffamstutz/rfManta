#include <Model/Primitives/MeshTriangle.h>

using namespace Manta;

// NOTE(arobison): I tried to create smooth surface derivatives here
// using the ATI PN Triangle paper's trianglur spline and computing
// dPdu and dPdv from it; not a good idea.  These splines can produce
// curvature discontinuties at triangles boundaries, which is exactly
// what we're trying to smooth over.

void MeshTriangle::computeSurfaceDerivatives(const RenderContext& context,
                                             RayPacket& rays) const
{
  const int which = myID*3;

  const unsigned int uv0_idx = mesh->texture_indices.size() ?
                               mesh->texture_indices[which] : Mesh::kNoTextureIndex;

  // No texture coords means use the default implementation
  if (uv0_idx == Mesh::kNoTextureIndex) {
    Primitive::computeSurfaceDerivatives(context, rays);
    return;
  }

  const unsigned int uv1_idx = mesh->texture_indices[which+1];
  const unsigned int uv2_idx = mesh->texture_indices[which+2];

  const Vector uv0 = mesh->texCoords[uv0_idx];
  const Vector uv1 = mesh->texCoords[uv1_idx];
  const Vector uv2 = mesh->texCoords[uv2_idx];

  const unsigned int vec0_idx = mesh->vertex_indices[which];
  const unsigned int vec1_idx = mesh->vertex_indices[which+1];
  const unsigned int vec2_idx = mesh->vertex_indices[which+2];

  const Vector vec0 = mesh->vertices[vec0_idx];
  const Vector vec1 = mesh->vertices[vec1_idx];
  const Vector vec2 = mesh->vertices[vec2_idx];

  // compute dPdu and dPdv based on face normals
  const Vector uv_diff0 = uv1 - uv0;
  const Vector uv_diff1 = uv2 - uv0;

  const Vector edge0 = vec1 - vec0;
  const Vector edge1 = vec2 - vec0;

  const Real determinant = (uv_diff0[0] * uv_diff1[1] -
                            uv_diff0[1] * uv_diff1[0]);

  Vector dPdu, dPdv;

  if(Abs(determinant) < 1e-8) {
    dPdu = edge0;
    dPdv = edge1;
  } else {
    Real inv_det = 1./determinant;
    dPdu = inv_det * (uv_diff1[1] * edge0 - uv_diff0[1] * edge1);
    dPdv = inv_det * (uv_diff0[0] * edge1 - uv_diff1[0] * edge0);
  }

  for(int i = rays.begin(); i != rays.end(); ++i) {
    rays.setSurfaceDerivativeU(i, dPdu);
    rays.setSurfaceDerivativeV(i, dPdv);
  }
  rays.setFlag( RayPacket::HaveSurfaceDerivatives );
}

namespace Manta {
  MANTA_REGISTER_CLASS(MeshTriangle);
}

void MeshTriangle::readwrite(ArchiveElement* archive)
{
  MantaRTTI<Primitive>::readwrite(archive, *this);
  archive->readwrite("myID", myID);
  archive->readwrite("mesh", mesh);
}

