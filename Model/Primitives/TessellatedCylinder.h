
#ifndef Manta_Model_TessellatedCylinder_h
#define Manta_Model_TessellatedCylinder_h

#include <Model/Groups/Mesh.h>
#include <Core/Geometry/Vector.h>
#include <Interface/Material.h>
namespace Manta
{
  //Creates a cylinder made of triangles.
  //Only use this over the normal Cylinder if you must have only triangles.
  class TessellatedCylinder : public Mesh {
  public:
    //tessellation*2 is the number of vertices of cylinder.
    TessellatedCylinder(Material* mat, const Vector& axis, const Vector& center,
                        Real radius, Real height, int tessellation);
    virtual ~TessellatedCylinder();
  };
}

#endif


