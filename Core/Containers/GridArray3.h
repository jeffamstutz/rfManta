#ifndef __MANTA_GRID_ARRAY3__
#define __MANTA_GRID_ARRAY3__

/* This creates a 3D grid of type T in a way that is cache
 * friendly. Traversing a ray through this grid should result in less
 * cache misses than traversing in a standard grid since adjacent
 * cells are usually adjacent in memory (in a standard grid, cells are
 * only adjacent in memory for one dimension of travel).
 */

namespace Manta {

  template<typename T>
  class GridArray3 {
   public:
    GridArray3() {
      nx = ny = nz = extra = 0;
      xidx = yidx = zidx = 0;
      data = 0;
    }
    GridArray3(int nx, int ny, int nz, int extra = 0) {
      xidx = yidx = zidx = 0;
      data = 0;
      resize(nx, ny, nz, extra);
    }
    ~GridArray3() {
      resize(0, 0, 0);
    }

    void resize(int newnx, int newny, int newnz, int newextra = 0) {
      if(xidx)
        delete[] xidx;
      if(yidx)
        delete[] yidx;
      if(zidx)
        delete[] zidx;
      if(data)
        delete[] data;
      nx = newnx; ny = newny; nz = newnz; extra = newextra;
      int total = nx*ny*nz+extra;
      if(total != 0){
        data = new T[total];
        allocateAndInitializeStride(xidx, nx, 1);
        allocateAndInitializeStride(yidx, ny, nx);
        allocateAndInitializeStride(zidx, nz, nx*ny);
      } else {
        xidx = yidx = zidx = 0;
        data = 0;
      }
    }

    void resizeZMajor(int newnx, int newny, int newnz, int newextra = 0) {
      if(xidx)
        delete[] xidx;
      if(yidx)
        delete[] yidx;
      if(zidx)
        delete[] zidx;
      if(data)
        delete[] data;
      nx = newnx; ny = newny; nz = newnz; extra = newextra;
      int total = nx*ny*nz+extra;
      if(total != 0){
        data = new T[total];
        allocateAndInitializeStride(xidx, nx, nz*ny);
        allocateAndInitializeStride(yidx, ny, nz);
        allocateAndInitializeStride(zidx, nz, 1);
      } else {
        xidx = yidx = zidx = 0;
        data = 0;
      }
    }

    void initialize(const T& value) {
      int total = nx*ny*nz+extra;
      for(int i=0;i<total;i++)
        data[i] = value;
    }

    T& operator()(int x, int y, int z) {
      return data[xidx[x] + yidx[y] + zidx[z]];
    }
    const T& operator()(int x, int y, int z) const {
      return data[xidx[x] + yidx[y] + zidx[z]];
    }
    int getIndex(int x, int y, int z) const {
      return xidx[x] + yidx[y] + zidx[z];
    }
    T& operator[](int idx) {
      return data[idx];
    }
    const T& operator[](int idx) const {
      return data[idx];
    }

    int getNx() const {
      return nx;
    }
    int getNy() const {
      return ny;
    }
    int getNz() const {
      return nz;
    }


   private:
    GridArray3(const GridArray3&);
    GridArray3& operator=(const GridArray3&);

    int nx, ny, nz, extra;
    int* xidx;
    int* yidx;
    int* zidx;
    T* data;

    void allocateAndInitializeStride(int*& idx, int num, int stride) {
      idx = new int[num];
      for(int i=0;i<num;i++)
        idx[i] = stride*i;
    }
  };
};

#endif
