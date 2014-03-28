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

#include <Model/Instances/InstanceRT.h>
#include <Model/Instances/MPT.h>
#include <Interface/RayPacket.h>
#include <Core/Exceptions/BadPrimitive.h>
#include <Core/Geometry/BBox.h>
#include <Core/Math/MiscMath.h>
#include <sstream>

using namespace Manta;
using namespace std;

InstanceRT::InstanceRT(Object* instance, const AffineTransform& transform_ )
  : instance(instance), transform( transform_ ), transform_inv( transform_.inverse() )
{
  // By default, use the texture coordinates of the child object
  tex = this;

  // Determine the scale of the matrix.  We could use eigenvalue analysis,
  // but this seems easier:  pass in the three axes and make sure that the
  // transformed lengths are all the same
  Vector v1 = transform.multiply_vector(Vector(1,0,0));
  Real   l1 = v1.normalize();
  Vector v2 = transform.multiply_vector(Vector(0,1,0));
  Real   l2 = v2.normalize();
  Vector v3 = transform.multiply_vector(Vector(0,0,1));
  Real   l3 = v3.normalize();

  if(Abs(l1-1) > (Real)1.e-7 ||
     Abs(l2-1) > (Real)1.e-7 ||
     Abs(l3-1) > (Real)1.e-7)
    {
      ostringstream msg;
      msg << "Nonuniform scale for InstanceRT, scalefactor=[" << l1 << ' '
          << l2 << ' ' << l3 << ']';
      throw BadPrimitive(msg.str());
  }
  
}

InstanceRT::~InstanceRT()
{
}

void InstanceRT::preprocess(const PreprocessContext& context)
{
  instance->preprocess(context);
}

void InstanceRT::computeBounds(const PreprocessContext& context, BBox& bbox) const
{
  BBox ibox;
  instance->computeBounds(context, ibox);

  // Transform the eight corners of the child bounding box
  for(int i=0;i<8;i++)
    bbox.extendByPoint(transform.multiply_point(ibox.getCorner(i)));
}

void InstanceRT::intersect(const RenderContext& context, RayPacket& rays) const
{
  RayPacketData raydata;
  RayPacket instance_rays(raydata, RayPacket::UnknownShape, rays.begin(), rays.end(),
                          rays.getDepth(), rays.getAllFlags());

  if(rays.getFlag(RayPacket::ConstantOrigin)){
    Vector o = transform_inv.multiply_point(rays.getOrigin(rays.begin()));

    for(int i = rays.begin();i<rays.end();i++){
      Vector dir = transform_inv.multiply_vector(rays.getDirection(i));

      instance_rays.setRay(i, o, dir);
      instance_rays.resetHit(i, rays.getMinT(i));
    }
  } else {
    for(int i = rays.begin();i<rays.end();i++){
      Vector o = transform_inv.multiply_point(rays.getOrigin(i));
      Vector dir = transform_inv.multiply_vector(rays.getDirection(i));

      instance_rays.setRay(i, o, dir);
      instance_rays.resetHit(i, rays.getMinT(i));
    }
  }
  instance->intersect(context, instance_rays);
  // Now if our instance rays were rescaled by a primitive (call to
  // normalizeDirections), we need to scale our rays too.
  if (instance_rays.getFlag(RayPacket::NormalizedDirections))
      rays.normalizeDirections();
  for(int i=rays.begin();i<rays.end();i++){
    if(instance_rays.wasHit(i)){
      // Instance was hit
      if(rays.hit(i, instance_rays.getMinT(i), material, this, tex)){
        // Instance is now the closest
        rays.scratchpad<MPT>(i) = MPT(instance_rays.getHitMaterial(i),
                                      instance_rays.getHitPrimitive(i),
                                      instance_rays.getHitTexCoordMapper(i));
      }
    }
  }
}

