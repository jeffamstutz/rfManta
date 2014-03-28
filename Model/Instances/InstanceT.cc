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

#include <Model/Instances/InstanceT.h>
#include <Model/Instances/MPT.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/BBox.h>

using namespace Manta;

InstanceT::InstanceT(Object* instance, const Vector& translation)
  : instance(instance), translation(translation)
{
  // By default, use the texture coordinates of the child object
  tex = this;
}

InstanceT::~InstanceT()
{
}

void InstanceT::preprocess(const PreprocessContext& context)
{
  instance->preprocess(context);
}

void InstanceT::computeBounds(const PreprocessContext& context, BBox& bbox) const
{
  BBox ibox;
  instance->computeBounds(context, ibox);
  bbox.extendByPoint(ibox.getMin()+translation);
  bbox.extendByPoint(ibox.getMax()+translation);
}

void InstanceT::intersect(const RenderContext& context, RayPacket& rays) const
{
  RayPacketData raydata;
  RayPacket instance_rays(raydata, RayPacket::UnknownShape, rays.begin(), rays.end(),
                          rays.getDepth(), rays.getAllFlags());

  if(rays.getFlag(RayPacket::ConstantOrigin)){
    Vector o(rays.getOrigin(rays.begin())-translation);
    for(int i = rays.begin();i<rays.end();i++){
      instance_rays.setRay(i, o, rays.getDirection(i));
      instance_rays.resetHit(i, rays.getMinT(i));
    }
  } else {
    for(int i = rays.begin();i<rays.end();i++){
      Vector o(rays.getOrigin(i)-translation);
      instance_rays.setRay(i, o, rays.getDirection(i));
      instance_rays.resetHit(i, rays.getMinT(i));
    }
  }
  instance->intersect(context, instance_rays);
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

void InstanceT::computeNormal(const RenderContext& context, RayPacket& rays) const
{
  Vector old_origins[RayPacket::MaxSize];

  // Save the original origins
  for(int i=rays.begin();i<rays.end();i++){
    old_origins[i] = rays.getOrigin(i);
    rays.setOrigin(i, old_origins[i]-translation);
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

  // Put the origins back
  for(int i=rays.begin();i<rays.end();i++){
    rays.setOrigin(i, old_origins[i]);
  }
  rays.resetFlag(RayPacket::HaveHitPositions);
}

void InstanceT::setTexCoordMapper(const TexCoordMapper* new_tex)
{
  tex = new_tex;
}

void InstanceT::computeTexCoords2(const RenderContext& context,
                                  RayPacket& rays) const
{
  Vector old_origins[RayPacket::MaxSize];

  // Save the original origins
  for(int i=rays.begin();i<rays.end();i++){
    old_origins[i] = rays.getOrigin(i);
    rays.setOrigin(i, old_origins[i]-translation);
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

  // Put the origins back
  for(int i=rays.begin();i<rays.end();i++){
    rays.setOrigin(i, old_origins[i]);
  }
}

void InstanceT::computeTexCoords3(const RenderContext& context,
                                  RayPacket& rays) const
{
  Vector old_origins[RayPacket::MaxSize];

  // Save the original origins
  for(int i=rays.begin();i<rays.end();i++){
    old_origins[i] = rays.getOrigin(i);
    rays.setOrigin(i, old_origins[i]-translation);
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

  // Put the origins back
  for(int i=rays.begin();i<rays.end();i++){
    rays.setOrigin(i, old_origins[i]);
  }
}
