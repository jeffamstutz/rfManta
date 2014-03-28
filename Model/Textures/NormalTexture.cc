
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

#include <Model/Textures/NormalTexture.h>

#include <Interface/RayPacket.h>

#include <Core/Color/Color.h>


using namespace Manta;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// NORMAL  NORMAL  NORMAL  NORMAL  NORMAL  NORMAL  NORMAL  NORMAL  NORMAL  NORM
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void NormalTexture::mapValues(Packet<Color>& results,
                              const RenderContext& context,
                              RayPacket& rays) const
{

  // Compute the normal for each ray.
  rays.computeNormals<true>( context );

  // Iterate over the packet and set the colors.
  for (int i=rays.begin();i<rays.end();++i) {

    // Copy the normal out.
    Vector normal = rays.getNormal(i);

    normal = (normal *(Real)0.5) + Vector( 0.5, 0.5, 0.5 );

    results.set(i, Color( RGB( (ColorComponent)normal[0],
                               (ColorComponent)normal[1],
                               (ColorComponent)normal[2] )));
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// RAY SIGNS  RAY SIGNS  RAY SIGNS  RAY SIGNS  RAY SIGNS  RAY SIGNS  RAY SIGNS
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void RaySignTexture::mapValues(Packet<Color>& results,
                               const RenderContext&,
                               RayPacket& rays) const
{

  // Compute the directions for each ray.
  rays.computeInverseDirections();
  rays.computeSigns();

  // Iterate over the packet and set the colors.
  for (int i=rays.begin();i<rays.end();++i) {
    
    // Copy the normal out.
    VectorT<int,3> sign = rays.getSigns( i );
    
    ColorComponent scale = 0.5;
    if (rays.getFlag( RayPacket::ConstantSigns )) {
      scale = 1.0;
    }
    
    results.set(i, Color( RGB( (scale*sign[0])*0.5+0.5,
                               (scale*sign[1])*0.5+0.5,
                               (scale*sign[2])*0.5+0.5 )));


  }
  
}
