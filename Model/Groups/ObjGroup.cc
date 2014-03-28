
#include <MantaTypes.h>
#include <Interface/Texture.h>
#include <Core/Color/Color.h>
#include <Core/Color/ColorSpace.h>
#include <Core/Exceptions/InternalError.h>
#include <Model/Groups/ObjGroup.h>
#include <Model/Textures/Constant.h>
#include <Model/Textures/ImageTexture.h>
#include <Model/Textures/NormalTexture.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Materials/Phong.h>
#include <Model/Materials/Transparent.h>
#include <Model/Materials/Dielectric.h>
#include <Model/Materials/Flat.h>
#include <Model/Primitives/WaldTriangle.h>
#include <Model/Primitives/KenslerShirleyTriangle.h>
#include <Model/Primitives/MovingKSTriangle.h>

#include <iostream>
#include <map>

using namespace Manta;
using namespace Glm;

// Check to see if the specified file can be loaded, otherwise use the
// specified color.
static Texture<Color> *check_for_texture( const string &path_name,
                                   const string &file_name,
                                   const Color &constant_color,
                                   const float* map_scaling ) {
  static std::map<string, Texture<Color>*> texture_cache;

  string tex_name = path_name + file_name;
  map<string, Texture<Color>*>::iterator iter = texture_cache.find(tex_name);
  if (iter != texture_cache.end())
    return iter->second;

    Texture<Color> *texture = 0;

    if (file_name.size()) {
        // Load the image texture.
        try {
          ImageTexture<Color>* it = LoadColorImageTexture( tex_name , &cerr);
          // The values are assigned to zero in in unintialized state
          if (map_scaling[0] != 0) {
            it->setScale(map_scaling[0], map_scaling[1]);
          }
          if (true/*bilinear_textures*/) {
            it->setInterpolationMethod(ImageTexture<Color>::Bilinear);
          }
          texture = it;
          texture_cache[tex_name] = texture;
        }
        catch (Exception &e) {
            std::cerr << "Could not load diffuse map: "
                      << file_name
                      << ": " << e.message() << std::endl;
        }
    }

    if (texture == 0) {

        // Otherwise use a constant color.
        texture = new Constant<Color>( constant_color );
    }

    return texture;
}



