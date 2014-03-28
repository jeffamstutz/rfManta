/*
 *  Volume.h
 *  Author: Carson brownlee (brownleeATcs.utah.edu)
 *  Description:  basic unoptomized volume renderer
 *
 *
 */

#ifndef VOLUME_H
#define VOLUME_H

#include <Core/Containers/GridArray3.h>
#include <Core/Geometry/BBox.h>
#include <Core/Geometry/Vector.h>

#include <Interface/Context.h>
#include <Interface/Material.h>
#include <Interface/Renderer.h>
#include <Interface/RayPacket.h>
#include <Interface/SampleGenerator.h>
#include <Interface/Scene.h>
#include <Model/Primitives/PrimitiveCommon.h>
#include <Model/Intersections/AxisAlignedBox.h>

#include <cassert>
#include <float.h>
#include <vector>
#include <iostream>
#include <cstdlib>
using namespace std;

namespace Manta
{

  struct RGBAColor
    {
      RGBAColor(Color c, float opacity) : color(c), a(opacity) {}
      RGBAColor(float r, float g, float b, float alpha) : color(Color(RGB(r,g,b))), a(alpha){ }
      Color color;
      float a; //opacity
    };

  struct ColorSlice
  {
    ColorSlice() : value(0), color(RGBAColor(Color(RGBColor(0,0,0)), 1.0)) {}
    ColorSlice(float v, RGBAColor c) : value(v), color(c) {}
    float value;  //0-1 position of slice
    RGBAColor color;
  };

  class RGBAColorMap
    {
    public:
      RGBAColorMap(unsigned int type=RGBAColorMap::InvRainbowIso, int numSlices = 256);
      RGBAColorMap(vector<Color>& colors, float* positions, float* opacities = NULL, int numSlices = 256);
      RGBAColorMap(vector<ColorSlice>& colors, int numSlices = 256);
      virtual ~RGBAColorMap();

      void fillColor(unsigned int type);
      virtual RGBAColor GetColor(float v);  // get the interpolated color at value v (0-1)
      RGBAColor GetColorAbs(float v);  //don't use now
      void SetColors(vector<ColorSlice>& colors);
      vector<ColorSlice> GetColors() { return _slices; }
      ColorSlice GetSlice(int i) { return _slices.at(i); }
      int GetNumSlices() { return _slices.size(); }
      void SetMinMax(float min, float max);
      void computeHash();
      void scaleAlphas(float t);
      void squish(float globalMin, float globalMax, float squishMin,
                  float squishMax);

      float tInc; // t increment value for volume renderers
      float oldTInc, squish_min, squish_scale;
      bool _colorScaled;
      unsigned long long course_hash;
      vector<ColorSlice> _slices;
      int _numSlices;

      enum {
        InvRainbowIso=0,
        InvRainbow,
        Rainbow,
        InvGreyScale,
        InvBlackBody,
        BlackBody,
        GreyScale,
        Unknown
      };

    protected:
      float _min, _max, _invRange;
    };

  struct VMCell
  {
    // Need to make sure we have a 64 bit thing
    unsigned long long course_hash;
    // The max of unsigned long long is ULLONG_MAX
    VMCell(): course_hash(0) {}

    void turn_on_bits(float min, float max, float data_min, float data_max) {
      // We know that we have 64 bits, so figure out where min and max map
      // into [0..63].
      int min_index = (int)((min-data_min)/(data_max-data_min)*63);
      /*      T max_index_prep = ((max-data_min)/(data_max-data_min)*63); */
      /*      int max_index = max_index_prep-(int)max_index_prep>0? */
      /*        (int)max_index_prep+1:(int)max_index_prep; */
      int max_index = (int)ceil(double((max-data_min)/(data_max-data_min)*63));
      // Do some checks

      // This will handle clamping, I hope.
      if (min_index > 63)
        min_index = 63;
      if (max_index < 0)
        max_index = 0;
      if (min_index < 0)
        min_index = 0;
      if (max_index > 63)
        max_index = 63;
      for (int i = min_index; i <= max_index; i++)
        course_hash |= 1ULL << i;
    }
    inline VMCell& operator |= (const VMCell& v) {
      course_hash |= v.course_hash;
      return *this;
    }
    inline bool operator & (const VMCell& v) {
      return (course_hash & v.course_hash) != 0;
    }
    friend ostream& operator<<(ostream& out,const VMCell& cell)  {
      for( int i = 0; i < 64; i++) {
        unsigned long long bit= cell.course_hash & (1ULL << i);
        if (bit)
          out << "1";
        else
          out << "0";
      }
      return out;
    }
  };
  struct CellData {
    VMCell cell;
  };

  template<class T>
    class Volume : public PrimitiveCommon, public Material
    {
    protected:
      class HVIsectContext {
      public:
        // These parameters could be modified and hold accumulating state
        Color total;
        float alpha;
        // These parameters should not change
        int dix_dx, diy_dy, diz_dz;
        VMCell transfunct;
        double t_inc;
        double t_min;
        double t_max;
        double t_inc_inv;
        Ray ray;
      };
    public:
      static void newFrame();
      typedef unsigned long long mcT;
      vector<mcT> cellVector_mc; // color hashes

