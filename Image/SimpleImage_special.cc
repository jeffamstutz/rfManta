#include <Core/Exceptions/InternalError.h>
#include <Core/Math/ExponSSE.h>
#include <Core/Math/SSEDefs.h>
#include <Image/SimpleImage.h>
#include <iostream>
#include <MantaSSE.h>

using namespace Manta;
using namespace std;

#define MANTA_USE_SRGB 0

#if MANTA_USE_SRGB
inline __m128 RGBtoSRGB(__m128 val) {
#if 1
  // NOTE(boulos): This conversion follows that given in the OpenGL
  // sRGB texture spec (question 14):
  // http://opengl.org/registry/specs/EXT/texture_sRGB.txt and that of
  // the framebuffer spec:
  // http://www.opengl.org/registry/specs/EXT/framebuffer_sRGB.txt
  __m128 use_gamma = _mm_cmplt_ps(_mm_set1_ps(.0031308f), val);
  __m128 linear_ver = _mm_mul_ps(val, _mm_set1_ps(12.92f));
  // NOTE(boulos): PowSSE doesn't seem to cut it for this exponent..,
  // we might want to do a lookup table or a separate SSE pow...
  __m128 gamma_ver  = _mm_sub_ps(_mm_mul_ps(_mm_set1_ps(1.055f), PowSSEMathH(val, _mm_set1_ps(0.41666f), use_gamma)), _mm_set1_ps(.055f));
  return mask4(use_gamma, gamma_ver, linear_ver);
#else
  return PowSSEMathH(val, _mm_set1_ps(1.f/2.2f));
#endif
}
#endif

