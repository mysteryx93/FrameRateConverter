// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
// http://www.avisynth.org

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .
//
// Linking Avisynth statically or dynamically with other modules is making a
// combined work based on Avisynth.  Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Avisynth give you
// permission to link Avisynth with independent modules that communicate with
// Avisynth solely through the interfaces defined in avisynth.h, regardless of the license
// terms of these independent modules, and to copy and distribute the
// resulting combined work under terms of your choice, provided that
// every copy of the combined work is accompanied by a complete copy of
// the source code of Avisynth (the version of Avisynth used to produce the
// combined work), being distributed under the terms of the GNU General
// Public License plus this exception.  An independent module is a module
// which is not derived from or based on Avisynth, such as 3rd-party filters,
// import and export plugins, or graphical user interfaces.


// Avisynth filter: YUV merge / Swap planes
// by Klaus Post (kp@interact.dk)
// adapted by Richard Berg (avisynth-dev@richardberg.net)
// iSSE code by Ian Brabham

#include "merge.h"
#include "merge_avx2.h"
#include <emmintrin.h>
#include <smmintrin.h>
#include "avs/alignment.h"
#include <stdint.h>


/* -----------------------------------
 *     weighted_merge_chroma_yuy2
 * -----------------------------------
 */
static void weighted_merge_chroma_yuy2_sse2(BYTE *src, const BYTE *chroma, int pitch, int chroma_pitch,int width, int height, int weight, int invweight )
{
  __m128i round_mask = _mm_set1_epi32(0x4000);
  __m128i mask = _mm_set_epi16(weight, invweight, weight, invweight, weight, invweight, weight, invweight);
  __m128i luma_mask = _mm_set1_epi16(0x00FF);

  int wMod16 = (width/16) * 16;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x += 16) {
      __m128i px1 = _mm_load_si128(reinterpret_cast<const __m128i*>(src+x)); 
      __m128i px2 = _mm_load_si128(reinterpret_cast<const __m128i*>(chroma+x)); 

      __m128i src_lo = _mm_unpacklo_epi16(px1, px2); 
      __m128i src_hi = _mm_unpackhi_epi16(px1, px2); 

      src_lo = _mm_srli_epi16(src_lo, 8);
      src_hi = _mm_srli_epi16(src_hi, 8);

      src_lo = _mm_madd_epi16(src_lo, mask);
      src_hi = _mm_madd_epi16(src_hi, mask);

      src_lo = _mm_add_epi32(src_lo, round_mask);
      src_hi = _mm_add_epi32(src_hi, round_mask);

      src_lo = _mm_srli_epi32(src_lo, 15);
      src_hi = _mm_srli_epi32(src_hi, 15);

      __m128i result_chroma = _mm_packs_epi32(src_lo, src_hi);
      result_chroma = _mm_slli_epi16(result_chroma, 8);

      __m128i result_luma = _mm_and_si128(px1, luma_mask);
      __m128i result = _mm_or_si128(result_chroma, result_luma);

      _mm_store_si128(reinterpret_cast<__m128i*>(src+x), result);
    }

    for (int x = wMod16; x < width; x+=2) {
      src[x+1] = (chroma[x+1] * weight + src[x+1] * invweight + 16384) >> 15;
    }

    src += pitch;
    chroma += chroma_pitch;
  }
}

#ifdef X86_32
static void weighted_merge_chroma_yuy2_mmx(BYTE *src, const BYTE *chroma, int pitch, int chroma_pitch,int width, int height, int weight, int invweight )
{
  __m64 round_mask = _mm_set1_pi32(0x4000);
  __m64 mask = _mm_set_pi16(weight, invweight, weight, invweight);
  __m64 luma_mask = _mm_set1_pi16(0x00FF);

  int wMod8 = (width/8) * 8;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod8; x += 8) {
      __m64 px1 = *reinterpret_cast<const __m64*>(src+x); //V1 Y3 U1 Y2 V0 Y1 U0 Y0
      __m64 px2 = *reinterpret_cast<const __m64*>(chroma+x); //v1 y3 u1 y2 v0 y1 u0 y0

      __m64 src_lo = _mm_unpacklo_pi16(px1, px2); //v0 y1 V0 Y1 u0 y0 U0 Y0
      __m64 src_hi = _mm_unpackhi_pi16(px1, px2); 

      src_lo = _mm_srli_pi16(src_lo, 8); //00 v0 00 V0 00 u0 00 U0
      src_hi = _mm_srli_pi16(src_hi, 8); 

      src_lo = _mm_madd_pi16(src_lo, mask);
      src_hi = _mm_madd_pi16(src_hi, mask);

      src_lo = _mm_add_pi32(src_lo, round_mask);
      src_hi = _mm_add_pi32(src_hi, round_mask);

      src_lo = _mm_srli_pi32(src_lo, 15);
      src_hi = _mm_srli_pi32(src_hi, 15);

      __m64 result_chroma = _mm_packs_pi32(src_lo, src_hi);
      result_chroma = _mm_slli_pi16(result_chroma, 8);

      __m64 result_luma = _mm_and_si64(px1, luma_mask);
      __m64 result = _mm_or_si64(result_chroma, result_luma);

      *reinterpret_cast<__m64*>(src+x) = result;
    }

    for (int x = wMod8; x < width; x+=2) {
      src[x+1] = (chroma[x+1] * weight + src[x+1] * invweight + 16384) >> 15;
    }

    src += pitch;
    chroma += chroma_pitch;
  }
  _mm_empty();
}
#endif

static void weighted_merge_chroma_yuy2_c(BYTE *src, const BYTE *chroma, int pitch, int chroma_pitch,int width, int height, int weight, int invweight) {
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; x+=2) {
      src[x+1] = (chroma[x+1] * weight + src[x+1] * invweight + 16384) >> 15;
    }
    src+=pitch;
    chroma+=chroma_pitch;
  }
}


/* -----------------------------------
 *      weighted_merge_luma_yuy2
 * -----------------------------------
 */