      Volume(GridArray3<T>* data, RGBAColorMap* colorMap, const BBox& bounds,
             double cellStepSize, int depth,
             double forceDataMin = -FLT_MAX, double forceDataMax = -FLT_MAX);
      virtual ~Volume();
      void setBounds(BBox bounds);
      void setColorMap(RGBAColorMap* map) { _colorMap = map; }
      void getMinMax(double* min, double* max){ *min = _dataMin; *max = _dataMax; }
      void computeHistogram(int numBuckets, int* histValues);
      virtual void preprocess(const PreprocessContext& context);
      virtual void shade(const RenderContext & context, RayPacket& rays) const;
      virtual void attenuateShadows(const RenderContext& context, RayPacket& shadowRays) const;
      virtual void intersect(const RenderContext& context, RayPacket& rays) const
      {
        const bool anyHit = rays.getFlag(RayPacket::AnyHit);
        if (anyHit)
          return; // For now, we make the incorrect assumption that volumes do
                  // not cast shadows.  TODO: Make volumes cast correct shadows...

          // This is so our t values have the same scale as other prims, e.g., TexTriangle
          rays.normalizeDirections();

          // Intersection algorithm requires inverse directions computed.
          rays.computeInverseDirections();
          rays.computeSigns();

          BBox enlargedBBox;
          enlargedBBox[0]=_bbox[0] - _bbox.diagonal().length()*T_EPSILON*.1;
          enlargedBBox[1]=_bbox[1] + _bbox.diagonal().length()*T_EPSILON*.1;


          // Iterate over each ray.
          for (int i=rays.begin();i<rays.end();++i) {
            Real tmin, tmax;
            // Check for an intersection.
            if (intersectAaBox( enlargedBBox,
                                              tmin,
                                              tmax,
                                              rays.getRay(i),
                                              rays.getSigns(i),
                                              rays.getInverseDirection(i))){
              // Check to see if we are inside the box.
              if (tmin > T_EPSILON)
                rays.hit( i, tmin, this, this, getTexCoordMapper() );
              // And use the max intersection if we are.
              else
                rays.hit( i, tmax, this, this, getTexCoordMapper() );
            }
          }
          return;
        }
      virtual void computeNormal(const RenderContext&, RayPacket& rays) const
        {
          rays.computeHitPositions();
          for(int i=rays.begin(); i<rays.end(); i++) {
            Vector hp = rays.getHitPosition(i);
            if (Abs(hp.x() - _bbox[0][0]) < 0.0001)
              rays.setNormal(i, Vector(-1, 0, 0 ));

            else if (Abs(hp.x() - _bbox[1][0]) < 0.0001)
              rays.setNormal(i, Vector( 1, 0, 0 ));

            else if (Abs(hp.y() - _bbox[0][1]) < 0.0001)
              rays.setNormal(i, Vector( 0,-1, 0 ));

            else if (Abs(hp.y() - _bbox[1][1]) < 0.0001)
              rays.setNormal(i, Vector( 0, 1, 0 ));

            else if (Abs(hp.z() - _bbox[0][2]) < 0.0001)
              rays.setNormal(i, Vector( 0, 0,-1 ));

            else
              rays.setNormal(i, Vector( 0, 0, 1 ));
          }
        }

      BBox getBounds();

      virtual void computeBounds(const PreprocessContext&, BBox& bbox_) const
        {
          //  bbox.extendByPoint(Point(xmin, ymin, zmin));
          //  bbox.extendByPoint(Point(xmax, ymax, zmax));
          bbox_.extendByPoint( _bbox[0] );
          bbox_.extendByPoint( _bbox[1] );
        }
      void setStepSize(T stepSize)
        {
          _stepSize = stepSize;
        }
      GridArray3<T>* _data;
      float getValue(int x, int y, int z);
      GridArray3<VMCell>* macrocells;
      void calc_mcell(int depth, int ix, int iy, int iz, VMCell& mcell);
      void parallel_calc_mcell(int cell);
      void isect(const int depth, double t_sample,
                 const double dtdx, const double dtdy, const double dtdz,
                 double next_x, double next_y, double next_z,
                 int ix, int iy, int iz,
                 const int startx, const int starty, const int startz,
                 const Vector& cellcorner, const Vector& celldir,
                 HVIsectContext &isctx) const;
      Vector diag;
      Vector inv_diag;
    protected:
      int _nx,_ny,_nz;
      RGBAColorMap* _colorMap;
      BBox _bounds;
      BBox _bbox;
      Vector _datadiag;
      Vector _sdiag;
      Vector _hierdiag;
      Vector _ihierdiag;
      int _depth;
      int* _xsize,*_ysize,*_zsize;
      double* _ixsize, *_iysize, *_izsize;
      Vector _cellSize;
      T _stepSize;
      float _dataMin;
      float _dataMax;
      float _colorScalar;
      float _maxDistance;
    };