template<>
void SimpleImage<ARGB8Pixel>::set(const Fragment& fragment)
{
  if(fragment.xPixelSize != 1 || fragment.yPixelSize != 1){
    splat(fragment);
  } else if(fragment.getFlag(Fragment::ConsecutiveX|Fragment::ConstantEye)){
    int b = fragment.begin();
    ARGB8Pixel* pix = eyeStart[fragment.getWhichEye(b)][fragment.getY(b)]+fragment.getX(b);
#if MANTA_SSE
    // If the fragment and the pixel are on an aligned boundary, use SSE
    if(((fragment.pixelBegin | fragment.pixelEnd) & 0x3) == 0){
      // Aligned for SSE
      __m128 scale = _mm_set1_ps( 255.99999f );
      int f = fragment.end()-3;
      for(int i=fragment.begin(); i< f;i+=4){
        __m128 r = _mm_load_ps(&fragment.color[0][i]);
        __m128 g = _mm_load_ps(&fragment.color[1][i]);
        __m128 b = _mm_load_ps(&fragment.color[2][i]);
#if MANTA_USE_SRGB
        r = RGBtoSRGB(r);
        g = RGBtoSRGB(g);
        b = RGBtoSRGB(b);
#endif
        r = _mm_mul_ps(r, scale);
        g = _mm_mul_ps(g, scale);
        b = _mm_mul_ps(b, scale);

        __m128i alpha = _mm_set1_epi32(255);  // alpha
        __m128i r32 = _mm_cvttps_epi32(r); // 32 bits: r0r1r2r3
        __m128i g32 = _mm_cvttps_epi32(g); // 32 bits: g0g1g2g3
        __m128i b32 = _mm_cvttps_epi32(b); // 32 bits: b0b1b2b3
        __m128i ag16 = _mm_packs_epi32(alpha, g32); // 16 bits: a0a1a2a3g0g1g2g3
        __m128i rb16 = _mm_packs_epi32(r32, b32); // 16 bits: r0r1r2r3b0b1b2b3
        __m128i ar16 = _mm_unpacklo_epi16(ag16, rb16); // 16 bits: a0r0a1r1a2r2a3r3
        __m128i gb16 = _mm_unpackhi_epi16(ag16, rb16); // 16 bits: g0b0g1b1g2b0g3b3
        __m128i argb16a = _mm_unpacklo_epi32(ar16, gb16); // 16 bits a0r0g0b0a1a1g1b1
        __m128i argb16b = _mm_unpackhi_epi32(ar16, gb16); // 16 bits a2r2g2b2a3r3g3b3
        __m128i result = _mm_packus_epi16(argb16a, argb16b); // 8 bits: a0r0g0b0...a3r3g3b3
        _mm_stream_si128((__m128i*)pix, result);
        pix += 4;
      }
    } else
#endif /* MANTA_SSE */
      {
        for(int i=fragment.begin(); i< fragment.end();i++)
          convertToPixel(*pix++, fragment.getColor(i).convertRGB());
      }
  } else if (fragment.getFlag(Fragment::ConstantEye) &&
             fragment.shape == Fragment::SquareShape) {
#ifdef MANTA_SSE
    int numPixels = fragment.end() - fragment.begin();
    // NOTE(boulos): For SSE we need at least a 4x4 fragment, if we
    // don't have that, skip out. We also need the same alignment
    // rules from above. Note that pixelEnd is 1 after the last value,
    // so it needs to be SIMD width aligned as well.
    if (numPixels >= 16 &&
        ((fragment.pixelBegin | fragment.pixelEnd) & 0x3) == 0) {
      // NOTE(boulos): Only power of 2 numPixels should pass
      bool isPowerOf2 = !(numPixels & (numPixels -1));
      if (!isPowerOf2) throw InternalError("Not power of 2..");
      int sqrtSize;
      switch (numPixels) {
      case 16:
        sqrtSize = 4;
        break;
      case 64:
        sqrtSize = 8;
        break;
      case 144:
        sqrtSize = 12;
        break;
      case 256:
        sqrtSize = 16;
        break;
      default:
        sqrtSize = static_cast<int>(Sqrt(static_cast<Real>(numPixels)));
        if (sqrtSize * sqrtSize != numPixels) throw InternalError("Not a perfect square");
        break;
      }
      int i = fragment.begin();
      int eye = fragment.getWhichEye(i);
      __m128 scale = _mm_set1_ps( 255.99999f );
      for (int y = 0; y < sqrtSize; y++) {
        ARGB8Pixel* pix = eyeStart[eye][fragment.getY(i)]+fragment.getX(i);
        for (int x = 0; x < sqrtSize; x+=4, i+=4) {
          __m128 r = _mm_load_ps(&fragment.color[0][i]);
          __m128 g = _mm_load_ps(&fragment.color[1][i]);
          __m128 b = _mm_load_ps(&fragment.color[2][i]);
#if MANTA_USE_SRGB
          r = RGBtoSRGB(r);
          g = RGBtoSRGB(g);
          b = RGBtoSRGB(b);
#endif
          r = _mm_mul_ps(r, scale);
          g = _mm_mul_ps(g, scale);
          b = _mm_mul_ps(b, scale);

          __m128i alpha = _mm_set1_epi32(255);  // alpha
          __m128i r32 = _mm_cvttps_epi32(r); // 32 bits: r0r1r2r3
          __m128i g32 = _mm_cvttps_epi32(g); // 32 bits: g0g1g2g3
          __m128i b32 = _mm_cvttps_epi32(b); // 32 bits: b0b1b2b3
          __m128i ag16 = _mm_packs_epi32(alpha, g32); // 16 bits: a0a1a2a3g0g1g2g3
          __m128i rb16 = _mm_packs_epi32(r32, b32); // 16 bits: r0r1r2r3b0b1b2b3
          __m128i ar16 = _mm_unpacklo_epi16(ag16, rb16); // 16 bits: a0r0a1r1a2r2a3r3
          __m128i gb16 = _mm_unpackhi_epi16(ag16, rb16); // 16 bits: g0b0g1b1g2b0g3b3
          __m128i argb16a = _mm_unpacklo_epi32(ar16, gb16); // 16 bits a0r0g0b0a1a1g1b1
          __m128i argb16b = _mm_unpackhi_epi32(ar16, gb16); // 16 bits a2r2g2b2a3r3g3b3
          __m128i result = _mm_packus_epi16(argb16a, argb16b); // 8 bits: a0r0g0b0...a3r3g3b3
          _mm_stream_si128((__m128i*)pix, result);
          pix += 4;
        }
      }
    } else
#endif
      {
        int eye = fragment.getWhichEye(fragment.begin());
        for(int i=fragment.begin(); i< fragment.end();i++) {
          ARGB8Pixel* pix = eyeStart[eye][fragment.getY(i)]+fragment.getX(i);
          convertToPixel(*pix, fragment.getColor(i).convertRGB());
        }
      }
  } else {
    for(int i=fragment.begin();i<fragment.end();i++){
      convertToPixel(eyeStart[fragment.getWhichEye(i)][fragment.getY(i)][fragment.getX(i)], fragment.getColor(i).convertRGB());
    }
  }
}

