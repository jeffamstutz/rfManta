
#ifndef Manta_Interface_RayPacket_h
#define Manta_Interface_RayPacket_h

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


#include <Core/Color/Color.h>
#include <Core/Geometry/Ray.h>
#include <Core/Geometry/VectorT.h>
#include <Core/Math/Expon.h>
#include <Core/Util/Assert.h>
#include <Core/Util/Align.h>
#include <Core/Util/StaticCheck.h>
#include <Interface/Primitive.h>
#include <Interface/TexCoordMapper.h>
#include <Parameters.h>
#include <RayPacketParameters.h>
#include <MantaSSE.h>

#ifdef MANTA_SSE
#include <Core/Math/SSEDefs.h>
#endif
#include <iosfwd>

#include <cstring>

namespace Manta {
  class Material;
  class RenderContext;

  class MANTA_ALIGN(16) RayPacketData {
  public:
    enum RayPacketDataSizes {
      // Scratchpads - 8 values of 4 bytes, 4 values of 8 bytes,
      MaxScratchpad4 = 8,
      MaxScratchpad8 = 4,
      MaxScratchpadSize = SCRATCHPAD_MAXSIZE,
      MaxSize              = RAYPACKET_MAXSIZE,
#ifdef MANTA_SSE
      SSE_MaxSize   = (RAYPACKET_MAXSIZE+3)/4,
#endif
    };
    RayPacketData()
    {
    }

    ~RayPacketData()
    {
    }

    // Pointer-based arrays

    // SWIG generated some error prone code when we used the types
    // directly.  Making a typedef seemed to fix it.
    typedef Primitive const* PrimitiveCP;
    typedef Material const*  MaterialCP;
    typedef TexCoordMapper const* TexCoordMapperCP;

    PrimitiveCP hitPrim[MaxSize];
    MaterialCP hitMatl[MaxSize];
    TexCoordMapperCP hitTex[MaxSize];

    // Real-based arrays
    MANTA_ALIGN(16) Real origin[3][MaxSize];
    MANTA_ALIGN(16) Real direction[3][MaxSize];
    MANTA_ALIGN(16) Real inverseDirection[3][MaxSize];
    MANTA_ALIGN(16) Real minT[MaxSize];
    MANTA_ALIGN(16) Real corner_dir[3][4];
    MANTA_ALIGN(16) Real time[MaxSize]; // Time for this ray in [0,1)

    MANTA_ALIGN(16) Real image[2][MaxSize];
    MANTA_ALIGN(16) Real normal[3][MaxSize];
    MANTA_ALIGN(16) Real ffnormal[3][MaxSize]; // Forward facing normals
    MANTA_ALIGN(16) Real geometricNormal[3][MaxSize];
    MANTA_ALIGN(16) Real ffgeometricNormal[3][MaxSize];
    MANTA_ALIGN(16) Real hitPosition[3][MaxSize];
    MANTA_ALIGN(16) Real texCoords[3][MaxSize];
    MANTA_ALIGN(16) Real dPdu[3][MaxSize];
    MANTA_ALIGN(16) Real dPdv[3][MaxSize];

    // Color-based arrays
    MANTA_ALIGN(16) Color::ComponentType color[Manta::Color::NumComponents][MaxSize];
    MANTA_ALIGN(16) Color::ComponentType importance[Manta::Color::NumComponents][MaxSize];   // 1-attenuation, where eye rays have importance == 1

    // Int-based arrays
    MANTA_ALIGN(16) int whichEye[MaxSize];
    MANTA_ALIGN(16) unsigned int sample_depth[MaxSize];
    MANTA_ALIGN(16) unsigned int sample_id[MaxSize];
    MANTA_ALIGN(16) unsigned int region_id[MaxSize];
    // NOTE(boulos): SSE has no good way to do bools. This also allows
    // us to do per-ray flags in the future. I'm also making this
    // ignoreEmittedLight since this will be the default for almost
    // everything, and it can be filled in using setzero
    MANTA_ALIGN(16) unsigned int ignoreEmittedLight[MaxSize];
    MANTA_ALIGN(16) int signs[3][MaxSize]; // 1=negative, 0=zero, positive