  template<class T>
    Volume<T>::Volume(GridArray3<T>* data, RGBAColorMap* colorMap,
                      const BBox& bounds, double cellStepSize,
                      int depth,
                      double forceDataMin, double forceDataMax)
    : _data(data), _colorMap(colorMap)
    {
      _bbox = bounds; // store the actual bounds.
      //slightly expand the bounds.
      _bounds[0] = bounds[0] - bounds.diagonal().length()*T_EPSILON;
      _bounds[1] = bounds[1] + bounds.diagonal().length()*T_EPSILON;

      Vector diag = _bounds.diagonal();

      _cellSize = diag*Vector( 1.0 / (double)(_data->getNx()-1),
                               1.0 / (double)(_data->getNy()-1),
                               1.0 / (double)(_data->getNz()-1));

      //_stepSize = cellStepSize*_cellSize.length()/sqrt(3.0);
      _stepSize = cellStepSize;
      _maxDistance = diag.length();

      float min = 0,max = 0;
      //_data->getMinMax(&min, &max);
      min = FLT_MAX;
      max = -FLT_MAX;
      //computeMinMax(min, max, *_data);
      unsigned int size = _data->getNx()*_data->getNy()*_data->getNz();
      for (int i = 0; i < (int)size; i++)
        {
          if ((*_data)[i] < min) min = (*_data)[i];
          if ((*_data)[i] > max) max = (*_data)[i];
        }
      if (forceDataMin != -FLT_MAX || forceDataMax != -FLT_MAX)
        {
          min = forceDataMin;
          max = forceDataMax;
        }
      _dataMin = min;
      _dataMax = max;
      _colorScalar = 1.0f/(_dataMax - _dataMin);
      cout << "datamin/max: " << min << " " << max << endl;

      _depth = depth;
      if (_depth <= 0)
        _depth=1;
      _datadiag = diag;
      _nx = _data->getNx();
      _ny = _data->getNy();
      _nz = _data->getNz();
      _sdiag = _datadiag/Vector(_nx-1,_ny-1,_nz-1);
      inv_diag  = diag.inverse();

      // Compute all the grid stuff
      _xsize=new int[depth];
      _ysize=new int[depth];
      _zsize=new int[depth];
      int tx=_nx-1;
      int ty=_ny-1;
      int tz=_nz-1;
      for(int i=depth-1;i>=0;i--){
        int nx=(int)(pow(tx, 1./(i+1))+.9);
        tx=(tx+nx-1)/nx;
        _xsize[depth-i-1]=nx;
        int ny=(int)(pow(ty, 1./(i+1))+.9);
        ty=(ty+ny-1)/ny;
        _ysize[depth-i-1]=ny;
        int nz=(int)(pow(tz, 1./(i+1))+.9);
        tz=(tz+nz-1)/nz;
        _zsize[depth-i-1]=nz;
      }
      _ixsize=new double[depth];
      _iysize=new double[depth];
      _izsize=new double[depth];
      cerr << "Calculating depths...\n";
      for(int i=0;i<depth;i++){
        cerr << "xsize=" << _xsize[i] << ", ysize=" << _ysize[i] << ", zsize=" << _zsize[i] << '\n';
        _ixsize[i]=1./_xsize[i];
        _iysize[i]=1./_ysize[i];
        _izsize[i]=1./_zsize[i];
      }
      cerr << "X: ";
      tx=1;
      for(int i=depth-1;i>=0;i--){
        cerr << _xsize[i] << ' ';
        tx*=_xsize[i];
      }
      cerr << "(" << tx << ")\n";
      if(tx<_nx-1){
        cerr << "TX TOO SMALL!\n";
        exit(1);
      }
      cerr << "Y: ";
      ty=1;
      for(int i=depth-1;i>=0;i--){
        cerr << _ysize[i] << ' ';
        ty*=_ysize[i];
      }
      cerr << "(" << ty << ")\n";
      if(ty<_ny-1){
        cerr << "TY TOO SMALL!\n";
        exit(1);
      }
      cerr << "Z: ";
      tz=1;
      for(int i=depth-1;i>=0;i--){
        cerr << _zsize[i] << ' ';
        tz*=_zsize[i];
      }
      cerr << "(" << tz << ")\n";
      if(tz<_nz-1){
        cerr << "TZ TOO SMALL!\n";
        exit(1);
      }
      _hierdiag=_datadiag*Vector(tx,ty,tz)/Vector(_nx-1,_ny-1,_nz-1);
      _ihierdiag=_hierdiag.inverse();

      if(depth==1){
        macrocells=0;
      } else {
        macrocells=new GridArray3<VMCell>[depth+1];
        int xs=1;
        int ys=1;
        int zs=1;
        for(int d=depth-1;d>=1;d--){
          xs*=_xsize[d];
          ys*=_ysize[d];
          zs*=_zsize[d];
          macrocells[d].resize(xs, ys, zs);
          cerr << "Depth " << d << ": " << xs << "x" << ys << "x" << zs << '\n';
        }
        cerr << "Building hierarchy\n";
        VMCell top;
        calc_mcell(depth-1,0,0,0,top);
        cerr << "done\n";
      }
      //cerr << "**************************************************\n";
      //print(cerr);
      //cerr << "**************************************************\n";
    }