template<>
void SimpleImage<BGRA8Pixel>::set(const Fragment& fragment)
{
  if(fragment.xPixelSize != 1 || fragment.yPixelSize != 1){
    splat(fragment);
  } else if(fragment.getFlag(Fragment::ConsecutiveX|Fragment::ConstantEye)){
    int b = fragment.begin();
    BGRA8Pixel* pix = eyeStart[fragment.getWhichEye(b)][fragment.getY(b)]+fragment.getX(b);
#if MANTA_SSE
    // If the fragment and the pixel are on an aligned boundary, use SSE
    if(((fragment.pixelBegin | fragment.pixelEnd) & 0x3) == 0){
      // Aligned for SSE
      __m128 scale = _mm_set1_ps( 255.99999f );
      int f = fragment.end()-3;
      for(int i=fragment.begin(); i< f;i+=4){
        __m128 r = _mm_load_ps(&fragment.color[0][i]);
        __m128 g = _mm_load_ps(&fragment.color[1][i]);
        __m128 b = _mm_load_ps(&fragment.color[2][i]);
        r = _mm_mul_ps(r, scale);
        g = _mm_mul_ps(g, scale);
        b = _mm_mul_ps(b, scale);
        __m128i alpha = _mm_set1_epi32(255);  // alpha a0a1a2a3
        __m128i r32 = _mm_cvttps_epi32(r); // 32 bits: r0r1r2r3
        __m128i g32 = _mm_cvttps_epi32(g); // 32 bits: g0g1g2g3
        __m128i b32 = _mm_cvttps_epi32(b); // 32 bits: b0b1b2b3

        // 16 bits: b0b1b2b3r0r1r2r3
        __m128i br16 = _mm_packs_epi32(b32, r32);

        // 16 bits: g0g1g2g3a0a1a2a3
        __m128i ga16 = _mm_packs_epi32(g32, alpha);

        // 16 bits: b0g0b1g1b2g2b3g3
        __m128i bg16 = _mm_unpacklo_epi16(br16, ga16);

        // 16 bits: r0a0r1a1r2a2r3a3
        __m128i ra16 = _mm_unpackhi_epi16(br16, ga16);

        // 16 bits: b0g0r0a0b1g1r1a1
        __m128i bgra16a = _mm_unpacklo_epi32(bg16, ra16);

        // 16 bits: b2g2r2a2b3g3r3a3
        __m128i bgra16b = _mm_unpackhi_epi32(bg16, ra16);

        // 32 bits: b0g0r0a0 ... b3g3r3a3
        __m128i result = _mm_packus_epi16(bgra16a, bgra16b);

        // Copy data over
        _mm_stream_si128((__m128i*)pix, result);

        pix += 4;
      }
    } else
#endif /* MANTA_SSE */
      {
        for(int i=fragment.begin(); i< fragment.end();i++)
          convertToPixel(*pix++, fragment.getColor(i).convertRGB());
      }
  } else {
    for(int i=fragment.begin();i<fragment.end();i++){
      convertToPixel(eyeStart[fragment.getWhichEye(i)][fragment.getY(i)][fragment.getX(i)], fragment.getColor(i).convertRGB());
    }
  }
}

