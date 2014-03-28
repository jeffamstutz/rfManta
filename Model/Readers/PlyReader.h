
#ifndef Manta_Model_PlyReader_h
#define Manta_Model_PlyReader_h

#include <Core/Geometry/AffineTransform.h>
#include <Model/Groups/Mesh.h>
#include <Model/Primitives/MeshTriangle.h>
#include <string>

namespace Manta {

  using std::string;
  
  class Material;
  class Group;
  
  extern "C" bool readPlyFile(const string fileName, const AffineTransform &t, 
                              Mesh *mesh, Material *m=0,
                              MeshTriangle::TriangleType triangleType = MeshTriangle::KENSLER_SHIRLEY_TRI);
}

#endif
