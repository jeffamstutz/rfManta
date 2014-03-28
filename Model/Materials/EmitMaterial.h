#ifndef MANTA_MATERIAL_EMITMATERIAL_H__
#define MANTA_MATERIAL_EMITMATERIAL_H__

/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005
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

#include <Core/Color/Color.h>

#include <Interface/Texture.h>
#include <Model/Materials/OpaqueShadower.h>

namespace Manta {

  /*
    This material simply uses the values of the texture as the final
    result disregarding any lighing parameters.
  */
  class EmitMaterial: public OpaqueShadower {
  public:
    EmitMaterial(const Color& color);
    EmitMaterial(const Texture<Color>* colortex);

    virtual void preprocess(const PreprocessContext&);
    virtual void shade(const RenderContext& context, RayPacket& rays) const;

  private:
    Texture<Color> const *colortex;
  };

} // end namespace Manta

#endif // #ifndef MANTA_MATERIAL_EMITMATERIAL_H__
