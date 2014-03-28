#ifndef CellSkipper_h
#define CellSkipper_h

#include <Model/Groups/Group.h>
#include <Core/Containers/GridArray3.h>
#include <Core/Geometry/Vector.h>
#include <Core/Geometry/BBox.h>
#include <Interface/RayPacket.h>
#include <Interface/AccelerationStructure.h>

namespace Manta {
  class CellSkipper : public AccelerationStructure {
  public:
    CellSkipper(Group *group=NULL);
    virtual ~CellSkipper();

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

   protected:
    void clearGrid();
    CellSkipper(const CellSkipper&);
    CellSkipper& operator=(const CellSkipper&);

    template<const bool posX, const bool posY, const bool posZ>
    void intersectRay( const RenderContext &context, RayPacket &rays ) const;

    void build(const PreprocessContext &context, const vector<Object*> &objs,
               const BBox &bounds);

    Vector min, max;
    Vector cellsize;
    Vector inv_cellsize;

    struct CellData {
      unsigned char distances[6];
      int listIndex;
      CellData() : listIndex(0)
      {
        for (int i=0; i < 6; ++i) distances[i] = 0;
      }
    };
    static const unsigned char INF; //max skip value


    GridArray3<CellData> cells;
    const Object** lists;

    bool cellHasObjects(int x, int y, int z) const {
      const int idx = cells.getIndex(x, y, z);
      const int start = cells[idx  ].listIndex;
      const int end   = cells[idx+1].listIndex;
      return end > start;
    }

    bool hasDirectNeighbor(int i, int j, int k, int direction) const;
    bool hasDiagonalNeighbor(int i, int j, int k, int direction) const;
    unsigned char getDirectCount(int i, int j, int k, int direction) const;
    void getDiagonalsCount(unsigned char diagonals[8], int i, int j, int k,
                           int direction) const;
    void setInitialDistances(int i, int j, int k, int direction);
    void setDistances(int i, int j, int k, int direction);
    void initializeCellSkipping();

    Group *currGroup;

    void transformToLattice(const Vector& v, int& x, int& y, int& z) const;
    void transformToLattice(const BBox& box,
                            int& sx, int& sy, int& sz,
                            int& ex, int& ey, int& ez) const;

  };
}

#endif