template<>
void SimpleImage<RGBA8Pixel>::set(const Fragment& fragment)
{
  if(fragment.xPixelSize != 1 || fragment.yPixelSize != 1){
    splat(fragment);
  } else if(fragment.getFlag(Fragment::ConsecutiveX|Fragment::ConstantEye)) {
    int b = fragment.begin();
    RGBA8Pixel* pix = eyeStart[fragment.getWhichEye(b)][fragment.getY(b)]+fragment.getX(b);
#if MANTA_SSE
    // If the fragment and the pixel are on an aligned boundary, use SSE
    if(((fragment.pixelBegin | fragment.pixelEnd) & 0x3) == 0){
      // Aligned for SSE
      __m128 scale = _mm_set1_ps( 255.99999f );
      int f = fragment.end()-3;
      for(int i=fragment.begin(); i< f;i+=4){
        __m128 r = _mm_load_ps(&fragment.color[0][i]);
        __m128 g = _mm_load_ps(&fragment.color[1][i]);
        __m128 b = _mm_load_ps(&fragment.color[2][i]);
        r = _mm_mul_ps(r, scale);
        g = _mm_mul_ps(g, scale);
        b = _mm_mul_ps(b, scale);
        __m128i alpha = _mm_set1_epi32(255);  // alpha a0a1a2a3
        __m128i r32 = _mm_cvttps_epi32(r); // 32 bits: r0r1r2r3
        __m128i g32 = _mm_cvttps_epi32(g); // 32 bits: g0g1g2g3
        __m128i b32 = _mm_cvttps_epi32(b); // 32 bits: b0b1b2b3

        // 16 bits: r0r1r2r3b0b1b2b3
        __m128i rb16 = _mm_packs_epi32(r32, b32);

        // 16 bits: g0g1g2g3a0a1a2a3
        __m128i ga16 = _mm_packs_epi32(g32, alpha);

        // 16 bits: r0g0r1g1r2g2r3g3
        __m128i rg16 = _mm_unpacklo_epi16(rb16, ga16);

        // 16 bits: b0a0b1a1b2a2b3a3
        __m128i ba16 = _mm_unpackhi_epi16(rb16, ga16);

        // 16 bits: r0g0b0a0r1g1b1a1
        __m128i rgba16a = _mm_unpacklo_epi32(rg16, ba16);

        // 16 bits: r2g2b2a2r3g3b3a3
        __m128i rgba16b = _mm_unpackhi_epi32(rg16, ba16);

        // 32 bits: r0g0b0a0 ... r3g3b3a3
        __m128i result = _mm_packus_epi16(rgba16a, rgba16b);

        // Copy data over
        _mm_stream_si128((__m128i*)pix, result);

        pix += 4;
      }
    } else
#endif /* MANTA_SSE */
      {
        for(int i=fragment.begin(); i< fragment.end();i++)
          convertToPixel(*pix++, fragment.getColor(i).convertRGB());
      }
  } else {
    for(int i=fragment.begin();i<fragment.end();i++){
      convertToPixel(eyeStart[fragment.getWhichEye(i)][fragment.getY(i)][fragment.getX(i)], fragment.getColor(i).convertRGB());
    }
  }
}

template<>
void SimpleImage<RGBAfloatPixel>::set(const Fragment& fragment)
{
  if(fragment.xPixelSize != 1 || fragment.yPixelSize != 1){
    splat(fragment);
  } else if(fragment.getFlag(Fragment::ConsecutiveX|Fragment::ConstantEye)){
    int b = fragment.begin();
    RGBAfloatPixel* pix = eyeStart[fragment.getWhichEye(b)][fragment.getY(b)]+fragment.getX(b);
#if MANTA_SSE
    float* fpix = reinterpret_cast<float*>(pix);
    // If the fragment and the pixel are on an aligned boundary, use SSE
    if(((fragment.pixelBegin | fragment.pixelEnd) & 0x3) == 0){
      // Aligned for SSE
      int f = fragment.end()-3;
      for(int i=fragment.begin(); i< f;i+=4){
        __m128 r = _mm_load_ps(&fragment.color[0][i]);
        __m128 g = _mm_load_ps(&fragment.color[1][i]);
        __m128 b = _mm_load_ps(&fragment.color[2][i]);
        __m128 a = _mm_set_ps1(1.0f);  // alpha a0a1a2a3
        // This will do the following
        // r = r0r1r2r3       r = r0g0b0a0
        // g = g0g1g2g3   =>  g = r1g1b1a1
        // b = b0b1b2b3       b = r2g2b2a2
        // a = a0a1a2a3       a = r3g3b3a3
        //
        // I found this method to be slightly faster and easier to use
        // than the swizzle4 code previously found in the repository.
        _MM_TRANSPOSE4_PS(r, g, b, a);

        // Copy data over
        _mm_stream_ps(fpix,    r);
        _mm_stream_ps(fpix+4,  g);
        _mm_stream_ps(fpix+8,  b);
        _mm_stream_ps(fpix+12, a);

        fpix += 16;
      }
    } else
#endif /* MANTA_SSE */
      {
        for(int i=fragment.begin(); i< fragment.end();i++)
          convertToPixel(*pix++, fragment.getColor(i).convertRGB());
      }
  } else {
    for(int i=fragment.begin();i<fragment.end();i++){
      convertToPixel(eyeStart[fragment.getWhichEye(i)][fragment.getY(i)][fragment.getX(i)], fragment.getColor(i).convertRGB());
    }
  }
}

