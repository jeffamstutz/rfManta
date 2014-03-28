
#include <Interface/RayPacket.h>
#include <MantaSSE.h>
#include <iostream>

using namespace Manta;

void RayPacket::actualNormalizeDirections()
{
#ifdef MANTA_SSE
  int b = (rayBegin + 3) & (~3);
  int e = rayEnd & (~3);
  if(b >= e){
    for(int i=rayBegin;i<rayEnd;i++){
      Real sum = 0;
      for(int j=0;j<3;j++)
        sum += data->direction[j][i] * data->direction[j][i];
      Real length = Sqrt(sum);
      data->minT[i] *= length;
      Real scale = 1/length;
      for(int j=0;j<3;j++)
        data->direction[j][i] *= scale;
    }
  } else {
    int i = rayBegin;
    for(;i<b;i++){
      Real sum = 0;
      for(int j=0;j<3;j++)
        sum += data->direction[j][i] * data->direction[j][i];
      Real length = Sqrt(sum);
      data->minT[i] *= length;
      Real scale = 1/length;
      for(int j=0;j<3;j++)
        data->direction[j][i] *= scale;
    }
    for(;i<e;i+=4){
      __m128 xd = _mm_load_ps(&data->direction[0][i]);
      __m128 yd = _mm_load_ps(&data->direction[1][i]);
      __m128 zd = _mm_load_ps(&data->direction[2][i]);
      __m128 sum = _mm_add_ps(_mm_add_ps(_mm_mul_ps(xd, xd), _mm_mul_ps(yd, yd)), _mm_mul_ps(zd, zd));
      __m128 scale =  _mm_rsqrt_ps(sum);
      // Do one newton-raphson iteration to get the accuracy we need
      scale = _mm_mul_ps(_mm_mul_ps(scale, _mm_sub_ps(_mm_set1_ps(3.f), _mm_mul_ps(sum, _mm_mul_ps(scale, scale)))), _mm_set1_ps(0.5f));
      _mm_store_ps(&data->direction[0][i], _mm_mul_ps(xd, scale));
      _mm_store_ps(&data->direction[1][i], _mm_mul_ps(yd, scale));
      _mm_store_ps(&data->direction[2][i], _mm_mul_ps(zd, scale));

      _mm_store_ps(&data->minT[i], _mm_div_ps(_mm_load_ps(&data->minT[i]), scale));
    }
    for(;i<rayEnd;i++){
      Real sum = 0;
      for(int j=0;j<3;j++)
        sum += data->direction[j][i] * data->direction[j][i];
      Real length = Sqrt(sum);
      data->minT[i] *= length;
      Real scale = 1/length;
      for(int j=0;j<3;j++)
        data->direction[j][i] *= scale;
    }
  }
#else
  for(int i=rayBegin;i<rayEnd;i++){
    Real sum = 0;
    for(int j=0;j<3;j++)
      sum += data->direction[j][i] * data->direction[j][i];
    Real length = Sqrt(sum);
    data->minT[i] *= length;
    Real scale = 1/length;
    for(int j=0;j<3;j++)
      data->direction[j][i] *= scale;
  }
#endif

  flags |= NormalizedDirections;

  if (flags & HaveInverseDirections)
    actualComputeInverseDirections();
}

void RayPacket::actualComputeInverseDirections()
{
#if MANTA_SSE
  int b = (rayBegin + 3) & (~3);
  int e = rayEnd & (~3);
  if (b >= e) {
    for(int i=rayBegin;i<rayEnd;i++)
      for(int j=0;j<3;j++)
        data->inverseDirection[j][i] = (Real)1/data->direction[j][i];
  } else {
    int i = rayBegin;
    // Do the non aligned in front
    for(;i<b;++i)
      for(int j=0;j<3;j++)
        data->inverseDirection[j][i] = (Real)1/data->direction[j][i];
    // Do the aligned in the middle
    for(;i<e;i+=4)
      for(int j=0;j<3;j++) {
        _mm_store_ps(&data->inverseDirection[j][i],
                     oneOver(_mm_load_ps(&data->direction[j][i])));
      }
    // Do the non aligned in end
    for(;i<rayEnd;++i)
      for(int j=0;j<3;j++)
        data->inverseDirection[j][i] = (Real)1/data->direction[j][i];
  }
#else
  for(int i=rayBegin;i<rayEnd;i++)
    for(int j=0;j<3;j++)
      data->inverseDirection[j][i] = (Real)1/data->direction[j][i];
#endif
  flags |= HaveInverseDirections;
}