void ObjGroup::create_materials( GLMmodel *model ) {

    //////////////////////////////////////////////////////////////////////////
    // Path to the model for loading textures.
    string model_path = model->pathname;

    size_t pos = model_path.rfind( '/' );
    if (pos != string::npos) {
        model_path = model_path.substr( 0, pos+1 );
    }

    material_array = new Material *[ model->nummaterials ];
    material_array_size = model->nummaterials;

    // Read in the materials.
    for (unsigned int i=0;i<model->nummaterials;++i) {

        //////////////////////////////////////////////////////////////////////
        // Copy out material attributes.
        string mtl_name(model->materials[i].name);
//         std::cerr << "Name ("<<mtl_name<<") ";

        // Color ambient (RGB( model->materials[i].ambient[0],
        //                     model->materials[i].ambient[1],
        //                     model->materials[i].ambient[2] ));

        Color diffuse (RGB( model->materials[i].diffuse[0],
                            model->materials[i].diffuse[1],
                            model->materials[i].diffuse[2] ));
        Color specular(RGB( model->materials[i].specular[0],
                            model->materials[i].specular[1],
                            model->materials[i].specular[2] ));
        Color filter  (RGB( model->materials[i].filter[0],
                            model->materials[i].filter[1],
                            model->materials[i].filter[2] ));

        float Ns     = model->materials[i].shininess;
        float Ni     = model->materials[i].refraction;
        float alpha  = model->materials[i].alpha;
        float reflectivity = model->materials[i].reflectivity;
        //int   shader = model->materials[i].shader;

        // Copy out texture names.
        // string ambient_map_name  = (model->materials[i].ambient_map[0]  != '\0') ?
        //   model->materials[i].ambient_map : "";
        string diffuse_map_name((model->materials[i].diffuse_map[0]  != '\0') ?
                                model->materials[i].diffuse_map : "");
        string specular_map_name((model->materials[i].specular_map[0] != '\0') ?
                                 model->materials[i].specular_map : "");
        float* diffuse_map_scaling = model->materials[i].diffuse_map_scaling;
        float* specular_map_scaling = model->materials[i].specular_map_scaling;

        int index = i;
        Texture<Color> *diffuse_map  = check_for_texture(model_path,
                                                         diffuse_map_name,
                                                         diffuse,
                                                         diffuse_map_scaling);

        //////////////////////////////////////////////////////////////////////
        // Check for a dielectric.

        if (Ni != 1 && Ni != 0) {
          // Constant textures for refraction parameters.
          // Create a dielectric shader.
          material_array[index] = new Dielectric(new Constant<Real>(Ni),
                                                 new Constant<Real>(1),
                                                 diffuse_map);
        }
        else if (filter != Color::white()) {
          // Note(Thiago): I realize a filter of white would mean completely
          // transparent and black would be completely opaque, so this should
          // really be comparing against not equal to black, but for some
          // reason, at least for the mtl of the models I tried, white is used
          // for opaque materials.  Ugh, so confusing...

          // ugly hack for computing alpha.  Really, the Transparent class
          // should be modified (or another material made) to have option of
          // using a filter color instead of an alpha...
          float alpha = filter.Mean();
          material_array[index] = new Transparent(diffuse_map, alpha);
        }
        else if (alpha < 1) {
          material_array[index] = new Transparent(diffuse_map, alpha);
        }

        //////////////////////////////////////////////////////////////////////
        // Check for a specular term.
        else if (Ns != 0) {
          Texture<Color> *specular_map = check_for_texture(model_path,
                                                           specular_map_name,
                                                           specular,
                                                           specular_map_scaling );
          Texture<ColorComponent> *reflection =
            new Constant<ColorComponent>( reflectivity );

          // Phong shader.
          material_array[index] = new Phong(diffuse_map, specular_map,
                                            static_cast<int>(Ns), reflection );
        }

        //////////////////////////////////////////////////////////////////////
        // Use a lambertian shader.
        else {
            // Lambertian Shader.
            material_array[index] = new Lambertian( diffuse_map );
        }
    }
}



