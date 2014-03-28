#ifndef Manta_Model_Intersections_TriangleBBoxOverlap_h
#define Manta_Model_Intersections_TriangleBBoxOverlap_h

#include <Model/Primitives/MeshTriangle.h>

namespace Manta {

  bool triBoxOverlap(const MeshTriangle* tri, const BBox& bbox);

};
#endif