void RayPacket::actualComputeHitPositions()
{
#ifdef MANTA_SSE
        int b = (rayBegin + 3) & (~3);
        int e = rayEnd & (~3);
        if(b >= e){
                for(int i = begin(); i < end(); i++){
                        for(int j=0;j<3;j++)
                                data->hitPosition[j][i] = data->origin[j][i] + data->direction[j][i] * data->minT[i];
                }
        } else {
                int i = rayBegin;
                for(;i<b;i++){
                        for(int j=0;j<3;j++)
                                data->hitPosition[j][i] = data->origin[j][i] + data->direction[j][i] * data->minT[i];
                }
                for(;i<e;i+=4){
                        __m128 minT = _mm_load_ps(&data->minT[i]);
                        _mm_store_ps(&data->hitPosition[0][i], _mm_add_ps(_mm_load_ps(&data->origin[0][i]), _mm_mul_ps(_mm_load_ps(&data->direction[0][i]), minT)));
                        _mm_store_ps(&data->hitPosition[1][i], _mm_add_ps(_mm_load_ps(&data->origin[1][i]), _mm_mul_ps(_mm_load_ps(&data->direction[1][i]), minT)));
                        _mm_store_ps(&data->hitPosition[2][i], _mm_add_ps(_mm_load_ps(&data->origin[2][i]), _mm_mul_ps(_mm_load_ps(&data->direction[2][i]), minT)));
                }
                for(;i<rayEnd;i++){
                        for(int j=0;j<3;j++)
                                data->hitPosition[j][i] = data->origin[j][i] + data->direction[j][i] * data->minT[i];
                }
        }
#else
        for(int i = rayBegin; i < rayEnd; i++){
                for(int j=0;j<3;j++)
                        data->hitPosition[j][i] = data->origin[j][i] + data->direction[j][i] * data->minT[i];
        }
#endif
  flags |= HaveHitPositions;
}


template void RayPacket::computeNormals<true>(const RenderContext& context);
template void RayPacket::computeNormals<false>(const RenderContext& context);

template<bool allHit>
void RayPacket::computeNormals(const RenderContext& context)
{
  if(flags & HaveNormals)
    return;

  // Compute normals
  for(int i=rayBegin;i<rayEnd;){
    if (!allHit) {
    /*
     * Compute normals, but watch for the hit information (currently
     * flagged by matl) This can be called on a packet where some of
     * the rays may have hit the background
     */
      while(!data->hitMatl[i]){
        i++;
        if(i >= rayEnd) {
          flags |= HaveNormals | HaveUnitNormals;
          return;
        }
      }
    }

    const Primitive* prim = data->hitPrim[i];
    int tend = i+1;
    while(tend < rayEnd && (allHit || data->hitMatl[tend]) && 
          data->hitPrim[tend] == prim)
      tend++;
    RayPacket subPacket(*this, i, tend);
    prim->computeNormal(context, subPacket);
    // BTW, == has higher precedence than &, so mind the ()'s.
    if ((subPacket.flags & HaveUnitNormals) == 0) {
      // Normalize the normals if they haven't been.
      for(int s=i;s<tend;++s){
        Real sum = 0;
        for(int j=0;j<3;++j)
          sum += data->normal[j][s] * data->normal[j][s];
        Real scale = 1/Sqrt(sum);
        for(int j=0;j<3;++j)
          data->normal[j][s] *= scale;
      }
    }
    i=tend;
  }
  flags |= HaveNormals | HaveUnitNormals;
}

template void RayPacket::computeFFNormals<true>(const RenderContext& context);
template void RayPacket::computeFFNormals<false>(const RenderContext& context);

template<bool allHit>
void RayPacket::computeFFNormals(const RenderContext& context)
{
  if(flags & HaveFFNormals)
    return;

  // We need to have normals
  computeNormals<allHit>(context);

  for(int i = rayBegin; i < rayEnd; ++i) {
    if (!allHit) {
      while(!data->hitMatl[i]) {
        i++;
        if(i >= rayEnd) {
          flags |= HaveFFNormals;
          return;
        }
      }
    }
    // Compute the dot product
    Vector normal(getNormal(i));
   if (Dot(normal, getDirection(i)) <= 0) {
      setFFNormal(i,  normal);
    } else {
      setFFNormal(i, -normal);
    }
  }

  flags |= HaveFFNormals;
}

