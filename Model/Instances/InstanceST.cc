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

#include <Model/Instances/InstanceST.h>
#include <Model/Instances/MPT.h>
#include <Interface/RayPacket.h>
#include <Core/Geometry/BBox.h>

using namespace Manta;

InstanceST::InstanceST(Object* instance, const Vector& scale,
                       const Vector& translation)
  : instance(instance), scale(scale), translation(translation)
{
  // By default, use the texture coordinates of the child object
  tex = this;
  if(scale.x() == scale.y() && scale.y() == scale.z())
    uniform_scale = true;
  else
    uniform_scale = false;
  inv_scale = Vector(1/scale.x(), 1/scale.y(), 1/scale.z());
}

InstanceST::~InstanceST()
{
}

void InstanceST::preprocess(const PreprocessContext& context)
{
  instance->preprocess(context);
}

void InstanceST::computeBounds(const PreprocessContext& context, BBox& bbox) const
{
  BBox ibox;
  instance->computeBounds(context, ibox);
  bbox.extendByPoint(ibox.getMin()*scale + translation);
  bbox.extendByPoint(ibox.getMax()*scale + translation);
}

void InstanceST::intersect(const RenderContext& context, RayPacket& rays) const
{
  RayPacketData raydata;
  RayPacket instance_rays(raydata, RayPacket::UnknownShape, rays.begin(), rays.end(),
                          rays.getDepth(), rays.getAllFlags());

  Real scales[RayPacket::MaxSize];
  Real inv_scales[RayPacket::MaxSize];
  if(uniform_scale){
    Real iscale = inv_scale.x();
    if(rays.getFlag(RayPacket::ConstantOrigin)){
      Vector o( (rays.getOrigin(rays.begin())-translation) * iscale );

      for(int i = rays.begin();i<rays.end();i++){
        instance_rays.setRay(i, o, rays.getDirection(i));
        instance_rays.resetHit(i, rays.getMinT(i)*iscale);
      }
    } else {
      for(int i = rays.begin();i<rays.end();i++){
        Vector o( (rays.getOrigin(i)-translation) * iscale );
        instance_rays.setRay(i, o, rays.getDirection(i));
        instance_rays.resetHit(i, rays.getMinT(i)*iscale);
      }
    }
  } else {
    if(rays.getFlag(RayPacket::ConstantOrigin)){
      Vector o( (rays.getOrigin(rays.begin())-translation) * inv_scale );
      for(int i = rays.begin();i<rays.end();i++){
        Vector dir(rays.getDirection(i)*inv_scale);
        Real length = dir.length();
        inv_scales[i] = length;
        Real ilength = 1/length;
        scales[i] = ilength;
        instance_rays.setRay(i, o, dir*ilength);
        instance_rays.resetHit(i, rays.getMinT(i)*length);
      }
    } else {
      for(int i = rays.begin();i<rays.end();i++){
        Vector o( (rays.getOrigin(i)-translation) * inv_scale );
        Vector dir(rays.getDirection(i)*inv_scale);
        Real length = dir.length();
        scales[i] = length;
        Real ilength = 1/length;
        inv_scales[i] = ilength;
        instance_rays.setRay(i, o, dir*ilength);
        instance_rays.resetHit(i, rays.getMinT(i)*length);
      }
    }
  }
  instance->intersect(context, instance_rays);
  if(uniform_scale){
    Real s = scale.x();
    for(int i=rays.begin();i<rays.end();i++){
      if(instance_rays.wasHit(i)){
        // Instance was hit
        if(rays.hit(i, instance_rays.getMinT(i)*s, material, this, tex)){
          // Instance is now the closest
          rays.scratchpad<MPT>(i) = MPT(instance_rays.getHitMaterial(i),
                                        instance_rays.getHitPrimitive(i),
                                        instance_rays.getHitTexCoordMapper(i));
        }
      }
    }
  } else {
    for(int i=rays.begin();i<rays.end();i++){
      if(instance_rays.wasHit(i)){
        // Instance was hit
        Real s = scales[i];
        if(rays.hit(i, instance_rays.getMinT(i)*s, material, this, tex)){
          // Instance is now the closest
          rays.scratchpad<MPTscale>(i) =
            MPTscale(instance_rays.getHitMaterial(i),
                     instance_rays.getHitPrimitive(i),
                     instance_rays.getHitTexCoordMapper(i),
                     s, inv_scales[i]);
        }
      }
    }
  }
}

