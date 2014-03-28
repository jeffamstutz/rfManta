/*
  For more information, please see: http://software.sci.utah.edu

  The MIT License

  Copyright (c) 2005-2007
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

#include <MantaSSE.h>
#include <Core/Math/TrigSSE.h>

#ifdef MANTA_SSE

///////////////////////
// SSE sine and cosine, based off of code available at:
// http://gruntthepeon.free.fr/ssemath/
// with the following license:

/* Copyright (C) 2007  Julien Pommier

   This software is provided 'as-is', without any express or implied
   warranty.  In no event will the authors be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
   3. This notice may not be removed or altered from any source distribution.

   (this is the zlib license)
*/

// end license
////////////////////////////

typedef union xmm_mm_union {
  __m128 xmm;
  __m64 mm[2];
} xmm_mm_union;

#define COPY_MM_TO_XMM(mm0_, mm1_, xmm_) {                    \
    xmm_mm_union u; u.mm[0]=mm0_; u.mm[1]=mm1_; xmm_ = u.xmm; \
  }

static const MANTA_ALIGN(16) sse_t _ps_cephes_FOPI = _mm_set_ps1(1.27323954473516);
static const MANTA_ALIGN(16) sse_t _ps_minus_cephes_DP1 = _mm_set_ps1(-0.78515625);
static const MANTA_ALIGN(16) sse_t _ps_minus_cephes_DP2 = _mm_set_ps1(-2.4187564849853515625e-4);
static const MANTA_ALIGN(16) sse_t _ps_minus_cephes_DP3 = _mm_set_ps1(-3.77489497744594108e-8);
static const MANTA_ALIGN(16) sse_t _ps_sincof_p0 = _mm_set_ps1(-1.9515295891E-4);
static const MANTA_ALIGN(16) sse_t _ps_sincof_p1 = _mm_set_ps1(8.3321608736E-3);
static const MANTA_ALIGN(16) sse_t _ps_sincof_p2 = _mm_set_ps1(-1.6666654611E-1);
static const MANTA_ALIGN(16) sse_t _ps_coscof_p0 = _mm_set_ps1(2.443315711809948E-005);
static const MANTA_ALIGN(16) sse_t _ps_coscof_p1 = _mm_set_ps1(-1.388731625493765E-003);
static const MANTA_ALIGN(16) sse_t _ps_coscof_p2 = _mm_set_ps1(4.166664568298827E-002);