    // Scratchpad
    MANTA_ALIGN(16) float scratchpad4[MaxScratchpad4][MaxSize];
    MANTA_ALIGN(16) double scratchpad8[MaxScratchpad8][MaxSize];

    // Char-based arrays
    char scratchpad_data[MaxSize][MaxScratchpadSize];
  };

  class RayPacket {
  public:
    enum RayPacketSizes {
      MaxSize               = RayPacketData::MaxSize,

#ifdef MANTA_SSE
      SSE_MaxSize           = RayPacketData::SSE_MaxSize,
#endif
    };

    enum RayPacketFlags {
      // Flags. Please use 8 digits to represent new ones (i.e. 0x00010000)
      ConstantOrigin           = 0x00000001,
      ConstantEye              = 0x00000002,
      HaveImageCoordinates     = 0x00000004,
      NormalizedDirections     = 0x00000008,
      HaveHitPositions         = 0x00000010,
      AnyHit                   = 0x00000020,
      HaveTexture3             = 0x00000040,
      HaveTexture2             = 0x00000080,
      HaveUnitNormals          = 0x00000100, // Used by prims that set
                                             // whether the normals computed
                                             // have been normalized.
      HaveNormals              = 0x00000200,
      HaveFFNormals            = 0x00000400,
      HaveInverseDirections    = 0x00000800,
      HaveSigns                = 0x00001000,
      ConstantSigns            = 0x00002000,
      HaveCornerRays           = 0x00004000,

      HaveSurfaceDerivatives   = 0x00008000,
      ConstantSampleRegion     = 0x00010000,

      ConstantPixel            = 0x00020000,

      HaveGeometricNormals     = 0x00040000,
      HaveUnitGeometricNormals = 0x00080000,
      HaveFFGeometricNormals   = 0x00100000,

      DebugPacket              = 0x80000000,
    };

    enum PacketShape {
      LinePacket, SquarePacket, UnknownShape
    };

    // Create a "toplevel" raypacket.  You need to call resetHits or
    // resetHit for every data element in the ray packet before you
    // start intersecting.  This will initialize minT and hitMatl.
    RayPacket(RayPacketData& data, PacketShape shape,
              int rayBegin, int rayEnd, int depth, int flags)
    : data(&data), shape(shape), rayBegin(rayBegin), rayEnd(rayEnd),
      depth(depth), flags(flags)
    {
    }

    // Create a subset of another raypacket
    RayPacket(RayPacket& parent, int rayBegin, int rayEnd)
    : data(parent.data), rayBegin(rayBegin), rayEnd(rayEnd),
      depth(parent.depth),
      flags(parent.flags)
    {
      shape = parent.shape;
      if(shape == SquarePacket){
       // A subset of a square is not necessarily a square
       shape = UnknownShape;
      }
    }

    ~RayPacket()
    {
    }

    // Raypacket flags
    int getAllFlags() const
    {
      return flags;
    }

    bool getFlag( int flag ) const
    {
      return (flags & flag) == flag;
    }

    void setAllFlags(int new_flags)
    {
      flags = new_flags;
    }
    void setFlag(int flag) {
      flags |= flag;
    }
    void resetFlag(int flag) {
      flags &= ~flag;
    }

    // Depth of rays for this raypacket
    int getDepth() const {
      return depth;
    }

    void setDepth(int new_depth) {
      depth=new_depth;
    }

    // Sample depth (number of samples taken so far)
    unsigned int getSampleDepth(unsigned int which) const {
      return data->sample_depth[which];
    }
    void setSampleDepth(unsigned int which, unsigned int new_depth) {
      data->sample_depth[which] = new_depth;
    }

    // Raypacket iteration
    int begin() const {
      return rayBegin;
    }
    int end() const {
      return rayEnd;
    }
    // This function is dangerous since most RayPacket loops go from
    // rayBegin to rayEnd instead of 0 to rayEnd.  Consider not using
    // it.
    void resize(int newSize)
    {
      rayBegin = 0; rayEnd = newSize;
    }
    void resize(int rayBegin_in, int rayEnd_in)
    {
      rayBegin = rayBegin_in; rayEnd = rayEnd_in;
    }

