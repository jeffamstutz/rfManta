
#include <Model/Readers/PlyReader.h>

#include <Model/Primitives/WaldTriangle.h>
#include <Model/Primitives/KenslerShirleyTriangle.h>
#include <Model/Materials/Lambertian.h>
#include <Core/Geometry/Vector.h>
#include <Core/Color/Color.h>
#include <Core/Color/RGBColor.h>
#include <Core/Exceptions/FileNotFound.h>
#include <Model/Readers/rply/rply.h>
#include <sstream>

#include <stdlib.h>

//TODO: normals?

using namespace Manta;

double maxColorValue;

bool coloredTriangleMode = false;

struct VertexData {
  Vector point;
  RGBColor color;
};

long vertices_start = 0;
VertexData vertexData;
static AffineTransform t;

static void addVertex(Mesh *mesh) {
  vertexData.point = t.multiply_point(vertexData.point);
  mesh->vertices.push_back(vertexData.point);
}

static int vertex_cb(p_ply_argument argument) {
     Mesh *mesh;
     ply_get_argument_user_data(argument, (void**) &mesh, NULL);

     const char* name;
     p_ply_property property;
     ply_get_argument_property(argument, &property, NULL, NULL);
     ply_get_property_info(property, &name, NULL, NULL, NULL);

     long vertexIndex;
     ply_get_argument_element(argument, NULL, &vertexIndex);

     if (static_cast<size_t>(vertexIndex + vertices_start) == mesh->vertices.size()+1) {
       addVertex(mesh);
     }

     if (strcmp(name, "x") == 0)
       vertexData.point[0] = ply_get_argument_value(argument);
     else if (strcmp(name, "y") == 0)
       vertexData.point[1] = ply_get_argument_value(argument);
     else if (strcmp(name, "z") == 0)
       vertexData.point[2] = ply_get_argument_value(argument);
     else if (strcmp(name, "diffuse_red") == 0)
       vertexData.color.setR(ply_get_argument_value(argument) / maxColorValue);
     else if (strcmp(name, "diffuse_green") == 0)
       vertexData.color.setG(ply_get_argument_value(argument) / maxColorValue);
     else if (strcmp(name, "diffuse_blue") == 0)
       vertexData.color.setB(ply_get_argument_value(argument) / maxColorValue);

     return 1;
}

struct face_cb_data {
  Mesh *mesh;
  WaldTriangle *WaldTris;
  KenslerShirleyTriangle *KSTris;
  size_t nextFreeTri;
  face_cb_data() : mesh(NULL), WaldTris(NULL), KSTris(NULL), nextFreeTri(0) {}
};

long firstVertexIndex, prevVertexIndex;
static int face_cb(p_ply_argument argument) {
     long length;
     long currVertexIndex;

     face_cb_data *data;
     ply_get_argument_user_data(argument, (void**) &data, NULL);

     long polyIndex, polyVertex;
     ply_get_argument_element(argument, NULL, &polyIndex);

     ply_get_argument_property(argument, NULL, &length, &polyVertex);

     //do this once only to add last vertex.
     if (polyIndex == 0 && polyVertex == -1) {
       addVertex(data->mesh);
     }

     currVertexIndex = static_cast<long>(ply_get_argument_value(argument));
//      cout <<currVertexIndex<<"\t"<<polyIndex <<"\t" << polyVertex<<"\n";

     switch (polyVertex)
     {
     case -1: return 1;
     case 0: firstVertexIndex = currVertexIndex;
       break;
     case 1: prevVertexIndex = currVertexIndex;
       break;
     default:
       data->mesh->vertex_indices.push_back(vertices_start+firstVertexIndex);
       data->mesh->vertex_indices.push_back(vertices_start+prevVertexIndex);
       data->mesh->vertex_indices.push_back(vertices_start+currVertexIndex);

       data->mesh->face_material.push_back(data->mesh->materials.size()-1);

       if (data->WaldTris)
         data->mesh->addTriangle(&data->WaldTris[data->nextFreeTri++]);
       else if (data->KSTris)
         data->mesh->addTriangle(&data->KSTris[data->nextFreeTri++]);


       if (coloredTriangleMode) {
         //TODO: handle colored triangles.
//         group->add(new VertexColoredTriangle(mat,
//                     vertices[firstVertexIndex].point,
//                     vertices[prevVertexIndex].point,
//                     vertices[currVertexIndex].point,
//                     Color(vertices[firstVertexIndex].color),
//                     Color(vertices[prevVertexIndex].color),
//                     Color(vertices[currVertexIndex].color)));
       }
       prevVertexIndex = currVertexIndex;
       break;
     }

     return 1;

}