ObjGroup::ObjGroup( const char *filename,
                    Material *defaultMaterial,
                    MeshTriangle::TriangleType triangleType) throw (InputError)
{

  // Load the model.
  GLMmodel *model = glmReadOBJ( filename );
  if (model == 0) {
    throw InputError( "Error opening input file." );
  }

  // Allocate storage for primitives and materials.
  create_materials( model );

  for (unsigned int i=1; i <= model->numvertices; ++i)
    vertices.push_back(Vector(model->vertices[i*3+0],
                              model->vertices[i*3+1],
                              model->vertices[i*3+2]));

  for (unsigned int i=1; i <= model->numnormals; ++i)
    vertexNormals.push_back(Vector(model->normals[i*3+0],
                                   model->normals[i*3+1],
                                   model->normals[i*3+2]));

  for (unsigned int i=1; i <= model->numtexcoords; ++i)
    texCoords.push_back(Vector(model->texcoords[i*2+0],
                               model->texcoords[i*2+1],
                               1));

//   for (unsigned int i=1; i <= model->numfacetnorms; ++i)
//     faceNormals.push_back(Vector(model->facetnorms[i*3+0],
//                                  model->facetnorms[i*3+1],
//                                  model->facetnorms[i*3+2]));

  if (defaultMaterial == NULL)
    defaultMaterial = new Lambertian( new NormalTexture() );
  materials.push_back(defaultMaterial);
  for (unsigned int i=0; i < model->nummaterials; ++i)
    materials.push_back(material_array[i]);

  WaldTriangle *wald_triangles = NULL;
  KenslerShirleyTriangle *KS_triangles = NULL;
  MovingKSTriangle* MovingKS_triangles = NULL;
  switch (triangleType) {
  case MeshTriangle::WALD_TRI:
    wald_triangles = new WaldTriangle[model->numtriangles];
    break;
  case MeshTriangle::KENSLER_SHIRLEY_TRI:
    KS_triangles = new KenslerShirleyTriangle[model->numtriangles];
    break;
  case MeshTriangle::MOVING_KS_TRI:
    MovingKS_triangles = new MovingKSTriangle[model->numtriangles];
    break;
  default:
    throw InternalError("Invalid triangle type");
    break;
  }

  // Read in the groups.
  GLMgroup *group = model->groups;
  // If any triangle has texture coordinates, they all have to have them.
  bool has_tex_coords = model->numtexcoords > 0;
  while (group != 0) {

    // Determine the material for this group.
    unsigned int material_index = 0;
    if (group->material < model->nummaterials) {
      material_index = group->material+1;
    }

    // Copy out triangles.
    int total_faces = group->numtriangles;
    for (int i=0;i<total_faces;++i) {

      for (int v=0;v<3;++v) {
        int index = model->triangles[ group->triangles[i] ].vindices[v];
        vertex_indices.push_back(index-1);

        index = model->triangles[ group->triangles[i] ].nindices[v];
        if (index > 0) {
          normal_indices.push_back(index-1);
        }
        else if (!vertexNormals.empty()) {
          //in case we are supposed to have vertex normals, but this
          //triangle doesn't have any, we'll use the face normal for
          //each of the vertex normals.
          const Vector &n0 = vertices[model->triangles[group->triangles[i]].vindices[0]-1];
          const Vector &n1 = vertices[model->triangles[group->triangles[i]].vindices[1]-1];
          const Vector &n2 = vertices[model->triangles[group->triangles[i]].vindices[2]-1];
          const Vector normal = Cross(n1-n0, n2-n0);
          normal_indices.push_back(vertexNormals.size());
          vertexNormals.push_back(normal);
        }

        index = model->triangles[ group->triangles[i] ].tindices[v];
        if (index > 0) {
          texture_indices.push_back(index-1);
        } else if (has_tex_coords) {
          // Again, we assume that if one triangle has a texture coordinate, that
          // all triangles do.
          texture_indices.push_back(Mesh::kNoTextureIndex);
        }
      }

      // we don't support face normals right now. If someone wants it,
      // feel free to finish implementing it.
//       index = model->triangles[ group->triangles[i] ].findex;
//       if (index > 0) {
//         face_normals.push_back(index-1);
//       }
//       else {
// //       have_face_normals = false;
//       }

      face_material.push_back(material_index);

      switch (triangleType) {
      case MeshTriangle::WALD_TRI:
        addTriangle(&wald_triangles[objs.size()]);
        break;
      case MeshTriangle::KENSLER_SHIRLEY_TRI:
        addTriangle(&KS_triangles[objs.size()]);
        break;
      case MeshTriangle::MOVING_KS_TRI:
        addTriangle(&MovingKS_triangles[objs.size()]);
        break;
      }
    }

    // Move to the next group.
    group = group->next;
  }

  // std::cerr << "Total triangles added: " << tri << std::endl;
  removeDegenerateTriangles();
}

 ObjGroup::~ObjGroup() {

   // Delete the individual materials.
   for (unsigned i=0;i<material_array_size;++i) {
     if (material_array[i])
       delete material_array[i];
   }

   // Delete the array.
   delete[] material_array;

   // Warning: A memory leak will occur since we are not deleting the triangles!
   // We clear the vector so that the parent class will not try to individually
   // delete the triangles that were created with new[].
   // Maybe it would be best to allocate the triangles individually...
  objs.clear();
}