    // Image space
    void setPixel(int which, int whichEye, Real imageX, Real imageY)
    {
      data->image[0][which] = imageX;
      data->image[1][which] = imageY;
      data->whichEye[which] = whichEye;
    }
    Real getImageCoordinates(int which, int dim) const
    {
      return data->image[dim][which];
    }
    int getWhichEye(int which) const
    {
      return data->whichEye[which];
    }

    // Rays
    void setRay(int which, const Vector& origin, const Vector& direction)
    {
      for(int i=0;i<3;i++)
        data->origin[i][which] = origin.data[i];
      for(int i=0;i<3;i++)
        data->direction[i][which] = direction.data[i];
    }
    void setRay(int which, const Ray& ray)
    {
      for(int i=0;i<3;i++)
        data->origin[i][which] = ray.origin()[i];
      for(int i=0;i<3;i++)
        data->direction[i][which] = ray.direction()[i];
    }
    Ray getRay(int which) const
    {
      return Ray(Vector(data->origin[0][which], data->origin[1][which], data->origin[2][which]),
                 Vector(data->direction[0][which], data->direction[1][which], data->direction[2][which]));
    }

    void setOrigin(int which, const Vector& origin)
    {
      for(int i=0;i<3;i++)
        data->origin[i][which] = origin[i];
    }
    void setDirection(int which, const Vector& direction)
    {
      for(int i=0;i<3;i++)
        data->direction[i][which] = direction[i];
    }

    ///////////////////////////////////////////////////////////////////////////
    // GET ORIGIN
    Vector getOrigin(int which) const
    {
      return Vector(data->origin[0][which], data->origin[1][which], data->origin[2][which]);
    }

    Real &getOrigin(int which, int i)
    {
      return data->origin[i][which];
    }
    const Real &getOrigin(int which, int i) const
    {
      return data->origin[i][which];
    }

    ///////////////////////////////////////////////////////////////////////////
    // GET DIRECTION
    Vector getDirection(int which) const {
      return Vector(data->direction[0][which], data->direction[1][which], data->direction[2][which]);
    }

    Real &getDirection(int which, int i) {
      return data->direction[i][which];
    }
    const Real &getDirection(int which, int i) const
    {
      return data->direction[i][which];
    }

    ///////////////////////////////////////////////////////////////////////////
    // GET INVERSE DIRECTION
    Vector getInverseDirection(int which) const {
      return Vector(data->inverseDirection[0][which], data->inverseDirection[1][which], data->inverseDirection[2][which]);
    }

    Real &getInverseDirection(int which, int i)
    {
      return data->inverseDirection[i][which];
    }

    const Real &getInverseDirection(int which, int i) const
    {
      return data->inverseDirection[i][which];
    }

    void normalizeDirections()
    {
      if(flags & NormalizedDirections)
        return;

      actualNormalizeDirections();
    }
    void computeInverseDirections()
    {
      if(flags & HaveInverseDirections)
        return;

      actualComputeInverseDirections();
    }
    void computeSigns()
    {
      if(flags & HaveSigns)
        return;
      for(int i=rayBegin;i<rayEnd;i++)
        for(int j = 0; j < 3; j++)
          data->signs[j][i] = data->direction[j][i] < 0;
      flags |= HaveSigns;
      flags &= ~ConstantSigns;
      int signs[3];
      for(int j=0;j<3;j++)
        signs[j] = data->signs[j][rayBegin];
      for(int i=rayBegin+1; i < rayEnd; i++)
        for(int j = 0; j < 3; j++)
          if(data->signs[j][i] != signs[j])
            return;
      flags |= ConstantSigns;
    }
    VectorT<int, 3> getSigns(int which) const
    {
      return VectorT<int, 3>(data->signs[0][which], data->signs[1][which], data->signs[2][which]);
    }
    int getSign(int which, int sign) const
    {
      return data->signs[sign][which];
    }

    // Return a bit mask representing the sign directions of the ray.
    int getSignMask(int which) const {
      return
        (data->signs[0][which]) |
        (data->signs[1][which] << 1) |
        (data->signs[2][which] << 2);
    }