static void weighted_merge_luma_yuy2_sse2(BYTE *src, const BYTE *luma, int pitch, int luma_pitch,int width, int height, int weight, int invweight)
{
  __m128i round_mask = _mm_set1_epi32(0x4000);
  __m128i mask = _mm_set_epi16(weight, invweight, weight, invweight, weight, invweight, weight, invweight);
  __m128i luma_mask = _mm_set1_epi16(0x00FF);
#pragma warning(push)
#pragma warning(disable: 4309)
  __m128i chroma_mask = _mm_set1_epi16(0xFF00);
#pragma warning(pop)

  int wMod16 = (width/16) * 16;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x += 16) {
      __m128i px1 = _mm_load_si128(reinterpret_cast<const __m128i*>(src+x)); //V1 Y3 U1 Y2 V0 Y1 U0 Y0
      __m128i px2 = _mm_load_si128(reinterpret_cast<const __m128i*>(luma+x)); //v1 y3 u1 y2 v0 y1 u0 y0

      __m128i src_lo = _mm_unpacklo_epi16(px1, px2); //v0 y1 V0 Y1 u0 y0 U0 Y0
      __m128i src_hi = _mm_unpackhi_epi16(px1, px2); 

      src_lo = _mm_and_si128(src_lo, luma_mask); //00 v0 00 V0 00 u0 00 U0
      src_hi = _mm_and_si128(src_hi, luma_mask); 

      src_lo = _mm_madd_epi16(src_lo, mask);
      src_hi = _mm_madd_epi16(src_hi, mask);

      src_lo = _mm_add_epi32(src_lo, round_mask);
      src_hi = _mm_add_epi32(src_hi, round_mask);

      src_lo = _mm_srli_epi32(src_lo, 15);
      src_hi = _mm_srli_epi32(src_hi, 15);

      __m128i result_luma = _mm_packs_epi32(src_lo, src_hi);

      __m128i result_chroma = _mm_and_si128(px1, chroma_mask);
      __m128i result = _mm_or_si128(result_chroma, result_luma);

      _mm_store_si128(reinterpret_cast<__m128i*>(src+x), result);
    }

    for (int x = wMod16; x < width; x+=2) {
      src[x] = (luma[x] * weight + src[x] * invweight + 16384) >> 15;
    }

    src += pitch;
    luma += luma_pitch;
  }
}

#ifdef X86_32
static void weighted_merge_luma_yuy2_mmx(BYTE *src, const BYTE *luma, int pitch, int luma_pitch,int width, int height, int weight, int invweight)
{
  __m64 round_mask = _mm_set1_pi32(0x4000);
  __m64 mask = _mm_set_pi16(weight, invweight, weight, invweight);
  __m64 luma_mask = _mm_set1_pi16(0x00FF);
#pragma warning(push)
#pragma warning(disable: 4309)
  __m64 chroma_mask = _mm_set1_pi16(0xFF00);
#pragma warning(pop)

  int wMod8 = (width/8) * 8;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod8; x += 8) {
      __m64 px1 = *reinterpret_cast<const __m64*>(src+x); //V1 Y3 U1 Y2 V0 Y1 U0 Y0
      __m64 px2 = *reinterpret_cast<const __m64*>(luma+x); //v1 y3 u1 y2 v0 y1 u0 y0

      __m64 src_lo = _mm_unpacklo_pi16(px1, px2); //v0 y1 V0 Y1 u0 y0 U0 Y0
      __m64 src_hi = _mm_unpackhi_pi16(px1, px2); 

      src_lo = _mm_and_si64(src_lo, luma_mask); //00 v0 00 V0 00 u0 00 U0
      src_hi = _mm_and_si64(src_hi, luma_mask); 

      src_lo = _mm_madd_pi16(src_lo, mask);
      src_hi = _mm_madd_pi16(src_hi, mask);

      src_lo = _mm_add_pi32(src_lo, round_mask);
      src_hi = _mm_add_pi32(src_hi, round_mask);

      src_lo = _mm_srli_pi32(src_lo, 15);
      src_hi = _mm_srli_pi32(src_hi, 15);

      __m64 result_luma = _mm_packs_pi32(src_lo, src_hi);

      __m64 result_chroma = _mm_and_si64(px1, chroma_mask);
      __m64 result = _mm_or_si64(result_chroma, result_luma);

      *reinterpret_cast<__m64*>(src+x) = result;
    }

    for (int x = wMod8; x < width; x+=2) {
      src[x] = (luma[x] * weight + src[x] * invweight + 16384) >> 15;
    }

    src += pitch;
    luma += luma_pitch;
  }
  _mm_empty();
}
#endif

static void weighted_merge_luma_yuy2_c(BYTE *src, const BYTE *luma, int pitch, int luma_pitch,int width, int height, int weight, int invweight) {
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; x+=2) {
      src[x] = (luma[x] * weight + src[x] * invweight + 16384) >> 15;
    }
    src+=pitch;
    luma+=luma_pitch;
  }
}


/* -----------------------------------
 *          replace_luma_yuy2
 * -----------------------------------
 */
static void replace_luma_yuy2_sse2(BYTE *src, const BYTE *luma, int pitch, int luma_pitch,int width, int height)
{
  int mod16_width = width / 16 * 16;
  __m128i luma_mask = _mm_set1_epi16(0x00FF);
#pragma warning(push)
#pragma warning(disable: 4309)
  __m128i chroma_mask = _mm_set1_epi16(0xFF00);
#pragma warning(pop)

  for(int y = 0; y < height; y++) {
    for(int x = 0; x < mod16_width; x+=16) {
      __m128i s = _mm_load_si128(reinterpret_cast<const __m128i*>(src+x));
      __m128i l = _mm_load_si128(reinterpret_cast<const __m128i*>(luma+x));

      __m128i s_chroma = _mm_and_si128(s, chroma_mask);
      __m128i l_luma = _mm_and_si128(l, luma_mask);

      __m128i result = _mm_or_si128(s_chroma, l_luma);

      _mm_store_si128(reinterpret_cast<__m128i*>(src+x), result);
    }

    for (int x = mod16_width; x < width; x+=2) {
      src[x] = luma[x];
    }
    src += pitch;
    luma += luma_pitch;
  }
}

#ifdef X86_32
static void replace_luma_yuy2_mmx(BYTE *src, const BYTE *luma, int pitch, int luma_pitch,int width, int height)
{
  int mod8_width = width / 8 * 8;
  __m64 luma_mask = _mm_set1_pi16(0x00FF);
#pragma warning(push)
#pragma warning(disable: 4309)
  __m64 chroma_mask = _mm_set1_pi16(0xFF00);
#pragma warning(pop)

  for(int y = 0; y < height; y++) {
    for(int x = 0; x < mod8_width; x+=8) {
      __m64 s = *reinterpret_cast<const __m64*>(src+x);
      __m64 l = *reinterpret_cast<const __m64*>(luma+x);

      __m64 s_chroma = _mm_and_si64(s, chroma_mask);
      __m64 l_luma = _mm_and_si64(l, luma_mask);

      __m64 result = _mm_or_si64(s_chroma, l_luma);

      *reinterpret_cast<__m64*>(src+x) = result;
    }

    for (int x = mod8_width; x < width; x+=2) {
      src[x] = luma[x];
    }
    src += pitch;
    luma += luma_pitch;
  }
  _mm_empty();
}
#endif

