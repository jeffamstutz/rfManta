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

#include <Model/Primitives/Ring.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/BBox.h>
#include <Core/Math/Expon.h>

using namespace Manta;
using namespace std;

Ring::Ring(Material* material, const Vector& center, const Vector& normal,
           Real radius, Real thickness)
  : PrimitiveCommon(material), center(center), normal(normal),
    radius2(radius*radius)
{
  this->normal.normalize();
  normal_dot_center = Dot(this->normal, center);
  Real outer_radius = radius+thickness;
  outer_radius2 = outer_radius * outer_radius;
}

Ring::~Ring()
{
}

void Ring::computeBounds(const PreprocessContext&, BBox& bbox) const
{
  bbox.extendByDisc(center, normal, Sqrt(outer_radius2));
}

void Ring::intersect(const RenderContext&, RayPacket& rays) const
{
  rays.normalizeDirections();
  for(int i=rays.begin(); i<rays.end(); i++)
  {
    Vector dir(rays.getDirection(i));
    Vector orig(rays.getOrigin(i));
    Real dt = Dot(dir, normal);
//     // Check for when the ray is parallel to the plane of the ring.
//     if(dt < 1.e-6 && dt > -1.e-6)
//       return;
//     // Compute the hit location on the plane and see if it would
//     // generate a hit.
    Real t = (normal_dot_center-Dot(normal, orig))/dt;
//     if(e.hitInfo.wasHit() && t>e.hitInfo.minT())
//       return;
    // Compute the location of the hit point and then see if the point
    // is inside the ring extents.
    Vector hitPosition(orig+dir*t);
    Real l = (hitPosition-center).length2();
    if(l > radius2 && l < outer_radius2)
      rays.hit(i, t, getMaterial(), this, getTexCoordMapper());
  }
}


void Ring::computeNormal(const RenderContext&, RayPacket& rays) const
{
  for(int i=rays.begin(); i<rays.end(); i++) {
    rays.setNormal(i, normal);
  }
}