//do some explicit instantiations so the linker is happy
template void RayPacket::computeGeometricNormals<true>(const RenderContext& context);
template void RayPacket::computeGeometricNormals<false>(const RenderContext& context);

template<bool allHit>
void RayPacket::computeGeometricNormals(const RenderContext& context)
{
  if(flags & HaveGeometricNormals)
    return;
  // Compute geometric normals
  for(int i=rayBegin;i<rayEnd;) {
    if (!allHit) {
      while(!data->hitMatl[i]) {
        i++;
        if(i >= rayEnd) {
          flags |= HaveGeometricNormals | HaveUnitGeometricNormals;
          return;
        }
      }
    }
    const Primitive* prim = data->hitPrim[i];
    int tend = i+1;
    while(tend < rayEnd && (allHit || data->hitMatl[tend]) && 
          data->hitPrim[tend] == prim)
      tend++;
    RayPacket subPacket(*this, i, tend);
    prim->computeGeometricNormal(context, subPacket);
    if((subPacket.flags & HaveUnitGeometricNormals) == 0) {
      for(int s=i;s<tend;++s) {
        Real sum = 0;
        for(int j=0;j<3;++j)
          sum += data->geometricNormal[j][s] * data->geometricNormal[j][s];
        Real scale = 1/Sqrt(sum);
        for(int j=0;j<3;++j)
          data->geometricNormal[j][s] *= scale;
      }
    }
    i=tend;
  }

  flags |= HaveGeometricNormals | HaveUnitGeometricNormals;
}

template void RayPacket::computeFFGeometricNormals<true>(const RenderContext& context);
template void RayPacket::computeFFGeometricNormals<false>(const RenderContext& context);

template<bool allHit>
void RayPacket::computeFFGeometricNormals(const RenderContext& context)
{
  if(flags & HaveFFGeometricNormals)
    return;

  // We need to have normals
  computeGeometricNormals<allHit>(context);

  for(int i=rayBegin; i<rayEnd; ++i) {
    if (!allHit) {
      while(!data->hitMatl[i]) {
        i++;
        if(i >= rayEnd) {
          flags |= HaveFFGeometricNormals;
          return;
        }
      }
    }
    // Compute the dot product
    Vector normal(getGeometricNormal(i));
    if (Dot(normal, getDirection(i)) <= 0) {
      setFFGeometricNormal(i,  normal);
    } else {
      setFFGeometricNormal(i, -normal);
    }
  }

  flags |= HaveFFGeometricNormals;
}

void RayPacket::actualComputeSurfaceDerivatives(const RenderContext& context) {
  // Compute surface derivatives in runs over Primitive*
  for(int i=rayBegin;i<rayEnd;){
    const Primitive* prim = data->hitPrim[i];
    int tend = i+1;
    while(tend < rayEnd && data->hitPrim[tend] == prim)
      tend++;
    RayPacket subPacket(*this, i, tend);
    prim->computeSurfaceDerivatives(context, subPacket);
    i=tend;
  }

  flags |= HaveSurfaceDerivatives;
}

void RayPacket::actualComputeTextureCoordinates2(const RenderContext& context)
{
  // Compute texture coordinates
  for(int i=rayBegin;i<rayEnd;){
    const TexCoordMapper* tex = data->hitTex[i];
    int tend = i+1;
    while(tend < rayEnd && data->hitTex[tend] == tex)
      tend++;
    RayPacket subPacket(*this, i, tend);
    tex->computeTexCoords2(context, subPacket);
    i=tend;
  }

  flags |= HaveTexture2;
}

void RayPacket::actualComputeTextureCoordinates3(const RenderContext& context)
{
  // Compute texture coordinates
  for(int i=rayBegin;i<rayEnd;){
    const TexCoordMapper* tex = data->hitTex[i];
    int tend = i+1;
    while(tend < rayEnd && data->hitTex[tend] == tex)
      tend++;
    RayPacket subPacket(*this, i, tend);
    tex->computeTexCoords3(context, subPacket);
    i=tend;
  }

  flags |= HaveTexture3;
}
namespace Manta {
  std::ostream& operator<< (std::ostream& os, const RayPacket& rays) {
    for (int i = rays.begin(); i < rays.end(); i++) {
      os << "ray_index " << i << " depth " << rays.getDepth() << " origin";
      os << " " << rays.getOrigin(i) << " direction " << rays.getDirection(i);
      os << " minT " << rays.getMinT(i) << " matl " << rays.getHitMaterial(i);
      os << std::endl;
    }
    return os;
  }
}