static void replace_luma_yuy2_c(BYTE *src, const BYTE *luma, int pitch, int luma_pitch,int width, int height ) {
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; x+=2) {
      src[x] = luma[x];
    }
    src+=pitch;
    luma+=luma_pitch;
  }
}


/* -----------------------------------
 *            average_plane
 * -----------------------------------
 */
template<typename pixel_t>
static void average_plane_sse2(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch, int rowsize, int height) {
  // width is RowSize here
  int mod16_width = rowsize / 16 * 16;

  for(int y = 0; y < height; y++) {
    for(int x = 0; x < mod16_width; x+=16) {
      __m128i src1  = _mm_load_si128(reinterpret_cast<const __m128i*>(p1+x));
      __m128i src2  = _mm_load_si128(reinterpret_cast<const __m128i*>(p2+x));
      __m128i dst;
      if constexpr(sizeof(pixel_t) == 1)
        dst  = _mm_avg_epu8(src1, src2); // 8 pixels
      else // pixel_size == 2
        dst = _mm_avg_epu16(src1, src2); // 4 pixels

      _mm_store_si128(reinterpret_cast<__m128i*>(p1+x), dst);
    }

    if (mod16_width != rowsize) {
      for (size_t x = mod16_width / sizeof(pixel_t); x < rowsize/sizeof(pixel_t); ++x) {
        reinterpret_cast<pixel_t *>(p1)[x] = (int(reinterpret_cast<pixel_t *>(p1)[x]) + reinterpret_cast<const pixel_t *>(p2)[x] + 1) >> 1;
      }
    }
    p1 += p1_pitch;
    p2 += p2_pitch;
  }
}

#ifdef X86_32
template<typename pixel_t>
static void average_plane_isse(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch, int rowsize, int height) {
  // width is RowSize here
  int mod8_width = rowsize / 8 * 8;

  for(int y = 0; y < height; y++) {
    for(int x = 0; x < mod8_width; x+=8) {
      __m64 src1 = *reinterpret_cast<const __m64*>(p1+x);
      __m64 src2 = *reinterpret_cast<const __m64*>(p2+x);
      __m64 dst;
      if constexpr(sizeof(pixel_t) == 1)
        dst = _mm_avg_pu8(src1, src2);  // 8 pixels
      else // pixel_size == 2
        dst = _mm_avg_pu16(src1, src2); // 4 pixels
      *reinterpret_cast<__m64*>(p1+x) = dst;
    }

    if (mod8_width != rowsize) {
      for (size_t x = mod8_width / sizeof(pixel_t); x < rowsize / sizeof(pixel_t); ++x) {
        reinterpret_cast<pixel_t *>(p1)[x] = (int(reinterpret_cast<pixel_t *>(p1)[x]) + reinterpret_cast<const pixel_t *>(p2)[x] + 1) >> 1;
      }
    }
    p1 += p1_pitch;
    p2 += p2_pitch;
  }
  _mm_empty();
}
#endif

// for uint8_t and uint16_t
template<typename pixel_t>
static void average_plane_c(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch, int rowsize, int height) {
  for (int y = 0; y < height; ++y) {
    for (size_t x = 0; x < rowsize / sizeof(pixel_t); ++x) {
      reinterpret_cast<pixel_t *>(p1)[x] = (int(reinterpret_cast<pixel_t *>(p1)[x]) + reinterpret_cast<const pixel_t *>(p2)[x] + 1) >> 1;
    }
    p1 += p1_pitch;
    p2 += p2_pitch;
  }
}
// for float
static void average_plane_c_float(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch, int rowsize, int height) {

  size_t rs = rowsize / sizeof(float);

  for (int y = 0; y < height; ++y) {
    for (size_t x = 0; x < rs; ++x) {
      reinterpret_cast<float *>(p1)[x] = (reinterpret_cast<float *>(p1)[x] + reinterpret_cast<const float *>(p2)[x]) / 2.0f;
    }
    p1 += p1_pitch;
    p2 += p2_pitch;
  }
}

/* -----------------------------------
 *       weighted_merge_planar
 * -----------------------------------
 */

template<bool lessthan16bit>
void weighted_merge_planar_uint16_sse2(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch, int rowsize, int height, float weight_f, int weight_i, int invweight_i) {
  AVS_UNUSED(weight_f);
  __m128i round_mask = _mm_set1_epi32(0x4000);
  __m128i zero = _mm_setzero_si128();
  __m128i mask = _mm_set1_epi32((weight_i << 16) + invweight_i);

  int wMod16 = (rowsize / 16) * 16;
  const __m128i signed_shifter = _mm_set1_epi16(-32768);

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x += 16) {
      __m128i px1 = _mm_load_si128(reinterpret_cast<const __m128i*>(p1 + x)); // y7y6 y5y4 y3y2 y1y0
      __m128i px2 = _mm_load_si128(reinterpret_cast<const __m128i*>(p2 + x)); // Y7Y6 Y5Y4 Y3Y2 Y1Y0

      if (!lessthan16bit) {
        px1 = _mm_add_epi16(px1, signed_shifter);
        px2 = _mm_add_epi16(px2, signed_shifter);
      }

      __m128i p03 = _mm_unpacklo_epi16(px1, px2); // Y3y3 Y2y2 Y1y1 Y0y0
      __m128i p47 = _mm_unpackhi_epi16(px1, px2); // Y7y7 Y6y6 Y5y5 Y4y4

      p03 = _mm_madd_epi16(p03, mask); // px1 * invweight + px2 * weight
      p47 = _mm_madd_epi16(p47, mask);

      p03 = _mm_add_epi32(p03, round_mask);
      p47 = _mm_add_epi32(p47, round_mask);

      p03 = _mm_srai_epi32(p03, 15);
      p47 = _mm_srai_epi32(p47, 15);

      auto p07 = _mm_packs_epi32(p03, p47);
      if (!lessthan16bit) {
        p07 = _mm_add_epi16(p07, signed_shifter);
      }

      __m128i result = p07;

      _mm_store_si128(reinterpret_cast<__m128i*>(p1 + x), result);
    }

    for (size_t x = wMod16 / sizeof(uint16_t); x < rowsize / sizeof(uint16_t); x++) {
      reinterpret_cast<uint16_t *>(p1)[x] = (reinterpret_cast<uint16_t *>(p1)[x] * invweight_i + reinterpret_cast<const uint16_t *>(p2)[x] * weight_i + 16384) >> 15;
    }

    p1 += p1_pitch;
    p2 += p2_pitch;
  }
}

