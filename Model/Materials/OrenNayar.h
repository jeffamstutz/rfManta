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

/*
 *  Coded by Leena Kora
 *    on 08/07/2007
 *
 *  This material class is for rough surfaced objects.
 *  This class takes two arguments one for primary color and other argument for providing surface roughness(0-1)
 */
 

#ifndef Manta_Model_OrenNayar_h
#define Manta_Model_OrenNayar_h

#include <Model/Materials/LitMaterial.h>
#include <Core/Color/Color.h>
#include <Interface/Texture.h>

namespace Manta{
  class LightSet;

 class OrenNayar : public LitMaterial {
  public:
    OrenNayar(const Color& color, const Real sigma);
    OrenNayar(const Texture<Color>* colorfn);
    OrenNayar() {  };
    virtual ~OrenNayar();

    virtual void shade(const RenderContext& context, RayPacket& rays) const;
  private:
    Real a, b;
    const Texture<Color>* colortex;
        
  };
}

#endif