    // Hit information
    void resetHits() {
#ifdef MANTA_SSE
      int b = (rayBegin + 3) & (~3);
      int e = rayEnd & (~3);
      if(b >= e){
        for(int i = rayBegin; i < rayEnd; i++){
          data->hitMatl[i] = 0;
          data->minT[i] = MAXT;
        }
      } else {
        int i = rayBegin;
        for(;i<b;i++){
          data->hitMatl[i] = 0;
          data->minT[i] = MAXT;
        }
        for(;i<e;i+=4){
#ifdef __x86_64
          _mm_store_ps((float*)&data->hitMatl[i], _mm_setzero_ps());
          _mm_store_ps((float*)&data->hitMatl[i+2], _mm_setzero_ps());
#else
          _mm_store_ps((float*)&data->hitMatl[i], _mm_setzero_ps());
#endif
          _mm_store_ps(&data->minT[i], _mm_set1_ps(MAXT));
        }
        for(;i<rayEnd;i++){
          data->hitMatl[i] = 0;
          data->minT[i] = MAXT;
        }
      }
#else
      for(int i = rayBegin; i < rayEnd; i++){
        data->hitMatl[i] = 0;
        data->minT[i] = MAXT;
      }
#endif
    }
    void resetHits(Real maxt) {
#ifdef MANTA_SSE
      int b = (rayBegin + 3) & (~3);
      int e = rayEnd & (~3);
      if(b >= e){
        for(int i = rayBegin; i < rayEnd; i++){
          data->hitMatl[i] = 0;
          data->minT[i] = maxt;
        }
      } else {
        int i = rayBegin;
        for(;i<b;i++){
          data->hitMatl[i] = 0;
          data->minT[i] = maxt;
        }
        for(;i<e;i+=4){
#ifdef __x86_64
          _mm_store_ps((float*)&data->hitMatl[i], _mm_setzero_ps());
          _mm_store_ps((float*)&data->hitMatl[i+2], _mm_setzero_ps());
#else
          _mm_store_ps((float*)&data->hitMatl[i], _mm_setzero_ps());
#endif
          _mm_store_ps(&data->minT[i], _mm_set1_ps(maxt));
        }
        for(;i<rayEnd;i++){
          data->hitMatl[i] = 0;
          data->minT[i] = maxt;
        }
      }
#else
      for(int i = rayBegin; i < rayEnd; i++){
        data->hitMatl[i] = 0;
        data->minT[i] = maxt;
      }
#endif
    }

    void resetHit(int which) {
      data->hitMatl[which] = 0;
      data->minT[which] = MAXT;
    }
    void resetHit(int which, Real maxt) {
      data->hitMatl[which] = 0;
      data->minT[which] = maxt;
    }
    void maskRay(int which) {
#ifdef __x86_64
      data->hitMatl[which] = (Material*)0xffffffffffffffff;
#else
      data->hitMatl[which] = (Material*)0xffffffff;
#endif
      data->minT[which] = -MAXT;
    }
    inline bool rayIsMasked(int which) const {
#ifdef __x86_64
      return data->hitMatl[which] == (Material*)0xffffffffffffffff;
#else
      return data->hitMatl[which] == (Material*)0xffffffff;
#endif
    }
    // TODO(boulos): Make this efficient
    inline bool groupIsMasked(int which) const {
      for (int i = 0; i < 4; i++) {
        if (!rayIsMasked(which + i)) return false;
      }
      return true;
    }
    Real &getMinT(int which) const
    {
      return data->minT[which];
    }
    void scaleMinT(int which, Real scale)
    {
      data->minT[which] *= scale;
    }
    void overrideMinT(int which, Real minT)
    {
      data->minT[which] = minT;
    }

    // Time for motion blur.
    Real& getTime(int which) const {
      return data->time[which];
    }
    void setTime(int which, Real new_time) {
      data->time[which] = new_time;
    }