void weighted_merge_planar_sse2(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch, int rowsize, int height, float weight_f, int weight_i, int invweight_i) {
  AVS_UNUSED(weight_f);
  // 8 bit only. SSE2 has weak support for unsigned 16 bit
  __m128i round_mask = _mm_set1_epi32(0x4000);
  __m128i zero = _mm_setzero_si128();
  __m128i mask = _mm_set_epi16(weight_i, invweight_i, weight_i, invweight_i, weight_i, invweight_i, weight_i, invweight_i);

  int wMod16 = (rowsize / 16) * 16;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x += 16) {
      __m128i px1 = _mm_load_si128(reinterpret_cast<const __m128i*>(p1 + x)); // y15y14 ... y7y6 y5y4 y3y2 y1y0
      __m128i px2 = _mm_load_si128(reinterpret_cast<const __m128i*>(p2 + x)); // Y15Y14 ... Y7Y6 Y5Y4 Y3Y2 Y1Y0

      __m128i p07 = _mm_unpacklo_epi8(px1, px2); // Y7y7 ..  Y3y3   Y2y2  Y1y1 Y0y0
      __m128i p815 = _mm_unpackhi_epi8(px1, px2); //Y15y15 ..Y11y11 Y10y10 Y9y9 Y8y8

      __m128i p03 = _mm_unpacklo_epi8(p07, zero);  //00Y3 00y3 00Y2 00y2 00Y1 00y1 00Y0 00y0 8*short
      __m128i p47 = _mm_unpackhi_epi8(p07, zero);
      __m128i p811 = _mm_unpacklo_epi8(p815, zero);
      __m128i p1215 = _mm_unpackhi_epi8(p815, zero);

      p03 = _mm_madd_epi16(p03, mask);
      p47 = _mm_madd_epi16(p47, mask);
      p811 = _mm_madd_epi16(p811, mask);
      p1215 = _mm_madd_epi16(p1215, mask);

      p03 = _mm_add_epi32(p03, round_mask);
      p47 = _mm_add_epi32(p47, round_mask);
      p811 = _mm_add_epi32(p811, round_mask);
      p1215 = _mm_add_epi32(p1215, round_mask);

      p03 = _mm_srli_epi32(p03, 15);
      p47 = _mm_srli_epi32(p47, 15);
      p811 = _mm_srli_epi32(p811, 15);
      p1215 = _mm_srli_epi32(p1215, 15);

      p07 = _mm_packs_epi32(p03, p47);
      p815 = _mm_packs_epi32(p811, p1215);

      __m128i result = _mm_packus_epi16(p07, p815);

      _mm_store_si128(reinterpret_cast<__m128i*>(p1 + x), result);
    }

    for (size_t x = wMod16 / sizeof(uint8_t); x < rowsize / sizeof(uint8_t); x++) {
      reinterpret_cast<uint8_t *>(p1)[x] = (reinterpret_cast<uint8_t *>(p1)[x] * invweight_i + reinterpret_cast<const uint8_t *>(p2)[x] * weight_i + 16384) >> 15;
    }

    p1 += p1_pitch;
    p2 += p2_pitch;
  }
}


void weighted_merge_planar_sse2_float(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch, int rowsize, int height, float weight_f, int weight_i, int invweight_i) {
  AVS_UNUSED(weight_i);
  AVS_UNUSED(invweight_i);

  float invweight_f = 1.0f - weight_f;
  auto mask = _mm_set1_ps(weight_f);

  int wMod16 = (rowsize / 16) * 16;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x += 16) {
      auto px1 = _mm_load_ps(reinterpret_cast<const float*>(p1 + x));
      auto px2 = _mm_load_ps(reinterpret_cast<const float*>(p2 + x));
      // p1:dst p2:src
      // ( 1- mask) * dst + mask * src =
      // dst - dst * mask + src * mask =
      // dst + mask * (src - dst)
      auto diff = _mm_sub_ps(px2, px1);
      auto tmp = _mm_mul_ps(diff, mask); // (p2-p1)*mask
      auto result = _mm_add_ps(tmp, px1); // +dst

      _mm_store_ps(reinterpret_cast<float *>(p1 + x), result);
    }

    for (size_t x = wMod16 / sizeof(float); x < rowsize / sizeof(float); x++) {
      reinterpret_cast<float *>(p1)[x] = reinterpret_cast<float *>(p1)[x] * invweight_f + reinterpret_cast<const float *>(p2)[x] * weight_f;
    }

    p1 += p1_pitch;
    p2 += p2_pitch;
  }
}

void average_plane_sse2_float(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch, int rowsize, int height) {
  auto OneHalf = _mm_set1_ps(0.5f);

  int wMod16 = (rowsize / 16) * 16;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x += 16) {
      auto px1 = _mm_load_ps(reinterpret_cast<const float*>(p1 + x));
      auto px2 = _mm_load_ps(reinterpret_cast<const float*>(p2 + x));

      auto result = _mm_mul_ps(_mm_add_ps(px1, px2), OneHalf); //

      _mm_store_ps(reinterpret_cast<float *>(p1 + x), result);
    }

    for (size_t x = wMod16 / sizeof(float); x < rowsize / sizeof(float); x++) {
      reinterpret_cast<float *>(p1)[x] = (reinterpret_cast<float *>(p1)[x] + reinterpret_cast<const float *>(p2)[x]) * 0.5f;
    }

    p1 += p1_pitch;
    p2 += p2_pitch;
  }
}