void InstanceRT::computeNormal(const RenderContext& context, RayPacket& rays) const
{
  Ray old_rays[RayPacket::MaxSize];

  // Save the original rays
  for(int i=rays.begin();i<rays.end();i++){
    old_rays[i] = rays.getRay(i);

    Vector o = transform_inv.multiply_point(old_rays[i].origin());
    Vector dir = transform_inv.multiply_vector(old_rays[i].direction());

    rays.setRay(i, o, dir);
  }
  rays.resetFlag(RayPacket::HaveHitPositions);

  int allUnitNormals = RayPacket::HaveUnitNormals;
  for(int i=rays.begin();i<rays.end();){
    const Primitive* prim = rays.scratchpad<MPT>(i).primitive;
    int end = i+1;
    while(end < rays.end() && rays.scratchpad<MPT>(end).primitive == prim)
      end++;
    RayPacket subPacket(rays, i, end);
    prim->computeNormal(context, subPacket);
    // If we ever encounter a case where the normals aren't normalized
    // turn the flag off.
    allUnitNormals &= subPacket.getAllFlags();
    i = end;
  }
  // If all the normals were unit length set the parent RayPacket's flag.
  rays.setFlag(allUnitNormals);

  // Put the rays back and fix the normals
  for(int i=rays.begin();i<rays.end();i++){
    rays.setRay(i, old_rays[i]);
    rays.setNormal(i, transform_inv.transpose_mult_vector(rays.getNormal(i)));
  }
  rays.resetFlag(RayPacket::HaveHitPositions);
}

void InstanceRT::setTexCoordMapper(const TexCoordMapper* new_tex)
{
  tex = new_tex;
}

void InstanceRT::computeTexCoords2(const RenderContext& context,
			       RayPacket& rays) const
{
  Ray old_rays[RayPacket::MaxSize];

  // Save the original rays
  for(int i=rays.begin();i<rays.end();i++){
    old_rays[i] = rays.getRay(i);

    Vector o = transform_inv.multiply_point(old_rays[i].origin());
    Vector dir = transform_inv.multiply_vector(old_rays[i].direction());

    rays.setRay(i, o, dir);
  }
  rays.resetFlag(RayPacket::HaveHitPositions);

  for(int i=rays.begin();i<rays.end();){
    const TexCoordMapper* tex = rays.scratchpad<MPT>(i).tex;
    int end = i+1;
    while(end < rays.end() && rays.scratchpad<MPT>(end).tex == tex)
      end++;
    RayPacket subPacket(rays, i, end);
    tex->computeTexCoords2(context, subPacket);
    i = end;
  }

  // Put the rays back and fix the normals
  for(int i=rays.begin();i<rays.end();i++){
    rays.setRay(i, old_rays[i]);
  }
  rays.resetFlag(RayPacket::HaveHitPositions);
}

void InstanceRT::computeTexCoords3(const RenderContext& context,
			       RayPacket& rays) const
{
  Ray old_rays[RayPacket::MaxSize];

  // Save the original rays
  for(int i=rays.begin();i<rays.end();i++){
    old_rays[i] = rays.getRay(i);

    Vector o = transform_inv.multiply_point(old_rays[i].origin());
    Vector dir = transform_inv.multiply_vector(old_rays[i].direction());

    rays.setRay(i, o, dir);
  }
  rays.resetFlag(RayPacket::HaveHitPositions);

  for(int i=rays.begin();i<rays.end();){
    const TexCoordMapper* tex = rays.scratchpad<MPT>(i).tex;
    int end = i+1;
    while(end < rays.end() && rays.scratchpad<MPT>(end).tex == tex)
      end++;
    RayPacket subPacket(rays, i, end);
    tex->computeTexCoords3(context, subPacket);
    i = end;
  }

  // Put the rays back and fix the normals
  for(int i=rays.begin();i<rays.end();i++){
    rays.setRay(i, old_rays[i]);
  }
  rays.resetFlag(RayPacket::HaveHitPositions);
}