sse_t Manta::sin4(sse_t x) {
  typedef __m128 v4sf;
  typedef __m64 v2si;

  v4sf xmm1, xmm2 = _mm_setzero_ps(), xmm3, sign_bit, y;
  v2si mm0, mm1, mm2, mm3;
  sign_bit = x;
  /* take the absolute value */
  x = _mm_and_ps(x, _mm_absmask);
  /* extract the sign bit (upper one) */
  sign_bit = _mm_and_ps(sign_bit, _mm_signbit);

  /* scale by 4/Pi */
  y = _mm_mul_ps(x, _ps_cephes_FOPI);

  /* store the integer part of y in mm0:mm1 */
  xmm2 = _mm_movehl_ps(xmm2, y);
  mm2 = _mm_cvttps_pi32(y);
  mm3 = _mm_cvttps_pi32(xmm2);

  /* j=(j+1) & (~1) (see the cephes sources) */
  mm2 = _mm_add_pi32(mm2, *((v2si*)&_mm_one_si128));
  mm3 = _mm_add_pi32(mm3, *((v2si*)&_mm_one_si128));
  mm2 = _mm_and_si64(mm2, *((v2si*)&_mm_inv_one_si128));
  mm3 = _mm_and_si64(mm3, *((v2si*)&_mm_inv_one_si128));

  y = _mm_cvtpi32x2_ps(mm2, mm3);
  //printf("\nsin_ps: mm2:mm3 = "); print2i(mm2); print2i(mm3); printf("\n");
  //printf("sin_ps: y="); print4(y); printf("\n");


  /* get the swap sign flag */
  mm0 = _mm_and_si64(mm2, *((v2si*)&_mm_four_si128));
  mm1 = _mm_and_si64(mm3, *((v2si*)&_mm_four_si128));
  mm0 = _mm_slli_pi32(mm0, 29);
  mm1 = _mm_slli_pi32(mm1, 29);

  /* get the polynom selection mask
     there is one polynom for 0 <= x <= Pi/4
     and another one for Pi/4<x<=Pi/2

     Both branches will be computed.
  */
  mm2 = _mm_and_si64(mm2, *((v2si*)&_mm_two_si128));
  mm3 = _mm_and_si64(mm3, *((v2si*)&_mm_two_si128));
  mm2 = _mm_cmpeq_pi32(mm2, _mm_setzero_si64());
  mm3 = _mm_cmpeq_pi32(mm3, _mm_setzero_si64());

  v4sf swap_sign_bit, poly_mask;
  COPY_MM_TO_XMM(mm0, mm1, swap_sign_bit);
  COPY_MM_TO_XMM(mm2, mm3, poly_mask);
  sign_bit = _mm_xor_ps(sign_bit, swap_sign_bit);

  /* The magic pass: "Extended precision modular arithmetic"
     x = ((x - y * DP1) - y * DP2) - y * DP3; */
  xmm1 = _ps_minus_cephes_DP1;
  xmm2 = _ps_minus_cephes_DP2;
  xmm3 = _ps_minus_cephes_DP3;
  xmm1 = _mm_mul_ps(y, xmm1);
  xmm2 = _mm_mul_ps(y, xmm2);
  xmm3 = _mm_mul_ps(y, xmm3);
  x = _mm_add_ps(x, xmm1);
  x = _mm_add_ps(x, xmm2);
  x = _mm_add_ps(x, xmm3);

  /* Evaluate the first polynom  (0 <= x <= Pi/4) */
  y = _ps_coscof_p0;
  v4sf z = _mm_mul_ps(x,x);

  y = _mm_mul_ps(y, z);
  y = _mm_add_ps(y, _ps_coscof_p1);
  y = _mm_mul_ps(y, z);
  y = _mm_add_ps(y, _ps_coscof_p2);
  y = _mm_mul_ps(y, z);
  y = _mm_mul_ps(y, z);
  v4sf tmp = _mm_mul_ps(z, _mm_one_half);
  y = _mm_sub_ps(y, tmp);
  y = _mm_add_ps(y, _mm_one);

  /* Evaluate the second polynom  (Pi/4 <= x <= 0) */

  v4sf y2 = _ps_sincof_p0;
  y2 = _mm_mul_ps(y2, z);
  y2 = _mm_add_ps(y2, _ps_sincof_p1);
  y2 = _mm_mul_ps(y2, z);
  y2 = _mm_add_ps(y2, _ps_sincof_p2);
  y2 = _mm_mul_ps(y2, z);
  y2 = _mm_mul_ps(y2, x);
  y2 = _mm_add_ps(y2, x);

  /* select the correct result from the two polynoms */
  xmm3 = poly_mask;
  y2 = _mm_and_ps(xmm3, y2); //, xmm3);
  y = _mm_andnot_ps(xmm3, y);
  y = _mm_add_ps(y,y2);
  /* update the sign */
  y = _mm_xor_ps(y, sign_bit);
  _mm_empty(); /* good-bye mmx */
  return y;
}