#ifdef X86_32
void weighted_merge_planar_mmx(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch, int rowsize, int height, float weight_f, int weight_i, int invweight_i) {
  __m64 round_mask = _mm_set1_pi32(0x4000);
  __m64 zero = _mm_setzero_si64();
  __m64 mask = _mm_set_pi16(weight_i, invweight_i, weight_i, invweight_i);

  int wMod8 = (rowsize/8) * 8;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod8; x += 8) {
      __m64 px1 = *(reinterpret_cast<const __m64*>(p1+x)); //y7y6 y5y4 y3y2 y1y0
      __m64 px2 = *(reinterpret_cast<const __m64*>(p2+x)); //Y7Y6 Y5Y4 Y3Y2 Y1Y0

      __m64 p0123 = _mm_unpacklo_pi8(px1, px2); //Y3y3 Y2y2 Y1y1 Y0y0
      __m64 p4567 = _mm_unpackhi_pi8(px1, px2); //Y7y7 Y6y6 Y5y5 Y4y4

      __m64 p01 = _mm_unpacklo_pi8(p0123, zero); //00Y1 00y1 00Y0 00y0
      __m64 p23 = _mm_unpackhi_pi8(p0123, zero); //00Y3 00y3 00Y2 00y2
      __m64 p45 = _mm_unpacklo_pi8(p4567, zero); //00Y5 00y5 00Y4 00y4
      __m64 p67 = _mm_unpackhi_pi8(p4567, zero); //00Y7 00y7 00Y6 00y6

      p01 = _mm_madd_pi16(p01, mask);
      p23 = _mm_madd_pi16(p23, mask);
      p45 = _mm_madd_pi16(p45, mask);
      p67 = _mm_madd_pi16(p67, mask);

      p01 = _mm_add_pi32(p01, round_mask);
      p23 = _mm_add_pi32(p23, round_mask);
      p45 = _mm_add_pi32(p45, round_mask);
      p67 = _mm_add_pi32(p67, round_mask);

      p01 = _mm_srli_pi32(p01, 15);
      p23 = _mm_srli_pi32(p23, 15);
      p45 = _mm_srli_pi32(p45, 15);
      p67 = _mm_srli_pi32(p67, 15);

      p0123 = _mm_packs_pi32(p01, p23);
      p4567 = _mm_packs_pi32(p45, p67);

      __m64 result = _mm_packs_pu16(p0123, p4567);

      *reinterpret_cast<__m64*>(p1+x) = result;
    }

    for (int x = wMod8; x < rowsize; x++) {
      p1[x] = (p1[x]*invweight_i + p2[x]*weight_i + 16384) >> 15;
    }

    p1 += p1_pitch;
    p2 += p2_pitch;
  }
  _mm_empty();
}
#endif


template<typename pixel_t>
void weighted_merge_planar_c(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch,int rowsize, int height, float weight_f, int weight_i, int invweight_i) {
  AVS_UNUSED(weight_f);
  for (int y=0;y<height;y++) {
    for (size_t x=0;x<rowsize / sizeof(pixel_t);x++) {
      (reinterpret_cast<pixel_t *>(p1))[x] = ((reinterpret_cast<pixel_t *>(p1))[x]*invweight_i + (reinterpret_cast<const pixel_t *>(p2))[x]*weight_i + 32768) >> 16;
    }
    p2+=p2_pitch;
    p1+=p1_pitch;
  }
}

void weighted_merge_planar_c_float(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch, int rowsize, int height, float weight_f, int weight_i, int invweight_i) {
  AVS_UNUSED(weight_i);
  AVS_UNUSED(invweight_i);
  float invweight_f = 1.0f - weight_f;
  size_t rs = rowsize / sizeof(float);

  for (int y = 0; y < height; ++y) {
    for (size_t x = 0; x < rs; ++x) {
      reinterpret_cast<float *>(p1)[x] = (reinterpret_cast<float *>(p1)[x] * invweight_f + reinterpret_cast<const float *>(p2)[x] * weight_f);
    }
    p1 += p1_pitch;
    p2 += p2_pitch;
  }
}
//
//
///********************************************************************
//***** Declare index of new filters for Avisynth's filter engine *****
//********************************************************************/
//extern const AVSFunction Merge_filters[] = {
//  { "Merge",       BUILTIN_FUNC_PREFIX, "cc[weight]f", MergeAll::Create },  // src, src2, weight
//  { "MergeChroma", BUILTIN_FUNC_PREFIX, "cc[weight]f", MergeChroma::Create },  // src, chroma src, weight
//  { "MergeChroma", BUILTIN_FUNC_PREFIX, "cc[chromaweight]f", MergeChroma::Create },  // Legacy!
//  { "MergeLuma",   BUILTIN_FUNC_PREFIX, "cc[weight]f", MergeLuma::Create },      // src, luma src, weight
//  { "MergeLuma",   BUILTIN_FUNC_PREFIX, "cc[lumaweight]f", MergeLuma::Create },      // Legacy!
//  { 0 }
//};

// also returns the proper integer weight/inverse weight for 8-16 bits
MergeFuncPtr getMergeFunc(int bits_per_pixel, int cpuFlags, BYTE *srcp, const BYTE *otherp, float weight_f, int &weight_i, int &invweight_i)
{
  const int pixelsize = bits_per_pixel == 8 ? 1 : (bits_per_pixel == 32 ? 4 : 2);

  // SIMD 8-16 bit: bitshift 15 integer arithmetic
  // C    8-16 bit: bitshift 16 integer arithmetic
  // SIMD/C Float: original float weight

  // set basic 8-16bit SIMD
  weight_i = (int)(weight_f * 32767.0f + 0.5f);
  invweight_i = 32767 - weight_i;

  if (pixelsize == 1) {
    if ((cpuFlags & CPUF_AVX2) && IsPtrAligned(srcp, 32) && IsPtrAligned(otherp, 32))
      return &weighted_merge_planar_avx2;
    if ((cpuFlags & CPUF_SSE2) && IsPtrAligned(srcp, 16) && IsPtrAligned(otherp, 16))
      return &weighted_merge_planar_sse2;
#ifdef X86_32
    if (cpuFlags & CPUF_MMX)
      return &weighted_merge_planar_mmx;
#endif
    // C: different scale!
    weight_i = (int)(weight_f * 65535.0f + 0.5f);
    invweight_i = 65535 - weight_i;
    return &weighted_merge_planar_c<uint8_t>;
  }
  if (pixelsize == 2) {
    if ((cpuFlags & CPUF_AVX2) && IsPtrAligned(srcp, 32) && IsPtrAligned(otherp, 32))
    {
      if (bits_per_pixel == 16)
        return &weighted_merge_planar_uint16_avx2<false>;
      return &weighted_merge_planar_uint16_avx2<true>;
    }
    if ((cpuFlags & CPUF_SSE2) && IsPtrAligned(srcp, 16) && IsPtrAligned(otherp, 16))
    {
      if (bits_per_pixel == 16)
        return &weighted_merge_planar_uint16_sse2<false>;
      return &weighted_merge_planar_uint16_sse2<true>;
    }
    // C: different scale!
    weight_i = (int)(weight_f * 65535.0f + 0.5f);
    invweight_i = 65535 - weight_i;
    return &weighted_merge_planar_c<uint16_t>;
  }

  // pixelsize == 4
  if ((cpuFlags & CPUF_SSE2) && IsPtrAligned(srcp, 16) && IsPtrAligned(otherp, 16))
    return &weighted_merge_planar_sse2_float;
  return &weighted_merge_planar_c_float;
}