  template<class T>
    Volume<T>::~Volume()
    {
      delete[] macrocells;
      delete[] _xsize;
      delete[] _ysize;
      delete[] _zsize;
      delete[] _ixsize;
      delete[] _iysize;
      delete[] _izsize;
    }

  template<class T>
    BBox Volume<T>::getBounds()
    {
      return _bounds;
    }

  template<class T>
    void Volume<T>::setBounds(BBox bounds)
    {
      //slightly expand the bounds.
      _bounds[0] = bounds[0] - bounds.diagonal().length()*T_EPSILON;
      _bounds[1] = bounds[1] + bounds.diagonal().length()*T_EPSILON;
      Vector diag = _bounds.diagonal();
      _bbox = bounds;

      _cellSize = diag*Vector( 1.0 / (double)(_data->getNx()-1),
                               1.0 / (double)(_data->getNy()-1),
                               1.0 / (double)(_data->getNz()-1));

      //_stepSize = cellStepSize*_cellSize.length()/sqrt(3.0);
      _maxDistance = diag.length();

      if (_depth <= 0)
        _depth=1;
      Vector old_datadiag = _datadiag;
      _datadiag = diag;
      _sdiag = _datadiag/Vector(_nx-1,_ny-1,_nz-1);
      inv_diag  = diag.inverse();

      _hierdiag=_hierdiag*_datadiag/old_datadiag;
      _ihierdiag=_hierdiag.inverse();
    }

  template<class T>
    void computeMinMax(float& min, float& max, GridArray3<T>& grid)
    {
      unsigned int i, total = grid.getNx() * grid.getNy() * grid.getNz();

      min = max = grid[0];
      for(i=1; i<total; i++)
        {
          if  (min > grid[i]) min = grid[i];
          else if (max < grid[i]) max = grid[i];
        }
    }


  template<class T>
    void Volume<T>::calc_mcell(int depth, int startx, int starty, int startz, VMCell& mcell)
    {
      int endx=startx+_xsize[depth];
      int endy=starty+_ysize[depth];
      int endz=startz+_zsize[depth];
      int nx = _nx;
      int ny = _ny;
      int nz = _nz;
      if(endx>nx-1)
        endx=nx-1;
      if(endy>ny-1)
        endy=ny-1;
      if(endz>nz-1)
        endz=nz-1;
      if(startx>=endx || starty>=endy || startz>=endz){
        /* This cell won't get used... */
        return;
      }
      if(depth==0){
        // We are at the data level.  Loop over each voxel and compute the
        // mcell for this group of voxels.
        GridArray3<T>& data = *_data;
        float data_min = _dataMin;
        float data_max = _dataMax;
        for(int ix=startx;ix<endx;ix++){
          for(int iy=starty;iy<endy;iy++){
            for(int iz=startz;iz<endz;iz++){
              float rhos[8];
              rhos[0]=data(ix, iy, iz);
              rhos[1]=data(ix, iy, iz+1);
              rhos[2]=data(ix, iy+1, iz);
              rhos[3]=data(ix, iy+1, iz+1);
              rhos[4]=data(ix+1, iy, iz);
              rhos[5]=data(ix+1, iy, iz+1);
              rhos[6]=data(ix+1, iy+1, iz);
              rhos[7]=data(ix+1, iy+1, iz+1);
              float minr=rhos[0];
              float maxr=rhos[0];
              for(int i=1;i<8;i++){
                if(rhos[i]<minr)
                  minr=rhos[i];
                if(rhos[i]>maxr)
                  maxr=rhos[i];
              }
              // Figure out what bits to turn on running from min to max.
              mcell.turn_on_bits(minr, maxr, data_min, data_max);
            }
          }
        }
      } else {
        int nxl=_xsize[depth-1];
        int nyl=_ysize[depth-1];
        int nzl=_zsize[depth-1];
        GridArray3<VMCell>& mcells=macrocells[depth];
        for(int x=startx;x<endx;x++){
          for(int y=starty;y<endy;y++){
            for(int z=startz;z<endz;z++){
              // Compute the mcell for this block and store it in tmp
              VMCell tmp;
              calc_mcell(depth-1, x*nxl, y*nyl, z*nzl, tmp);
              // Stash it away
              mcells(x,y,z)=tmp;
              // Now aggregate all the mcells created for this depth by
              // doing a bitwise or.
              mcell |= tmp;
            }
          }
        }
      }
    }