sse_t Manta::cos4(sse_t x) {
  typedef __m128 v4sf;
  typedef __m64 v2si;

  v4sf xmm1, xmm2 = _mm_setzero_ps(), xmm3, y;
  v2si mm0, mm1, mm2, mm3;
  /* take the absolute value */
  x = _mm_and_ps(x, _mm_absmask);

  /* scale by 4/Pi */
  y = _mm_mul_ps(x, _ps_cephes_FOPI);

  /* store the integer part of y in mm0:mm1 */
  xmm2 = _mm_movehl_ps(xmm2, y);
  mm2 = _mm_cvttps_pi32(y);
  mm3 = _mm_cvttps_pi32(xmm2);

  /* j=(j+1) & (~1) (see the cephes sources) */
  mm2 = _mm_add_pi32(mm2, *((v2si*)&_mm_one_si128));
  mm3 = _mm_add_pi32(mm3, *((v2si*)&_mm_one_si128));
  mm2 = _mm_and_si64(mm2, *((v2si*)&_mm_inv_one_si128));
  mm3 = _mm_and_si64(mm3, *((v2si*)&_mm_inv_one_si128));

  y = _mm_cvtpi32x2_ps(mm2, mm3);


  mm2 = _mm_sub_pi32(mm2, *((v2si*)&_mm_two_si128));
  mm3 = _mm_sub_pi32(mm3, *((v2si*)&_mm_two_si128));

  /* get the swap sign flag in mm0:mm1 and the
     polynom selection mask in mm2:mm3 */

  mm0 = _mm_andnot_si64(mm2, *((v2si*)&_mm_four_si128));
  mm1 = _mm_andnot_si64(mm3, *((v2si*)&_mm_four_si128));
  mm0 = _mm_slli_pi32(mm0, 29);
  mm1 = _mm_slli_pi32(mm1, 29);

  mm2 = _mm_and_si64(mm2, *((v2si*)&_mm_two_si128));
  mm3 = _mm_and_si64(mm3, *((v2si*)&_mm_two_si128));

  mm2 = _mm_cmpeq_pi32(mm2, _mm_setzero_si64());
  mm3 = _mm_cmpeq_pi32(mm3, _mm_setzero_si64());

  v4sf sign_bit, poly_mask;
  COPY_MM_TO_XMM(mm0, mm1, sign_bit);
  COPY_MM_TO_XMM(mm2, mm3, poly_mask);

  /* The magic pass: "Extended precision modular arithmetic"
     x = ((x - y * DP1) - y * DP2) - y * DP3; */
  xmm1 = _ps_minus_cephes_DP1;
  xmm2 = _ps_minus_cephes_DP2;
  xmm3 = _ps_minus_cephes_DP3;
  xmm1 = _mm_mul_ps(y, xmm1);
  xmm2 = _mm_mul_ps(y, xmm2);
  xmm3 = _mm_mul_ps(y, xmm3);
  x = _mm_add_ps(x, xmm1);
  x = _mm_add_ps(x, xmm2);
  x = _mm_add_ps(x, xmm3);

  /* Evaluate the first polynom  (0 <= x <= Pi/4) */
  y = _ps_coscof_p0;
  v4sf z = _mm_mul_ps(x,x);

  y = _mm_mul_ps(y, z);
  y = _mm_add_ps(y, _ps_coscof_p1);
  y = _mm_mul_ps(y, z);
  y = _mm_add_ps(y, _ps_coscof_p2);
  y = _mm_mul_ps(y, z);
  y = _mm_mul_ps(y, z);
  v4sf tmp = _mm_mul_ps(z, _mm_one_half);
  y = _mm_sub_ps(y, tmp);
  y = _mm_add_ps(y, _mm_one);

  /* Evaluate the second polynom  (Pi/4 <= x <= 0) */

  v4sf y2 = _ps_sincof_p0;
  y2 = _mm_mul_ps(y2, z);
  y2 = _mm_add_ps(y2, _ps_sincof_p1);
  y2 = _mm_mul_ps(y2, z);
  y2 = _mm_add_ps(y2, _ps_sincof_p2);
  y2 = _mm_mul_ps(y2, z);
  y2 = _mm_mul_ps(y2, x);
  y2 = _mm_add_ps(y2, x);

  /* select the correct result from the two polynoms */
  xmm3 = poly_mask;
  y2 = _mm_and_ps(xmm3, y2); //, xmm3);
  y = _mm_andnot_ps(xmm3, y);
  y = _mm_add_ps(y,y2);
  /* update the sign */
  y = _mm_xor_ps(y, sign_bit);

  _mm_empty(); /* good-bye mmx */
  return y;
}

