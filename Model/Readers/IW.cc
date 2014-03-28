/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005-2006
  Scientific Computing and Imaging Institute, University of Utah

  License for the specific language governing rights and limitations under
  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#include <MantaTypes.h>
#include <Core/Color/ColorSpace.h>
#include <Core/Color/ColorSpace_fancy.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Primitives/WaldTriangle.h>
#include <Model/Primitives/KenslerShirleyTriangle.h>
#include <Model/Primitives/MovingKSTriangle.h>
#include <Model/Readers/IW.h>

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>

using namespace Manta;
using namespace std;

Mesh* Manta::readIW(const string &filename, 
                    const MeshTriangle::TriangleType triangleType)
{
  IWObject *iwobject = IWObjectRead(filename.c_str());
  if (iwobject == NULL) return NULL; //should we throw an error?

  Mesh *mesh = new Mesh;
  
  mesh->vertices = iwobject->vertices;
  mesh->vertexNormals = iwobject->vtxnormals;

  //It's possible the .iw file has a lot of redundant shaders, so this
  //material_cache will remove the redundancies.
  map<string, int> material_cache;
  for (size_t i=0; i < iwobject->shaders.size(); ++i) {
    const string colorName = iwobject->shaders[i].toString();
    map<string, int>::iterator iter = material_cache.find(colorName);
    if (iter == material_cache.end()) {
      //haven't seen this before, so let's add it to the cache.
      mesh->materials.push_back(new Lambertian(iwobject->shaders[i]));
      material_cache[colorName] = mesh->materials.size()-1;
    }
  }
  
  WaldTriangle *wald_triangles = NULL;
  KenslerShirleyTriangle *KS_triangles = NULL;
  MovingKSTriangle* MovingKS_triangles = NULL;
  switch (triangleType) {
  case MeshTriangle::WALD_TRI:
    wald_triangles = new WaldTriangle[iwobject->triangles.size()];
    break;
  case MeshTriangle::KENSLER_SHIRLEY_TRI:
    KS_triangles = new KenslerShirleyTriangle[iwobject->triangles.size()];
    break;
  case MeshTriangle::MOVING_KS_TRI:
    MovingKS_triangles = new MovingKSTriangle[iwobject->triangles.size()];
    break;
  default:
    throw InternalError("Invalid triangle type");
    break;
  }

  for (size_t i=0; i < iwobject->triangles.size(); ++i) {
    mesh->vertex_indices.push_back(iwobject->triangles[i][0]);
    mesh->vertex_indices.push_back(iwobject->triangles[i][1]);
    mesh->vertex_indices.push_back(iwobject->triangles[i][2]);
    mesh->normal_indices.push_back(iwobject->triangles[i][0]);
    mesh->normal_indices.push_back(iwobject->triangles[i][1]);
    mesh->normal_indices.push_back(iwobject->triangles[i][2]);

    const int shaderID = iwobject->triangles[i][3];
    const string colorName = iwobject->shaders[shaderID].toString();
    const int id = material_cache[colorName];
    mesh->face_material.push_back(id);

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
  
  fprintf(stderr, "compact shader :%10llu\n",
          (unsigned long long)mesh->materials.size());
  

  return mesh;
}

// Return null if there was a problem
IWObject* Manta::IWObjectRead(const char* filename) {

  // Attempt to open the file
  ifstream geomf(filename);
  if (!geomf) {
        fprintf(stderr, "Error: cannot open %s for loading\n", filename);
    return 0;
  }

  IWObject* iw = new IWObject();

  string line;
  getline(geomf, line);
  while(geomf.good()) {
    istringstream line_buf(line);
    string token;
    line_buf >> token;
    if (token == "newobject") {
      // Don't do anything yet.  We don't know how to handle multiple
      // objects.
    } else if (token == "vertices:") {
      /////////////////////////////////////////////////////////////////
      // Load the vertices
      unsigned int num_vertices;
      line_buf >> num_vertices;

      // Allocate space
      iw->vertices.reserve(num_vertices);

      // Read them in
      for(unsigned int vi = 0; vi < num_vertices; ++vi) {
        float x,y,z;
        getline(geomf, line);
        sscanf(line.c_str(), "\t%f %f %f\n", &x, &y, &z);
        iw->vertices.push_back(Vector(x,y,z));
      }
      
      // Check to make sure we read the right number
      if (iw->vertices.size() != num_vertices) {
        cerr << "Read "<<iw->vertices.size()<<" vertices and "
             << "expected "<<num_vertices<<" vertices\n";
        delete iw;
        return 0;
      }
    } else if (token == "vtxnormals:") {
      /////////////////////////////////////////////////////////////////
      // Load the vertex normals
      unsigned int num_vtxnormals;
      line_buf >> num_vtxnormals;

      // Allocate space
      iw->vtxnormals.reserve(num_vtxnormals);

      // Read them in
      for(unsigned int vi = 0; vi < num_vtxnormals; ++vi) {
        float x,y,z;
        getline(geomf, line);
        sscanf(line.c_str(), "\t%f %f %f\n", &x, &y, &z);
        iw->vtxnormals.push_back(Vector(x,y,z));
      }

      // Check to make sure we read the right number
      if (iw->vtxnormals.size() != num_vtxnormals) {
        cerr << "Read "<<iw->vtxnormals.size()<<" normals and "
             << "expected "<<num_vtxnormals<<" normals\n";
        delete iw;
        return 0;
      }
    } else if (token == "triangles:") {
      /////////////////////////////////////////////////////////////////
      // Load the triangles
      unsigned int num_tris;
      line_buf >> num_tris;

      // Allocate space
      iw->triangles.reserve(num_tris);

      // Read them in
      for(unsigned int ti = 0; ti < num_tris; ++ti) {
        unsigned int data[4];
        getline(geomf, line);
        sscanf(line.c_str(), "\t%u %u %u %u\n", data, data+1, data+2, data+3);
        iw->triangles.push_back(VectorT<unsigned int, 4>(data));
      }

      // Check to make sure we read the right number
      if (iw->triangles.size() != num_tris) {
        cerr << "Read "<<iw->triangles.size()<<" triangles and "
             << "expected "<<num_tris<<" triangles\n";
        delete iw;
        return 0;
      }
    } else if (token == "endobject") {
      // Do nothing right now
    } else if (token == "camera") {
      // Do nothing right now
    } else if (token == "shaders") {
      /////////////////////////////////////////////////////////////////
      // Load the triangles
      unsigned int num_shaders;
      line_buf >> num_shaders;

      // Allocate space, need to resize, becase we can't simply just
      // push_back as the shader has the index as part of the file spec.
      iw->shaders.resize(num_shaders, Color(RGBColor(1,0,1)));

      // Read them in
      for(unsigned int i = 0; i < num_shaders; ++i) {
        float r, g, b;
        unsigned int index;
        getline(geomf, line);
        sscanf(line.c_str(), "shader %u diffuse (%f,%f,%f)",
               &index, &r, &g, &b);
        iw->shaders[index] = Color(RGBColor(r,g,b));
      }
    } else {
      if (token.length())
        cerr << "Unknown token: ("<<token<<")\n";
    }
    getline(geomf, line);
  }

  geomf.close();

  // Print some stats
  fprintf(stderr, "vertices       :%10llu\n",
          (unsigned long long)iw->vertices.size());
  fprintf(stderr, "vertex normals :%10llu\n",
          (unsigned long long)iw->vtxnormals.size());
  fprintf(stderr, "triangles      :%10llu\n",
          (unsigned long long)iw->triangles.size());
  fprintf(stderr, "shaders        :%10llu\n",
          (unsigned long long)iw->shaders.size());
  
  //////////////////////////////////////////
  // Let's do some error checking:

  // If there are vertex normals then the number must be greater than
  // or equal to the number of vertices.
  if (iw->vtxnormals.size() > 0 &&
      iw->vtxnormals.size() < iw->vertices.size() ) {
    fprintf(stderr, "Number of vertex normals (%llu) less than number or vertices (%llu)\n",
            (unsigned long long)iw->vtxnormals.size(),
            (unsigned long long)iw->vertices.size());
    delete iw;
    return 0;
  }
  
  // Make sure that the triangle indices and shader indicies are cool
  for(size_t i = 0; i < iw->triangles.size(); ++i) {
    unsigned int a, b, c, group;
    a = iw->triangles[i][0];
    b = iw->triangles[i][1];
    c = iw->triangles[i][2];
    group = iw->triangles[i][3];

    if (a >= iw->vertices.size()) {
      fprintf(stderr, "Vertex index (%u) out of range (%llu)\n",
              a, (unsigned long long)iw->vertices.size());
      delete iw;
      return 0;
    }

    if (b >= iw->vertices.size()) {
      fprintf(stderr, "Vertex index (%u) out of range (%llu)\n",
              b, (unsigned long long)iw->vertices.size());
      delete iw;
      return 0;
    }

    if (c >= iw->vertices.size()) {
      fprintf(stderr, "Vertex index (%u) out of range (%llu)\n",
              c, (unsigned long long)iw->vertices.size());
      delete iw;
      return 0;
    }

    if (group >= iw->shaders.size()) {
      fprintf(stderr, "Vertex index (%u) out of range (%llu)\n",
              group, (unsigned long long)iw->shaders.size());
      delete iw;
      return 0;
    }
    
  }

  return iw;
}

IWTree* Manta::IWTreeRead(const char* filename)
{
  unsigned int num_tri_indices = 0;
  unsigned int num_nodes = 0;
  unsigned int num_empty_nodes = 0;

  ifstream bspf(filename);
  if (!bspf) {
        fprintf(stderr, "Error: cannot open %s for loading\n", filename);
    return 0;
  }

  IWNode* head = 0;
  IWNode* top = head;

  string line;
  getline(bspf, line);
  while(bspf.good()) {
//     fprintf (stderr, "line(%u): %s\n", bline_len, bline);
    // Check the first token
    char token[2];
    //    int num = sscanf(line.c_str(), "%1s", token);
    //    fprintf(stderr, "num = %d, token = %s\n", num, token);
    if (strcmp(token, "N") == 0) {
      num_nodes ++;
//       fprintf(stderr, "Node %u found\n", num_nodes);
      
      IWNode* node = new IWNode(top);
      // Is this the first node?
      if (top) {
        if (top->left) {
          top->setRight(node);
        } else {
          top->setLeft(node);
        }
      } else {
        // Set the head to the first node
        head = node;
      }
      // Decend
      top = node;

      char axis_c;
      sscanf(line.c_str(), "%*s %c=%f,d=%*d", &axis_c, &(node->split));
//       fprintf(stderr, "axis = %c, split = %g\n", axis_c, node->split);
      switch (axis_c) {
      case 'x': node->flags |= 0; break;
      case 'y': node->flags |= 1; break;
      case 'z': node->flags |= 2; break;
      }

    } else if (strcmp(token, "L") == 0) {
      IWNode* node = new IWNode(top);

      if (top->left) {
        top->setRight(node);
        // Finish up this level
        while (top && top->right) {
          top = top->parent;
        }
      } else {
        top->setLeft(node);
      }

      // Grab how many objects we have
      sscanf(line.c_str(), "%*s %u", &(node->num_tris));
//       fprintf(stderr, "L %u\n", node->num_tris);
      num_tri_indices += node->num_tris;
      if (node->num_tris) {
        num_nodes++;

        // If you ever change what type triIndices is you will need to
        // update this bit of code for the right type.
        int* tri_ids_p = node->tri_ids = new int[node->num_tris];
        int pos = 0; // Where to pick up parsing next.  This will give
                     // us an offset to our token string, so that we
                     // can grab the next integer one at a time.
        sscanf(line.c_str(), "%*s %*u %n", &pos);
        //        fprintf(stderr, "pos = %d\n", pos);
        char const* bline_p = line.c_str()+pos;
        for(unsigned int i = 0; i < node->num_tris; ++i) {
          sscanf(bline_p, "%d %n", tri_ids_p, &pos);
          //          fprintf(stderr, "pos = %d, %d\n", pos, *tri_ids_p);
//           fprintf(stderr, "tri (%d) = %d\n", i, *tri_ids_p);
          bline_p += pos;
          ++tri_ids_p;
        }
      } else {
        // else empty leaf
        num_empty_nodes++;
      }
    } else {
      fprintf(stderr, "Unrecongnized token (%c)\n", token[0]);
    }
    getline(bspf, line);
  }

  bspf.close();
  
  IWTree* tree = new IWTree();
  tree->head = head;
  tree->num_nodes = num_nodes;
  tree->num_empty_nodes = num_empty_nodes;
  tree->num_tri_indices = num_tri_indices;

  return tree;
}