    bool hit(int which, Real t, const Material* matl, const Primitive* prim,
             const TexCoordMapper* tex) {
      if(t > T_EPSILON && t < data->minT[which]){
        data->minT[which] = t;
        data->hitMatl[which] = matl;
        data->hitPrim[which] = prim;
        data->hitTex[which] = tex;
        return true;
      } else {
        return false;
      }
    }
#ifdef MANTA_SSE
  private:
    inline __m128 hitFinal(int start, __m128 mask, __m128 t,
                           const Material* matl, const Primitive* prim,
                           const TexCoordMapper* tex) {
      mask = _mm_and_ps(mask, _mm_cmplt_ps(t, _mm_load_ps(&data->minT[start])));
      int hit = _mm_movemask_ps(mask);
      if(hit == 15){
        // All of them hit, so we can store them all
        _mm_store_ps(&data->minT[start], t);
#ifdef __x86_64
        // hitMatl
        _mm_store_si128((__m128i*)&data->hitMatl[start], _mm_set1_epi64x((long long)matl));
        _mm_store_si128((__m128i*)&data->hitMatl[start+2], _mm_set1_epi64x((long long)matl));
        // hitPrim
        _mm_store_si128((__m128i*)&data->hitPrim[start], _mm_set1_epi64x((long long)prim));
        _mm_store_si128((__m128i*)&data->hitPrim[start+2], _mm_set1_epi64x((long long)prim));
        // hitTex
        _mm_store_si128((__m128i*)&data->hitTex[start], _mm_set1_epi64x((long long)tex));
        _mm_store_si128((__m128i*)&data->hitTex[start+2], _mm_set1_epi64x((long long)tex));
#else // __x86_64
        _mm_store_si128((__m128i*)&data->hitMatl[start], _mm_set1_epi32((int)matl));
        _mm_store_si128((__m128i*)&data->hitPrim[start], _mm_set1_epi32((int)prim));
        _mm_store_si128((__m128i*)&data->hitTex[start], _mm_set1_epi32((int)tex));
#endif // __x86_64
      } else if(hit != 0){
        // Some of them hit, some of them do not
        // Intel has a masked move instruction but it seems to be really slow
        // It is faster to load the old data and mask it in manually
        _mm_store_ps(&data->minT[start],
                     _mm_or_ps(_mm_and_ps(mask, t),
                               _mm_andnot_ps(mask, _mm_load_ps(&data->minT[start]))));
#ifdef __x86_64
        __m128i lomask = _mm_castps_si128(_mm_unpacklo_ps(mask, mask));
        __m128i himask = _mm_castps_si128(_mm_unpackhi_ps(mask, mask));
        // hitMatl
        _mm_store_si128((__m128i*)&data->hitMatl[start],
                        _mm_or_si128(_mm_and_si128(lomask, _mm_set1_epi64x((long long)matl)),
                                     _mm_andnot_si128(lomask, _mm_load_si128((__m128i*)&data->hitMatl[start]))));
        _mm_store_si128((__m128i*)&data->hitMatl[start+2],
                        _mm_or_si128(_mm_and_si128(himask, _mm_set1_epi64x((long long)matl)),
                                     _mm_andnot_si128(himask, _mm_load_si128((__m128i*)&data->hitMatl[start+2]))));
        // hitPrim
        _mm_store_si128((__m128i*)&data->hitPrim[start],
                        _mm_or_si128(_mm_and_si128(lomask, _mm_set1_epi64x((long long)prim)),
                                     _mm_andnot_si128(lomask, _mm_load_si128((__m128i*)&data->hitPrim[start]))));
        _mm_store_si128((__m128i*)&data->hitPrim[start+2],
                        _mm_or_si128(_mm_and_si128(himask, _mm_set1_epi64x((long long)prim)),
                                     _mm_andnot_si128(himask, _mm_load_si128((__m128i*)&data->hitPrim[start+2]))));
        // hitTex
        _mm_store_si128((__m128i*)&data->hitTex[start],
                        _mm_or_si128(_mm_and_si128(lomask, _mm_set1_epi64x((long long)tex)),
                                     _mm_andnot_si128(lomask, _mm_load_si128((__m128i*)&data->hitTex[start]))));
        _mm_store_si128((__m128i*)&data->hitTex[start+2],
                        _mm_or_si128(_mm_and_si128(himask, _mm_set1_epi64x((long long)tex)),
                                     _mm_andnot_si128(himask, _mm_load_si128((__m128i*)&data->hitTex[start+2]))));
#else // __x86_64
        __m128i imask = _mm_castps_si128(mask);
        _mm_store_si128((__m128i*)&data->hitMatl[start],
                        _mm_or_si128(_mm_and_si128(imask, _mm_set1_epi32((int)matl)),
                                     _mm_andnot_si128(imask, _mm_load_si128((__m128i*)&data->hitMatl[start]))));
        _mm_store_si128((__m128i*)&data->hitPrim[start],
                        _mm_or_si128(_mm_and_si128(imask, _mm_set1_epi32((int)prim)),
                                     _mm_andnot_si128(imask, _mm_load_si128((__m128i*)&data->hitPrim[start]))));
        _mm_store_si128((__m128i*)&data->hitTex[start],
                        _mm_or_si128(_mm_and_si128(imask, _mm_set1_epi32((int)tex)),
                                     _mm_andnot_si128(imask, _mm_load_si128((__m128i*)&data->hitTex[start]))));
#endif // __x86_64
      }
      return mask;
    }
  public:
    //
    // These are hit functions for SSE.  There are four versions: the
    // first three have a mask for prior selections (one with a
    // complemented mask).  The fourth has no mask.  Both call the
    // above private function to do the bulk of the work.
    //

