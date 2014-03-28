
#ifndef Manta_Model_Group_h
#define Manta_Model_Group_h

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


#include <Core/Geometry/BBox.h>
#include <Core/Persistent/MantaRTTI.h>
#include <Core/Thread/Barrier.h>
#include <Core/Thread/Mutex.h>
#include <Core/Util/Assert.h>
#include <Interface/Object.h>
#include <Interface/InterfaceRTTI.h>

#include <vector>
#include <string>

namespace Manta {
  using namespace std;
  class Group : public Object {
  public:
    Group(const vector<string>& args);
    Group();
    virtual ~Group();

#ifndef SWIG
    virtual Group* clone(CloneDepth depth, Clonable* incoming=NULL);

    virtual InterpErr serialInterpolate(const std::vector<keyframe_t>& keyframes);
    virtual InterpErr parallelInterpolate(const std::vector<keyframe_t>& keyframes,
                                          int proc, int numProc);
#endif
    virtual bool isParallel() const { return true; }

    virtual void add(Object* obj);
    
    // Remove the specified pointer from the Group. The pointer will
    // be deleted if delete_ptr == true.
    virtual void remove(Object* obj, bool delete_ptr );    
    virtual void set( size_t i, Object* obj );

    inline Object* get( size_t i ) {
      ASSERT( i < objs.size() );
      return objs[i];
    }

    inline const Object* get(size_t i) const
    {
      ASSERT( i < objs.size() );
      return objs[i];
    }

    const vector<Object*> &getVectorOfObjects() const { return objs; };

    //whether the group has been modified (is dirty) and needs state,
    //such as the bounding box, to be updated.
    bool isDirty() const;
    void setDirty();

    void shrinkTo(size_t firstNumObjs, bool deleteRemainder);
    size_t size() const { return objs.size(); }

    virtual void preprocess(const PreprocessContext&);
    virtual void intersect(const RenderContext& context, RayPacket& rays) const;

    //if the objects contained by the Group are modified, then
    //setDirty() should be called before calling computeBounds so a
    //new bound is calculated.
    virtual void computeBounds(const PreprocessContext& context,
                               BBox& bbox) const;
    virtual void computeBounds(const PreprocessContext& context,
                               int proc, int numProcs) const;

    static Group* create(const vector<string>& args);

    virtual void addToUpdateGraph(ObjectUpdateGraph* graph,
                                  ObjectUpdateGraphNode* parent);

    void readwrite(ArchiveElement* archive);
  protected:
    vector<Object*> objs;
    vector<Object*>::iterator  parallelSplit; //point to the start of the parallel objects
    mutable BBox bbox;
    mutable bool dirtybbox;
    mutable Barrier barrier;
    mutable Mutex mutex;

  };

  MANTA_DECLARE_RTTI_DERIVEDCLASS(Group, Object, ConcreteClass, readwriteMethod);
}

#endif
