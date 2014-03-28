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

#include <Model/Primitives/Plane.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/BBox.h>
#include <Core/Math/MiscMath.h>
#include <Core/Math/Trig.h>

using namespace Manta;
using namespace std;

Plane::Plane(Material* material, const Vector& normal, const Vector& center):
  PrimitiveCommon(material), normal(normal), center(center)
{
  this->normal.normalize();
}

Plane::~Plane()
{
}

void
Plane::computeBounds(const PreprocessContext& context,
                     BBox& bbox) const
{
  // Since the plane is infinate, this doesn't make much sense.  I'm
  // inclined to at least extend the bounding box by the center, so it
  // would at least show up somehow.
  bbox.extendByPoint(center);
}

void
Plane::intersect(const RenderContext& context, RayPacket& rays) const
{
  // This is a simple first pass.  We should do different
  // optimizations in the future.
  rays.normalizeDirections();
  for(int i = rays.begin();i<rays.end();i++){
    Real dn = Dot( rays.getDirection(i), normal );
    if (dn != 0) {
      // Ray isn't parallel to the plane.
      Real ao = Dot( (center-rays.getOrigin(i)), normal );
      Real t = ao/dn;
      rays.hit(i, t, getMaterial(), this, getTexCoordMapper());
    }
  }
}

void
Plane::computeNormal(const RenderContext& context, RayPacket& rays) const
{
  // We don't need to do anything except stuff the normal in the the
  // RayPacket.
  for(int i = rays.begin();i<rays.end();i++){
    rays.setNormal(i, normal);
  }
  rays.setFlag(RayPacket::HaveUnitNormals);
}