bool
Manta::readPlyFile(const string fileName, const AffineTransform &_t,
                   Mesh *mesh, Material *m,
                   MeshTriangle::TriangleType triangleType) {
     long nVertices, nTriangles;
     size_t objs_start = mesh->size();
     unsigned int vertex_indices_start = mesh->vertex_indices.size();
     vertices_start = mesh->vertices.size();

     t = _t;

     p_ply ply = ply_open(fileName.c_str(), NULL);
     if (!ply)
     {
         ostringstream msg;
         msg << "An error occured while trying to load the following ply file: " << fileName;
         throw FileNotFound(msg.str());
     }


     if (!ply_read_header(ply)) return false;

     nVertices = ply_set_read_cb(ply, "vertex", "x", vertex_cb, mesh, 0);

     ply_set_read_cb(ply, "vertex", "y", vertex_cb,
                     mesh, 0);
     ply_set_read_cb(ply, "vertex", "z", vertex_cb,
                     mesh, 0);

     //get vertex color property
     p_ply_property property;

     //try and get diffuse RGB color property
     property = ply_find_property(ply_find_element(ply, "vertex"), "diffuse_red");

     ply_set_read_cb(ply, "vertex", "diffuse_red", vertex_cb,
                     mesh, 0);
     ply_set_read_cb(ply, "vertex", "diffuse_blue", vertex_cb,
                     mesh, 0);
     ply_set_read_cb(ply, "vertex", "diffuse_green", vertex_cb,
                     mesh, 0);


     /*if we add other color types, need to try getting their property
      *if current property is still NULL.
      */

     Material *defaultMaterial = NULL;

     if (property)
     {
       //TODO: get vertex colored triangles working again
       /*
         //TODO: we might not want to overwrite the matl passed in to us...
         defaultMaterial = new Lambertian(new TriVerTexture());
         coloredTriangleMode = true;
         e_ply_type type;
         ply_get_property_info(property,  NULL, &type, NULL, NULL);
         if (type)
         {
             switch(type)
             {
             //TODO: handle all the other types.
             case PLY_CHAR: maxColorValue=255;
                 break;
             case PLY_UCHAR: maxColorValue=511;
                 break;
             case PLY_DOUBLE:
             case PLY_FLOAT: maxColorValue=1;
                 break;
             default: maxColorValue=1; //shouldn't get here. Need to
                                       //add case for this new type
                 break;
             }
         }
       */
     }

     face_cb_data face_data;
     face_data.mesh = mesh;

     nTriangles = ply_set_read_cb(ply, "face", "vertex_indices", face_cb, &face_data, 0);

     // Note(thiago): By allocating all the triangles in an array instead of
     // one at a time, we can lower the memory overhead and allow for triangle
     // data to be contiguous in memory instead of having gaps. Lower memory
     // usage is obviously good; getting rid of the gaps between triangles is
     // also good since it can lead to more efficient cache usage and better
     // performance.  The gaps occur because new will often try to allocate
     // memory in chunks.  For instance, calling new twice to allocate a 4B
     // struct might place the second object 16B to 32B away from the first
     // instead of 4B.  The in between space is wasted.  I've verified this
     // exact behavior in test code.
     switch (triangleType) {
     case MeshTriangle::WALD_TRI:
       face_data.WaldTris = new WaldTriangle[nTriangles];
       break;
     case MeshTriangle::KENSLER_SHIRLEY_TRI:
       face_data.KSTris = new KenslerShirleyTriangle[nTriangles];
       break;
     default:
       face_data.KSTris = new KenslerShirleyTriangle[nTriangles];
     }

     if (defaultMaterial)
     { } //do nothing
     else if (m)
      defaultMaterial = m;
     else
      defaultMaterial = new Lambertian(Color(RGBColor(1,1,1)));

     mesh->materials.push_back(defaultMaterial);

     if (!ply_read(ply)) {
         //need to clean up group.
         mesh->shrinkTo(objs_start, false);
         mesh->vertices.resize(vertices_start);
         mesh->vertex_indices.resize(vertex_indices_start);
         if (!m && defaultMaterial)
             delete defaultMaterial;

         delete[] face_data.WaldTris;
         delete[] face_data.KSTris;

         return false;
     }

     ply_close(ply);

     mesh->removeDegenerateTriangles();

     return true;
}
