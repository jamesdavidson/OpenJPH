//***************************************************************************/
// This software is released under the 2-Clause BSD license, included
// below.
//
// Copyright (c) 2019, Aous Naman 
// Copyright (c) 2019, Kakadu Software Pty Ltd, Australia
// Copyright (c) 2019, The University of New South Wales, Australia
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//***************************************************************************/
// This file is part of the OpenJPH software implementation.
// File: ojph_transform_sse.cpp
// Author: Aous Naman
// Date: 28 August 2019
//***************************************************************************/

#include "ojph_arch.h"
#if defined(OJPH_ARCH_I386) || defined(OJPH_ARCH_X86_64)

#include <cstdio>
#include <xmmintrin.h>

#include "ojph_defs.h"
#include "ojph_mem.h"
#include "ojph_params.h"
#include "../codestream/ojph_params_local.h"

#include "ojph_transform.h"
#include "ojph_transform_local.h"

namespace ojph {
  namespace local {

    //////////////////////////////////////////////////////////////////////////
    static inline
    void sse_deinterleave32(float* dpl, float* dph, float* sp, int width)
    {
      for (; width > 0; width -= 8, sp += 8, dpl += 4, dph += 4)
      {
        __m128 a = _mm_load_ps(sp);
        __m128 b = _mm_load_ps(sp + 4);
        __m128 c = _mm_shuffle_ps(a, b, _MM_SHUFFLE(2, 0, 2, 0));
        __m128 d = _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 1, 3, 1));
        _mm_store_ps(dpl, c);
        _mm_store_ps(dph, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline
    void sse_interleave32(float* dp, float* spl, float* sph, int width)                      \
    {
      for (; width > 0; width -= 8, dp += 8, spl += 4, sph += 4)
      {
        __m128 a = _mm_load_ps(spl);
        __m128 b = _mm_load_ps(sph);
        __m128 c = _mm_unpacklo_ps(a, b);
        __m128 d = _mm_unpackhi_ps(a, b);
        _mm_store_ps(dp, c);
        _mm_store_ps(dp + 4, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    static inline void sse_multiply_const(float* p, float f, int width)
    {
      __m128 factor = _mm_set1_ps(f);
      for (; width > 0; width -= 4, p += 4)
      {
        __m128 s = _mm_load_ps(p);
        _mm_store_ps(p, _mm_mul_ps(factor, s));
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void sse_irv_vert_step(const lifting_step* s, const line_buf* sig, 
                           const line_buf* other, const line_buf* aug, 
                           ui32 repeat, bool synthesis)
    {
      float a = s->irv.Aatk;
      if (synthesis)
        a = -a;

      __m128 factor = _mm_set1_ps(a);

      float* dst = aug->f32;
      const float* src1 = sig->f32, * src2 = other->f32;
      int i = (int)repeat;
      for ( ; i > 0; i -= 4, dst += 4, src1 += 4, src2 += 4)
      {
        __m128 s1 = _mm_load_ps(src1);
        __m128 s2 = _mm_load_ps(src2);
        __m128 d  = _mm_load_ps(dst);
        d = _mm_add_ps(d, _mm_mul_ps(factor, _mm_add_ps(s1, s2)));
        _mm_store_ps(dst, d);
      }
    }

    //////////////////////////////////////////////////////////////////////////
    void sse_irv_vert_times_K(float K, const line_buf* aug, ui32 repeat)
    {
      sse_multiply_const(aug->f32, K, (int)repeat);
    }

    /////////////////////////////////////////////////////////////////////////
    void sse_irv_horz_ana(const param_atk* atk, const line_buf* ldst, 
                          const line_buf* hdst, const line_buf* src, 
                          ui32 width, bool even)
    {
      if (width > 1)
      {
        // split src into ldst and hdst
        {
          float* dpl = even ? ldst->f32 : hdst->f32;
          float* dph = even ? hdst->f32 : ldst->f32;
          float* sp = src->f32;
          int w = (int)width;
          sse_deinterleave32(dpl, dph, sp, w);
        }

        // the actual horizontal transform
        float* hp = hdst->f32, * lp = ldst->f32;
        ui32 l_width = (width + (even ? 1 : 0)) >> 1;  // low pass
        ui32 h_width = (width + (even ? 0 : 1)) >> 1;  // high pass
        ui32 num_steps = atk->get_num_steps();
        for (ui32 j = num_steps; j > 0; --j)
        {
          const lifting_step* s = atk->get_step(j - 1);
          const float a = s->irv.Aatk;

          // extension
          lp[-1] = lp[0];
          lp[l_width] = lp[l_width - 1];
          // lifting step
          const float* sp = lp;
          float* dp = hp;
          int i = (int)h_width;
          __m128 f = _mm_set1_ps(a);
          if (even)
          {
            for (; i > 0; i -= 4, sp += 4, dp += 4)
            {
              __m128 m = _mm_load_ps(sp);
              __m128 n = _mm_loadu_ps(sp + 1);
              __m128 p = _mm_load_ps(dp);
              p = _mm_add_ps(p, _mm_mul_ps(f, _mm_add_ps(m, n)));
              _mm_store_ps(dp, p);
            }
          }
          else
          {
            for (; i > 0; i -= 4, sp += 4, dp += 4)
            {
              __m128 m = _mm_load_ps(sp);
              __m128 n = _mm_loadu_ps(sp - 1);
              __m128 p = _mm_load_ps(dp);
              p = _mm_add_ps(p, _mm_mul_ps(f, _mm_add_ps(m, n)));
              _mm_store_ps(dp, p);
            }
          }

          // swap buffers
          float* t = lp; lp = hp; hp = t;
          even = !even;
          ui32 w = l_width; l_width = h_width; h_width = w;
        }

        { // multiply by K or 1/K
          float K = atk->get_K();
          float K_inv = 1.0f / K;
          sse_multiply_const(lp, K_inv, (int)l_width);
          sse_multiply_const(hp, K, (int)h_width);
        }
      }
      else {
        if (even)
          ldst->f32[0] = src->f32[0];
        else
          hdst->f32[0] = src->f32[0] * 2.0f;
      }
    }
    
    //////////////////////////////////////////////////////////////////////////
    void sse_irv_horz_syn(const param_atk* atk, const line_buf* dst, 
                          const line_buf* lsrc, const line_buf* hsrc, 
                          ui32 width, bool even)
    {
      if (width > 1)
      {
        bool ev = even;
        float* oth = hsrc->f32, * aug = lsrc->f32;
        ui32 aug_width = (width + (even ? 1 : 0)) >> 1;  // low pass
        ui32 oth_width = (width + (even ? 0 : 1)) >> 1;  // high pass

        { // multiply by K or 1/K
          float K = atk->get_K();
          float K_inv = 1.0f / K;
          sse_multiply_const(aug, K, (int)aug_width);
          sse_multiply_const(oth, K_inv, (int)oth_width);
        }

        // the actual horizontal transform
        ui32 num_steps = atk->get_num_steps();
        for (ui32 j = 0; j < num_steps; ++j)
        {
          const lifting_step* s = atk->get_step(j);
          const float a = s->irv.Aatk;

          // extension
          oth[-1] = oth[0];
          oth[oth_width] = oth[oth_width - 1];
          // lifting step
          const float* sp = oth;
          float* dp = aug;
          int i = (int)aug_width;
          __m128 f = _mm_set1_ps(a);
          if (ev)
          {
            for ( ; i > 0; i -= 4, sp += 4, dp += 4)
            {
              __m128 m = _mm_load_ps(sp);
              __m128 n = _mm_loadu_ps(sp - 1);
              __m128 p = _mm_load_ps(dp);
              p = _mm_sub_ps(p, _mm_mul_ps(f, _mm_add_ps(m, n)));
              _mm_store_ps(dp, p);
            }
          }
          else
          {
            for ( ; i > 0; i -= 4, sp += 4, dp += 4)
            {
              __m128 m = _mm_load_ps(sp);
              __m128 n = _mm_loadu_ps(sp + 1);
              __m128 p = _mm_load_ps(dp);
              p = _mm_sub_ps(p, _mm_mul_ps(f, _mm_add_ps(m, n)));
              _mm_store_ps(dp, p);
            }
          }

          // swap buffers
          float* t = aug; aug = oth; oth = t;
          ev = !ev;
          ui32 w = aug_width; aug_width = oth_width; oth_width = w;
        }

        // combine both lsrc and hsrc into dst
        {
          float* dp = dst->f32;
          float* spl = even ? lsrc->f32 : hsrc->f32;
          float* sph = even ? hsrc->f32 : lsrc->f32;
          int w = (int)width;
          sse_interleave32(dp, spl, sph, w);
        }
      }
      else {
        if (even)
          dst->f32[0] = lsrc->f32[0];
        else
          dst->f32[0] = hsrc->f32[0] * 0.5f;
      }
    }

  } // !local
} // !ojph

#endif