template<>
void SimpleImage<RGBZfloatPixel>::splat(const Fragment& fragment)
{
    // NOTE(boulos): This branch tries to copy fragments where a
    // single sample splats onto several pixels
    for (int i = fragment.begin(); i < fragment.end(); i++) {
	RGBZfloatPixel pix;
	convertToPixel(pix, fragment.getColor(i).convertRGB(), fragment.getDepth(i));

	// start splatting
	int y_index = fragment.getY(i);
	int last_x = std::min(xres - fragment.getX(i), fragment.xPixelSize);
	for (int y = 0; y < fragment.yPixelSize; y++) {
	    if (y_index >= yres)
		break;

	    //RGBZfloatPixel* row = eyeStart[fragment.getWhichEye(i)][y_index];
	    for (int x = 0; x < last_x; x++)
		y_index++;
	}
    }
}

template<>
void SimpleImage<RGBZfloatPixel>::set(const Fragment& fragment)
{
    if (fragment.xPixelSize != 1 || fragment.yPixelSize != 1) {
	splat(fragment);
    } else if (fragment.getFlag(Fragment::ConsecutiveX | Fragment::ConstantEye)) {
	int b = fragment.begin();
	RGBZfloatPixel* pix = eyeStart[fragment.getWhichEye(b)][fragment.getY(b)]+ fragment.getX(b);
	for (int i = fragment.begin(); i < fragment.end(); i++)
	    convertToPixel(*pix++, fragment.getColor(i).convertRGB(), fragment.getDepth(i));
    } else {
	for (int i = fragment.begin(); i < fragment.end(); i++) {
	    convertToPixel(eyeStart[fragment.getWhichEye(i)][fragment.getY(i)][fragment.getX(i)],
			   fragment.getColor(i).convertRGB(), fragment.getDepth(i));
	}
    }
}

template<>
void SimpleImage<RGBA8ZfloatPixel>::splat(const Fragment& fragment)
{
    // NOTE(boulos): This branch tries to copy fragments where a
    // single sample splats onto several pixels
    for (int i = fragment.begin(); i < fragment.end(); i++) {
	RGBA8ZfloatPixel pix;
	convertToPixel(pix, fragment.getColor(i).convertRGB(), fragment.getDepth(i));

	// start splatting
	int y_index = fragment.getY(i);
	int last_x = std::min(xres - fragment.getX(i), fragment.xPixelSize);
	for (int y = 0; y < fragment.yPixelSize; y++) {
	    if (y_index >= yres)
		break;

	    //RGBA8ZfloatPixel* row = eyeStart[fragment.getWhichEye(i)][y_index];
	    for (int x = 0; x < last_x; x++)
		y_index++;
	}
    }
}

template<>
void SimpleImage<RGBA8ZfloatPixel>::set(const Fragment& fragment)
{
    if (fragment.xPixelSize != 1 || fragment.yPixelSize != 1) {
	splat(fragment);
    } else if (fragment.getFlag(Fragment::ConsecutiveX | Fragment::ConstantEye)) {
	int b = fragment.begin();
	RGBA8ZfloatPixel* pix = eyeStart[fragment.getWhichEye(b)][fragment.getY(b)]+ fragment.getX(b);
	for (int i = fragment.begin(); i < fragment.end(); i++)
	    convertToPixel(*pix++, fragment.getColor(i).convertRGB(), fragment.getDepth(i));
    } else {
	for (int i = fragment.begin(); i < fragment.end(); i++) {
	    convertToPixel(eyeStart[fragment.getWhichEye(i)][fragment.getY(i)][fragment.getX(i)],
			   fragment.getColor(i).convertRGB(), fragment.getDepth(i));
	}
    }
}
