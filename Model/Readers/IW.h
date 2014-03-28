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

#ifndef Manta_Model_IW_h
#define Manta_Model_IW_h

/* This reads in geometry from an .iw format that is used locally by a
   few of us.  Not really useful if you don't have some lying
   around. */

/*
  The iw-file looks like:

newobject 0
vertices: <numvertices>
\tx y z
\tx y z
... ("numvertices" times, that's the vertex positions)
vtxnormals: <numvertices> # This can be zero
\tnx ny nz
\tnx ny nz
...
triangles: <numtriangles>
\ta b c s
\ta b c s
... ## each triangle consists of vertex ID's a, b, and c ('0' is
    ## the first vertex from 'vertices' above);
    ## 's' is the shader ID (ignore for this test)
endobject
camera pos (%f,%f,%f) to (%f,%f,%f) up (%f,%f,%f)
shaders <num shaders>
shader <index> <type> <parameters>
...
   ## type can be:
   ## diffuse (%f,%f,%f) # where %f is [0-1].

*/

#include <Core/Geometry/Vector.h>
#include <Core/Geometry/VectorT.h>
#include <Core/Color/ColorSpace.h>
#include <Model/Groups/Mesh.h>
#include <MantaTypes.h>
#include <vector>

namespace Manta {

  class IWObject;
  class IWNode;
  class IWTree;
  
  Mesh* readIW(const std::string &filename,
               const MeshTriangle::TriangleType triangleType=
               MeshTriangle::KENSLER_SHIRLEY_TRI);

  // Return null if there was a problem
  IWObject* IWObjectRead(const char* filename);
  IWTree*   IWTreeRead(const char* filename);

  class IWObject {
  public:
    IWObject() {}
    
    std::vector<Vector> vertices;
    std::vector<Vector> vtxnormals;
    std::vector<Color> shaders;
    // First three indicies are the indicies of verticies/vtxnormals
    // the last index is the shaders index.
    std::vector<VectorT<unsigned int, 4> > triangles;

    Vector va(unsigned int index) { return vertices[triangles[index][0]]; }
    Vector vb(unsigned int index) { return vertices[triangles[index][1]]; }
    Vector vc(unsigned int index) { return vertices[triangles[index][2]]; }
    Color  color(unsigned int index) { return shaders[triangles[index][3]]; }

    Vector vna(unsigned int index) { return vtxnormals[triangles[index][0]]; }
    Vector vnb(unsigned int index) { return vtxnormals[triangles[index][1]]; }
    Vector vnc(unsigned int index) { return vtxnormals[triangles[index][2]]; }
  private:
    // Copying could be nasty, so don't allow it.
    IWObject( const Vector& copy ) {}
  };

  struct IWNode {
    IWNode()
      : left(0), right(0), parent(0),
        split(0), flags(0),
        num_tris(0), tri_ids(0),
        index(0)
    {}
    IWNode(IWNode* parent)
      : left(0), right(0), parent(parent),
        split(0), flags(0),
        num_tris(0), tri_ids(0),
        index(0)
    {}
    ~IWNode() {
      if (left) delete left;
      if (right) delete right;
      if (tri_ids) delete[] tri_ids;
    }

    void setRight(IWNode* node) {
      right = node;
    }
    void setLeft(IWNode* node) {
      left = node;
    }
    bool isNode() const {
      return (left || right);
    }
    bool isLeaf() const {
      return !(left || right);
    }
  
    IWNode *left, *right, *parent;
    float split;
    int flags;
    unsigned int num_tris;
    int* tri_ids;
    unsigned int index;
  };

  struct IWTree {
    IWTree(): head(NULL) {}
    
    ~IWTree() {
      if (head) delete head;
    }
    
    IWNode* head;
    unsigned int num_nodes;
    unsigned int num_empty_nodes; // These are leaves with no triangles
    unsigned int num_tri_indices;
  };
  
} // end namespace Manta

#endif // Manta_Model_IW_h