void Manta::sincos4(sse_t x, sse_t* s, sse_t* c) {
  typedef __m128 v4sf;
  typedef __m64 v2si;

  v4sf xmm1, xmm2, xmm3 = _mm_setzero_ps(), sign_bit_sin, y;
  v2si mm0, mm1, mm2, mm3, mm4, mm5;
  sign_bit_sin = x;
  /* take the absolute value */
  x = _mm_and_ps(x, _mm_absmask);
  /* extract the sign bit (upper one) */
  sign_bit_sin = _mm_and_ps(sign_bit_sin, _mm_signbit);

  /* scale by 4/Pi */
  y = _mm_mul_ps(x, _ps_cephes_FOPI);

  /* store the integer part of y in mm0:mm1 */
  xmm3 = _mm_movehl_ps(xmm3, y);
  mm2 = _mm_cvttps_pi32(y);
  mm3 = _mm_cvttps_pi32(xmm3);

  /* j=(j+1) & (~1) (see the cephes sources) */
  mm2 = _mm_add_pi32(mm2, *((v2si*)&_mm_one_si128));
  mm3 = _mm_add_pi32(mm3, *((v2si*)&_mm_one_si128));
  mm2 = _mm_and_si64(mm2, *((v2si*)&_mm_inv_one_si128));
  mm3 = _mm_and_si64(mm3, *((v2si*)&_mm_inv_one_si128));

  y = _mm_cvtpi32x2_ps(mm2, mm3);

  mm4 = mm2;
  mm5 = mm3;

  /* get the swap sign flag for the sine */
  mm0 = _mm_and_si64(mm2, *((v2si*)&_mm_four_si128));
  mm1 = _mm_and_si64(mm3, *((v2si*)&_mm_four_si128));
  mm0 = _mm_slli_pi32(mm0, 29);
  mm1 = _mm_slli_pi32(mm1, 29);
  v4sf swap_sign_bit_sin;
  COPY_MM_TO_XMM(mm0, mm1, swap_sign_bit_sin);

  /* get the polynom selection mask for the sine */

  mm2 = _mm_and_si64(mm2, *((v2si*)&_mm_two_si128));
  mm3 = _mm_and_si64(mm3, *((v2si*)&_mm_two_si128));
  mm2 = _mm_cmpeq_pi32(mm2, _mm_setzero_si64());
  mm3 = _mm_cmpeq_pi32(mm3, _mm_setzero_si64());
  v4sf poly_mask;
  COPY_MM_TO_XMM(mm2, mm3, poly_mask);

  /* The magic pass: "Extended precision modular arithmetic"
     x = ((x - y * DP1) - y * DP2) - y * DP3; */
  xmm1 = _ps_minus_cephes_DP1;
  xmm2 = _ps_minus_cephes_DP2;
  xmm3 = _ps_minus_cephes_DP3;
  xmm1 = _mm_mul_ps(y, xmm1);
  xmm2 = _mm_mul_ps(y, xmm2);
  xmm3 = _mm_mul_ps(y, xmm3);
  x = _mm_add_ps(x, xmm1);
  x = _mm_add_ps(x, xmm2);
  x = _mm_add_ps(x, xmm3);


  /* get the sign flag for the cosine */

  mm4 = _mm_sub_pi32(mm4, *((v2si*)&_mm_two_si128));
  mm5 = _mm_sub_pi32(mm5, *((v2si*)&_mm_two_si128));
  mm4 = _mm_andnot_si64(mm4, *((v2si*)&_mm_four_si128));
  mm5 = _mm_andnot_si64(mm5, *((v2si*)&_mm_four_si128));
  mm4 = _mm_slli_pi32(mm4, 29);
  mm5 = _mm_slli_pi32(mm5, 29);
  v4sf sign_bit_cos;
  COPY_MM_TO_XMM(mm4, mm5, sign_bit_cos);

  sign_bit_sin = _mm_xor_ps(sign_bit_sin, swap_sign_bit_sin);


  /* Evaluate the first polynom  (0 <= x <= Pi/4) */
  v4sf z = _mm_mul_ps(x,x);
  y = _ps_coscof_p0;

  y = _mm_mul_ps(y, z);
  y = _mm_add_ps(y, _ps_coscof_p1);
  y = _mm_mul_ps(y, z);
  y = _mm_add_ps(y, _ps_coscof_p2);
  y = _mm_mul_ps(y, z);
  y = _mm_mul_ps(y, z);
  v4sf tmp = _mm_mul_ps(z, _mm_one_half);
  y = _mm_sub_ps(y, tmp);
  y = _mm_add_ps(y, _mm_one);

  /* Evaluate the second polynom  (Pi/4 <= x <= 0) */

  v4sf y2 = _ps_sincof_p0;
  y2 = _mm_mul_ps(y2, z);
  y2 = _mm_add_ps(y2, _ps_sincof_p1);
  y2 = _mm_mul_ps(y2, z);
  y2 = _mm_add_ps(y2, _ps_sincof_p2);
  y2 = _mm_mul_ps(y2, z);
  y2 = _mm_mul_ps(y2, x);
  y2 = _mm_add_ps(y2, x);

  /* select the correct result from the two polynoms */
  xmm3 = poly_mask;
  v4sf ysin2 = _mm_and_ps(xmm3, y2);
  v4sf ysin1 = _mm_andnot_ps(xmm3, y);
  y2 = _mm_sub_ps(y2,ysin2);
  y = _mm_sub_ps(y, ysin1);

  xmm1 = _mm_add_ps(ysin1,ysin2);
  xmm2 = _mm_add_ps(y,y2);

  /* update the sign */
  *s = _mm_xor_ps(xmm1, sign_bit_sin);
  *c = _mm_xor_ps(xmm2, sign_bit_cos);
  _mm_empty(); /* good-bye mmx */
}

#endif
