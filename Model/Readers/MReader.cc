#include <Model/Readers/MReader.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Textures/NormalTexture.h>
#include <Model/Primitives/WaldTriangle.h>
#include <Model/Primitives/KenslerShirleyTriangle.h>
#include <Model/Primitives/MovingKSTriangle.h>

#include <iostream>
#include <fstream>

using namespace Manta;
using namespace std;
 
Mesh* Manta::readM(const std::string &filename,
                    Material *defaultMaterial,
                    const MeshTriangle::TriangleType triangleType)
{
  ifstream in(filename.c_str());
  if (!in) {
    cerr << "Error trying to read in .m file: " 
         << filename << endl;
    return NULL;
  }

  Mesh *mesh = new Mesh;

  if (defaultMaterial == NULL)
    defaultMaterial = new Lambertian( new NormalTexture() );
  mesh->materials.push_back(defaultMaterial);


  while (in.good()) {
    string type;
    in >> type;

    if (type == "Vertex") {
      size_t i;
      in >> i;
      if (mesh->vertices.size() < i)
        mesh->vertices.resize(i);

      in >> mesh->vertices[i-1];
    }
    else if (type == "Face") {
      size_t i;
      in >> i;

      if (mesh->vertex_indices.size() < i*3)
        mesh->vertex_indices.resize(i*3);
      
      int idx;
      in >> idx; mesh->vertex_indices[i*3-3] = idx-1;
      in >> idx; mesh->vertex_indices[i*3-2] = idx-1;
      in >> idx; mesh->vertex_indices[i*3-1] = idx-1;

      mesh->face_material.push_back(0);
    }
    else {
      //do nothing.
    }
  }

  size_t numTri = mesh->face_material.size();

  WaldTriangle *wald_triangles = NULL;
  KenslerShirleyTriangle *KS_triangles = NULL;
  MovingKSTriangle* MovingKS_triangles = NULL;
  switch (triangleType) {
  case MeshTriangle::WALD_TRI:
    wald_triangles = new WaldTriangle[numTri];
    break;
  case MeshTriangle::KENSLER_SHIRLEY_TRI:
    KS_triangles = new KenslerShirleyTriangle[numTri];
    break;
  case MeshTriangle::MOVING_KS_TRI:
    MovingKS_triangles = new MovingKSTriangle[numTri];
    break;
  default:
    throw InternalError("Invalid triangle type");
    break;
  }

  for (size_t i=0; i < numTri; ++i) {
    switch (triangleType) {
    case MeshTriangle::WALD_TRI:
      mesh->addTriangle(&wald_triangles[i]);
      break;
    case MeshTriangle::KENSLER_SHIRLEY_TRI:
      mesh->addTriangle(&KS_triangles[i]);
      break;
    case MeshTriangle::MOVING_KS_TRI:
      mesh->addTriangle(&MovingKS_triangles[i]);
      break;
    }
  }

  return mesh;
}
