#ifndef Manta_Model_MReader_h
#define Manta_Model_MReader_h

/* This reads in geometry from an .m format.
  The m-file looks like:

Vertex 1  -0.0396703 0.036174 -0.00733977
#this is a comment
Vertex 2  0.0171854 0.035921 0.00432788
Vertex 3  -0.0291577 0.0380555 0.005854
Vertex 4  -0.0310017 0.0339922 0.010953
Vertex 5  0.0189601 0.036126 0.0191857
Vertex 6  -0.0438465 0.0359346 0.00839642
Face 1  25732 14061 11668
Face 2  27453 19874 33417
Face 3  2453 1753 1189
Face 4  15343 27545 5744

*/

#include <Core/Geometry/Vector.h>
#include <Core/Geometry/VectorT.h>
#include <Model/Groups/Mesh.h>
#include <Model/Primitives/MeshTriangle.h>
#include <MantaTypes.h>
#include <string>

namespace Manta {

  //returns NULL if problem reading in m file.
  Mesh* readM(const std::string &filename,
              Material *defaultMaterial=NULL,
              const MeshTriangle::TriangleType triangleType=
              MeshTriangle::KENSLER_SHIRLEY_TRI);

} // end namespace Manta

#endif // Manta_Model_MReader_h
