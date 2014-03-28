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

#ifndef Manta_Model_InstanceT_h
#define Manta_Model_InstanceT_h

#include <Interface/Primitive.h>
#include <Interface/TexCoordMapper.h>
#include <Core/Geometry/Vector.h>
#include <Model/Instances/InstanceMaterial.h>

namespace Manta {

  class InstanceT : public Primitive,
                    public InstanceMaterial,
                    public TexCoordMapper
  {
  public:
    InstanceT(Object* instance, const Vector& translation);
    virtual ~InstanceT();

    // Generic
    virtual void preprocess(const PreprocessContext&);
    virtual void computeBounds(const PreprocessContext& context,
                               BBox& bbox) const;

    // From Primitive
    virtual void intersect(const RenderContext& context, RayPacket& rays) const;
    virtual void computeNormal(const RenderContext& context, RayPacket& rays) const;
    virtual void setTexCoordMapper(const TexCoordMapper* new_tex);

    // From TexCoordMapper
    virtual void computeTexCoords2(const RenderContext& context,
				   RayPacket& rays) const;
    virtual void computeTexCoords3(const RenderContext& context,
				   RayPacket& rays) const;
  private:
    Object* instance;
    Vector translation;
    const TexCoordMapper* tex;
  };
}

#endif
