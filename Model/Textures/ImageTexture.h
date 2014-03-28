/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005
  Scientific Computing and Imaging Institue, University of Utah

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

/*
  Author: James Bigler
  Date: Nov. 2005

  This class is used for 2D texturing.  The constructor takes an Image
  class and makes a copy of it while converting it to type Color.  No
  sense in converting everytime you want to do lookups.

  Some basic texturing parameters are supported.  Bilinear and nearest
  neighbor interpolation methods can be selected after construction.
  You can also select to wrap boundaries or clamp them.  You should
  also be able to set these via a transaction as well.

  With bilinear interpolation with wrapped boundaries will interpolate
  across the edges.
*/



#ifndef Manta_Model_ImageTexture_h
#define Manta_Model_ImageTexture_h

#include <Model/Textures/ImageTexture.h>
#include <Interface/RayPacket.h>
#include <Interface/Image.h>

#include <Core/Exceptions/InternalError.h>

#include <Core/Geometry/Vector.h>
#include <Core/Math/MiscMath.h>
#include <Core/Math/Trig.h>

#include <Interface/Texture.h>

#include <Core/Containers/Array2.h>
#include <Core/Math/MiscMath.h>

#include <string>
#include <iosfwd>

namespace Manta {

  class RayPacket;
  class RenderContext;
  class Image;

  template<typename ValueType> class ImageTexture;

  // This will potentially throw a InputError exception if there was a
  // problem.  If stream is non null it will write out chatty stuff to
  // that.
  ImageTexture<Color>* LoadColorImageTexture( const std::string& file_name,
                                              std::ostream* stream = 0,
                                              bool linearize = false);

  // Whatever type ValueType ends up being, it needs to have
  // ValueType::ScalarType defined.  I'm not sure what to do for
  // things that aren't.  If the occasion arrises such that you need
  // something else, then you should make this a double template on
  // ScalarType.
  template< typename ValueType >
  class ImageTexture : public Texture< ValueType > {
  public:
    ImageTexture(const Image* image, bool linearize = true);
    virtual ~ImageTexture() {}

    virtual void mapValues(Packet<ValueType>& results,
                           const RenderContext&,
                           RayPacket& rays) const;

    template<class TU, class TV>
    void setScale(const TU& u_scale, const TV& v_scale) {
      // We don't know if T will match ValueType::ScalarType, so we
      // need to copy the components over one at a time to do the
      // implicit cast.
      scale[0] = static_cast<ScalarType>(u_scale);
      scale[1] = static_cast<ScalarType>(v_scale);
    }

    enum {
      NearestNeighbor,
      Bilinear
    };

    enum {
      Wrap,
      Clamp
    };

    void setUEdgeBehavior(int new_behavior) {
      setEdgeBehavior(new_behavior, u_edge);
    }
    void setVEdgeBehavior(int new_behavior) {
      setEdgeBehavior(new_behavior, v_edge);
    }

    void setInterpolationMethod(int new_method);

  private:
    typedef typename ValueType::ComponentType ScalarType;

    inline void BL_edge_behavior(ScalarType val, int behavior, int size,
                                 int& low, int& high,
                                 ScalarType& weight_high) const {
      switch (behavior) {
      case Wrap:
        // When we wrap [0,1] maps to [0, 1, ..., size-1, 0].  This is
        // so we interpolate from texture[size-1] to texture[0].
        val *= size;
        low = static_cast<int>(val);
        weight_high = val - low;
        // If val is negative then we need to look right (negative)
        // for the high interpolant.
        if (val >= 0) {
          high = low + 1;
        } else {
          high = low - 1;
          // Here we get our negative index from [0..-size-1].  Add
          // size back in and you get the right shift.  You need to
          // make sure you do the %size afterwards, though.  This will
          // catch the case where low%size + size == size.
          low  =  low%size + size;
          high = high%size + size;
          // If val was negative then weight_high will also be
          // negative.  We don't need a negative number for
          // interpolation.
          weight_high = -weight_high;
        }
        if (low >= size)
          low  %= size;
        if (high >= size)
          high %= size;
        break;
      case Clamp:
        if (val < 0) {
          low = 0;
          high = 0;
          weight_high = 0;
        } else {
          val *= size-1;
          low = static_cast<int>(val);
          if (low > size-2) {
            low = size-2;
          } else {
            weight_high = val - low;
          }
          high = low + 1;
        }
        break;
      }
    }
    inline int NN_edge_behavior(int val, int behavior, int size) const {
      int result;
      switch (behavior) {
      case Wrap:
        if (val >= 0)
          // result will be [0,size-1]
          result = val % size;
        else
          // val%size -> [-(size-1),0]
          // val%size+size-1 -> [0,size-1]
          result = val%size + size-1;
        break;
      case Clamp:
      default:
        result = Manta::Clamp(val, 0, size-1);
        break;
      }
      return result;
    }

    void setEdgeBehavior(int new_behavior, int& edge);