    // Mask is all ones for rays that actually hit, all zeros otherwise
    inline __m128 hitWithMask(int start, __m128 mask, __m128 t,
                              const Material* matl, const Primitive* prim,
                              const TexCoordMapper* tex) {
      mask = _mm_and_ps(mask, _mm_cmpgt_ps(t, _mm_set1_ps(T_EPSILON)));
      return hitFinal(start, mask, t, matl, prim, tex);
    }
    // Mask is all zeros for days that actually hit, all ones otherwise
    inline __m128 hitWithComplementedMask(int start, __m128 mask, __m128 t,
                                          const Material* matl,
                                          const Primitive* prim,
                                          const TexCoordMapper* tex) {
      mask = _mm_andnot_ps(mask, _mm_cmpgt_ps(t, _mm_set1_ps(T_EPSILON)));
      return hitFinal(start, mask, t, matl, prim, tex);
    }
    // Mask is all ones for rays that actually hit, all zeros
    // otherwise. It is assumed that mask already contains the results
    // of checking that t > epsilon.
    inline __m128 hitWithoutTminCheck(int start, __m128 mask, __m128 t,
                                      const Material* matl,
                                      const Primitive* prim,
                                      const TexCoordMapper* tex) {
      return hitFinal(start, mask, t, matl, prim, tex);
    }
    // No mask
    inline __m128 hit(int start, __m128 t, const Material* matl, const Primitive* prim, const TexCoordMapper* tex) {
      __m128 mask = _mm_cmpgt_ps(t, _mm_set1_ps(T_EPSILON));
      return hitFinal(start, mask, t, matl, prim, tex);
    }

#endif // #ifdef MANTA_SSE

    bool wasHit(int which) const
    {
      return data->hitMatl[which] != 0;
    }

#ifdef MANTA_SSE
    ///////////////////////////////////////////////////////////
    //
    // Returns a SSE based mask for the rays that have hitMatl == 0
    //
    // We are using SSE integer instructions to avoid weird problems
    // when casting pointers to floats and doing comparisons.
    // Unfortunately, there aren't any != (cmpneq) integer
    // instructions.  This is why the implementation is for
    // wereNotHitSSE, and wereHitSSE takes the inverse of
    // wereNotHitSSE.
    //
    // 32 bit: mask = (hitMatl == 0)
    //
    // 64 bit: mask = (hitMatl[low_bits] == 0) && (hitMatl[high_bits] == 0)
    inline __m128 wereNotHitSSE(int start) const
    {
#ifdef __x86_64
      __m128 masklo = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_load_si128((__m128i*)&data->hitMatl[start]),
                                                       _mm_setzero_si128()));
      __m128 maskhi = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_load_si128((__m128i*)&data->hitMatl[start+2]),
                                                       _mm_setzero_si128()));
      return  _mm_and_ps(_mm_shuffle_ps(masklo, maskhi, _MM_SHUFFLE(2, 0, 2, 0)),
                         _mm_shuffle_ps(masklo, maskhi, _MM_SHUFFLE(3, 1, 3, 1)));
#else // __x86_64
      return  _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_load_si128((__m128i*)&data->hitMatl[start]),
                                               _mm_setzero_si128()));

