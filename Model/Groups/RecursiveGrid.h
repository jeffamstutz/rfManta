#ifndef RecursiveGrid_h
#define RecursiveGrid_h

#include <Model/Groups/Group.h>
#include <Model/Groups/Mesh.h>
#include <Core/Containers/GridArray3.h>
#include <Core/Geometry/Vector.h>
#include <Core/Geometry/BBox.h>
#include <Interface/RayPacket.h>
#include <Interface/AccelerationStructure.h>

#include <vector>

namespace Manta {
  class RecursiveGrid : public AccelerationStructure {
   public:
    RecursiveGrid(int numLevels = 3, Group *group=NULL);
    virtual ~RecursiveGrid();

    void setGroup(Group* new_group);
    void groupDirty() {} // does nothing for now
    Group* getGroup() const;

    virtual void addToUpdateGraph(ObjectUpdateGraph* graph,
                                  ObjectUpdateGraphNode* parent) { }

    void rebuild(int proc=0, int numProcs=1);

    virtual void preprocess( PreprocessContext const & );
    virtual void intersect( RenderContext const &context, RayPacket &rays ) const;
    void computeBounds(const PreprocessContext&,
                       BBox& bbox) const
    {
      if (currGroup) {
        BBox bounds(min, max); //note: min/max are slightly larger than actual bounds!
        bbox.extendByBox(bounds);
      }
    }

    //These two functions are supposed to be protected...
    virtual void build(const PreprocessContext &context, const vector<Object*> &objs,
                       const BBox &bounds, const int depth, int totalObjects);

   protected:
    void clearGrid();
    RecursiveGrid(const RecursiveGrid&);
    RecursiveGrid& operator=(const RecursiveGrid&);

    //templated on positive direction in x, y, and z.
    template<const bool posX, const bool posY, const bool posZ>
    void intersectRay( const RenderContext &context, RayPacket &rays ) const;



    //for performance measurements. Can be removed.
    //#define COLLECT_STATS
    static long nIntersects;
    static long nCellTraversals;
    static long nGridTraversals;
    static long nTotalRays;
    static long nCells;
    static long nGrids[10];
    static long nFilledCells;
    static long nTriRefs;
    void update(int proc, int numProcs);

    Vector min, max;
    Vector cellsize;
    Vector inv_cellsize;
    GridArray3<int> cells;
    const Object** lists;
    GridArray3<bool> subGridList;

    Group *currGroup;

    int numLevels;

    void transformToLattice(const Vector& v, int& x, int& y, int& z) const;
    void transformToLattice(const BBox& box,
                            int& sx, int& sy, int& sz,
                            int& ex, int& ey, int& ez) const;

  };
}

#endif
