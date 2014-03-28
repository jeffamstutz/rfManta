#include <Model/Primitives/TessellatedCylinder.h>
#include <Model/Primitives/KenslerShirleyTriangle.h>
#include <Core/Geometry/AffineTransform.h>
#include <assert.h>

using namespace Manta;

TessellatedCylinder::TessellatedCylinder(Material* mat, 
      const Vector& axis, const Vector& center, Real radius,
      Real height, int tessellation)
{
  assert(tessellation>2);

  AffineTransform t;
  t.initWithRotation(axis, Vector(0,1,0));

  materials.push_back(mat);

  //generate vertices

  // The first half is the top, second half bottom vertices of cylinder
  vertices.resize( (tessellation+1)*2 );

  const float angleIncr = M_PI*2 / tessellation; 
  for (int i=0; i < tessellation; ++i) {
    Real x = radius * cos(angleIncr*i);
    Real y = radius * sin(angleIncr*i);
    
    vertices[i]    = t.multiply_point(Vector(x, y, height)) + center;
    vertices[i+tessellation+1] = t.multiply_point(Vector(x, y, 0)) + center;
  }
  vertices[tessellation] = vertices[0];
  vertices[tessellation+1 + tessellation] = vertices[tessellation+1];

  //make disks
  for (int i=0; i < tessellation-2; ++i) {

    vertex_indices.push_back(tessellation-1);
    vertex_indices.push_back(i);
    vertex_indices.push_back(i+1);
    addTriangle(new KenslerShirleyTriangle(this, size()));
    face_material.push_back(0);

    vertex_indices.push_back(tessellation+1 + tessellation-1);
    vertex_indices.push_back(tessellation+1 + i);
    vertex_indices.push_back(tessellation+1 + i+1);
    addTriangle(new KenslerShirleyTriangle(this, size()));
    face_material.push_back(0);
  }
  
  //make actual cylinder part
  for (int i=0; i < tessellation; ++i) {
    vertex_indices.push_back(i);
    vertex_indices.push_back(i+1);
    vertex_indices.push_back(tessellation+1 + i);
    addTriangle(new KenslerShirleyTriangle(this, size()));
    face_material.push_back(0);

    vertex_indices.push_back(i+1);
    vertex_indices.push_back(tessellation+1 + i+1);
    vertex_indices.push_back(tessellation+1 + i);
    addTriangle(new KenslerShirleyTriangle(this, size()));
    face_material.push_back(0);
  }
}

TessellatedCylinder::~TessellatedCylinder()
{
  for (unsigned int i=0; i < objs.size(); ++i)
    delete objs[i];
}