void
InstanceST::computeNormal(const RenderContext& context, RayPacket& rays) const
{
  Real old_minT[RayPacket::MaxSize];
  Vector old_origins[RayPacket::MaxSize];
  Ray old_rays[RayPacket::MaxSize];

  if(uniform_scale){
    // Save the original origins and minT
    Real is = inv_scale.x();
    for(int i=rays.begin();i<rays.end();i++){
      old_origins[i] = rays.getOrigin(i);
      rays.setOrigin(i, (old_origins[i]-translation) * is);
      old_minT[i] = rays.getMinT(i);
      rays.scaleMinT(i, is);
    }
  } else {
    // Save the original rays and minT
    for(int i=rays.begin();i<rays.end();i++){
      old_rays[i] = rays.getRay(i);
      Vector dir(old_rays[i].direction()*inv_scale);
      Real ilength = rays.scratchpad<MPTscale>(i).scale;
      Vector o( (old_rays[i].origin()-translation) * inv_scale );
      rays.setRay(i, o, dir*ilength);
      old_minT[i] = rays.getMinT(i);
      Real length = rays.scratchpad<MPTscale>(i).inv_scale;
      rays.scaleMinT(i, length);
    }
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
  if(uniform_scale){
    // Put the origins and minT back
    for(int i=rays.begin();i<rays.end();i++){
      rays.setOrigin(i, old_origins[i]);
      rays.overrideMinT(i, old_minT[i]);
    }
    // If all the normals were unit length set the parent RayPacket's flag.
    rays.setFlag(allUnitNormals);
  } else {
    // Put the origins and minT back, scale normal
    for(int i=rays.begin();i<rays.end();i++){
      rays.setRay(i, old_rays[i]);
      rays.overrideMinT(i, old_minT[i]);
      rays.setNormal(i, rays.getNormal(i) * scale);
    }
  }
  rays.resetFlag(RayPacket::HaveHitPositions);
}

void InstanceST::setTexCoordMapper(const TexCoordMapper* new_tex)
{
  tex = new_tex;
}

void InstanceST::computeTexCoords2(const RenderContext& context,
                                   RayPacket& rays) const
{
  Real old_minT[RayPacket::MaxSize];
  Vector old_origins[RayPacket::MaxSize];
  Ray old_rays[RayPacket::MaxSize];

  if(uniform_scale){
    // Save the original origins and minT
    Real is = inv_scale.x();
    for(int i=rays.begin();i<rays.end();i++){
      old_origins[i] = rays.getOrigin(i);
      rays.setOrigin(i, (old_origins[i]-translation) * is);
      old_minT[i] = rays.getMinT(i);
      rays.scaleMinT(i, is);
    }
  } else {
    // Save the original rays and minT
    for(int i=rays.begin();i<rays.end();i++){
      old_rays[i] = rays.getRay(i);
      Vector dir(old_rays[i].direction()*inv_scale);
      Real ilength = rays.scratchpad<MPTscale>(i).scale;
      Vector o( (old_rays[i].origin()-translation) * inv_scale );
      rays.setRay(i, o, dir*ilength);
      old_minT[i] = rays.getMinT(i);
      Real length = rays.scratchpad<MPTscale>(i).inv_scale;
      rays.scaleMinT(i, length);
    }
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

  if(uniform_scale){
    // Put the origins and minT back
    for(int i=rays.begin();i<rays.end();i++){
      rays.setOrigin(i, old_origins[i]);
      rays.overrideMinT(i, old_minT[i]);
    }
  } else {
    // Put the origins and minT back, scale normal
    for(int i=rays.begin();i<rays.end();i++){
      rays.setRay(i, old_rays[i]);
      rays.overrideMinT(i, old_minT[i]);
    }
  }
  rays.resetFlag(RayPacket::HaveHitPositions);
}

void InstanceST::computeTexCoords3(const RenderContext& context,
                                   RayPacket& rays) const
{
  Real old_minT[RayPacket::MaxSize];
  Vector old_origins[RayPacket::MaxSize];
  Ray old_rays[RayPacket::MaxSize];

  if(uniform_scale){
    // Save the original origins and minT
    Real is = inv_scale.x();
    for(int i=rays.begin();i<rays.end();i++){
      old_origins[i] = rays.getOrigin(i);
      rays.setOrigin(i, (old_origins[i]-translation) * is);
      old_minT[i] = rays.getMinT(i);
      rays.scaleMinT(i, is);
    }
  } else {
    // Save the original rays and minT
    for(int i=rays.begin();i<rays.end();i++){
      old_rays[i] = rays.getRay(i);
      Vector dir(old_rays[i].direction()*inv_scale);
      Real ilength = rays.scratchpad<MPTscale>(i).scale;
      Vector o( (old_rays[i].origin()-translation) * inv_scale );
      rays.setRay(i, o, dir*ilength);
      old_minT[i] = rays.getMinT(i);
      Real length = rays.scratchpad<MPTscale>(i).inv_scale;
      rays.scaleMinT(i, length);
    }
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

  if(uniform_scale){
    // Put the origins and minT back
    for(int i=rays.begin();i<rays.end();i++){
      rays.setOrigin(i, old_origins[i]);
      rays.overrideMinT(i, old_minT[i]);
    }
  } else {
    // Put the origins and minT back, scale normal
    for(int i=rays.begin();i<rays.end();i++){
      rays.setRay(i, old_rays[i]);
      rays.overrideMinT(i, old_minT[i]);
    }
  }
  rays.resetFlag(RayPacket::HaveHitPositions);
}