#endif // __x86_64
    }
    ///////////////////////////////////////////////////////////
    //
    // Returns a SSE based mask for the rays that have hitMatl != 0
    //
    inline __m128 wereHitSSE(int start) const {
      return _mm_cmpeq_ps(wereNotHitSSE(start), _mm_setzero_ps());
    }
#endif // #ifdef MANTA_SSE

    void setHitMaterial(int which, const Material* matl)
    {
      data->hitMatl[which] = matl;
    }
    void setHitPrimitive(int which, const Primitive* prim)
    {
      data->hitPrim[which] = prim;
    }
    void setHitTexCoordMapper(int which, const TexCoordMapper* tex)
    {
      data->hitTex[which] = tex;
    }
    const Material* getHitMaterial(int which) const
    {
      return data->hitMatl[which];
    }
    const Primitive* getHitPrimitive(int which) const
    {
      return data->hitPrim[which];
    }
    const TexCoordMapper* getHitTexCoordMapper(int which) const
    {
      return data->hitTex[which];
    }

    // Final result
    void setColor(int which, const Color& color)
    {
      for(int i=0;i<Color::NumComponents;i++)
        data->color[i][which] = color[i];
    }
    Color getColor(int which) const
    {
      Color result;
      for(int i=0;i<Color::NumComponents;i++)
        result[i] = data->color[i][which];
      return result;
    }

    // Attenuation for ray tree pruning
    void setImportance(int which, const Color& importance)
    {
      for(int i=0;i<Color::NumComponents;i++)
        data->importance[i][which] = importance[i];
    }
    Color getImportance(int which) const
    {
      Color result;
      for(int i=0;i<Color::NumComponents;i++)
        result[i] = data->importance[i][which];
      return result;
    }
    void initializeImportance()
    {
      for(int j=0;j<Color::NumComponents;j++)
        for(int i=rayBegin;i<rayEnd;i++)
          data->importance[j][i] = 1;
    }
    // Ignore Emitted Light
    int getIgnoreEmittedLight(int which) const {
      return data->ignoreEmittedLight[which];
    }
    void setIgnoreEmittedLight(int which, int new_ignore) {
      data->ignoreEmittedLight[which] = new_ignore;
    }


    // Texture Coordinates
    void setTexCoords(int which, const Vector& tc)
    {
      for(int i=0;i<3;i++)
        data->texCoords[i][which] = tc[i];
    }
    void setTexCoords(int which, const VectorT<Real, 2>& tc)
    {
      for(int i=0;i<2;i++)
        data->texCoords[i][which] = tc[i];
    }
    Vector getTexCoords(int which) const
    {
      return Vector(data->texCoords[0][which], data->texCoords[1][which], data->texCoords[2][which]);
    }
    VectorT<Real, 2> getTexCoords2(int which) const
    {
      return VectorT<Real, 2>(data->texCoords[0][which], data->texCoords[1][which]);
    }

    Real getTexCoords2(int which, int dim)
    {
      return data->texCoords[dim][which];
    }

    void computeTextureCoordinates2(const RenderContext& context)
    {
      if(flags & (HaveTexture2|HaveTexture3))
        return;
      actualComputeTextureCoordinates2(context);
    }
    void computeTextureCoordinates3(const RenderContext& context)
    {
      if(flags & HaveTexture3)
        return;
      actualComputeTextureCoordinates3(context);
    }


    // Normals
    void setNormal(int which, const Vector& normal)
    {
      for(int i=0;i<3;i++)
        data->normal[i][which] = normal[i];
    }
    void setFFNormal(int which, const Vector& normal)
    {
      for(int i=0;i<3;i++)
        data->ffnormal[i][which] = normal[i];
    }
    void setSurfaceDerivativeU(int which, const Vector& u) {
      for(int i=0;i<3;i++)
        data->dPdu[i][which] = u[i];
    }
    void setSurfaceDerivativeV(int which, const Vector& v) {
      for(int i=0;i<3;i++)
        data->dPdv[i][which] = v[i];
    }
    void setGeometricNormal(int which, const Vector& gnormal)
    {
      for(int i=0;i<3;i++)
        data->geometricNormal[i][which] = gnormal[i];
    }
    void setFFGeometricNormal(int which, const Vector& normal)
    {
      for(int i=0;i<3;++i)
        data->ffgeometricNormal[i][which] = normal[i];
    }
    Vector getNormal(int which) const
    {
      return Vector(data->normal[0][which], data->normal[1][which], data->normal[2][which]);
    }
    Vector getFFNormal(int which) const
    {
      return Vector(data->ffnormal[0][which], data->ffnormal[1][which], data->ffnormal[2][which]);
    }
    Vector getGeometricNormal(int which) const
    {
      return Vector(data->geometricNormal[0][which],
                    data->geometricNormal[1][which],
                    data->geometricNormal[2][which]);
    }
    Vector getFFGeometricNormal(int which) const
    {
      return Vector(data->ffgeometricNormal[0][which],
                    data->ffgeometricNormal[1][which],
                    data->ffgeometricNormal[2][which]);
    }
    Vector getSurfaceDerivativeU(int which) const {
      return Vector(data->dPdu[0][which], data->dPdu[1][which], data->dPdu[2][which]);
    }
    Vector getSurfaceDerivativeV(int which) const {
      return Vector(data->dPdv[0][which], data->dPdv[1][which], data->dPdv[2][which]);
    }

    template<bool allHit>
    void computeNormals(const RenderContext& context);

    // Compute the forward facing normals.  These are normals that all
    // point towards the ray origin.
    template<bool allHit>
    void computeFFNormals(const RenderContext& context);

    template<bool allHit>
    void computeGeometricNormals(const RenderContext& context);

    template<bool allHit>
    void computeFFGeometricNormals(const RenderContext& context);

    // Compute the surface derivatives
    void computeSurfaceDerivatives(const RenderContext& context) {
      if(flags & HaveSurfaceDerivatives)
        return;

      actualComputeSurfaceDerivatives(context);
    }

    // Hit positions
    Vector getHitPosition(int which) const
    {
      return Vector(data->hitPosition[0][which], data->hitPosition[1][which], data->hitPosition[2][which]);
    }
    void setHitPosition(int which, const Vector& hit) const
    {
      for(int i=0;i<3;i++)
        data->hitPosition[i][which] = hit[i];
    }
    void computeHitPositions()
    {
      if(flags & HaveHitPositions)
        return;

      actualComputeHitPositions();
    }

    // This is for the non-vertical scratchpad
    template<class T> T& scratchpad(int which) {
      MANTA_STATIC_CHECK( sizeof( T ) <= RayPacketData::MaxScratchpadSize,
                          Scratchpad_size_isnt_big_enough_for_conversion);
      return *(T*)data->scratchpad_data[which];
    }
    // This will copy the contects of the scratchpad from the incoming
    // RayPacket.
    void copyScratchpad(int which, const RayPacket& copy, int which_copy) {
      memcpy( data->scratchpad_data[which],
              copy.data->scratchpad_data[which_copy],
              RayPacketData::MaxScratchpadSize);
    }
    // This is for the vertical scratchpad
    template<class T> T* getScratchpad(int idx) {
      MANTA_STATIC_CHECK ( sizeof(T) == 4 || sizeof(T) == 8,
                           size_of_type_isnt_4_or_8);
      ASSERT(idx >= 0);
      if(sizeof(T) == 4){
        ASSERT(idx < RayPacketData::MaxScratchpad4);
        return (T*)data->scratchpad4[idx];
      } else {
        ASSERT(idx < RayPacketData::MaxScratchpad8);
        return (T*)data->scratchpad8[idx];
      }
    }

  private:
    void actualNormalizeDirections();
    void actualComputeInverseDirections();
    void actualComputeHitPositions();
    void actualComputeSurfaceDerivatives(const RenderContext& context);
    void actualComputeTextureCoordinates2(const RenderContext& context);
    void actualComputeTextureCoordinates3(const RenderContext& context);

    // Prevent accidental copying of RayPackets
    RayPacket(const RayPacket&);
    RayPacket& operator=(const RayPacket&);

  public:
    RayPacketData* data;
    PacketShape shape;
    int rayBegin;
    int rayEnd;
    int depth;
    int flags;
  };

  typedef MANTA_ALIGN(16) Color::ComponentType ColorArray[Color::NumComponents][RayPacket::MaxSize];

  std::ostream& operator<< (std::ostream& os, const RayPacket& rays);
} // end namespace Manta

#endif
