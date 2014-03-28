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
  This maps a texture onto an imaginary globe that surround the scene.
 */

#ifndef Manta_Model_TextureBackground_h
#define Manta_Model_TextureBackground_h

#include <Interface/Background.h>
#include <Model/TexCoordMappers/UniformMapper.h>
#include <Interface/Texture.h>
#include <Core/Color/Color.h>

namespace Manta {

  class TextureBackground : public Background, public UniformMapper {
  public:
    // When mapper is 0 this class will be used as the mapper.  I
    // can't set the default value of the parameter to this without
    // the compiler complaining about it.
    TextureBackground(const Texture<Color>* colortex,
                      const TexCoordMapper* mapper = 0);
    virtual ~TextureBackground();

    // Interface to Background
    virtual void preprocess(const PreprocessContext& context);
    virtual void shade(const RenderContext& context, RayPacket& rays) const;

    // Interface to TexCoordMapper
    virtual void computeTexCoords2(const RenderContext& context,
				   RayPacket& rays) const;
  private:
    // This is the texture to be mapped
    const Texture<Color>* colortex;
    // This is the texture coordinate mapper
    const TexCoordMapper* mapper;
  };
}

#endif
