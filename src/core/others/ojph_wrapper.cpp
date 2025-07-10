/****************************************************************************/
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
/****************************************************************************/
// This file is part of the OpenJPH software implementation.
// File: ojph_wrapper.cpp
// Author: Aous Naman
// Date: 22 October 2019
/****************************************************************************/

#include <exception>

#include "ojph_arch.h"
#include "ojph_file.h"
#include "ojph_mem.h"
#include "ojph_params.h"
#include "ojph_codestream.h"

struct j2k_struct
{
  ojph::codestream codestream;
  ojph::mem_infile mem_file;
};

j2k_struct* cpp_create_j2c_data(void)
{
  return new j2k_struct;
}

void cpp_init_j2c_data(j2k_struct *j2c, const uint8_t *data, size_t size)
{
  try {
    j2c->mem_file.open(data, size);
    j2c->codestream.read_headers(&j2c->mem_file);
  }
  catch (const std::exception& e)
  {
    const char *p = e.what();
    if (strncmp(p, "ojph error", 10) != 0)
      printf("%s\n", p);
  }
}

void cpp_parse_j2c_data(j2k_struct *j2c)
{
  try {
    j2c->codestream.set_planar(false);
    j2c->codestream.create();
  }
  catch (const std::exception& e)
  {
    const char *p = e.what();
    if (strncmp(p, "ojph error", 10) != 0)
      printf("%s\n", p);
  }
}

void cpp_release_j2c_data(j2k_struct* j2c)
{
  if (j2c)
  {
    delete j2c;
  }
}

signed int* cpp_pull_j2c_line(j2k_struct* j2c)
{
  try {
    ojph::ui32 comp_num;
    ojph::line_buf *line = j2c->codestream.pull(comp_num);
    return line->i32;
  }
  catch (const std::exception& e)
  {
    const char *p = e.what();
    if (strncmp(p, "ojph error", 10) != 0)
      printf("%s\n", p);
  }
  return NULL;
}

void cpp_restrict_input_resolution(j2k_struct* j2c,
                                   int skipped_res_for_read,
                                   int skipped_res_for_recon)
{
  j2c->codestream.restrict_input_resolution(skipped_res_for_read,
                                            skipped_res_for_recon);
}