static void merge_plane(BYTE* srcp, const BYTE* otherp, int src_pitch, int other_pitch, int src_rowsize, int src_height, float weight, int pixelsize, int bits_per_pixel, IScriptEnvironment *env) {
  if ((weight > 0.4961f) && (weight < 0.5039f))
  {
    //average of two planes
    if (pixelsize != 4) // 1 or 2
    {
      if ((env->GetCPUFlags() & CPUF_AVX2) && IsPtrAligned(srcp, 32) && IsPtrAligned(otherp, 32)) {
        if(pixelsize==1)
          average_plane_avx2<uint8_t>(srcp, otherp, src_pitch, other_pitch, src_rowsize, src_height);
        else // pixel_size==2
          average_plane_avx2<uint16_t>(srcp, otherp, src_pitch, other_pitch, src_rowsize, src_height);
      }
      else if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16) && IsPtrAligned(otherp, 16)) {
        if(pixelsize==1)
          average_plane_sse2<uint8_t>(srcp, otherp, src_pitch, other_pitch, src_rowsize, src_height);
        else // pixel_size==2
          average_plane_sse2<uint16_t>(srcp, otherp, src_pitch, other_pitch, src_rowsize, src_height);
      }
      else
#ifdef X86_32
        if (env->GetCPUFlags() & CPUF_INTEGER_SSE) {
          if (pixelsize == 1)
            average_plane_isse<uint8_t>(srcp, otherp, src_pitch, other_pitch, src_rowsize, src_height);
          else // pixel_size==2
            average_plane_isse<uint16_t>(srcp, otherp, src_pitch, other_pitch, src_rowsize, src_height);
        }
        else
#endif
        {
          if (pixelsize == 1)
            average_plane_c<uint8_t>(srcp, otherp, src_pitch, other_pitch, src_rowsize, src_height);
          else // pixel_size==2
            average_plane_c<uint16_t>(srcp, otherp, src_pitch, other_pitch, src_rowsize, src_height);
        }
    }
    else // if (pixelsize == 4)
    {
      if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16) && IsPtrAligned(otherp, 16))
        average_plane_sse2_float(srcp, otherp, src_pitch, other_pitch, src_rowsize, src_height);
      else
        average_plane_c_float(srcp, otherp, src_pitch, other_pitch, src_rowsize, src_height);
    }

  } 
  else
  {
    int weight_i;
    int invweight_i;
    MergeFuncPtr weighted_merge_planar = getMergeFunc(bits_per_pixel, env->GetCPUFlags(), srcp, otherp, weight, /*out*/weight_i, /*out*/invweight_i);
    weighted_merge_planar(srcp, otherp, src_pitch, other_pitch, src_rowsize, src_height, weight, weight_i, invweight_i);
  }
}

/****************************
******   Merge Chroma   *****
****************************/

MergeChroma::MergeChroma(PClip _child, PClip _clip, float _weight, IScriptEnvironment* env)
  : GenericVideoFilter(_child), clip(_clip), weight(_weight)
{
  const VideoInfo& vi2 = clip->GetVideoInfo();

  if (!(vi.IsYUV() || vi.IsYUVA()) || !(vi2.IsYUV() || vi2.IsYUVA()))
    env->ThrowError("MergeChroma: YUV data only (no RGB); use ConvertToYUY2, ConvertToYV12/16/24 or ConvertToYUVxxx");

  if (!(vi.IsSameColorspace(vi2)))
    env->ThrowError("MergeChroma: YUV images must have same data type.");

  if (vi.width!=vi2.width || vi.height!=vi2.height)
    env->ThrowError("MergeChroma: Images must have same width and height!");

  if (weight<0.0f) weight=0.0f;
  if (weight>1.0f) weight=1.0f;

  pixelsize = vi.ComponentSize();
  bits_per_pixel = vi.BitsPerComponent();
}


PVideoFrame __stdcall MergeChroma::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);

  if (weight<0.0039f) return src;

  PVideoFrame chroma = clip->GetFrame(n, env);

  int h = src->GetHeight();
  int w = src->GetRowSize(); // width in pixels

  if (weight<0.9961f) {
    if (vi.IsYUY2()) {
      env->MakeWritable(&src);
      BYTE* srcp = src->GetWritePtr();
      const BYTE* chromap = chroma->GetReadPtr();

      int src_pitch = src->GetPitch(); 
      int chroma_pitch = chroma->GetPitch();

      if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16) && IsPtrAligned(chromap, 16))
      {
        weighted_merge_chroma_yuy2_sse2(srcp,chromap,src_pitch,chroma_pitch,w,h,(int)(weight*32768.0f),32768-(int)(weight*32768.0f));
      }
      else
#ifdef X86_32
      if (env->GetCPUFlags() & CPUF_MMX)
      {
        weighted_merge_chroma_yuy2_mmx(srcp,chromap,src_pitch,chroma_pitch,w,h,(int)(weight*32768.0f),32768-(int)(weight*32768.0f));
      }
      else