  template<class T>
    void Volume<T>::preprocess(const PreprocessContext& context)
    {

    }


#define RAY_TERMINATION_THRESHOLD 0.9
  template<class T>
    void Volume<T>::isect(const int depth, double t_sample,
                          const double dtdx, const double dtdy, const double dtdz,
                          double next_x, double next_y, double next_z,
                          int ix, int iy, int iz,
                          const int startx, const int starty, const int startz,
                          const Vector& cellcorner, const Vector& celldir,
                          HVIsectContext &isctx) const
    {
      int cx=_xsize[depth];
      int cy=_ysize[depth];
      int cz=_zsize[depth];

      GridArray3<T>& data = *_data;

      if(depth==0){
        int nx = _nx;
        int ny = _ny;
        int nz = _nz;
        for(;;){
          int gx=startx+ix;
          int gy=starty+iy;
          int gz=startz+iz;

          // t is our t_sample

          // If we have valid samples
          if(gx<nx-1 && gy<ny-1 && gz<nz-1){
            float rhos[8];
            rhos[0]=data(gx, gy, gz);
            rhos[1]=data(gx, gy, gz+1);
            rhos[2]=data(gx, gy+1, gz);
            rhos[3]=data(gx, gy+1, gz+1);
            rhos[4]=data(gx+1, gy, gz);
            rhos[5]=data(gx+1, gy, gz+1);
            rhos[6]=data(gx+1, gy+1, gz);
            rhos[7]=data(gx+1, gy+1, gz+1);

            ////////////////////////////////////////////////////////////
            // get the weights

            Vector weights = cellcorner+celldir*t_sample;
            double x_weight_high = weights.x()-ix;
            double y_weight_high = weights.y()-iy;
            double z_weight_high = weights.z()-iz;


            double lz1, lz2, lz3, lz4, ly1, ly2, value;
            lz1 = rhos[0] * (1 - z_weight_high) + rhos[1] * z_weight_high;
            lz2 = rhos[2] * (1 - z_weight_high) + rhos[3] * z_weight_high;
            lz3 = rhos[4] * (1 - z_weight_high) + rhos[5] * z_weight_high;
            lz4 = rhos[6] * (1 - z_weight_high) + rhos[7] * z_weight_high;

            ly1 = lz1 * (1 - y_weight_high) + lz2 * y_weight_high;
            ly2 = lz3 * (1 - y_weight_high) + lz4 * y_weight_high;

            value = ly1 * (1 - x_weight_high) + ly2 * x_weight_high;
            value = (value - _dataMin)*_colorScalar;

            RGBAColor color = _colorMap->GetColor(value);
            float alpha_factor = color.a*(1-isctx.alpha);
            if (alpha_factor > 0.001) {
              // the point is contributing, so compute the color

              /*Light* light=isctx.cx->scene->light(0);
                if (light->isOn()) {

                // compute the gradient
                Vector gradient;
                float dx = ly2 - ly1;

                float dy, dy1, dy2;
                dy1 = lz2 - lz1;
                dy2 = lz4 - lz3;
                dy = dy1 * (1 - x_weight_high) + dy2 * x_weight_high;

                float dz, dz1, dz2, dz3, dz4, dzly1, dzly2;
                dz1 = rhos[1] - rhos[0];
                dz2 = rhos[3] - rhos[2];
                dz3 = rhos[5] - rhos[4];
                dz4 = rhos[7] - rhos[6];
                dzly1 = dz1 * (1 - y_weight_high) + dz2 * y_weight_high;
                dzly2 = dz3 * (1 - y_weight_high) + dz4 * y_weight_high;
                dz = dzly1 * (1 - x_weight_high) + dzly2 * x_weight_high;
                float length2 = dx*dx+dy*dy+dz*dz;
                if (length2){
                // this lets the compiler use a special 1/sqrt() operation
                float ilength2 = 1/sqrt(length2);
                gradient = Vector(dx*ilength2, dy*ilength2,dz*ilength2);
                } else {
                gradient = Vector(0,0,0);
                }

                Vector light_dir;
                Point current_p = isctx.ray.origin() + isctx.ray.direction()*t_sample - this->min.vector();
                light_dir = light->get_pos()-current_p;

                Color temp = color(gradient, isctx.ray.direction(),
                light_dir.normal(),
                *(this->dpy->lookup_color(value)),
                light->get_color());
                isctx.total += temp * alpha_factor;
                } else {*/
              isctx.total += color.color*(alpha_factor);
              isctx.alpha += alpha_factor;
            }

          }

          // Update the new position
          t_sample += isctx.t_inc;

          // If we have overstepped the limit of the ray
          if(t_sample >= isctx.t_max)
            break;

          // Check to see if we leave the level or not
          bool break_forloop = false;
          while (t_sample > next_x) {
            // Step in x...
            next_x+=dtdx;
            ix+=isctx.dix_dx;
            if(ix<0 || ix>=cx) {
              break_forloop = true;
              break;
            }
          }
          while (t_sample > next_y) {
            next_y+=dtdy;
            iy+=isctx.diy_dy;
            if(iy<0 || iy>=cy) {
              break_forloop = true;
              break;
            }
          }
          while (t_sample > next_z) {
            next_z+=dtdz;
            iz+=isctx.diz_dz;
            if(iz<0 || iz>=cz) {
              break_forloop = true;
              break;
            }
          }

          if (break_forloop)
            break;

          // This does early ray termination when we don't have anymore
          // color to collect.
          if (isctx.alpha >= RAY_TERMINATION_THRESHOLD)
            break;
        }
      } else {
        GridArray3<VMCell>& mcells=macrocells[depth];
        for(;;){
          int gx=startx+ix;
          int gy=starty+iy;
          int gz=startz+iz;
          bool hit = mcells(gx,gy,gz) & isctx.transfunct;
          if(hit){
            // Do this cell...
            int new_cx=_xsize[depth-1];
            int new_cy=_ysize[depth-1];
            int new_cz=_zsize[depth-1];
            int new_ix=(int)((cellcorner.x()+t_sample*celldir.x()-ix)*new_cx);
            int new_iy=(int)((cellcorner.y()+t_sample*celldir.y()-iy)*new_cy);
            int new_iz=(int)((cellcorner.z()+t_sample*celldir.z()-iz)*new_cz);
            if(new_ix<0)
              new_ix=0;
            else if(new_ix>=new_cx)
              new_ix=new_cx-1;
            if(new_iy<0)
              new_iy=0;
            else if(new_iy>=new_cy)
              new_iy=new_cy-1;
            if(new_iz<0)
              new_iz=0;
            else if(new_iz>=new_cz)
              new_iz=new_cz-1;
            double new_dtdx=dtdx*_ixsize[depth-1];
            double new_dtdy=dtdy*_iysize[depth-1];
            double new_dtdz=dtdz*_izsize[depth-1];
            const Vector dir(isctx.ray.direction());
            double new_next_x;
            if(dir.x() >= 0){
              new_next_x=next_x-dtdx+new_dtdx*(new_ix+1);
            } else {
              new_next_x=next_x-new_ix*new_dtdx;
            }
            double new_next_y;
            if(dir.y() >= 0){
              new_next_y=next_y-dtdy+new_dtdy*(new_iy+1);
            } else {
              new_next_y=next_y-new_iy*new_dtdy;
            }
            double new_next_z;
            if(dir.z() >= 0){
              new_next_z=next_z-dtdz+new_dtdz*(new_iz+1);
            } else {
              new_next_z=next_z-new_iz*new_dtdz;
            }
            int new_startx=gx*new_cx;
            int new_starty=gy*new_cy;
            int new_startz=gz*new_cz;
            Vector cellsize(new_cx, new_cy, new_cz);
            isect(depth-1, t_sample,
                  new_dtdx, new_dtdy, new_dtdz,
                  new_next_x, new_next_y, new_next_z,
                  new_ix, new_iy, new_iz,
                  new_startx, new_starty, new_startz,
                  (cellcorner-Vector(ix, iy, iz))*cellsize, celldir*cellsize,
                  isctx);
          }

          // We need to determine where the next sample is.  We do this
          // using the closest crossing in x/y/z.  This will be the next
          // sample point.
          double closest;
          if(next_x < next_y && next_x < next_z){
            // next_x is the closest
            closest = next_x;
          } else if(next_y < next_z){
            closest = next_y;
          } else {
            closest = next_z;
          }

          double step = ceil((closest - t_sample)*isctx.t_inc_inv);
          t_sample += isctx.t_inc * step;

          if(t_sample >= isctx.t_max)
            break;

          // Now that we have the next sample point, we need to determine
          // which cell it ended up in.  There are cases (grazing corners
          // for example) which will make the next sample not be in the
          // next cell.  Because this case can happen, this code will try
          // to determine which cell the sample will live.
          //
          // Perhaps there is a way to use cellcorner and cellsize to get
          // ix/y/z.  The result would have to be cast to an int, and then
          // next_x/y/z would have to be updated as needed. (next_x +=
          // dtdx * (newix - ix).  The advantage of something like this
          // would be the lack of branches, but it does use casts.
          bool break_forloop = false;
          while (t_sample > next_x) {
            // Step in x...
            next_x+=dtdx;
            ix+=isctx.dix_dx;
            if(ix<0 || ix>=cx) {
              break_forloop = true;
              break;
            }
          }
          while (t_sample > next_y) {
            next_y+=dtdy;
            iy+=isctx.diy_dy;
            if(iy<0 || iy>=cy) {
              break_forloop = true;
              break;
            }
          }
          while (t_sample > next_z) {
            next_z+=dtdz;
            iz+=isctx.diz_dz;
            if(iz<0 || iz>=cz) {
              break_forloop = true;
              break;
            }
          }
          if (break_forloop)
            break;

          if (isctx.alpha >= RAY_TERMINATION_THRESHOLD)
            break;
        }
      }
    }

#define RAY_TERMINATION 0.98
  template<class T>
    void Volume<T>::shade(const RenderContext & context, RayPacket& rays) const
    {
      rays.normalizeDirections();
      rays.computeHitPositions();


      Real t_inc = _stepSize;

      RayPacketData rpData1;
      RayPacket lRays1(rpData1, RayPacket::UnknownShape, rays.begin(), rays.end(), rays.getDepth()+1,
                       RayPacket::NormalizedDirections);
      Real tMins[rays.end()]; //rays.ray start of volume
      Real tMaxs[rays.end()]; //rays.ray end of volume or hit something in volume
      float alphas[rays.end()];
      Color totals[RayPacket::MaxSize];

      for(int rayIndex = rays.begin(); rayIndex < rays.end(); rayIndex++) {
        Vector origin = rays.getOrigin(rayIndex);
        if (_bounds.contains(origin)) {
          //we are inside the volume.
          tMins[rayIndex] = 0;
        }
        else {
          //we hit the volume from the outside so need to move the ray
          //origin to the volume hitpoint.
          origin = rays.getHitPosition(rayIndex);
          tMins[rayIndex] = rays.getMinT(rayIndex);
        }
        if (!(_bounds.contains(rays.getHitPosition(rayIndex))))
          {
            Real tmin, tmax;
            intersectAaBox( _bounds,
                                          tmin,
                                          tmax,
                                          rays.getRay(rayIndex),
                                          rays.getSigns(rayIndex),
                                          rays.getInverseDirection(rayIndex));
            tMins[rayIndex] = tmin;
            origin = rays.getOrigin(rayIndex) + rays.getDirection(rayIndex)*tmin;
          }
        lRays1.setRay(rayIndex, origin, rays.getDirection(rayIndex));

        alphas[rayIndex] = 0;
      }
      lRays1.resetHits();
      context.scene->getObject()->intersect(context, lRays1);
      lRays1.computeHitPositions();
      for(int i = rays.begin(); i < rays.end(); i++) {
        //did we hit something, and if so, was it inside the volume.
        if (lRays1.wasHit(i) && _bounds.contains(lRays1.getHitPosition(i)))
          tMaxs[i] = lRays1.getMinT(i) + tMins[i];
        else {
          //it's possible that we hit a corner of the volume on entry
          //and due to precision issues we can't hit the exit part of
          //the volume. These are rare, but they do happen. Note that
          //just checking wasHit won't work if there's geometry outside
          //the volume.
          tMaxs[i] = -1;
        }
        totals[i] = Color(RGB(0,0,0));
      }

      for(int rayIndex = rays.begin(); rayIndex < rays.end(); rayIndex++) {
        Ray ray = rays.getRay(rayIndex);
        Real t_min = tMins[rayIndex];
        Real t_max = tMaxs[rayIndex];

        const Vector dir(ray.direction());
        const Vector orig(ray.origin());
        int dix_dx;
        int ddx;
        if(dir.x() >= 0){
          dix_dx=1;
          ddx=1;
        } else {
          dix_dx=-1;
          ddx=0;
        }
        int diy_dy;
        int ddy;
        if(dir.y() >= 0){
          diy_dy=1;
          ddy=1;
        } else {
          diy_dy=-1;
          ddy=0;
        }
        int diz_dz;
        int ddz;
        if(dir.z() >= 0){
          diz_dz=1;
          ddz=1;
        } else {
          diz_dz=-1;
          ddz=0;
        }

        Vector start_p(orig+dir*t_min);
        Vector s((start_p-_bounds.getMin())*_ihierdiag);
        int cx=_xsize[_depth-1];
        int cy=_ysize[_depth-1];
        int cz=_zsize[_depth-1];
        int ix=(int)(s.x()*cx);
        int iy=(int)(s.y()*cy);
        int iz=(int)(s.z()*cz);
        if(ix>=cx)
          ix--;
        if(iy>=cy)
          iy--;
        if(iz>=cz)
          iz--;
        if(ix<0)
          ix++;
        if(iy<0)
          iy++;
        if(iz<0)
          iz++;


        double next_x, next_y, next_z;
        double dtdx, dtdy, dtdz;

        double icx=_ixsize[_depth-1];
        double x=_bounds.getMin().x()+_hierdiag.x()*double(ix+ddx)*icx;
        double xinv_dir=1./dir.x();
        next_x=Abs((x-orig.x())*xinv_dir); //take Abs so we don't get -inf
        dtdx=dix_dx*_hierdiag.x()*icx*xinv_dir; //this is +inf when dir.x == 0

        double icy=_iysize[_depth-1];
        double y=_bounds.getMin().y()+_hierdiag.y()*double(iy+ddy)*icy;
        double yinv_dir=1./dir.y();
        next_y=Abs((y-orig.y())*yinv_dir);
        dtdy=diy_dy*_hierdiag.y()*icy*yinv_dir;

        double icz=_izsize[_depth-1];
        double z=_bounds.getMin().z()+_hierdiag.z()*double(iz+ddz)*icz;
        double zinv_dir=1./dir.z();
        next_z=Abs((z-orig.z())*zinv_dir);
        dtdz=diz_dz*_hierdiag.z()*icz*zinv_dir;

        Vector cellsize(cx,cy,cz);
        // cellcorner and celldir can be used to get the location in terms
        // of the metacell in index space.
        //
        // For example if you wanted to get the location at time t (world
        // space units) in terms of indexspace you would do the following
        // computation:
        //
        // Vector pos = cellcorner + celldir * t + Vector(startx, starty, startz);
        //
        // If you wanted to get how far you are inside a given cell you
        // could use the following code:
        //
        // Vector weights = cellcorner + celldir * t - Vector(ix, iy, iz);
        Vector cellcorner((orig-_bounds.getMin())*_ihierdiag*cellsize);
        Vector celldir(dir*_ihierdiag*cellsize);

        HVIsectContext isctx;
        isctx.total = totals[rayIndex];
        isctx.alpha = alphas[rayIndex];
        isctx.dix_dx = dix_dx;
        isctx.diy_dy = diy_dy;
        isctx.diz_dz = diz_dz;
        isctx.transfunct.course_hash = _colorMap->course_hash;
        isctx.t_inc = t_inc;
        isctx.t_min = t_min;
        isctx.t_max = t_max;
        isctx.t_inc_inv = 1/isctx.t_inc;
        isctx.ray = ray;

        isect(_depth-1, t_min, dtdx, dtdy, dtdz, next_x, next_y, next_z,
              ix, iy, iz, 0, 0, 0,
              cellcorner, celldir,
              isctx);

        alphas[rayIndex] = isctx.alpha;
        totals[rayIndex] = isctx.total;
      }
      const bool depth = (rays.getDepth() < context.scene->getRenderParameters().maxDepth);
      int start = -1;
      for(int i = rays.begin(); i < rays.end(); i++) {
        if (alphas[i] < RAY_TERMINATION) {
          if (start < 0)
            start = i;

          const Ray ray = rays.getRay(i);
          if (lRays1.getHitMaterial(i) == this) //did we hit the outside of the volume?
            lRays1.setRay(i, ray.origin() + ray.direction()*tMaxs[i],
                          ray.direction());
          else if (tMaxs[i] == -1) //we weren't supposed to be in the volume
            lRays1.setRay(i, ray.origin()+ray.direction()*(tMins[i]),
                          ray.direction());
          else //we hit something else inside the volume
               //Let's take an epsilon step back so we can hit this when we trace a ray.
            lRays1.setRay(i, ray.origin()+ray.direction()*(tMaxs[i]-T_EPSILON*2),
                          ray.direction());
        }
        else {
          //we don't want to trace the terminated ray.
          if (start >= 0) {
            //let's trace these rays
            lRays1.resize(start, i);
            context.sample_generator->setupChildPacket(context, rays, lRays1);
            context.renderer->traceRays(context, lRays1);
            for(int k = start; k < i; k++) {
              Color bgColor(RGB(0,0,0));
              if (depth)
                bgColor = lRays1.getColor(k);
              totals[k] += bgColor*(1.-alphas[k]);
              rays.setColor(k, totals[k]);
            }
            start = -1;
          }
          rays.setColor(i, totals[i]);
        }
      }
      if (start >= 0) {
        //let's trace just these active rays
        lRays1.resize(start, rays.end());

        context.sample_generator->setupChildPacket(context, rays, lRays1);
        context.renderer->traceRays(context, lRays1);
        for(int i = start; i < rays.end(); i++) {
          Color bgColor(RGB(0,0,0));
          if (depth)
            bgColor = lRays1.getColor(i);
          totals[i] += bgColor*(1.-alphas[i]);
          rays.setColor(i, totals[i]);
        }
      }
    }

