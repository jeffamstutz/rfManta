
#ifndef MODEL_GROUPS_OBJGROUP__H
#define MODEL_GROUPS_OBJGROUP__H

#include <Model/Primitives/MeshTriangle.h>
#include <Model/Groups/Mesh.h>
#include <Core/Exceptions/InputError.h>
#include <Model/Readers/glm/glm.h>

namespace Manta {

  class Material;
  
  class ObjGroup : public Mesh {
  public:
    ObjGroup( const char *filename,
              Material *defaultMaterial=NULL,
              MeshTriangle::TriangleType triangleType = MeshTriangle::KENSLER_SHIRLEY_TRI)
      throw (InputError);
    virtual ~ObjGroup();

  protected:
    virtual void create_materials( Glm::GLMmodel *model );

    Material **material_array;
    unsigned material_array_size;
  };
};

#endif