#endif
      {
        weighted_merge_chroma_yuy2_c(srcp,chromap,src_pitch,chroma_pitch,w,h,(int)(weight*32768.0f),32768-(int)(weight*32768.0f));
      }
    } else {  // Planar YUV
      env->MakeWritable(&src);
      src->GetWritePtr(PLANAR_Y); //Must be requested

      BYTE* srcpU = (BYTE*)src->GetWritePtr(PLANAR_U);
      BYTE* chromapU = (BYTE*)chroma->GetReadPtr(PLANAR_U);
      BYTE* srcpV = (BYTE*)src->GetWritePtr(PLANAR_V);
      BYTE* chromapV = (BYTE*)chroma->GetReadPtr(PLANAR_V);
      int src_pitch_uv = src->GetPitch(PLANAR_U);
      int chroma_pitch_uv = chroma->GetPitch(PLANAR_U);
      int src_rowsize_u = src->GetRowSize(PLANAR_U_ALIGNED);
      int src_rowsize_v = src->GetRowSize(PLANAR_V_ALIGNED);
      int src_height_uv = src->GetHeight(PLANAR_U);

      merge_plane(srcpU, chromapU, src_pitch_uv, chroma_pitch_uv, src_rowsize_u, src_height_uv, weight, pixelsize, bits_per_pixel, env);
      merge_plane(srcpV, chromapV, src_pitch_uv, chroma_pitch_uv, src_rowsize_v, src_height_uv, weight, pixelsize, bits_per_pixel, env);

      if(vi.IsYUVA())
        merge_plane(src->GetWritePtr(PLANAR_A), chroma->GetReadPtr(PLANAR_A), src->GetPitch(PLANAR_A), chroma->GetPitch(PLANAR_A),
          src->GetRowSize(PLANAR_A_ALIGNED), src->GetHeight(PLANAR_A), weight, pixelsize, bits_per_pixel, env);
    }
  } else { // weight == 1.0
    if (vi.IsYUY2()) {
      const BYTE* srcp = src->GetReadPtr();
      env->MakeWritable(&chroma);
      BYTE* chromap = chroma->GetWritePtr();

      int src_pitch = src->GetPitch();  
      int chroma_pitch = chroma->GetPitch(); 

      if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(chromap, 16) && IsPtrAligned(srcp, 16))
      {
        replace_luma_yuy2_sse2(chromap,srcp,chroma_pitch,src_pitch,w,h);  // Just swap luma/chroma
      }
      else
#ifdef X86_32
      if (env->GetCPUFlags() & CPUF_MMX)
      {
        replace_luma_yuy2_mmx(chromap,srcp,chroma_pitch,src_pitch,w,h);  // Just swap luma/chroma
      }
      else
#endif
      {
        replace_luma_yuy2_c(chromap,srcp,chroma_pitch,src_pitch,w,h);  // Just swap luma/chroma
      }

      return chroma;
    } else {
      if (src->IsWritable()) {
        src->GetWritePtr(PLANAR_Y); //Must be requested
        env->BitBlt(src->GetWritePtr(PLANAR_U),src->GetPitch(PLANAR_U),chroma->GetReadPtr(PLANAR_U),chroma->GetPitch(PLANAR_U),chroma->GetRowSize(PLANAR_U),chroma->GetHeight(PLANAR_U));
        env->BitBlt(src->GetWritePtr(PLANAR_V),src->GetPitch(PLANAR_V),chroma->GetReadPtr(PLANAR_V),chroma->GetPitch(PLANAR_V),chroma->GetRowSize(PLANAR_V),chroma->GetHeight(PLANAR_V));
        if(vi.IsYUVA())
          env->BitBlt(src->GetWritePtr(PLANAR_A),src->GetPitch(PLANAR_A),chroma->GetReadPtr(PLANAR_A),chroma->GetPitch(PLANAR_A),chroma->GetRowSize(PLANAR_A),chroma->GetHeight(PLANAR_A));
      }
      else { // avoid the cost of 2 chroma blits
        PVideoFrame dst = env->NewVideoFrame(vi);
        
        env->BitBlt(dst->GetWritePtr(PLANAR_Y),dst->GetPitch(PLANAR_Y),src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y),src->GetRowSize(PLANAR_Y),src->GetHeight(PLANAR_Y));
        env->BitBlt(dst->GetWritePtr(PLANAR_U),dst->GetPitch(PLANAR_U),chroma->GetReadPtr(PLANAR_U),chroma->GetPitch(PLANAR_U),chroma->GetRowSize(PLANAR_U),chroma->GetHeight(PLANAR_U));
        env->BitBlt(dst->GetWritePtr(PLANAR_V),dst->GetPitch(PLANAR_V),chroma->GetReadPtr(PLANAR_V),chroma->GetPitch(PLANAR_V),chroma->GetRowSize(PLANAR_V),chroma->GetHeight(PLANAR_V));
        if(vi.IsYUVA())
          env->BitBlt(dst->GetWritePtr(PLANAR_A),dst->GetPitch(PLANAR_A),chroma->GetReadPtr(PLANAR_A),chroma->GetPitch(PLANAR_A),chroma->GetRowSize(PLANAR_A),chroma->GetHeight(PLANAR_A));

        return dst;
      }
    }
  }
  return src;
}


AVSValue __cdecl MergeChroma::Create(AVSValue args, void* , IScriptEnvironment* env)
{
  return new MergeChroma(args[0].AsClip(), args[1].AsClip(), (float)args[2].AsFloat(1.0f), env);
}


/**************************
******   Merge Luma   *****
**************************/


MergeLuma::MergeLuma(PClip _child, PClip _clip, float _weight, IScriptEnvironment* env)
  : GenericVideoFilter(_child), clip(_clip), weight(_weight)
{
  const VideoInfo& vi2 = clip->GetVideoInfo();

  if (!(vi.IsYUV() || vi.IsYUVA()) || !(vi2.IsYUV() || vi2.IsYUVA()))
    env->ThrowError("MergeLuma: YUV data only (no RGB); use ConvertToYUY2, ConvertToYV12/16/24 or ConvertToYUVxxx");

  pixelsize = vi.ComponentSize();
  bits_per_pixel = vi.BitsPerComponent();

  if (!vi.IsSameColorspace(vi2)) {  // Since this is luma we allow all planar formats to be merged.
    if (!(vi.IsPlanar() && vi2.IsPlanar())) {
      env->ThrowError("MergeLuma: YUV data is not same type. YUY2 and planar images doesn't mix.");
    }
    if (pixelsize != vi2.ComponentSize()) {
      env->ThrowError("MergeLuma: YUV data bit depth is not same.");
    }
  }

  if (vi.width!=vi2.width || vi.height!=vi2.height)
    env->ThrowError("MergeLuma: Images must have same width and height!");

  if (weight<0.0f) weight=0.0f;
  if (weight>1.0f) weight=1.0f;

}


PVideoFrame __stdcall MergeLuma::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);

  if (weight<0.0039f) return src;

  PVideoFrame luma = clip->GetFrame(n, env);

  if (vi.IsYUY2()) {
    env->MakeWritable(&src);
    BYTE* srcp = src->GetWritePtr();
    const BYTE* lumap = luma->GetReadPtr();

    int isrc_pitch = src->GetPitch(); 
    int iluma_pitch = luma->GetPitch();

    int h = src->GetHeight();
    int w = src->GetRowSize();

    if (weight<0.9961f)
    {
      if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16) && IsPtrAligned(lumap, 16))
      {
        weighted_merge_luma_yuy2_sse2(srcp, lumap, isrc_pitch, iluma_pitch, w, h, (int)(weight*32768.0f), 32768-(int)(weight*32768.0f));
      }
      else
#ifdef X86_32
      if (env->GetCPUFlags() & CPUF_MMX)
      {
        weighted_merge_luma_yuy2_mmx(srcp, lumap, isrc_pitch, iluma_pitch, w, h, (int)(weight*32768.0f), 32768-(int)(weight*32768.0f));
      }
      else
#endif
      {
        weighted_merge_luma_yuy2_c(srcp, lumap, isrc_pitch, iluma_pitch, w, h, (int)(weight*32768.0f), 32768-(int)(weight*32768.0f));
      }
    }
    else
    {
      if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16) && IsPtrAligned(lumap, 16))
      {
        replace_luma_yuy2_sse2(srcp,lumap,isrc_pitch,iluma_pitch,w,h);
      }
      else