extern "C"
{
  j2k_struct* ojph_create_j2c_data(void)
  {
    return cpp_create_j2c_data();
  }

  void ojph_init_j2c_data(j2k_struct *j2c, const uint8_t *data, size_t size)
  {
    cpp_init_j2c_data(j2c, data, size);
  }

  int ojph_get_j2c_width(j2k_struct* j2c, int comp_num)
  {
    ojph::param_siz siz = j2c->codestream.access_siz();
    return siz.get_recon_width(comp_num);
  }

  int ojph_get_j2c_height(j2k_struct* j2c, int comp_num)
  {
    ojph::param_siz siz = j2c->codestream.access_siz();
    return siz.get_recon_height(comp_num);
  }

  int ojph_get_j2c_bit_depth(j2k_struct* j2c, int comp_num)
  {
    ojph::param_siz siz = j2c->codestream.access_siz();
    if (comp_num >= 0 && comp_num < (int)siz.get_num_components())
      return siz.get_bit_depth(comp_num);
    else
      return -1;
  }

  int ojph_get_j2c_is_signed(j2k_struct* j2c, int comp_num)
  {
    ojph::param_siz siz = j2c->codestream.access_siz();
    if (comp_num >= 0 && comp_num < (int)siz.get_num_components())
      return siz.is_signed(comp_num) ? 1 : 0;
    else
      return -1;
  }

  int ojph_get_j2c_num_components(j2k_struct* j2c)
  {
    ojph::param_siz siz = j2c->codestream.access_siz();
    return siz.get_num_components();
  }

  int ojph_get_j2c_downsampling_x(j2k_struct* j2c, int comp_num)
  {
    ojph::param_siz siz = j2c->codestream.access_siz();
    if (comp_num >= 0 && comp_num < (int)siz.get_num_components())
    { return siz.get_downsampling(comp_num).x; }
    else
    { return -1; }
  }

  int ojph_get_j2c_downsampling_y(j2k_struct* j2c, int comp_num)
  {
    ojph::param_siz siz = j2c->codestream.access_siz();
    if (comp_num >= 0 && comp_num < (int)siz.get_num_components())
    { return siz.get_downsampling(comp_num).y; }
    else
    { return -1; }
  }

  void ojph_parse_j2c_data(j2k_struct *j2c)
  {
    cpp_parse_j2c_data(j2c);
  }

  void ojph_restrict_input_resolution(j2k_struct* j2c,
                                 int skipped_res_for_read,
                                 int skipped_res_for_recon)
  {
    cpp_restrict_input_resolution(j2c, skipped_res_for_read,
                                  skipped_res_for_recon);
  }

  void ojph_enable_resilience(j2k_struct* j2c)
  {
    j2c->codestream.enable_resilience();
  }

  signed int* ojph_pull_j2c_line(j2k_struct* j2c)
  {
    return cpp_pull_j2c_line(j2c);
  }

  void ojph_pull_j2c_lines(j2k_struct* j2c, int n, signed int* dst, int dst_size)
  {
    int stride = j2c->codestream.access_siz().get_recon_width(0) * sizeof(ojph::ui32);
    // printf("stride: %d\n", stride);

    // signed int *dst_limit = dst + dst_size;
    // for (int i = 0; i < n; ++i)
    // {
    //   printf("%d\n", i);
    //   if (dst >= dst_limit)
    //     break;
    //   line = j2c->codestream.pull(comp_num);
    //   if (!line)
    //     break;
    //   memcpy(dst, line->i32, stride);
    //   dst += stride;
    // }

    if (!j2c->codestream.is_planar())
    {
      ojph::param_siz siz = j2c->codestream.access_siz();
      for (ojph::ui32 i = 0; i < n; ++i)
      {
        for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
        {
          ojph::ui32 comp_num;
          ojph::line_buf *line = j2c->codestream.pull(comp_num);
          assert(comp_num == c);
          // printf("line->size: %d\n", line->size * (int)sizeof(ojph::ui32));
          memcpy(dst, line->i32, stride); // base->write(line, comp_num);
          dst += stride;
        }
      }
    }
    return ;
  }

  void ojph_pull_j2c_rgba(j2k_struct* j2c, int n, int32_t* dst, int dst_size)
  {
    ojph::param_siz siz = j2c->codestream.access_siz();
    int ncomps = siz.get_num_components();
    int width = siz.get_recon_width(0); // assuming all components have the same width

    // printf("j2c->codestream.is_planar(): %d\n", j2c->codestream.is_planar());

    if (!j2c->codestream.is_planar())
    {
      int* temp = new int[ncomps * width];
      int32_t* dst_limit = dst + dst_size / sizeof(int32_t);
      int stride = ncomps * width * sizeof(int);
      // printf("stride: %d\n", stride);
      for (int i = 0; i < n; ++i)
      {
        // printf("i: %d\n", i);
        for (int c = 0; c < ncomps; ++c)
        {
          ojph::ui32 comp_num;
          ojph::line_buf *line = j2c->codestream.pull(comp_num);
          // printf("comp_num: %d\n", comp_num);
          assert(comp_num == c);

          // interleave channels into temp and clamp to 8-bit range
          for (int j = 0; j < width; ++j)
          {
            int val = line->i32[j];
            int min, max;
            min = 0;
            max = 255;
            val = val >= min ? val : min;
            val = val <= max ? val : max;
            temp[c + j * ncomps] = val;
          }
        }
        // copy one row at a time to dst
        memcpy(dst, temp, stride);
        dst += stride / sizeof(int);
        assert(dst <= dst_limit);
      }
      delete[] temp;
    }
    return ;
  }

  // void ojph_pull_j2c_rgb(j2k_struct* j2c, int n, uint8_t* dst, int dst_size)
  // {
  //   ojph::ui32 ncomps = 4;
  //   int stride = j2c->codestream.access_siz().get_recon_width(0); // rgb is three channels

  //   if (!j2c->codestream.is_planar())
  //   {
  //     ojph::param_siz siz = j2c->codestream.access_siz();
  //     for (ojph::ui32 i = 0; i < n; ++i)
  //     {
  //       assert(ncomps == siz.get_num_components());
  //       for (ojph::ui32 c = 0; c < ncomps; ++c)
  //       {
  //         ojph::ui32 comp_num;
  //         ojph::line_buf *line = j2c->codestream.pull(comp_num);
  //         // assuming that lines are in the order r, g, b, a then cycle through
  //         // them but skip a and return in b, g, r order (works by accident?)
  //         if (c == 3)
  //         {
  //           continue;
  //         }
  //         for (ojph::ui32 j = c; j < stride; j=j+3)
  //         {
  //           dst[j] = (uint8_t)(line->i32[j] & 0xff);
  //         }
  //       }
  //       dst += stride;
  //     }
  //   }
  //   return ;
  // }

  void ojph_release_j2c_data(j2k_struct* j2c)
  {
    cpp_release_j2c_data(j2c);
  }
}

    // if (codestream.is_planar())
    // {
    //   ojph::param_siz siz = codestream.access_siz();
    //   for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
    //   {
    //     ojph::ui32 height = siz.get_recon_height(c);
    //     for (ojph::ui32 i = height; i > 0; --i)
    //     {
    //       ojph::ui32 comp_num;
    //       ojph::line_buf *line = codestream.pull(comp_num);
    //       assert(comp_num == c);
    //       base->write(line, comp_num);
    //     }
    //   }
    // }
    // else
    // {
    //   ojph::param_siz siz = codestream.access_siz();
    //   ojph::ui32 height = siz.get_recon_height(0);
    //   for (ojph::ui32 i = 0; i < height; ++i)
    //   {
    //     for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
    //     {
    //       ojph::ui32 comp_num;
    //       ojph::line_buf *line = codestream.pull(comp_num);
    //       assert(comp_num == c);
    //       base->write(line, comp_num);
    //     }
    //   }
    // }
