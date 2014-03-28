
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

#ifndef Manta_Model_NormalTexture_h
#define Manta_Model_NormalTexture_h

#include <Interface/Texture.h>
#include <Interface/RayPacket.h>
#include <Model/Textures/Constant.h>

// Abe Stephens

namespace Manta {

  class RayPacket;
  class RenderContext;
  
  class NormalTexture : public Texture<Color> {
  public:
    NormalTexture() { }
    virtual ~NormalTexture() { }
    
    virtual void mapValues(Packet<Color>& results,
                           const RenderContext&,
                           RayPacket& rays) const;
  };
  
  class RaySignTexture : public Texture<Color> {
  public:
    RaySignTexture() { }
    virtual ~RaySignTexture() { }
    
    virtual void mapValues(Packet<Color>& results,
                           const RenderContext&,
                           RayPacket& rays) const;
  };  
  
  template<class ScratchPadInfo, class ValueType = Color >
  class WireframeTexture : public Texture<ValueType> {
  public:
    WireframeTexture( Texture<ValueType> *texture_,
                      const ValueType &wire_value_ )
      : texture( texture_ ), wire_value( wire_value_ )
    { }
    
    WireframeTexture( const ValueType &color_,
                      const ValueType &wire_value_ )
      : wire_value( wire_value_ )
    {
      texture = new Constant<ValueType>( color_ );
    }
    
    virtual ~WireframeTexture() { }
    
    virtual void mapValues(Packet<ValueType>& results,
                           const RenderContext&,
                           RayPacket& rays) const;
  private:
    
    ///////////////////////////////////////////////////////////////////////////
    // Note: This method must be implemented for each possible
    // template parameter type explicitly. It is responsible for
    // copying the coordinates out of the ray packet scratch pad.
    void getBarycentricCoords( RayPacket &rays, int which,
                               Real &a, Real &b, Real &c ) const;

    Texture<ValueType> *texture;
    ValueType wire_value;
  };

  // Non specialized template functions
  template<class ScratchPadInfo, class ValueType>
  void WireframeTexture<ScratchPadInfo,ValueType>::
  mapValues(Packet<ValueType>& results,
            const RenderContext& context,
            RayPacket& rays) const
  {

    // Compute the normal for each ray.
    rays.computeNormals<true>( context );

    // Lookup dependent texture values.
    Packet<ValueType> texture_values;
    texture->mapValues( texture_values, context, rays );

    // Iterate over the packet and set the colors.
    for (int i=rays.begin();i<rays.end();++i) {

      // Lookup barycentric coordinates.
      Real a, b, c;
      getBarycentricCoords( rays, i, a, b, c );

      if ((a < (Real)0.05) || (b < (Real)0.05) || (c < (Real)0.05)) {
        results.set(i, wire_value);
      }
      else {
        results.set(i, texture_values.get(i));
      }

    }
  }
  
};

#endif