#ifdef X86_32
      if (env->GetCPUFlags() & CPUF_MMX)
      {
        replace_luma_yuy2_mmx(srcp,lumap,isrc_pitch,iluma_pitch,w,h);
      }
      else
#endif
      {
        replace_luma_yuy2_c(srcp,lumap,isrc_pitch,iluma_pitch,w,h);
      }
    }
    return src;
  }  // Planar
  if (weight>0.9961f) {
    // 2nd clip weight is almost 100%: no merge, just copy
    const VideoInfo& vi2 = clip->GetVideoInfo();
    if (luma->IsWritable() && vi.IsSameColorspace(vi2)) {
      if (luma->GetRowSize(PLANAR_U)) {
        luma->GetWritePtr(PLANAR_Y); //Must be requested BUT only if we actually do something
        env->BitBlt(luma->GetWritePtr(PLANAR_U),luma->GetPitch(PLANAR_U),src->GetReadPtr(PLANAR_U),src->GetPitch(PLANAR_U),src->GetRowSize(PLANAR_U),src->GetHeight(PLANAR_U));
        env->BitBlt(luma->GetWritePtr(PLANAR_V),luma->GetPitch(PLANAR_V),src->GetReadPtr(PLANAR_V),src->GetPitch(PLANAR_V),src->GetRowSize(PLANAR_V),src->GetHeight(PLANAR_V));
      }
      if (luma->GetPitch(PLANAR_A)) // copy Alpha if exists
        env->BitBlt(luma->GetWritePtr(PLANAR_A),luma->GetPitch(PLANAR_A),src->GetReadPtr(PLANAR_A),src->GetPitch(PLANAR_A),src->GetRowSize(PLANAR_A),src->GetHeight(PLANAR_A));

      return luma;
    }
    else { // avoid the cost of 2 chroma blits
      PVideoFrame dst = env->NewVideoFrame(vi);
      
      env->BitBlt(dst->GetWritePtr(PLANAR_Y),dst->GetPitch(PLANAR_Y),luma->GetReadPtr(PLANAR_Y),luma->GetPitch(PLANAR_Y),luma->GetRowSize(PLANAR_Y),luma->GetHeight(PLANAR_Y));
      if (src->GetRowSize(PLANAR_U) && dst->GetRowSize(PLANAR_U)) {
        env->BitBlt(dst->GetWritePtr(PLANAR_U),dst->GetPitch(PLANAR_U),src->GetReadPtr(PLANAR_U),src->GetPitch(PLANAR_U),src->GetRowSize(PLANAR_U),src->GetHeight(PLANAR_U));
        env->BitBlt(dst->GetWritePtr(PLANAR_V),dst->GetPitch(PLANAR_V),src->GetReadPtr(PLANAR_V),src->GetPitch(PLANAR_V),src->GetRowSize(PLANAR_V),src->GetHeight(PLANAR_V));
      }
      if (dst->GetPitch(PLANAR_A) && src->GetPitch(PLANAR_A)) // copy Alpha if in both clip exists
        env->BitBlt(dst->GetWritePtr(PLANAR_A),dst->GetPitch(PLANAR_A),src->GetReadPtr(PLANAR_A),src->GetPitch(PLANAR_A),src->GetRowSize(PLANAR_A),src->GetHeight(PLANAR_A));

      return dst;
    }
  } else { // weight <= 0.9961f
    env->MakeWritable(&src);
    BYTE* srcpY = (BYTE*)src->GetWritePtr(PLANAR_Y);
    BYTE* lumapY = (BYTE*)luma->GetReadPtr(PLANAR_Y);
    int src_pitch = src->GetPitch(PLANAR_Y);
    int luma_pitch = luma->GetPitch(PLANAR_Y);
    int src_rowsize = src->GetRowSize(PLANAR_Y);
    int src_height = src->GetHeight(PLANAR_Y);

    merge_plane(srcpY, lumapY, src_pitch, luma_pitch, src_rowsize, src_height, weight, pixelsize, bits_per_pixel, env);
  }

  return src;
}


AVSValue __cdecl MergeLuma::Create(AVSValue args, void* , IScriptEnvironment* env)
{
  return new MergeLuma(args[0].AsClip(), args[1].AsClip(), (float)args[2].AsFloat(1.0f), env);
}


/*************************
******   Merge All   *****
*************************/


MergeAll::MergeAll(PClip _child, PClip _clip, float _weight, IScriptEnvironment* env)
  : GenericVideoFilter(_child), clip(_clip), weight(_weight)
{
  const VideoInfo& vi2 = clip->GetVideoInfo();

  if (!vi.IsSameColorspace(vi2))
    env->ThrowError("Merge: Pixel types are not the same. Both must be the same.");

  if (vi.width!=vi2.width || vi.height!=vi2.height)
    env->ThrowError("Merge: Images must have same width and height!");

  pixelsize = vi.ComponentSize();
  bits_per_pixel = vi.BitsPerComponent();

  if (weight<0.0f) weight=0.0f;
  if (weight>1.0f) weight=1.0f;
}


PVideoFrame __stdcall MergeAll::GetFrame(int n, IScriptEnvironment* env)
{
  if (weight<0.0039f) return child->GetFrame(n, env);
  if (weight>0.9961f) return clip->GetFrame(n, env);

  PVideoFrame src  = child->GetFrame(n, env);
  PVideoFrame src2 =  clip->GetFrame(n, env);

  env->MakeWritable(&src);
  BYTE* srcp  = src->GetWritePtr();
  const BYTE* srcp2 = src2->GetReadPtr();

  const int src_pitch = src->GetPitch();
  const int src_rowsize = src->GetRowSize();

  merge_plane(srcp, srcp2, src_pitch, src2->GetPitch(), src_rowsize, src->GetHeight(), weight, pixelsize, bits_per_pixel, env);

  if (vi.IsPlanar()) {
    const int planesYUV[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A};
    const int planesRGB[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A};
    const int *planes = (vi.IsYUV() || vi.IsYUVA()) ? planesYUV : planesRGB;
    // first plane is already processed
    for (int p = 1; p < vi.NumComponents(); p++) {
      const int plane = planes[p];
      merge_plane(src->GetWritePtr(plane), src2->GetReadPtr(plane), src->GetPitch(plane), src2->GetPitch(plane), src->GetRowSize(plane), src->GetHeight(plane), weight, pixelsize, bits_per_pixel, env);
    }
  }

  return src;
}


AVSValue __cdecl MergeAll::Create(AVSValue args, void* , IScriptEnvironment* env)
{
  return new MergeAll(args[0].AsClip(), args[1].AsClip(), (float)args[2].AsFloat(0.5f), env);
}