  private:
    // We make a copy of the data
    Array2<ValueType> texture;
    // You can scale the normal [(0,0)..(1,1)] texture coordinates,
    VectorT< ScalarType, 2 > scale;
    // Linear, nearest neighbor?
    int interpolation_method;
    // Edge behavior
    int u_edge, v_edge;
  };

  template< class ValueType >
  ImageTexture< ValueType >::ImageTexture(const Image* image, bool linearize) :
    scale(VectorT< ScalarType, 2 >(1,1)),
    interpolation_method(NearestNeighbor),
    u_edge(Wrap), v_edge(Wrap)
  {
    bool stereo;
    int xres, yres;
    image->getResolution(stereo, xres, yres);
    if (stereo) {
      throw InternalError( "ImageTexture doesn't support stereo currently");
    }
    texture.resize(xres, yres);

    if (linearize) {
      // Now copy over the data
      for(int y = 0; y < yres; y++) {
        for(int x = 0; x < xres; x+=Fragment::MaxSize) {
          int end = x + Fragment::MaxSize;
          if (end > xres) end = xres;
          Fragment fragment(x, end, y, 0);
          image->get(fragment);
          for(int i = fragment.begin(); i < fragment.end(); i++) {
            ValueType new_value = fragment.getColor(i);
            for (int c = 0; c < ValueType::NumComponents; c++) {
              new_value[c] = ValueType::linearize(new_value[c]);
            }
            texture(x+i, y) = new_value;
          }
        }
      }
    } else {
      // Now copy over the data
      for(int y = 0; y < yres; y++) {
        for(int x = 0; x < xres; x+=Fragment::MaxSize) {
          int end = x + Fragment::MaxSize;
          if (end > xres) end = xres;
          Fragment fragment(x, end, y, 0);
          image->get(fragment);
          for(int i = fragment.begin(); i < fragment.end(); i++) {
            texture(x+i, y) = fragment.getColor(i);
          }
        }
      }
    }
  }

  template< class ValueType >
  void ImageTexture< ValueType >::mapValues(Packet<ValueType>& results,
                                            const RenderContext& context,
                                            RayPacket& rays) const
  {
    rays.computeTextureCoordinates2( context );
    VectorT<ScalarType, 2> tex_coords[RayPacket::MaxSize];

    // Grab the texture coordinates
    for( int i = rays.begin(); i < rays.end(); ++i ) {
      tex_coords[i] = rays.getTexCoords2(i);
      tex_coords[i] *= scale;
    }

    int xres = texture.dim1();
    int yres = texture.dim2();

    // Grab the textures and do the interpolation as needed
    switch (interpolation_method) {
    case Bilinear:
      {
        for( int i = rays.begin(); i < rays.end(); ++i) {
          ScalarType x = tex_coords[i].x();
          ScalarType y = tex_coords[i].y();
          // Initialize these variables to something to quiet the
          // warnings.
          int x_low = 0, y_low = 0, x_high = 0, y_high = 0;
          ScalarType x_weight_high = 0, y_weight_high = 0;

          BL_edge_behavior(x, u_edge, xres, x_low, x_high, x_weight_high);
          BL_edge_behavior(y, v_edge, yres, y_low, y_high, y_weight_high);

          // Do the interpolation
          ValueType a, b;
          a = ( texture(x_low,  y_low )*(1-x_weight_high) +
                texture(x_high, y_low )*   x_weight_high);
          b = ( texture(x_low,  y_high)*(1-x_weight_high) +
                texture(x_high, y_high)*   x_weight_high);
          results.set(i, a*(1-y_weight_high) + b*y_weight_high);
        }
      }
      break;
    case NearestNeighbor:
      {
        for( int i = rays.begin(); i < rays.end(); ++i) {
          int tx = NN_edge_behavior(static_cast<int>(tex_coords[i].x()*xres),
                                    u_edge, texture.dim1());
          int ty = NN_edge_behavior(static_cast<int>(tex_coords[i].y()*yres),
                                    v_edge, texture.dim2());
          results.set(i, texture(tx, ty));
        }
      }
      break;
    }
  } // end mapValues

  template< class ValueType >
  void ImageTexture< ValueType >::setEdgeBehavior(int new_behavior,
                                                  int& edge) {
    switch (new_behavior) {
    case Wrap:
    case Clamp:
      edge = new_behavior;
      break;
    default:
      throw InternalError( "ImageTexture::setEdgeBehavior doesn't support this edge behavior");
      break;
    }
  }

  template< class ValueType >
  void ImageTexture< ValueType >::setInterpolationMethod(int new_method) {
    switch (new_method) {
    case NearestNeighbor:
    case Bilinear:
      interpolation_method = new_method;
      break;
    default:
      throw InternalError( "ImageTexture::setInterpolationMethod doesn't support this interpolation method");
      break;
    }
  }

} // end namespace Manta

#endif // Manta_Model_ImageTexture_h