  template<class T>
    void Volume<T>::computeHistogram(int numBuckets, int* histValues)
    {
      float dataMin = _dataMin;
      float dataMax = _dataMax;
      int nx = _data->getNx()-1;
      int ny = _data->getNy()-1;
      int nz = _data->getNz()-1;
      float scale = (numBuckets-1)/(dataMax-dataMin);
      for(int i =0; i<numBuckets;i++)
        histValues[i] = 0;
      for(int ix=0;ix<nx;ix++){
        for(int iy=0;iy<ny;iy++){
          for(int iz=0;iz<nz;iz++){
            float p000=(*_data)(ix,iy,iz);
            float p001=(*_data)(ix,iy,iz+1);
            float p010=(*_data)(ix,iy+1,iz);
            float p011=(*_data)(ix,iy+1,iz+1);
            float p100=(*_data)(ix+1,iy,iz);
            float p101=(*_data)(ix+1,iy,iz+1);
            float p110=(*_data)(ix+1,iy+1,iz);
            float p111=(*_data)(ix+1,iy+1,iz+1);
            float min=std::min(std::min(std::min(p000, p001), std::min(p010, p011)), std::min(std::min(p100, p101), std::min(p110, p111)));
            float max=std::max(std::max(std::max(p000, p001), std::max(p010, p011)), std::max(std::max(p100, p101), std::max(p110, p111)));
            int nmin=(int)((min-dataMin)*scale);
            int nmax=(int)((max-dataMin)*scale+.999999);
            if(nmax>=numBuckets)
              nmax=numBuckets-1;
            if(nmin<0)
              nmin=0;
            for(int i=nmin;i<nmax;i++){
              histValues[i]++;
            }
          }
        }
      }
    }

  template<class T>
    void Volume<T>::attenuateShadows(const RenderContext& context, RayPacket& shadowRays) const
    {

    }

  template<class T>
    float Volume<T>::getValue(int x, int y, int z)
    {
      return _data(x, y, z);
    }
}//namespace manta

#endif
