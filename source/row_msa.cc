/*
 *  Copyright 2016 The LibYuv Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS. All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "libyuv/row.h"

// This module is for GCC MSA
#if !defined(LIBYUV_DISABLE_MSA) && defined(__mips_msa)
#include "libyuv/macros_msa.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

// Fill YUV -> RGB conversion constants into vectors
#define YUVTORGB_SETUP(yuvconst, ub, vr, ug, vg, bb, bg, br, yg) \
  {                                                              \
    ub = __msa_fill_w(yuvconst->kUVToB[0]);                      \
    vr = __msa_fill_w(yuvconst->kUVToR[1]);                      \
    ug = __msa_fill_w(yuvconst->kUVToG[0]);                      \
    vg = __msa_fill_w(yuvconst->kUVToG[1]);                      \
    bb = __msa_fill_w(yuvconst->kUVBiasB[0]);                    \
    bg = __msa_fill_w(yuvconst->kUVBiasG[0]);                    \
    br = __msa_fill_w(yuvconst->kUVBiasR[0]);                    \
    yg = __msa_fill_w(yuvconst->kYToRgb[0]);                     \
  }

// Load YUV 422 pixel data
#define READYUV422(psrc_y, psrc_u, psrc_v, out_y, out_u, out_v)  \
  {                                                              \
    uint64 y_m;                                                  \
    uint32 u_m, v_m;                                             \
    v4i32 zero_m = {0};                                          \
    y_m = LD(psrc_y);                                            \
    u_m = LW(psrc_u);                                            \
    v_m = LW(psrc_v);                                            \
    out_y = (v16u8)__msa_insert_d((v2i64)zero_m, 0, (int64)y_m); \
    out_u = (v16u8)__msa_insert_w(zero_m, 0, (int32)u_m);        \
    out_v = (v16u8)__msa_insert_w(zero_m, 0, (int32)v_m);        \
  }

// Convert 8 pixels of YUV 420 to RGB.
#define YUVTORGB(in_y, in_uv, ubvr, ugvg, bb, bg, br, yg, out_b, out_g, out_r) \
  {                                                                            \
    v8i16 vec0_m, vec1_m;                                                      \
    v4i32 reg0_m, reg1_m, reg2_m, reg3_m, reg4_m;                              \
    v4i32 reg5_m, reg6_m, reg7_m;                                              \
    v4i32 max = __msa_ldi_w(255);                                              \
    v16i8 zero = {0};                                                          \
                                                                               \
    vec0_m = (v8i16)__msa_ilvr_b((v16i8)in_y, (v16i8)in_y);                    \
    vec1_m = (v8i16)__msa_ilvr_b((v16i8)zero, (v16i8)in_uv);                   \
    reg0_m = (v4i32)__msa_ilvr_h((v8i16)zero, (v8i16)vec0_m);                  \
    reg1_m = (v4i32)__msa_ilvl_h((v8i16)zero, (v8i16)vec0_m);                  \
    reg2_m = (v4i32)__msa_ilvr_h((v8i16)zero, (v8i16)vec1_m);                  \
    reg3_m = (v4i32)__msa_ilvl_h((v8i16)zero, (v8i16)vec1_m);                  \
    reg0_m *= yg;                                                              \
    reg1_m *= yg;                                                              \
    reg2_m *= ubvr;                                                            \
    reg3_m *= ubvr;                                                            \
    reg0_m = __msa_srai_w(reg0_m, 16);                                         \
    reg1_m = __msa_srai_w(reg1_m, 16);                                         \
    reg4_m = __msa_dotp_s_w((v8i16)vec1_m, (v8i16)ugvg);                       \
    reg5_m = __msa_ilvev_w(reg2_m, reg2_m);                                    \
    reg6_m = __msa_ilvev_w(reg3_m, reg3_m);                                    \
    reg7_m = __msa_ilvr_w(reg4_m, reg4_m);                                     \
    reg2_m = __msa_ilvod_w(reg2_m, reg2_m);                                    \
    reg3_m = __msa_ilvod_w(reg3_m, reg3_m);                                    \
    reg4_m = __msa_ilvl_w(reg4_m, reg4_m);                                     \
    reg5_m = reg0_m - reg5_m;                                                  \
    reg6_m = reg1_m - reg6_m;                                                  \
    reg2_m = reg0_m - reg2_m;                                                  \
    reg3_m = reg1_m - reg3_m;                                                  \
    reg7_m = reg0_m - reg7_m;                                                  \
    reg4_m = reg1_m - reg4_m;                                                  \
    reg5_m += bb;                                                              \
    reg6_m += bb;                                                              \
    reg7_m += bg;                                                              \
    reg4_m += bg;                                                              \
    reg2_m += br;                                                              \
    reg3_m += br;                                                              \
    reg5_m = __msa_srai_w(reg5_m, 6);                                          \
    reg6_m = __msa_srai_w(reg6_m, 6);                                          \
    reg7_m = __msa_srai_w(reg7_m, 6);                                          \
    reg4_m = __msa_srai_w(reg4_m, 6);                                          \
    reg2_m = __msa_srai_w(reg2_m, 6);                                          \
    reg3_m = __msa_srai_w(reg3_m, 6);                                          \
    reg5_m = __msa_maxi_s_w(reg5_m, 0);                                        \
    reg6_m = __msa_maxi_s_w(reg6_m, 0);                                        \
    reg7_m = __msa_maxi_s_w(reg7_m, 0);                                        \
    reg4_m = __msa_maxi_s_w(reg4_m, 0);                                        \
    reg2_m = __msa_maxi_s_w(reg2_m, 0);                                        \
    reg3_m = __msa_maxi_s_w(reg3_m, 0);                                        \
    reg5_m = __msa_min_s_w(max, reg5_m);                                       \
    reg6_m = __msa_min_s_w(max, reg6_m);                                       \
    reg7_m = __msa_min_s_w(max, reg7_m);                                       \
    reg4_m = __msa_min_s_w(max, reg4_m);                                       \
    reg2_m = __msa_min_s_w(max, reg2_m);                                       \
    reg3_m = __msa_min_s_w(max, reg3_m);                                       \
    out_b = __msa_pckev_h((v8i16)reg6_m, (v8i16)reg5_m);                       \
    out_g = __msa_pckev_h((v8i16)reg4_m, (v8i16)reg7_m);                       \
    out_r = __msa_pckev_h((v8i16)reg3_m, (v8i16)reg2_m);                       \
  }

// Pack and Store 8 ARGB values.
#define STOREARGB(in0, in1, in2, in3, pdst_argb)           \
  {                                                        \
    v8i16 vec0_m, vec1_m;                                  \
    v16u8 dst0_m, dst1_m;                                  \
    vec0_m = (v8i16)__msa_ilvev_b((v16i8)in1, (v16i8)in0); \
    vec1_m = (v8i16)__msa_ilvev_b((v16i8)in3, (v16i8)in2); \
    dst0_m = (v16u8)__msa_ilvr_h(vec1_m, vec0_m);          \
    dst1_m = (v16u8)__msa_ilvl_h(vec1_m, vec0_m);          \
    ST_UB2(dst0_m, dst1_m, pdst_argb, 16);                 \
  }

void MirrorRow_MSA(const uint8* src, uint8* dst, int width) {
  int x;
  v16u8 src0, src1, src2, src3;
  v16u8 dst0, dst1, dst2, dst3;
  v16i8 shuffler = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  src += width - 64;

  for (x = 0; x < width; x += 64) {
    LD_UB4(src, 16, src3, src2, src1, src0);
    VSHF_B2_UB(src3, src3, src2, src2, shuffler, shuffler, dst3, dst2);
    VSHF_B2_UB(src1, src1, src0, src0, shuffler, shuffler, dst1, dst0);
    ST_UB4(dst0, dst1, dst2, dst3, dst, 16);
    dst += 64;
    src -= 64;
  }
}

void ARGBMirrorRow_MSA(const uint8* src, uint8* dst, int width) {
  int x;
  v16u8 src0, src1, src2, src3;
  v16u8 dst0, dst1, dst2, dst3;
  v16i8 shuffler = {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};
  src += width * 4 - 64;

  for (x = 0; x < width; x += 16) {
    LD_UB4(src, 16, src3, src2, src1, src0);
    VSHF_B2_UB(src3, src3, src2, src2, shuffler, shuffler, dst3, dst2);
    VSHF_B2_UB(src1, src1, src0, src0, shuffler, shuffler, dst1, dst0);
    ST_UB4(dst0, dst1, dst2, dst3, dst, 16);
    dst += 64;
    src -= 64;
  }
}

void I422ToYUY2Row_MSA(const uint8* src_y,
                       const uint8* src_u,
                       const uint8* src_v,
                       uint8* dst_yuy2,
                       int width) {
  int x;
  v16u8 src_u0, src_v0, src_y0, src_y1, vec_uv0, vec_uv1;
  v16u8 dst_yuy2_0, dst_yuy2_1, dst_yuy2_2, dst_yuy2_3;

  for (x = 0; x < width; x += 32) {
    src_u0 = LD_UB(src_u);
    src_v0 = LD_UB(src_v);
    LD_UB2(src_y, 16, src_y0, src_y1);
    ILVRL_B2_UB(src_v0, src_u0, vec_uv0, vec_uv1);
    ILVRL_B2_UB(vec_uv0, src_y0, dst_yuy2_0, dst_yuy2_1);
    ILVRL_B2_UB(vec_uv1, src_y1, dst_yuy2_2, dst_yuy2_3);
    ST_UB4(dst_yuy2_0, dst_yuy2_1, dst_yuy2_2, dst_yuy2_3, dst_yuy2, 16);
    src_u += 16;
    src_v += 16;
    src_y += 32;
    dst_yuy2 += 64;
  }
}

void I422ToUYVYRow_MSA(const uint8* src_y,
                       const uint8* src_u,
                       const uint8* src_v,
                       uint8* dst_uyvy,
                       int width) {
  int x;
  v16u8 src_u0, src_v0, src_y0, src_y1, vec_uv0, vec_uv1;
  v16u8 dst_uyvy0, dst_uyvy1, dst_uyvy2, dst_uyvy3;

  for (x = 0; x < width; x += 32) {
    src_u0 = LD_UB(src_u);
    src_v0 = LD_UB(src_v);
    LD_UB2(src_y, 16, src_y0, src_y1);
    ILVRL_B2_UB(src_v0, src_u0, vec_uv0, vec_uv1);
    ILVRL_B2_UB(src_y0, vec_uv0, dst_uyvy0, dst_uyvy1);
    ILVRL_B2_UB(src_y1, vec_uv1, dst_uyvy2, dst_uyvy3);
    ST_UB4(dst_uyvy0, dst_uyvy1, dst_uyvy2, dst_uyvy3, dst_uyvy, 16);
    src_u += 16;
    src_v += 16;
    src_y += 32;
    dst_uyvy += 64;
  }
}

void I422ToARGBRow_MSA(const uint8* src_y,
                       const uint8* src_u,
                       const uint8* src_v,
                       uint8* rgb_buf,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  int x;
  v16u8 src0, src1, src2;
  v8i16 vec0, vec1, vec2;
  v4i32 vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg, vec_br, vec_yg;
  v4i32 vec_ubvr, vec_ugvg;
  v16u8 const_255 = (v16u8)__msa_ldi_b(255);

  YUVTORGB_SETUP(yuvconstants, vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg,
                 vec_br, vec_yg);
  vec_ubvr = __msa_ilvr_w(vec_vr, vec_ub);
  vec_ugvg = (v4i32)__msa_ilvev_h((v8i16)vec_vg, (v8i16)vec_ug);

  for (x = 0; x < width; x += 8) {
    READYUV422(src_y, src_u, src_v, src0, src1, src2);
    src1 = (v16u8)__msa_ilvr_b((v16i8)src2, (v16i8)src1);
    YUVTORGB(src0, src1, vec_ubvr, vec_ugvg, vec_bb, vec_bg, vec_br, vec_yg,
             vec0, vec1, vec2);
    STOREARGB(vec0, vec1, vec2, const_255, rgb_buf);
    src_y += 8;
    src_u += 4;
    src_v += 4;
    rgb_buf += 32;
  }
}

void I422ToRGBARow_MSA(const uint8* src_y,
                       const uint8* src_u,
                       const uint8* src_v,
                       uint8* rgb_buf,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  int x;
  v16u8 src0, src1, src2;
  v8i16 vec0, vec1, vec2;
  v4i32 vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg, vec_br, vec_yg;
  v4i32 vec_ubvr, vec_ugvg;
  v16u8 const_255 = (v16u8)__msa_ldi_b(255);

  YUVTORGB_SETUP(yuvconstants, vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg,
                 vec_br, vec_yg);
  vec_ubvr = __msa_ilvr_w(vec_vr, vec_ub);
  vec_ugvg = (v4i32)__msa_ilvev_h((v8i16)vec_vg, (v8i16)vec_ug);

  for (x = 0; x < width; x += 8) {
    READYUV422(src_y, src_u, src_v, src0, src1, src2);
    src1 = (v16u8)__msa_ilvr_b((v16i8)src2, (v16i8)src1);
    YUVTORGB(src0, src1, vec_ubvr, vec_ugvg, vec_bb, vec_bg, vec_br, vec_yg,
             vec0, vec1, vec2);
    STOREARGB(const_255, vec0, vec1, vec2, rgb_buf);
    src_y += 8;
    src_u += 4;
    src_v += 4;
    rgb_buf += 32;
  }
}

void I422AlphaToARGBRow_MSA(const uint8* src_y,
                            const uint8* src_u,
                            const uint8* src_v,
                            const uint8* src_a,
                            uint8* rgb_buf,
                            const struct YuvConstants* yuvconstants,
                            int width) {
  int x;
  int64 data_a;
  v16u8 src0, src1, src2, src3;
  v8i16 vec0, vec1, vec2;
  v4i32 vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg, vec_br, vec_yg;
  v4i32 vec_ubvr, vec_ugvg;
  v4i32 zero = {0};

  YUVTORGB_SETUP(yuvconstants, vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg,
                 vec_br, vec_yg);
  vec_ubvr = __msa_ilvr_w(vec_vr, vec_ub);
  vec_ugvg = (v4i32)__msa_ilvev_h((v8i16)vec_vg, (v8i16)vec_ug);

  for (x = 0; x < width; x += 8) {
    data_a = LD(src_a);
    READYUV422(src_y, src_u, src_v, src0, src1, src2);
    src1 = (v16u8)__msa_ilvr_b((v16i8)src2, (v16i8)src1);
    src3 = (v16u8)__msa_insert_d((v2i64)zero, 0, data_a);
    YUVTORGB(src0, src1, vec_ubvr, vec_ugvg, vec_bb, vec_bg, vec_br, vec_yg,
             vec0, vec1, vec2);
    src3 = (v16u8)__msa_ilvr_b((v16i8)src3, (v16i8)src3);
    STOREARGB(vec0, vec1, vec2, src3, rgb_buf);
    src_y += 8;
    src_u += 4;
    src_v += 4;
    src_a += 8;
    rgb_buf += 32;
  }
}

void I422ToRGB24Row_MSA(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* rgb_buf,
                        const struct YuvConstants* yuvconstants,
                        int32 width) {
  int x;
  int64 data_u, data_v;
  v16u8 src0, src1, src2, src3, src4, dst0, dst1, dst2;
  v8i16 vec0, vec1, vec2, vec3, vec4, vec5;
  v4i32 vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg, vec_br, vec_yg;
  v4i32 vec_ubvr, vec_ugvg;
  v16u8 reg0, reg1, reg2, reg3;
  v2i64 zero = {0};
  v16i8 shuffler0 = {0, 1, 16, 2, 3, 17, 4, 5, 18, 6, 7, 19, 8, 9, 20, 10};
  v16i8 shuffler1 = {0, 21, 1, 2, 22, 3, 4, 23, 5, 6, 24, 7, 8, 25, 9, 10};
  v16i8 shuffler2 = {26, 6,  7,  27, 8,  9,  28, 10,
                     11, 29, 12, 13, 30, 14, 15, 31};

  YUVTORGB_SETUP(yuvconstants, vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg,
                 vec_br, vec_yg);
  vec_ubvr = __msa_ilvr_w(vec_vr, vec_ub);
  vec_ugvg = (v4i32)__msa_ilvev_h((v8i16)vec_vg, (v8i16)vec_ug);

  for (x = 0; x < width; x += 16) {
    src0 = (v16u8)__msa_ld_b((v16u8*)src_y, 0);
    data_u = LD(src_u);
    data_v = LD(src_v);
    src1 = (v16u8)__msa_insert_d(zero, 0, data_u);
    src2 = (v16u8)__msa_insert_d(zero, 0, data_v);
    src1 = (v16u8)__msa_ilvr_b((v16i8)src2, (v16i8)src1);
    src3 = (v16u8)__msa_sldi_b((v16i8)src0, (v16i8)src0, 8);
    src4 = (v16u8)__msa_sldi_b((v16i8)src1, (v16i8)src1, 8);
    YUVTORGB(src0, src1, vec_ubvr, vec_ugvg, vec_bb, vec_bg, vec_br, vec_yg,
             vec0, vec1, vec2);
    YUVTORGB(src3, src4, vec_ubvr, vec_ugvg, vec_bb, vec_bg, vec_br, vec_yg,
             vec3, vec4, vec5);
    reg0 = (v16u8)__msa_ilvev_b((v16i8)vec1, (v16i8)vec0);
    reg2 = (v16u8)__msa_ilvev_b((v16i8)vec4, (v16i8)vec3);
    reg3 = (v16u8)__msa_pckev_b((v16i8)vec5, (v16i8)vec2);
    reg1 = (v16u8)__msa_sldi_b((v16i8)reg2, (v16i8)reg0, 11);
    dst0 = (v16u8)__msa_vshf_b(shuffler0, (v16i8)reg3, (v16i8)reg0);
    dst1 = (v16u8)__msa_vshf_b(shuffler1, (v16i8)reg3, (v16i8)reg1);
    dst2 = (v16u8)__msa_vshf_b(shuffler2, (v16i8)reg3, (v16i8)reg2);
    ST_UB2(dst0, dst1, rgb_buf, 16);
    ST_UB(dst2, (rgb_buf + 32));
    src_y += 16;
    src_u += 8;
    src_v += 8;
    rgb_buf += 48;
  }
}

// TODO(fbarchard): Consider AND instead of shift to isolate 5 upper bits of R.
void I422ToRGB565Row_MSA(const uint8* src_y,
                         const uint8* src_u,
                         const uint8* src_v,
                         uint8* dst_rgb565,
                         const struct YuvConstants* yuvconstants,
                         int width) {
  int x;
  v16u8 src0, src1, src2, dst0;
  v8i16 vec0, vec1, vec2;
  v4i32 vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg, vec_br, vec_yg;
  v4i32 vec_ubvr, vec_ugvg;

  YUVTORGB_SETUP(yuvconstants, vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg,
                 vec_br, vec_yg);
  vec_ubvr = __msa_ilvr_w(vec_vr, vec_ub);
  vec_ugvg = (v4i32)__msa_ilvev_h((v8i16)vec_vg, (v8i16)vec_ug);

  for (x = 0; x < width; x += 8) {
    READYUV422(src_y, src_u, src_v, src0, src1, src2);
    src1 = (v16u8)__msa_ilvr_b((v16i8)src2, (v16i8)src1);
    YUVTORGB(src0, src1, vec_ubvr, vec_ugvg, vec_bb, vec_bg, vec_br, vec_yg,
             vec0, vec2, vec1);
    vec0 = __msa_srai_h(vec0, 3);
    vec1 = __msa_srai_h(vec1, 3);
    vec2 = __msa_srai_h(vec2, 2);
    vec1 = __msa_slli_h(vec1, 11);
    vec2 = __msa_slli_h(vec2, 5);
    vec0 |= vec1;
    dst0 = (v16u8)(vec2 | vec0);
    ST_UB(dst0, dst_rgb565);
    src_y += 8;
    src_u += 4;
    src_v += 4;
    dst_rgb565 += 16;
  }
}

// TODO(fbarchard): Consider AND instead of shift to isolate 4 upper bits of G.
void I422ToARGB4444Row_MSA(const uint8* src_y,
                           const uint8* src_u,
                           const uint8* src_v,
                           uint8* dst_argb4444,
                           const struct YuvConstants* yuvconstants,
                           int width) {
  int x;
  v16u8 src0, src1, src2, dst0;
  v8i16 vec0, vec1, vec2;
  v8u16 reg0, reg1, reg2;
  v4i32 vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg, vec_br, vec_yg;
  v4i32 vec_ubvr, vec_ugvg;
  v8u16 const_0xF000 = (v8u16)__msa_fill_h(0xF000);

  YUVTORGB_SETUP(yuvconstants, vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg,
                 vec_br, vec_yg);
  vec_ubvr = __msa_ilvr_w(vec_vr, vec_ub);
  vec_ugvg = (v4i32)__msa_ilvev_h((v8i16)vec_vg, (v8i16)vec_ug);

  for (x = 0; x < width; x += 8) {
    READYUV422(src_y, src_u, src_v, src0, src1, src2);
    src1 = (v16u8)__msa_ilvr_b((v16i8)src2, (v16i8)src1);
    YUVTORGB(src0, src1, vec_ubvr, vec_ugvg, vec_bb, vec_bg, vec_br, vec_yg,
             vec0, vec1, vec2);
    reg0 = (v8u16)__msa_srai_h(vec0, 4);
    reg1 = (v8u16)__msa_srai_h(vec1, 4);
    reg2 = (v8u16)__msa_srai_h(vec2, 4);
    reg1 = (v8u16)__msa_slli_h((v8i16)reg1, 4);
    reg2 = (v8u16)__msa_slli_h((v8i16)reg2, 8);
    reg1 |= const_0xF000;
    reg0 |= reg2;
    dst0 = (v16u8)(reg1 | reg0);
    ST_UB(dst0, dst_argb4444);
    src_y += 8;
    src_u += 4;
    src_v += 4;
    dst_argb4444 += 16;
  }
}

void I422ToARGB1555Row_MSA(const uint8* src_y,
                           const uint8* src_u,
                           const uint8* src_v,
                           uint8* dst_argb1555,
                           const struct YuvConstants* yuvconstants,
                           int width) {
  int x;
  v16u8 src0, src1, src2, dst0;
  v8i16 vec0, vec1, vec2;
  v8u16 reg0, reg1, reg2;
  v4i32 vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg, vec_br, vec_yg;
  v4i32 vec_ubvr, vec_ugvg;
  v8u16 const_0x8000 = (v8u16)__msa_fill_h(0x8000);

  YUVTORGB_SETUP(yuvconstants, vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg,
                 vec_br, vec_yg);
  vec_ubvr = __msa_ilvr_w(vec_vr, vec_ub);
  vec_ugvg = (v4i32)__msa_ilvev_h((v8i16)vec_vg, (v8i16)vec_ug);

  for (x = 0; x < width; x += 8) {
    READYUV422(src_y, src_u, src_v, src0, src1, src2);
    src1 = (v16u8)__msa_ilvr_b((v16i8)src2, (v16i8)src1);
    YUVTORGB(src0, src1, vec_ubvr, vec_ugvg, vec_bb, vec_bg, vec_br, vec_yg,
             vec0, vec1, vec2);
    reg0 = (v8u16)__msa_srai_h(vec0, 3);
    reg1 = (v8u16)__msa_srai_h(vec1, 3);
    reg2 = (v8u16)__msa_srai_h(vec2, 3);
    reg1 = (v8u16)__msa_slli_h((v8i16)reg1, 5);
    reg2 = (v8u16)__msa_slli_h((v8i16)reg2, 10);
    reg1 |= const_0x8000;
    reg0 |= reg2;
    dst0 = (v16u8)(reg1 | reg0);
    ST_UB(dst0, dst_argb1555);
    src_y += 8;
    src_u += 4;
    src_v += 4;
    dst_argb1555 += 16;
  }
}

void YUY2ToYRow_MSA(const uint8* src_yuy2, uint8* dst_y, int width) {
  int x;
  v16u8 src0, src1, src2, src3, dst0, dst1;

  for (x = 0; x < width; x += 32) {
    LD_UB4(src_yuy2, 16, src0, src1, src2, src3);
    dst0 = (v16u8)__msa_pckev_b((v16i8)src1, (v16i8)src0);
    dst1 = (v16u8)__msa_pckev_b((v16i8)src3, (v16i8)src2);
    ST_UB2(dst0, dst1, dst_y, 16);
    src_yuy2 += 64;
    dst_y += 32;
  }
}

void YUY2ToUVRow_MSA(const uint8* src_yuy2,
                     int src_stride_yuy2,
                     uint8* dst_u,
                     uint8* dst_v,
                     int width) {
  const uint8* src_yuy2_next = src_yuy2 + src_stride_yuy2;
  int x;
  v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
  v16u8 vec0, vec1, dst0, dst1;

  for (x = 0; x < width; x += 32) {
    LD_UB4(src_yuy2, 16, src0, src1, src2, src3);
    LD_UB4(src_yuy2_next, 16, src4, src5, src6, src7);
    src0 = (v16u8)__msa_pckod_b((v16i8)src1, (v16i8)src0);
    src1 = (v16u8)__msa_pckod_b((v16i8)src3, (v16i8)src2);
    src2 = (v16u8)__msa_pckod_b((v16i8)src5, (v16i8)src4);
    src3 = (v16u8)__msa_pckod_b((v16i8)src7, (v16i8)src6);
    vec0 = __msa_aver_u_b(src0, src2);
    vec1 = __msa_aver_u_b(src1, src3);
    dst0 = (v16u8)__msa_pckev_b((v16i8)vec1, (v16i8)vec0);
    dst1 = (v16u8)__msa_pckod_b((v16i8)vec1, (v16i8)vec0);
    ST_UB(dst0, dst_u);
    ST_UB(dst1, dst_v);
    src_yuy2 += 64;
    src_yuy2_next += 64;
    dst_u += 16;
    dst_v += 16;
  }
}

void YUY2ToUV422Row_MSA(const uint8* src_yuy2,
                        uint8* dst_u,
                        uint8* dst_v,
                        int width) {
  int x;
  v16u8 src0, src1, src2, src3, dst0, dst1;

  for (x = 0; x < width; x += 32) {
    LD_UB4(src_yuy2, 16, src0, src1, src2, src3);
    src0 = (v16u8)__msa_pckod_b((v16i8)src1, (v16i8)src0);
    src1 = (v16u8)__msa_pckod_b((v16i8)src3, (v16i8)src2);
    dst0 = (v16u8)__msa_pckev_b((v16i8)src1, (v16i8)src0);
    dst1 = (v16u8)__msa_pckod_b((v16i8)src1, (v16i8)src0);
    ST_UB(dst0, dst_u);
    ST_UB(dst1, dst_v);
    src_yuy2 += 64;
    dst_u += 16;
    dst_v += 16;
  }
}

void UYVYToYRow_MSA(const uint8* src_uyvy, uint8* dst_y, int width) {
  int x;
  v16u8 src0, src1, src2, src3, dst0, dst1;

  for (x = 0; x < width; x += 32) {
    LD_UB4(src_uyvy, 16, src0, src1, src2, src3);
    dst0 = (v16u8)__msa_pckod_b((v16i8)src1, (v16i8)src0);
    dst1 = (v16u8)__msa_pckod_b((v16i8)src3, (v16i8)src2);
    ST_UB2(dst0, dst1, dst_y, 16);
    src_uyvy += 64;
    dst_y += 32;
  }
}

void UYVYToUVRow_MSA(const uint8* src_uyvy,
                     int src_stride_uyvy,
                     uint8* dst_u,
                     uint8* dst_v,
                     int width) {
  const uint8* src_uyvy_next = src_uyvy + src_stride_uyvy;
  int x;
  v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
  v16u8 vec0, vec1, dst0, dst1;

  for (x = 0; x < width; x += 32) {
    LD_UB4(src_uyvy, 16, src0, src1, src2, src3);
    LD_UB4(src_uyvy_next, 16, src4, src5, src6, src7);
    src0 = (v16u8)__msa_pckev_b((v16i8)src1, (v16i8)src0);
    src1 = (v16u8)__msa_pckev_b((v16i8)src3, (v16i8)src2);
    src2 = (v16u8)__msa_pckev_b((v16i8)src5, (v16i8)src4);
    src3 = (v16u8)__msa_pckev_b((v16i8)src7, (v16i8)src6);
    vec0 = __msa_aver_u_b(src0, src2);
    vec1 = __msa_aver_u_b(src1, src3);
    dst0 = (v16u8)__msa_pckev_b((v16i8)vec1, (v16i8)vec0);
    dst1 = (v16u8)__msa_pckod_b((v16i8)vec1, (v16i8)vec0);
    ST_UB(dst0, dst_u);
    ST_UB(dst1, dst_v);
    src_uyvy += 64;
    src_uyvy_next += 64;
    dst_u += 16;
    dst_v += 16;
  }
}

void UYVYToUV422Row_MSA(const uint8* src_uyvy,
                        uint8* dst_u,
                        uint8* dst_v,
                        int width) {
  int x;
  v16u8 src0, src1, src2, src3, dst0, dst1;

  for (x = 0; x < width; x += 32) {
    LD_UB4(src_uyvy, 16, src0, src1, src2, src3);
    src0 = (v16u8)__msa_pckev_b((v16i8)src1, (v16i8)src0);
    src1 = (v16u8)__msa_pckev_b((v16i8)src3, (v16i8)src2);
    dst0 = (v16u8)__msa_pckev_b((v16i8)src1, (v16i8)src0);
    dst1 = (v16u8)__msa_pckod_b((v16i8)src1, (v16i8)src0);
    ST_UB(dst0, dst_u);
    ST_UB(dst1, dst_v);
    src_uyvy += 64;
    dst_u += 16;
    dst_v += 16;
  }
}

void ARGBToYRow_MSA(const uint8* src_argb0, uint8* dst_y, int width) {
  int x;
  v16u8 src0, src1, src2, src3, vec0, vec1, vec2, vec3, dst0;
  v8u16 reg0, reg1, reg2, reg3, reg4, reg5;
  v16i8 zero = {0};
  v8u16 const_0x19 = (v8u16)__msa_ldi_h(0x19);
  v8u16 const_0x81 = (v8u16)__msa_ldi_h(0x81);
  v8u16 const_0x42 = (v8u16)__msa_ldi_h(0x42);
  v8u16 const_0x1080 = (v8u16)__msa_fill_h(0x1080);

  for (x = 0; x < width; x += 16) {
    src0 = (v16u8)__msa_ld_b((v16u8*)src_argb0, 0);
    src1 = (v16u8)__msa_ld_b((v16u8*)src_argb0, 16);
    src2 = (v16u8)__msa_ld_b((v16u8*)src_argb0, 32);
    src3 = (v16u8)__msa_ld_b((v16u8*)src_argb0, 48);
    vec0 = (v16u8)__msa_pckev_b((v16i8)src1, (v16i8)src0);
    vec1 = (v16u8)__msa_pckev_b((v16i8)src3, (v16i8)src2);
    vec2 = (v16u8)__msa_pckod_b((v16i8)src1, (v16i8)src0);
    vec3 = (v16u8)__msa_pckod_b((v16i8)src3, (v16i8)src2);
    reg0 = (v8u16)__msa_ilvev_b(zero, (v16i8)vec0);
    reg1 = (v8u16)__msa_ilvev_b(zero, (v16i8)vec1);
    reg2 = (v8u16)__msa_ilvev_b(zero, (v16i8)vec2);
    reg3 = (v8u16)__msa_ilvev_b(zero, (v16i8)vec3);
    reg4 = (v8u16)__msa_ilvod_b(zero, (v16i8)vec0);
    reg5 = (v8u16)__msa_ilvod_b(zero, (v16i8)vec1);
    reg0 *= const_0x19;
    reg1 *= const_0x19;
    reg2 *= const_0x81;
    reg3 *= const_0x81;
    reg4 *= const_0x42;
    reg5 *= const_0x42;
    reg0 += reg2;
    reg1 += reg3;
    reg0 += reg4;
    reg1 += reg5;
    reg0 += const_0x1080;
    reg1 += const_0x1080;
    reg0 = (v8u16)__msa_srai_h((v8i16)reg0, 8);
    reg1 = (v8u16)__msa_srai_h((v8i16)reg1, 8);
    dst0 = (v16u8)__msa_pckev_b((v16i8)reg1, (v16i8)reg0);
    ST_UB(dst0, dst_y);
    src_argb0 += 64;
    dst_y += 16;
  }
}

void ARGBToUVRow_MSA(const uint8* src_argb0,
                     int src_stride_argb,
                     uint8* dst_u,
                     uint8* dst_v,
                     int width) {
  int x;
  const uint8* src_argb0_next = src_argb0 + src_stride_argb;
  v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
  v16u8 vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7, vec8, vec9;
  v8u16 reg0, reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8, reg9;
  v16u8 dst0, dst1;
  v8u16 const_0x70 = (v8u16)__msa_ldi_h(0x70);
  v8u16 const_0x4A = (v8u16)__msa_ldi_h(0x4A);
  v8u16 const_0x26 = (v8u16)__msa_ldi_h(0x26);
  v8u16 const_0x5E = (v8u16)__msa_ldi_h(0x5E);
  v8u16 const_0x12 = (v8u16)__msa_ldi_h(0x12);
  v8u16 const_0x8080 = (v8u16)__msa_fill_h(0x8080);

  for (x = 0; x < width; x += 32) {
    src0 = (v16u8)__msa_ld_b((v16u8*)src_argb0, 0);
    src1 = (v16u8)__msa_ld_b((v16u8*)src_argb0, 16);
    src2 = (v16u8)__msa_ld_b((v16u8*)src_argb0, 32);
    src3 = (v16u8)__msa_ld_b((v16u8*)src_argb0, 48);
    src4 = (v16u8)__msa_ld_b((v16u8*)src_argb0, 64);
    src5 = (v16u8)__msa_ld_b((v16u8*)src_argb0, 80);
    src6 = (v16u8)__msa_ld_b((v16u8*)src_argb0, 96);
    src7 = (v16u8)__msa_ld_b((v16u8*)src_argb0, 112);
    vec0 = (v16u8)__msa_pckev_b((v16i8)src1, (v16i8)src0);
    vec1 = (v16u8)__msa_pckev_b((v16i8)src3, (v16i8)src2);
    vec2 = (v16u8)__msa_pckev_b((v16i8)src5, (v16i8)src4);
    vec3 = (v16u8)__msa_pckev_b((v16i8)src7, (v16i8)src6);
    vec4 = (v16u8)__msa_pckod_b((v16i8)src1, (v16i8)src0);
    vec5 = (v16u8)__msa_pckod_b((v16i8)src3, (v16i8)src2);
    vec6 = (v16u8)__msa_pckod_b((v16i8)src5, (v16i8)src4);
    vec7 = (v16u8)__msa_pckod_b((v16i8)src7, (v16i8)src6);
    vec8 = (v16u8)__msa_pckev_b((v16i8)vec1, (v16i8)vec0);
    vec9 = (v16u8)__msa_pckev_b((v16i8)vec3, (v16i8)vec2);
    vec4 = (v16u8)__msa_pckev_b((v16i8)vec5, (v16i8)vec4);
    vec5 = (v16u8)__msa_pckev_b((v16i8)vec7, (v16i8)vec6);
    vec0 = (v16u8)__msa_pckod_b((v16i8)vec1, (v16i8)vec0);
    vec1 = (v16u8)__msa_pckod_b((v16i8)vec3, (v16i8)vec2);
    reg0 = __msa_hadd_u_h(vec8, vec8);
    reg1 = __msa_hadd_u_h(vec9, vec9);
    reg2 = __msa_hadd_u_h(vec4, vec4);
    reg3 = __msa_hadd_u_h(vec5, vec5);
    reg4 = __msa_hadd_u_h(vec0, vec0);
    reg5 = __msa_hadd_u_h(vec1, vec1);
    src0 = (v16u8)__msa_ld_b((v16u8*)src_argb0_next, 0);
    src1 = (v16u8)__msa_ld_b((v16u8*)src_argb0_next, 16);
    src2 = (v16u8)__msa_ld_b((v16u8*)src_argb0_next, 32);
    src3 = (v16u8)__msa_ld_b((v16u8*)src_argb0_next, 48);
    src4 = (v16u8)__msa_ld_b((v16u8*)src_argb0_next, 64);
    src5 = (v16u8)__msa_ld_b((v16u8*)src_argb0_next, 80);
    src6 = (v16u8)__msa_ld_b((v16u8*)src_argb0_next, 96);
    src7 = (v16u8)__msa_ld_b((v16u8*)src_argb0_next, 112);
    vec0 = (v16u8)__msa_pckev_b((v16i8)src1, (v16i8)src0);
    vec1 = (v16u8)__msa_pckev_b((v16i8)src3, (v16i8)src2);
    vec2 = (v16u8)__msa_pckev_b((v16i8)src5, (v16i8)src4);
    vec3 = (v16u8)__msa_pckev_b((v16i8)src7, (v16i8)src6);
    vec4 = (v16u8)__msa_pckod_b((v16i8)src1, (v16i8)src0);
    vec5 = (v16u8)__msa_pckod_b((v16i8)src3, (v16i8)src2);
    vec6 = (v16u8)__msa_pckod_b((v16i8)src5, (v16i8)src4);
    vec7 = (v16u8)__msa_pckod_b((v16i8)src7, (v16i8)src6);
    vec8 = (v16u8)__msa_pckev_b((v16i8)vec1, (v16i8)vec0);
    vec9 = (v16u8)__msa_pckev_b((v16i8)vec3, (v16i8)vec2);
    vec4 = (v16u8)__msa_pckev_b((v16i8)vec5, (v16i8)vec4);
    vec5 = (v16u8)__msa_pckev_b((v16i8)vec7, (v16i8)vec6);
    vec0 = (v16u8)__msa_pckod_b((v16i8)vec1, (v16i8)vec0);
    vec1 = (v16u8)__msa_pckod_b((v16i8)vec3, (v16i8)vec2);
    reg0 += __msa_hadd_u_h(vec8, vec8);
    reg1 += __msa_hadd_u_h(vec9, vec9);
    reg2 += __msa_hadd_u_h(vec4, vec4);
    reg3 += __msa_hadd_u_h(vec5, vec5);
    reg4 += __msa_hadd_u_h(vec0, vec0);
    reg5 += __msa_hadd_u_h(vec1, vec1);
    reg0 = (v8u16)__msa_srai_h((v8i16)reg0, 2);
    reg1 = (v8u16)__msa_srai_h((v8i16)reg1, 2);
    reg2 = (v8u16)__msa_srai_h((v8i16)reg2, 2);
    reg3 = (v8u16)__msa_srai_h((v8i16)reg3, 2);
    reg4 = (v8u16)__msa_srai_h((v8i16)reg4, 2);
    reg5 = (v8u16)__msa_srai_h((v8i16)reg5, 2);
    reg6 = reg0 * const_0x70;
    reg7 = reg1 * const_0x70;
    reg8 = reg2 * const_0x4A;
    reg9 = reg3 * const_0x4A;
    reg6 += const_0x8080;
    reg7 += const_0x8080;
    reg8 += reg4 * const_0x26;
    reg9 += reg5 * const_0x26;
    reg0 *= const_0x12;
    reg1 *= const_0x12;
    reg2 *= const_0x5E;
    reg3 *= const_0x5E;
    reg4 *= const_0x70;
    reg5 *= const_0x70;
    reg2 += reg0;
    reg3 += reg1;
    reg4 += const_0x8080;
    reg5 += const_0x8080;
    reg6 -= reg8;
    reg7 -= reg9;
    reg4 -= reg2;
    reg5 -= reg3;
    reg6 = (v8u16)__msa_srai_h((v8i16)reg6, 8);
    reg7 = (v8u16)__msa_srai_h((v8i16)reg7, 8);
    reg4 = (v8u16)__msa_srai_h((v8i16)reg4, 8);
    reg5 = (v8u16)__msa_srai_h((v8i16)reg5, 8);
    dst0 = (v16u8)__msa_pckev_b((v16i8)reg7, (v16i8)reg6);
    dst1 = (v16u8)__msa_pckev_b((v16i8)reg5, (v16i8)reg4);
    ST_UB(dst0, dst_u);
    ST_UB(dst1, dst_v);
    src_argb0 += 128;
    src_argb0_next += 128;
    dst_u += 16;
    dst_v += 16;
  }
}

void ARGBToRGB24Row_MSA(const uint8* src_argb, uint8* dst_rgb, int width) {
  int x;
  v16u8 src0, src1, src2, src3, dst0, dst1, dst2;
  v16i8 shuffler0 = {0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, 16, 17, 18, 20};
  v16i8 shuffler1 = {5,  6,  8,  9,  10, 12, 13, 14,
                     16, 17, 18, 20, 21, 22, 24, 25};
  v16i8 shuffler2 = {10, 12, 13, 14, 16, 17, 18, 20,
                     21, 22, 24, 25, 26, 28, 29, 30};

  for (x = 0; x < width; x += 16) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_argb, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_argb, 16);
    src2 = (v16u8)__msa_ld_b((v16i8*)src_argb, 32);
    src3 = (v16u8)__msa_ld_b((v16i8*)src_argb, 48);
    dst0 = (v16u8)__msa_vshf_b(shuffler0, (v16i8)src1, (v16i8)src0);
    dst1 = (v16u8)__msa_vshf_b(shuffler1, (v16i8)src2, (v16i8)src1);
    dst2 = (v16u8)__msa_vshf_b(shuffler2, (v16i8)src3, (v16i8)src2);
    ST_UB2(dst0, dst1, dst_rgb, 16);
    ST_UB(dst2, (dst_rgb + 32));
    src_argb += 64;
    dst_rgb += 48;
  }
}

void ARGBToRAWRow_MSA(const uint8* src_argb, uint8* dst_rgb, int width) {
  int x;
  v16u8 src0, src1, src2, src3, dst0, dst1, dst2;
  v16i8 shuffler0 = {2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, 18, 17, 16, 22};
  v16i8 shuffler1 = {5,  4,  10, 9,  8,  14, 13, 12,
                     18, 17, 16, 22, 21, 20, 26, 25};
  v16i8 shuffler2 = {8,  14, 13, 12, 18, 17, 16, 22,
                     21, 20, 26, 25, 24, 30, 29, 28};

  for (x = 0; x < width; x += 16) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_argb, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_argb, 16);
    src2 = (v16u8)__msa_ld_b((v16i8*)src_argb, 32);
    src3 = (v16u8)__msa_ld_b((v16i8*)src_argb, 48);
    dst0 = (v16u8)__msa_vshf_b(shuffler0, (v16i8)src1, (v16i8)src0);
    dst1 = (v16u8)__msa_vshf_b(shuffler1, (v16i8)src2, (v16i8)src1);
    dst2 = (v16u8)__msa_vshf_b(shuffler2, (v16i8)src3, (v16i8)src2);
    ST_UB2(dst0, dst1, dst_rgb, 16);
    ST_UB(dst2, (dst_rgb + 32));
    src_argb += 64;
    dst_rgb += 48;
  }
}

void ARGBToRGB565Row_MSA(const uint8* src_argb, uint8* dst_rgb, int width) {
  int x;
  v16u8 src0, src1, dst0;
  v16u8 vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7;
  v16i8 zero = {0};

  for (x = 0; x < width; x += 8) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_argb, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_argb, 16);
    vec0 = (v16u8)__msa_srai_b((v16i8)src0, 3);
    vec1 = (v16u8)__msa_slli_b((v16i8)src0, 3);
    vec2 = (v16u8)__msa_srai_b((v16i8)src0, 5);
    vec4 = (v16u8)__msa_srai_b((v16i8)src1, 3);
    vec5 = (v16u8)__msa_slli_b((v16i8)src1, 3);
    vec6 = (v16u8)__msa_srai_b((v16i8)src1, 5);
    vec1 = (v16u8)__msa_sldi_b(zero, (v16i8)vec1, 1);
    vec2 = (v16u8)__msa_sldi_b(zero, (v16i8)vec2, 1);
    vec5 = (v16u8)__msa_sldi_b(zero, (v16i8)vec5, 1);
    vec6 = (v16u8)__msa_sldi_b(zero, (v16i8)vec6, 1);
    vec3 = (v16u8)__msa_sldi_b(zero, (v16i8)src0, 2);
    vec7 = (v16u8)__msa_sldi_b(zero, (v16i8)src1, 2);
    vec0 = __msa_binsli_b(vec0, vec1, 2);
    vec1 = __msa_binsli_b(vec2, vec3, 4);
    vec4 = __msa_binsli_b(vec4, vec5, 2);
    vec5 = __msa_binsli_b(vec6, vec7, 4);
    vec0 = (v16u8)__msa_ilvev_b((v16i8)vec1, (v16i8)vec0);
    vec4 = (v16u8)__msa_ilvev_b((v16i8)vec5, (v16i8)vec4);
    dst0 = (v16u8)__msa_pckev_h((v8i16)vec4, (v8i16)vec0);
    ST_UB(dst0, dst_rgb);
    src_argb += 32;
    dst_rgb += 16;
  }
}

void ARGBToARGB1555Row_MSA(const uint8* src_argb, uint8* dst_rgb, int width) {
  int x;
  v16u8 src0, src1, dst0;
  v16u8 vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7, vec8, vec9;
  v16i8 zero = {0};

  for (x = 0; x < width; x += 8) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_argb, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_argb, 16);
    vec0 = (v16u8)__msa_srai_b((v16i8)src0, 3);
    vec1 = (v16u8)__msa_slli_b((v16i8)src0, 2);
    vec2 = (v16u8)__msa_srai_b((v16i8)vec0, 3);
    vec1 = (v16u8)__msa_sldi_b(zero, (v16i8)vec1, 1);
    vec2 = (v16u8)__msa_sldi_b(zero, (v16i8)vec2, 1);
    vec3 = (v16u8)__msa_srai_b((v16i8)src0, 1);
    vec5 = (v16u8)__msa_srai_b((v16i8)src1, 3);
    vec6 = (v16u8)__msa_slli_b((v16i8)src1, 2);
    vec7 = (v16u8)__msa_srai_b((v16i8)vec5, 3);
    vec6 = (v16u8)__msa_sldi_b(zero, (v16i8)vec6, 1);
    vec7 = (v16u8)__msa_sldi_b(zero, (v16i8)vec7, 1);
    vec8 = (v16u8)__msa_srai_b((v16i8)src1, 1);
    vec3 = (v16u8)__msa_sldi_b(zero, (v16i8)vec3, 2);
    vec8 = (v16u8)__msa_sldi_b(zero, (v16i8)vec8, 2);
    vec4 = (v16u8)__msa_sldi_b(zero, (v16i8)src0, 3);
    vec9 = (v16u8)__msa_sldi_b(zero, (v16i8)src1, 3);
    vec0 = __msa_binsli_b(vec0, vec1, 2);
    vec5 = __msa_binsli_b(vec5, vec6, 2);
    vec1 = __msa_binsli_b(vec2, vec3, 5);
    vec6 = __msa_binsli_b(vec7, vec8, 5);
    vec1 = __msa_binsli_b(vec1, vec4, 0);
    vec6 = __msa_binsli_b(vec6, vec9, 0);
    vec0 = (v16u8)__msa_ilvev_b((v16i8)vec1, (v16i8)vec0);
    vec1 = (v16u8)__msa_ilvev_b((v16i8)vec6, (v16i8)vec5);
    dst0 = (v16u8)__msa_pckev_h((v8i16)vec1, (v8i16)vec0);
    ST_UB(dst0, dst_rgb);
    src_argb += 32;
    dst_rgb += 16;
  }
}

void ARGBToARGB4444Row_MSA(const uint8* src_argb, uint8* dst_rgb, int width) {
  int x;
  v16u8 src0, src1;
  v16u8 vec0, vec1;
  v16u8 dst0;
  v16i8 zero = {0};

  for (x = 0; x < width; x += 8) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_argb, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_argb, 16);
    vec0 = (v16u8)__msa_srai_b((v16i8)src0, 4);
    vec1 = (v16u8)__msa_srai_b((v16i8)src1, 4);
    src0 = (v16u8)__msa_sldi_b(zero, (v16i8)src0, 1);
    src1 = (v16u8)__msa_sldi_b(zero, (v16i8)src1, 1);
    vec0 = __msa_binsli_b(vec0, src0, 3);
    vec1 = __msa_binsli_b(vec1, src1, 3);
    dst0 = (v16u8)__msa_pckev_b((v16i8)vec1, (v16i8)vec0);
    ST_UB(dst0, dst_rgb);
    src_argb += 32;
    dst_rgb += 16;
  }
}

void ARGBToUV444Row_MSA(const uint8* src_argb,
                        uint8* dst_u,
                        uint8* dst_v,
                        int32 width) {
  int32 x;
  v16u8 src0, src1, src2, src3, reg0, reg1, reg2, reg3, dst0, dst1;
  v8u16 vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7;
  v8u16 vec8, vec9, vec10, vec11;
  v8u16 const_112 = (v8u16)__msa_ldi_h(112);
  v8u16 const_74 = (v8u16)__msa_ldi_h(74);
  v8u16 const_38 = (v8u16)__msa_ldi_h(38);
  v8u16 const_94 = (v8u16)__msa_ldi_h(94);
  v8u16 const_18 = (v8u16)__msa_ldi_h(18);
  v8u16 const_32896 = (v8u16)__msa_fill_h(32896);
  v16i8 zero = {0};

  for (x = width; x > 0; x -= 16) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_argb, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_argb, 16);
    src2 = (v16u8)__msa_ld_b((v16i8*)src_argb, 32);
    src3 = (v16u8)__msa_ld_b((v16i8*)src_argb, 48);
    reg0 = (v16u8)__msa_pckev_b((v16i8)src1, (v16i8)src0);
    reg1 = (v16u8)__msa_pckev_b((v16i8)src3, (v16i8)src2);
    reg2 = (v16u8)__msa_pckod_b((v16i8)src1, (v16i8)src0);
    reg3 = (v16u8)__msa_pckod_b((v16i8)src3, (v16i8)src2);
    src0 = (v16u8)__msa_pckev_b((v16i8)reg1, (v16i8)reg0);
    src1 = (v16u8)__msa_pckev_b((v16i8)reg3, (v16i8)reg2);
    src2 = (v16u8)__msa_pckod_b((v16i8)reg1, (v16i8)reg0);
    vec0 = (v8u16)__msa_ilvr_b(zero, (v16i8)src0);
    vec1 = (v8u16)__msa_ilvl_b(zero, (v16i8)src0);
    vec2 = (v8u16)__msa_ilvr_b(zero, (v16i8)src1);
    vec3 = (v8u16)__msa_ilvl_b(zero, (v16i8)src1);
    vec4 = (v8u16)__msa_ilvr_b(zero, (v16i8)src2);
    vec5 = (v8u16)__msa_ilvl_b(zero, (v16i8)src2);
    vec10 = vec0 * const_18;
    vec11 = vec1 * const_18;
    vec8 = vec2 * const_94;
    vec9 = vec3 * const_94;
    vec6 = vec4 * const_112;
    vec7 = vec5 * const_112;
    vec0 *= const_112;
    vec1 *= const_112;
    vec2 *= const_74;
    vec3 *= const_74;
    vec4 *= const_38;
    vec5 *= const_38;
    vec8 += vec10;
    vec9 += vec11;
    vec6 += const_32896;
    vec7 += const_32896;
    vec0 += const_32896;
    vec1 += const_32896;
    vec2 += vec4;
    vec3 += vec5;
    vec0 -= vec2;
    vec1 -= vec3;
    vec6 -= vec8;
    vec7 -= vec9;
    vec0 = (v8u16)__msa_srai_h((v8i16)vec0, 8);
    vec1 = (v8u16)__msa_srai_h((v8i16)vec1, 8);
    vec6 = (v8u16)__msa_srai_h((v8i16)vec6, 8);
    vec7 = (v8u16)__msa_srai_h((v8i16)vec7, 8);
    dst0 = (v16u8)__msa_pckev_b((v16i8)vec1, (v16i8)vec0);
    dst1 = (v16u8)__msa_pckev_b((v16i8)vec7, (v16i8)vec6);
    ST_UB(dst0, dst_u);
    ST_UB(dst1, dst_v);
    src_argb += 64;
    dst_u += 16;
    dst_v += 16;
  }
}

void ARGBMultiplyRow_MSA(const uint8* src_argb0,
                         const uint8* src_argb1,
                         uint8* dst_argb,
                         int width) {
  int x;
  v16u8 src0, src1, dst0;
  v8u16 vec0, vec1, vec2, vec3;
  v4u32 reg0, reg1, reg2, reg3;
  v8i16 zero = {0};

  for (x = 0; x < width; x += 4) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_argb0, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_argb1, 0);
    vec0 = (v8u16)__msa_ilvr_b((v16i8)src0, (v16i8)src0);
    vec1 = (v8u16)__msa_ilvl_b((v16i8)src0, (v16i8)src0);
    vec2 = (v8u16)__msa_ilvr_b((v16i8)zero, (v16i8)src1);
    vec3 = (v8u16)__msa_ilvl_b((v16i8)zero, (v16i8)src1);
    reg0 = (v4u32)__msa_ilvr_h(zero, (v8i16)vec0);
    reg1 = (v4u32)__msa_ilvl_h(zero, (v8i16)vec0);
    reg2 = (v4u32)__msa_ilvr_h(zero, (v8i16)vec1);
    reg3 = (v4u32)__msa_ilvl_h(zero, (v8i16)vec1);
    reg0 *= (v4u32)__msa_ilvr_h(zero, (v8i16)vec2);
    reg1 *= (v4u32)__msa_ilvl_h(zero, (v8i16)vec2);
    reg2 *= (v4u32)__msa_ilvr_h(zero, (v8i16)vec3);
    reg3 *= (v4u32)__msa_ilvl_h(zero, (v8i16)vec3);
    reg0 = (v4u32)__msa_srai_w((v4i32)reg0, 16);
    reg1 = (v4u32)__msa_srai_w((v4i32)reg1, 16);
    reg2 = (v4u32)__msa_srai_w((v4i32)reg2, 16);
    reg3 = (v4u32)__msa_srai_w((v4i32)reg3, 16);
    vec0 = (v8u16)__msa_pckev_h((v8i16)reg1, (v8i16)reg0);
    vec1 = (v8u16)__msa_pckev_h((v8i16)reg3, (v8i16)reg2);
    dst0 = (v16u8)__msa_pckev_b((v16i8)vec1, (v16i8)vec0);
    ST_UB(dst0, dst_argb);
    src_argb0 += 16;
    src_argb1 += 16;
    dst_argb += 16;
  }
}

void ARGBAddRow_MSA(const uint8* src_argb0,
                    const uint8* src_argb1,
                    uint8* dst_argb,
                    int width) {
  int x;
  v16u8 src0, src1, src2, src3, dst0, dst1;

  for (x = 0; x < width; x += 8) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_argb0, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_argb0, 16);
    src2 = (v16u8)__msa_ld_b((v16i8*)src_argb1, 0);
    src3 = (v16u8)__msa_ld_b((v16i8*)src_argb1, 16);
    dst0 = __msa_adds_u_b(src0, src2);
    dst1 = __msa_adds_u_b(src1, src3);
    ST_UB2(dst0, dst1, dst_argb, 16);
    src_argb0 += 32;
    src_argb1 += 32;
    dst_argb += 32;
  }
}

void ARGBSubtractRow_MSA(const uint8* src_argb0,
                         const uint8* src_argb1,
                         uint8* dst_argb,
                         int width) {
  int x;
  v16u8 src0, src1, src2, src3, dst0, dst1;

  for (x = 0; x < width; x += 8) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_argb0, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_argb0, 16);
    src2 = (v16u8)__msa_ld_b((v16i8*)src_argb1, 0);
    src3 = (v16u8)__msa_ld_b((v16i8*)src_argb1, 16);
    dst0 = __msa_subs_u_b(src0, src2);
    dst1 = __msa_subs_u_b(src1, src3);
    ST_UB2(dst0, dst1, dst_argb, 16);
    src_argb0 += 32;
    src_argb1 += 32;
    dst_argb += 32;
  }
}

void ARGBAttenuateRow_MSA(const uint8* src_argb, uint8* dst_argb, int width) {
  int x;
  v16u8 src0, src1, dst0, dst1;
  v8u16 vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7, vec8, vec9;
  v4u32 reg0, reg1, reg2, reg3, reg4, reg5, reg6, reg7;
  v8i16 zero = {0};
  v16u8 mask = {0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255};

  for (x = 0; x < width; x += 8) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_argb, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_argb, 16);
    vec0 = (v8u16)__msa_ilvr_b((v16i8)src0, (v16i8)src0);
    vec1 = (v8u16)__msa_ilvl_b((v16i8)src0, (v16i8)src0);
    vec2 = (v8u16)__msa_ilvr_b((v16i8)src1, (v16i8)src1);
    vec3 = (v8u16)__msa_ilvl_b((v16i8)src1, (v16i8)src1);
    vec4 = (v8u16)__msa_fill_h(vec0[3]);
    vec5 = (v8u16)__msa_fill_h(vec0[7]);
    vec6 = (v8u16)__msa_fill_h(vec1[3]);
    vec7 = (v8u16)__msa_fill_h(vec1[7]);
    vec4 = (v8u16)__msa_pckev_d((v2i64)vec5, (v2i64)vec4);
    vec5 = (v8u16)__msa_pckev_d((v2i64)vec7, (v2i64)vec6);
    vec6 = (v8u16)__msa_fill_h(vec2[3]);
    vec7 = (v8u16)__msa_fill_h(vec2[7]);
    vec8 = (v8u16)__msa_fill_h(vec3[3]);
    vec9 = (v8u16)__msa_fill_h(vec3[7]);
    vec6 = (v8u16)__msa_pckev_d((v2i64)vec7, (v2i64)vec6);
    vec7 = (v8u16)__msa_pckev_d((v2i64)vec9, (v2i64)vec8);
    reg0 = (v4u32)__msa_ilvr_h(zero, (v8i16)vec4);
    reg1 = (v4u32)__msa_ilvl_h(zero, (v8i16)vec4);
    reg2 = (v4u32)__msa_ilvr_h(zero, (v8i16)vec5);
    reg3 = (v4u32)__msa_ilvl_h(zero, (v8i16)vec5);
    reg4 = (v4u32)__msa_ilvr_h(zero, (v8i16)vec6);
    reg5 = (v4u32)__msa_ilvl_h(zero, (v8i16)vec6);
    reg6 = (v4u32)__msa_ilvr_h(zero, (v8i16)vec7);
    reg7 = (v4u32)__msa_ilvl_h(zero, (v8i16)vec7);
    reg0 *= (v4u32)__msa_ilvr_h(zero, (v8i16)vec0);
    reg1 *= (v4u32)__msa_ilvl_h(zero, (v8i16)vec0);
    reg2 *= (v4u32)__msa_ilvr_h(zero, (v8i16)vec1);
    reg3 *= (v4u32)__msa_ilvl_h(zero, (v8i16)vec1);
    reg4 *= (v4u32)__msa_ilvr_h(zero, (v8i16)vec2);
    reg5 *= (v4u32)__msa_ilvl_h(zero, (v8i16)vec2);
    reg6 *= (v4u32)__msa_ilvr_h(zero, (v8i16)vec3);
    reg7 *= (v4u32)__msa_ilvl_h(zero, (v8i16)vec3);
    reg0 = (v4u32)__msa_srai_w((v4i32)reg0, 24);
    reg1 = (v4u32)__msa_srai_w((v4i32)reg1, 24);
    reg2 = (v4u32)__msa_srai_w((v4i32)reg2, 24);
    reg3 = (v4u32)__msa_srai_w((v4i32)reg3, 24);
    reg4 = (v4u32)__msa_srai_w((v4i32)reg4, 24);
    reg5 = (v4u32)__msa_srai_w((v4i32)reg5, 24);
    reg6 = (v4u32)__msa_srai_w((v4i32)reg6, 24);
    reg7 = (v4u32)__msa_srai_w((v4i32)reg7, 24);
    vec0 = (v8u16)__msa_pckev_h((v8i16)reg1, (v8i16)reg0);
    vec1 = (v8u16)__msa_pckev_h((v8i16)reg3, (v8i16)reg2);
    vec2 = (v8u16)__msa_pckev_h((v8i16)reg5, (v8i16)reg4);
    vec3 = (v8u16)__msa_pckev_h((v8i16)reg7, (v8i16)reg6);
    dst0 = (v16u8)__msa_pckev_b((v16i8)vec1, (v16i8)vec0);
    dst1 = (v16u8)__msa_pckev_b((v16i8)vec3, (v16i8)vec2);
    dst0 = __msa_bmnz_v(dst0, src0, mask);
    dst1 = __msa_bmnz_v(dst1, src1, mask);
    ST_UB2(dst0, dst1, dst_argb, 16);
    src_argb += 32;
    dst_argb += 32;
  }
}

void ARGBToRGB565DitherRow_MSA(const uint8* src_argb,
                               uint8* dst_rgb,
                               uint32 dither4,
                               int width) {
  int x;
  v16u8 src0, src1, dst0, vec0, vec1;
  v8i16 vec_d0;
  v8i16 reg0, reg1, reg2;
  v16i8 zero = {0};
  v8i16 max = __msa_ldi_h(0xFF);

  vec_d0 = (v8i16)__msa_fill_w(dither4);
  vec_d0 = (v8i16)__msa_ilvr_b(zero, (v16i8)vec_d0);

  for (x = 0; x < width; x += 8) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_argb, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_argb, 16);
    vec0 = (v16u8)__msa_pckev_b((v16i8)src1, (v16i8)src0);
    vec1 = (v16u8)__msa_pckod_b((v16i8)src1, (v16i8)src0);
    reg0 = (v8i16)__msa_ilvev_b(zero, (v16i8)vec0);
    reg1 = (v8i16)__msa_ilvev_b(zero, (v16i8)vec1);
    reg2 = (v8i16)__msa_ilvod_b(zero, (v16i8)vec0);
    reg0 += vec_d0;
    reg1 += vec_d0;
    reg2 += vec_d0;
    reg0 = __msa_maxi_s_h((v8i16)reg0, 0);
    reg1 = __msa_maxi_s_h((v8i16)reg1, 0);
    reg2 = __msa_maxi_s_h((v8i16)reg2, 0);
    reg0 = __msa_min_s_h((v8i16)max, (v8i16)reg0);
    reg1 = __msa_min_s_h((v8i16)max, (v8i16)reg1);
    reg2 = __msa_min_s_h((v8i16)max, (v8i16)reg2);
    reg0 = __msa_srai_h(reg0, 3);
    reg2 = __msa_srai_h(reg2, 3);
    reg1 = __msa_srai_h(reg1, 2);
    reg2 = __msa_slli_h(reg2, 11);
    reg1 = __msa_slli_h(reg1, 5);
    reg0 |= reg1;
    dst0 = (v16u8)(reg0 | reg2);
    ST_UB(dst0, dst_rgb);
    src_argb += 32;
    dst_rgb += 16;
  }
}

void ARGBShuffleRow_MSA(const uint8* src_argb,
                        uint8* dst_argb,
                        const uint8* shuffler,
                        int width) {
  int x;
  v16u8 src0, src1, dst0, dst1;
  v16i8 vec0;
  v16i8 shuffler_vec = {0, 0, 0, 0, 4, 4, 4, 4, 8, 8, 8, 8, 12, 12, 12, 12};
  int32 val = LW((int32*)shuffler);

  vec0 = (v16i8)__msa_fill_w(val);
  shuffler_vec += vec0;

  for (x = 0; x < width; x += 8) {
    src0 = (v16u8)__msa_ld_b((v16u8*)src_argb, 0);
    src1 = (v16u8)__msa_ld_b((v16u8*)src_argb, 16);
    dst0 = (v16u8)__msa_vshf_b(shuffler_vec, (v16i8)src0, (v16i8)src0);
    dst1 = (v16u8)__msa_vshf_b(shuffler_vec, (v16i8)src1, (v16i8)src1);
    ST_UB2(dst0, dst1, dst_argb, 16);
    src_argb += 32;
    dst_argb += 32;
  }
}

void ARGBShadeRow_MSA(const uint8* src_argb,
                      uint8* dst_argb,
                      int width,
                      uint32 value) {
  int x;
  v16u8 src0, dst0;
  v8u16 vec0, vec1;
  v4u32 reg0, reg1, reg2, reg3, rgba_scale;
  v8i16 zero = {0};

  rgba_scale[0] = value;
  rgba_scale = (v4u32)__msa_ilvr_b((v16i8)rgba_scale, (v16i8)rgba_scale);
  rgba_scale = (v4u32)__msa_ilvr_h(zero, (v8i16)rgba_scale);

  for (x = 0; x < width; x += 4) {
    src0 = (v16u8)__msa_ld_b((v16u8*)src_argb, 0);
    vec0 = (v8u16)__msa_ilvr_b((v16i8)src0, (v16i8)src0);
    vec1 = (v8u16)__msa_ilvl_b((v16i8)src0, (v16i8)src0);
    reg0 = (v4u32)__msa_ilvr_h(zero, (v8i16)vec0);
    reg1 = (v4u32)__msa_ilvl_h(zero, (v8i16)vec0);
    reg2 = (v4u32)__msa_ilvr_h(zero, (v8i16)vec1);
    reg3 = (v4u32)__msa_ilvl_h(zero, (v8i16)vec1);
    reg0 *= rgba_scale;
    reg1 *= rgba_scale;
    reg2 *= rgba_scale;
    reg3 *= rgba_scale;
    reg0 = (v4u32)__msa_srai_w((v4i32)reg0, 24);
    reg1 = (v4u32)__msa_srai_w((v4i32)reg1, 24);
    reg2 = (v4u32)__msa_srai_w((v4i32)reg2, 24);
    reg3 = (v4u32)__msa_srai_w((v4i32)reg3, 24);
    vec0 = (v8u16)__msa_pckev_h((v8i16)reg1, (v8i16)reg0);
    vec1 = (v8u16)__msa_pckev_h((v8i16)reg3, (v8i16)reg2);
    dst0 = (v16u8)__msa_pckev_b((v16i8)vec1, (v16i8)vec0);
    ST_UB(dst0, dst_argb);
    src_argb += 16;
    dst_argb += 16;
  }
}

void ARGBGrayRow_MSA(const uint8* src_argb, uint8* dst_argb, int width) {
  int x;
  v16u8 src0, src1, vec0, vec1, dst0, dst1;
  v8u16 reg0;
  v16u8 const_0x26 = (v16u8)__msa_ldi_h(0x26);
  v16u8 const_0x4B0F = (v16u8)__msa_fill_h(0x4B0F);

  for (x = 0; x < width; x += 8) {
    src0 = (v16u8)__msa_ld_b((v16u8*)src_argb, 0);
    src1 = (v16u8)__msa_ld_b((v16u8*)src_argb, 16);
    vec0 = (v16u8)__msa_pckev_h((v8i16)src1, (v8i16)src0);
    vec1 = (v16u8)__msa_pckod_h((v8i16)src1, (v8i16)src0);
    reg0 = __msa_dotp_u_h(vec0, const_0x4B0F);
    reg0 = __msa_dpadd_u_h(reg0, vec1, const_0x26);
    reg0 = (v8u16)__msa_srari_h((v8i16)reg0, 7);
    vec0 = (v16u8)__msa_ilvev_b((v16i8)reg0, (v16i8)reg0);
    vec1 = (v16u8)__msa_ilvod_b((v16i8)vec1, (v16i8)vec0);
    dst0 = (v16u8)__msa_ilvr_b((v16i8)vec1, (v16i8)vec0);
    dst1 = (v16u8)__msa_ilvl_b((v16i8)vec1, (v16i8)vec0);
    ST_UB2(dst0, dst1, dst_argb, 16);
    src_argb += 32;
    dst_argb += 32;
  }
}

void ARGBSepiaRow_MSA(uint8* dst_argb, int width) {
  int x;
  v16u8 src0, src1, dst0, dst1, vec0, vec1, vec2, vec3, vec4, vec5;
  v8u16 reg0, reg1, reg2;
  v16u8 const_0x4411 = (v16u8)__msa_fill_h(0x4411);
  v16u8 const_0x23 = (v16u8)__msa_ldi_h(0x23);
  v16u8 const_0x5816 = (v16u8)__msa_fill_h(0x5816);
  v16u8 const_0x2D = (v16u8)__msa_ldi_h(0x2D);
  v16u8 const_0x6218 = (v16u8)__msa_fill_h(0x6218);
  v16u8 const_0x32 = (v16u8)__msa_ldi_h(0x32);
  v8u16 const_0xFF = (v8u16)__msa_ldi_h(0xFF);

  for (x = 0; x < width; x += 8) {
    src0 = (v16u8)__msa_ld_b((v16u8*)dst_argb, 0);
    src1 = (v16u8)__msa_ld_b((v16u8*)dst_argb, 16);
    vec0 = (v16u8)__msa_pckev_h((v8i16)src1, (v8i16)src0);
    vec1 = (v16u8)__msa_pckod_h((v8i16)src1, (v8i16)src0);
    vec3 = (v16u8)__msa_pckod_b((v16i8)vec1, (v16i8)vec1);
    reg0 = (v8u16)__msa_dotp_u_h(vec0, const_0x4411);
    reg1 = (v8u16)__msa_dotp_u_h(vec0, const_0x5816);
    reg2 = (v8u16)__msa_dotp_u_h(vec0, const_0x6218);
    reg0 = (v8u16)__msa_dpadd_u_h(reg0, vec1, const_0x23);
    reg1 = (v8u16)__msa_dpadd_u_h(reg1, vec1, const_0x2D);
    reg2 = (v8u16)__msa_dpadd_u_h(reg2, vec1, const_0x32);
    reg0 = (v8u16)__msa_srai_h((v8i16)reg0, 7);
    reg1 = (v8u16)__msa_srai_h((v8i16)reg1, 7);
    reg2 = (v8u16)__msa_srai_h((v8i16)reg2, 7);
    reg1 = (v8u16)__msa_min_u_h((v8u16)reg1, const_0xFF);
    reg2 = (v8u16)__msa_min_u_h((v8u16)reg2, const_0xFF);
    vec0 = (v16u8)__msa_pckev_b((v16i8)reg0, (v16i8)reg0);
    vec1 = (v16u8)__msa_pckev_b((v16i8)reg1, (v16i8)reg1);
    vec2 = (v16u8)__msa_pckev_b((v16i8)reg2, (v16i8)reg2);
    vec4 = (v16u8)__msa_ilvr_b((v16i8)vec2, (v16i8)vec0);
    vec5 = (v16u8)__msa_ilvr_b((v16i8)vec3, (v16i8)vec1);
    dst0 = (v16u8)__msa_ilvr_b((v16i8)vec5, (v16i8)vec4);
    dst1 = (v16u8)__msa_ilvl_b((v16i8)vec5, (v16i8)vec4);
    ST_UB2(dst0, dst1, dst_argb, 16);
    dst_argb += 32;
  }
}

void ARGB4444ToARGBRow_MSA(const uint8* src_argb4444,
                           uint8* dst_argb,
                           int width) {
  int x;
  v16u8 src0, src1;
  v8u16 vec0, vec1, vec2, vec3;
  v16u8 dst0, dst1, dst2, dst3;

  for (x = 0; x < width; x += 16) {
    src0 = (v16u8)__msa_ld_b((v16u8*)src_argb4444, 0);
    src1 = (v16u8)__msa_ld_b((v16u8*)src_argb4444, 16);
    vec0 = (v8u16)__msa_andi_b(src0, 0x0F);
    vec1 = (v8u16)__msa_andi_b(src1, 0x0F);
    vec2 = (v8u16)__msa_andi_b(src0, 0xF0);
    vec3 = (v8u16)__msa_andi_b(src1, 0xF0);
    vec0 |= (v8u16)__msa_slli_b((v16i8)vec0, 4);
    vec1 |= (v8u16)__msa_slli_b((v16i8)vec1, 4);
    vec2 |= (v8u16)__msa_srli_b((v16i8)vec2, 4);
    vec3 |= (v8u16)__msa_srli_b((v16i8)vec3, 4);
    dst0 = (v16u8)__msa_ilvr_b((v16i8)vec2, (v16i8)vec0);
    dst1 = (v16u8)__msa_ilvl_b((v16i8)vec2, (v16i8)vec0);
    dst2 = (v16u8)__msa_ilvr_b((v16i8)vec3, (v16i8)vec1);
    dst3 = (v16u8)__msa_ilvl_b((v16i8)vec3, (v16i8)vec1);
    ST_UB4(dst0, dst1, dst2, dst3, dst_argb, 16);
    src_argb4444 += 32;
    dst_argb += 64;
  }
}

void ARGB1555ToARGBRow_MSA(const uint8* src_argb1555,
                           uint8* dst_argb,
                           int width) {
  int x;
  v8u16 src0, src1;
  v8u16 vec0, vec1, vec2, vec3, vec4, vec5;
  v16u8 reg0, reg1, reg2, reg3, reg4, reg5, reg6;
  v16u8 dst0, dst1, dst2, dst3;
  v8u16 const_0x1F = (v8u16)__msa_ldi_h(0x1F);

  for (x = 0; x < width; x += 16) {
    src0 = (v8u16)__msa_ld_h((v8u16*)src_argb1555, 0);
    src1 = (v8u16)__msa_ld_h((v8u16*)src_argb1555, 16);
    vec0 = src0 & const_0x1F;
    vec1 = src1 & const_0x1F;
    src0 = (v8u16)__msa_srli_h((v8i16)src0, 5);
    src1 = (v8u16)__msa_srli_h((v8i16)src1, 5);
    vec2 = src0 & const_0x1F;
    vec3 = src1 & const_0x1F;
    src0 = (v8u16)__msa_srli_h((v8i16)src0, 5);
    src1 = (v8u16)__msa_srli_h((v8i16)src1, 5);
    vec4 = src0 & const_0x1F;
    vec5 = src1 & const_0x1F;
    src0 = (v8u16)__msa_srli_h((v8i16)src0, 5);
    src1 = (v8u16)__msa_srli_h((v8i16)src1, 5);
    reg0 = (v16u8)__msa_pckev_b((v16i8)vec1, (v16i8)vec0);
    reg1 = (v16u8)__msa_pckev_b((v16i8)vec3, (v16i8)vec2);
    reg2 = (v16u8)__msa_pckev_b((v16i8)vec5, (v16i8)vec4);
    reg3 = (v16u8)__msa_pckev_b((v16i8)src1, (v16i8)src0);
    reg4 = (v16u8)__msa_slli_b((v16i8)reg0, 3);
    reg5 = (v16u8)__msa_slli_b((v16i8)reg1, 3);
    reg6 = (v16u8)__msa_slli_b((v16i8)reg2, 3);
    reg4 |= (v16u8)__msa_srai_b((v16i8)reg0, 2);
    reg5 |= (v16u8)__msa_srai_b((v16i8)reg1, 2);
    reg6 |= (v16u8)__msa_srai_b((v16i8)reg2, 2);
    reg3 = -reg3;
    reg0 = (v16u8)__msa_ilvr_b((v16i8)reg6, (v16i8)reg4);
    reg1 = (v16u8)__msa_ilvl_b((v16i8)reg6, (v16i8)reg4);
    reg2 = (v16u8)__msa_ilvr_b((v16i8)reg3, (v16i8)reg5);
    reg3 = (v16u8)__msa_ilvl_b((v16i8)reg3, (v16i8)reg5);
    dst0 = (v16u8)__msa_ilvr_b((v16i8)reg2, (v16i8)reg0);
    dst1 = (v16u8)__msa_ilvl_b((v16i8)reg2, (v16i8)reg0);
    dst2 = (v16u8)__msa_ilvr_b((v16i8)reg3, (v16i8)reg1);
    dst3 = (v16u8)__msa_ilvl_b((v16i8)reg3, (v16i8)reg1);
    ST_UB4(dst0, dst1, dst2, dst3, dst_argb, 16);
    src_argb1555 += 32;
    dst_argb += 64;
  }
}

void RGB565ToARGBRow_MSA(const uint8* src_rgb565, uint8* dst_argb, int width) {
  int x;
  v8u16 src0, src1, vec0, vec1, vec2, vec3, vec4, vec5;
  v8u16 reg0, reg1, reg2, reg3, reg4, reg5;
  v16u8 res0, res1, res2, res3, dst0, dst1, dst2, dst3;
  v16u8 const_0xFF = (v16u8)__msa_ldi_b(0xFF);
  v8u16 const_0x1F = (v8u16)__msa_ldi_h(0x1F);
  v8u16 const_0x7E0 = (v8u16)__msa_fill_h(0x7E0);
  v8u16 const_0xF800 = (v8u16)__msa_fill_h(0xF800);

  for (x = 0; x < width; x += 16) {
    src0 = (v8u16)__msa_ld_h((v8u16*)src_rgb565, 0);
    src1 = (v8u16)__msa_ld_h((v8u16*)src_rgb565, 16);
    vec0 = src0 & const_0x1F;
    vec1 = src0 & const_0x7E0;
    vec2 = src0 & const_0xF800;
    vec3 = src1 & const_0x1F;
    vec4 = src1 & const_0x7E0;
    vec5 = src1 & const_0xF800;
    reg0 = (v8u16)__msa_slli_h((v8i16)vec0, 3);
    reg1 = (v8u16)__msa_srli_h((v8i16)vec1, 3);
    reg2 = (v8u16)__msa_srli_h((v8i16)vec2, 8);
    reg3 = (v8u16)__msa_slli_h((v8i16)vec3, 3);
    reg4 = (v8u16)__msa_srli_h((v8i16)vec4, 3);
    reg5 = (v8u16)__msa_srli_h((v8i16)vec5, 8);
    reg0 |= (v8u16)__msa_srli_h((v8i16)vec0, 2);
    reg1 |= (v8u16)__msa_srli_h((v8i16)vec1, 9);
    reg2 |= (v8u16)__msa_srli_h((v8i16)vec2, 13);
    reg3 |= (v8u16)__msa_srli_h((v8i16)vec3, 2);
    reg4 |= (v8u16)__msa_srli_h((v8i16)vec4, 9);
    reg5 |= (v8u16)__msa_srli_h((v8i16)vec5, 13);
    res0 = (v16u8)__msa_ilvev_b((v16i8)reg2, (v16i8)reg0);
    res1 = (v16u8)__msa_ilvev_b((v16i8)const_0xFF, (v16i8)reg1);
    res2 = (v16u8)__msa_ilvev_b((v16i8)reg5, (v16i8)reg3);
    res3 = (v16u8)__msa_ilvev_b((v16i8)const_0xFF, (v16i8)reg4);
    dst0 = (v16u8)__msa_ilvr_b((v16i8)res1, (v16i8)res0);
    dst1 = (v16u8)__msa_ilvl_b((v16i8)res1, (v16i8)res0);
    dst2 = (v16u8)__msa_ilvr_b((v16i8)res3, (v16i8)res2);
    dst3 = (v16u8)__msa_ilvl_b((v16i8)res3, (v16i8)res2);
    ST_UB4(dst0, dst1, dst2, dst3, dst_argb, 16);
    src_rgb565 += 32;
    dst_argb += 64;
  }
}

void RGB24ToARGBRow_MSA(const uint8* src_rgb24, uint8* dst_argb, int width) {
  int x;
  v16u8 src0, src1, src2;
  v16u8 vec0, vec1, vec2;
  v16u8 dst0, dst1, dst2, dst3;
  v16u8 const_0xFF = (v16u8)__msa_ldi_b(0xFF);
  v16i8 shuffler = {0, 1, 2, 16, 3, 4, 5, 17, 6, 7, 8, 18, 9, 10, 11, 19};

  for (x = 0; x < width; x += 16) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_rgb24, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_rgb24, 16);
    src2 = (v16u8)__msa_ld_b((v16i8*)src_rgb24, 32);
    vec0 = (v16u8)__msa_sldi_b((v16i8)src1, (v16i8)src0, 12);
    vec1 = (v16u8)__msa_sldi_b((v16i8)src2, (v16i8)src1, 8);
    vec2 = (v16u8)__msa_sldi_b((v16i8)src2, (v16i8)src2, 4);
    dst0 = (v16u8)__msa_vshf_b(shuffler, (v16i8)const_0xFF, (v16i8)src0);
    dst1 = (v16u8)__msa_vshf_b(shuffler, (v16i8)const_0xFF, (v16i8)vec0);
    dst2 = (v16u8)__msa_vshf_b(shuffler, (v16i8)const_0xFF, (v16i8)vec1);
    dst3 = (v16u8)__msa_vshf_b(shuffler, (v16i8)const_0xFF, (v16i8)vec2);
    ST_UB4(dst0, dst1, dst2, dst3, dst_argb, 16);
    src_rgb24 += 48;
    dst_argb += 64;
  }
}

void RAWToARGBRow_MSA(const uint8* src_raw, uint8* dst_argb, int width) {
  int x;
  v16u8 src0, src1, src2;
  v16u8 vec0, vec1, vec2;
  v16u8 dst0, dst1, dst2, dst3;
  v16u8 const_0xFF = (v16u8)__msa_ldi_b(0xFF);
  v16i8 mask = {2, 1, 0, 16, 5, 4, 3, 17, 8, 7, 6, 18, 11, 10, 9, 19};

  for (x = 0; x < width; x += 16) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_raw, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_raw, 16);
    src2 = (v16u8)__msa_ld_b((v16i8*)src_raw, 32);
    vec0 = (v16u8)__msa_sldi_b((v16i8)src1, (v16i8)src0, 12);
    vec1 = (v16u8)__msa_sldi_b((v16i8)src2, (v16i8)src1, 8);
    vec2 = (v16u8)__msa_sldi_b((v16i8)src2, (v16i8)src2, 4);
    dst0 = (v16u8)__msa_vshf_b(mask, (v16i8)const_0xFF, (v16i8)src0);
    dst1 = (v16u8)__msa_vshf_b(mask, (v16i8)const_0xFF, (v16i8)vec0);
    dst2 = (v16u8)__msa_vshf_b(mask, (v16i8)const_0xFF, (v16i8)vec1);
    dst3 = (v16u8)__msa_vshf_b(mask, (v16i8)const_0xFF, (v16i8)vec2);
    ST_UB4(dst0, dst1, dst2, dst3, dst_argb, 16);
    src_raw += 48;
    dst_argb += 64;
  }
}

void ARGB1555ToYRow_MSA(const uint8* src_argb1555, uint8* dst_y, int width) {
  int x;
  v8u16 src0, src1, vec0, vec1, vec2, vec3, vec4, vec5;
  v8u16 reg0, reg1, reg2, reg3, reg4, reg5;
  v16u8 dst0;
  v8u16 const_0x19 = (v8u16)__msa_ldi_h(0x19);
  v8u16 const_0x81 = (v8u16)__msa_ldi_h(0x81);
  v8u16 const_0x42 = (v8u16)__msa_ldi_h(0x42);
  v8u16 const_0x1F = (v8u16)__msa_ldi_h(0x1F);
  v8u16 const_0x1080 = (v8u16)__msa_fill_h(0x1080);

  for (x = 0; x < width; x += 16) {
    src0 = (v8u16)__msa_ld_b((v8i16*)src_argb1555, 0);
    src1 = (v8u16)__msa_ld_b((v8i16*)src_argb1555, 16);
    vec0 = src0 & const_0x1F;
    vec1 = src1 & const_0x1F;
    src0 = (v8u16)__msa_srai_h((v8i16)src0, 5);
    src1 = (v8u16)__msa_srai_h((v8i16)src1, 5);
    vec2 = src0 & const_0x1F;
    vec3 = src1 & const_0x1F;
    src0 = (v8u16)__msa_srai_h((v8i16)src0, 5);
    src1 = (v8u16)__msa_srai_h((v8i16)src1, 5);
    vec4 = src0 & const_0x1F;
    vec5 = src1 & const_0x1F;
    reg0 = (v8u16)__msa_slli_h((v8i16)vec0, 3);
    reg1 = (v8u16)__msa_slli_h((v8i16)vec1, 3);
    reg0 |= (v8u16)__msa_srai_h((v8i16)vec0, 2);
    reg1 |= (v8u16)__msa_srai_h((v8i16)vec1, 2);
    reg2 = (v8u16)__msa_slli_h((v8i16)vec2, 3);
    reg3 = (v8u16)__msa_slli_h((v8i16)vec3, 3);
    reg2 |= (v8u16)__msa_srai_h((v8i16)vec2, 2);
    reg3 |= (v8u16)__msa_srai_h((v8i16)vec3, 2);
    reg4 = (v8u16)__msa_slli_h((v8i16)vec4, 3);
    reg5 = (v8u16)__msa_slli_h((v8i16)vec5, 3);
    reg4 |= (v8u16)__msa_srai_h((v8i16)vec4, 2);
    reg5 |= (v8u16)__msa_srai_h((v8i16)vec5, 2);
    reg0 *= const_0x19;
    reg1 *= const_0x19;
    reg2 *= const_0x81;
    reg3 *= const_0x81;
    reg4 *= const_0x42;
    reg5 *= const_0x42;
    reg0 += reg2;
    reg1 += reg3;
    reg0 += reg4;
    reg1 += reg5;
    reg0 += const_0x1080;
    reg1 += const_0x1080;
    reg0 = (v8u16)__msa_srai_h((v8i16)reg0, 8);
    reg1 = (v8u16)__msa_srai_h((v8i16)reg1, 8);
    dst0 = (v16u8)__msa_pckev_b((v16i8)reg1, (v16i8)reg0);
    ST_UB(dst0, dst_y);
    src_argb1555 += 32;
    dst_y += 16;
  }
}

void RGB565ToYRow_MSA(const uint8* src_rgb565, uint8* dst_y, int width) {
  int x;
  v8u16 src0, src1, vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7;
  v8u16 reg0, reg1, reg2, reg3, reg4, reg5;
  v4u32 res0, res1, res2, res3;
  v16u8 dst0;
  v4u32 const_0x810019 = (v4u32)__msa_fill_w(0x810019);
  v4u32 const_0x010042 = (v4u32)__msa_fill_w(0x010042);
  v8i16 const_0x1080 = __msa_fill_h(0x1080);
  v8u16 const_0x1F = (v8u16)__msa_ldi_h(0x1F);
  v8u16 const_0x7E0 = (v8u16)__msa_fill_h(0x7E0);
  v8u16 const_0xF800 = (v8u16)__msa_fill_h(0xF800);

  for (x = 0; x < width; x += 16) {
    src0 = (v8u16)__msa_ld_b((v8i16*)src_rgb565, 0);
    src1 = (v8u16)__msa_ld_b((v8i16*)src_rgb565, 16);
    vec0 = src0 & const_0x1F;
    vec1 = src0 & const_0x7E0;
    vec2 = src0 & const_0xF800;
    vec3 = src1 & const_0x1F;
    vec4 = src1 & const_0x7E0;
    vec5 = src1 & const_0xF800;
    reg0 = (v8u16)__msa_slli_h((v8i16)vec0, 3);
    reg1 = (v8u16)__msa_srli_h((v8i16)vec1, 3);
    reg2 = (v8u16)__msa_srli_h((v8i16)vec2, 8);
    reg3 = (v8u16)__msa_slli_h((v8i16)vec3, 3);
    reg4 = (v8u16)__msa_srli_h((v8i16)vec4, 3);
    reg5 = (v8u16)__msa_srli_h((v8i16)vec5, 8);
    reg0 |= (v8u16)__msa_srli_h((v8i16)vec0, 2);
    reg1 |= (v8u16)__msa_srli_h((v8i16)vec1, 9);
    reg2 |= (v8u16)__msa_srli_h((v8i16)vec2, 13);
    reg3 |= (v8u16)__msa_srli_h((v8i16)vec3, 2);
    reg4 |= (v8u16)__msa_srli_h((v8i16)vec4, 9);
    reg5 |= (v8u16)__msa_srli_h((v8i16)vec5, 13);
    vec0 = (v8u16)__msa_ilvr_h((v8i16)reg1, (v8i16)reg0);
    vec1 = (v8u16)__msa_ilvl_h((v8i16)reg1, (v8i16)reg0);
    vec2 = (v8u16)__msa_ilvr_h((v8i16)reg4, (v8i16)reg3);
    vec3 = (v8u16)__msa_ilvl_h((v8i16)reg4, (v8i16)reg3);
    vec4 = (v8u16)__msa_ilvr_h(const_0x1080, (v8i16)reg2);
    vec5 = (v8u16)__msa_ilvl_h(const_0x1080, (v8i16)reg2);
    vec6 = (v8u16)__msa_ilvr_h(const_0x1080, (v8i16)reg5);
    vec7 = (v8u16)__msa_ilvl_h(const_0x1080, (v8i16)reg5);
    res0 = __msa_dotp_u_w(vec0, (v8u16)const_0x810019);
    res1 = __msa_dotp_u_w(vec1, (v8u16)const_0x810019);
    res2 = __msa_dotp_u_w(vec2, (v8u16)const_0x810019);
    res3 = __msa_dotp_u_w(vec3, (v8u16)const_0x810019);
    res0 = __msa_dpadd_u_w(res0, vec4, (v8u16)const_0x010042);
    res1 = __msa_dpadd_u_w(res1, vec5, (v8u16)const_0x010042);
    res2 = __msa_dpadd_u_w(res2, vec6, (v8u16)const_0x010042);
    res3 = __msa_dpadd_u_w(res3, vec7, (v8u16)const_0x010042);
    res0 = (v4u32)__msa_srai_w((v4i32)res0, 8);
    res1 = (v4u32)__msa_srai_w((v4i32)res1, 8);
    res2 = (v4u32)__msa_srai_w((v4i32)res2, 8);
    res3 = (v4u32)__msa_srai_w((v4i32)res3, 8);
    vec0 = (v8u16)__msa_pckev_h((v8i16)res1, (v8i16)res0);
    vec1 = (v8u16)__msa_pckev_h((v8i16)res3, (v8i16)res2);
    dst0 = (v16u8)__msa_pckev_b((v16i8)vec1, (v16i8)vec0);
    ST_UB(dst0, dst_y);
    src_rgb565 += 32;
    dst_y += 16;
  }
}

void RGB24ToYRow_MSA(const uint8* src_argb0, uint8* dst_y, int width) {
  int x;
  v16u8 src0, src1, src2, reg0, reg1, reg2, reg3, dst0;
  v8u16 vec0, vec1, vec2, vec3;
  v8u16 const_0x8119 = (v8u16)__msa_fill_h(0x8119);
  v8u16 const_0x42 = (v8u16)__msa_fill_h(0x42);
  v8u16 const_0x1080 = (v8u16)__msa_fill_h(0x1080);
  v16i8 mask0 = {0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12};
  v16i8 mask1 = {12, 13, 14, 15, 15, 16, 17, 18,
                 18, 19, 20, 21, 21, 22, 23, 24};
  v16i8 mask2 = {8, 9, 10, 11, 11, 12, 13, 14, 14, 15, 16, 17, 17, 18, 19, 20};
  v16i8 mask3 = {4, 5, 6, 7, 7, 8, 9, 10, 10, 11, 12, 13, 13, 14, 15, 16};
  v16i8 zero = {0};

  for (x = 0; x < width; x += 16) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_argb0, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_argb0, 16);
    src2 = (v16u8)__msa_ld_b((v16i8*)src_argb0, 32);
    reg0 = (v16u8)__msa_vshf_b(mask0, zero, (v16i8)src0);
    reg1 = (v16u8)__msa_vshf_b(mask1, (v16i8)src1, (v16i8)src0);
    reg2 = (v16u8)__msa_vshf_b(mask2, (v16i8)src2, (v16i8)src1);
    reg3 = (v16u8)__msa_vshf_b(mask3, zero, (v16i8)src2);
    vec0 = (v8u16)__msa_pckev_h((v8i16)reg1, (v8i16)reg0);
    vec1 = (v8u16)__msa_pckev_h((v8i16)reg3, (v8i16)reg2);
    vec2 = (v8u16)__msa_pckod_h((v8i16)reg1, (v8i16)reg0);
    vec3 = (v8u16)__msa_pckod_h((v8i16)reg3, (v8i16)reg2);
    vec0 = __msa_dotp_u_h((v16u8)vec0, (v16u8)const_0x8119);
    vec1 = __msa_dotp_u_h((v16u8)vec1, (v16u8)const_0x8119);
    vec0 = __msa_dpadd_u_h(vec0, (v16u8)vec2, (v16u8)const_0x42);
    vec1 = __msa_dpadd_u_h(vec1, (v16u8)vec3, (v16u8)const_0x42);
    vec0 += const_0x1080;
    vec1 += const_0x1080;
    vec0 = (v8u16)__msa_srai_h((v8i16)vec0, 8);
    vec1 = (v8u16)__msa_srai_h((v8i16)vec1, 8);
    dst0 = (v16u8)__msa_pckev_b((v16i8)vec1, (v16i8)vec0);
    ST_UB(dst0, dst_y);
    src_argb0 += 48;
    dst_y += 16;
  }
}

void RAWToYRow_MSA(const uint8* src_argb0, uint8* dst_y, int width) {
  int x;
  v16u8 src0, src1, src2, reg0, reg1, reg2, reg3, dst0;
  v8u16 vec0, vec1, vec2, vec3;
  v8u16 const_0x8142 = (v8u16)__msa_fill_h(0x8142);
  v8u16 const_0x19 = (v8u16)__msa_fill_h(0x19);
  v8u16 const_0x1080 = (v8u16)__msa_fill_h(0x1080);
  v16i8 mask0 = {0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12};
  v16i8 mask1 = {12, 13, 14, 15, 15, 16, 17, 18,
                 18, 19, 20, 21, 21, 22, 23, 24};
  v16i8 mask2 = {8, 9, 10, 11, 11, 12, 13, 14, 14, 15, 16, 17, 17, 18, 19, 20};
  v16i8 mask3 = {4, 5, 6, 7, 7, 8, 9, 10, 10, 11, 12, 13, 13, 14, 15, 16};
  v16i8 zero = {0};

  for (x = 0; x < width; x += 16) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_argb0, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_argb0, 16);
    src2 = (v16u8)__msa_ld_b((v16i8*)src_argb0, 32);
    reg0 = (v16u8)__msa_vshf_b(mask0, zero, (v16i8)src0);
    reg1 = (v16u8)__msa_vshf_b(mask1, (v16i8)src1, (v16i8)src0);
    reg2 = (v16u8)__msa_vshf_b(mask2, (v16i8)src2, (v16i8)src1);
    reg3 = (v16u8)__msa_vshf_b(mask3, zero, (v16i8)src2);
    vec0 = (v8u16)__msa_pckev_h((v8i16)reg1, (v8i16)reg0);
    vec1 = (v8u16)__msa_pckev_h((v8i16)reg3, (v8i16)reg2);
    vec2 = (v8u16)__msa_pckod_h((v8i16)reg1, (v8i16)reg0);
    vec3 = (v8u16)__msa_pckod_h((v8i16)reg3, (v8i16)reg2);
    vec0 = __msa_dotp_u_h((v16u8)vec0, (v16u8)const_0x8142);
    vec1 = __msa_dotp_u_h((v16u8)vec1, (v16u8)const_0x8142);
    vec0 = __msa_dpadd_u_h(vec0, (v16u8)vec2, (v16u8)const_0x19);
    vec1 = __msa_dpadd_u_h(vec1, (v16u8)vec3, (v16u8)const_0x19);
    vec0 += const_0x1080;
    vec1 += const_0x1080;
    vec0 = (v8u16)__msa_srai_h((v8i16)vec0, 8);
    vec1 = (v8u16)__msa_srai_h((v8i16)vec1, 8);
    dst0 = (v16u8)__msa_pckev_b((v16i8)vec1, (v16i8)vec0);
    ST_UB(dst0, dst_y);
    src_argb0 += 48;
    dst_y += 16;
  }
}

void ARGB1555ToUVRow_MSA(const uint8* src_argb1555,
                         int src_stride_argb1555,
                         uint8* dst_u,
                         uint8* dst_v,
                         int width) {
  int x;
  const uint16* s = (const uint16*)src_argb1555;
  const uint16* t = (const uint16*)(src_argb1555 + src_stride_argb1555);
  int64_t res0, res1;
  v8u16 src0, src1, src2, src3, reg0, reg1, reg2, reg3;
  v8u16 vec0, vec1, vec2, vec3, vec4, vec5, vec6;
  v16u8 dst0;
  v8u16 const_0x70 = (v8u16)__msa_ldi_h(0x70);
  v8u16 const_0x4A = (v8u16)__msa_ldi_h(0x4A);
  v8u16 const_0x26 = (v8u16)__msa_ldi_h(0x26);
  v8u16 const_0x5E = (v8u16)__msa_ldi_h(0x5E);
  v8u16 const_0x12 = (v8u16)__msa_ldi_h(0x12);
  v8u16 const_0x8080 = (v8u16)__msa_fill_h(0x8080);
  v8u16 const_0x1F = (v8u16)__msa_ldi_h(0x1F);

  for (x = 0; x < width; x += 16) {
    src0 = (v8u16)__msa_ld_b((v8i16*)s, 0);
    src1 = (v8u16)__msa_ld_b((v8i16*)s, 16);
    src2 = (v8u16)__msa_ld_b((v8i16*)t, 0);
    src3 = (v8u16)__msa_ld_b((v8i16*)t, 16);
    vec0 = src0 & const_0x1F;
    vec1 = src1 & const_0x1F;
    vec0 += src2 & const_0x1F;
    vec1 += src3 & const_0x1F;
    vec0 = (v8u16)__msa_pckev_b((v16i8)vec1, (v16i8)vec0);
    src0 = (v8u16)__msa_srai_h((v8i16)src0, 5);
    src1 = (v8u16)__msa_srai_h((v8i16)src1, 5);
    src2 = (v8u16)__msa_srai_h((v8i16)src2, 5);
    src3 = (v8u16)__msa_srai_h((v8i16)src3, 5);
    vec2 = src0 & const_0x1F;
    vec3 = src1 & const_0x1F;
    vec2 += src2 & const_0x1F;
    vec3 += src3 & const_0x1F;
    vec2 = (v8u16)__msa_pckev_b((v16i8)vec3, (v16i8)vec2);
    src0 = (v8u16)__msa_srai_h((v8i16)src0, 5);
    src1 = (v8u16)__msa_srai_h((v8i16)src1, 5);
    src2 = (v8u16)__msa_srai_h((v8i16)src2, 5);
    src3 = (v8u16)__msa_srai_h((v8i16)src3, 5);
    vec4 = src0 & const_0x1F;
    vec5 = src1 & const_0x1F;
    vec4 += src2 & const_0x1F;
    vec5 += src3 & const_0x1F;
    vec4 = (v8u16)__msa_pckev_b((v16i8)vec5, (v16i8)vec4);
    vec0 = __msa_hadd_u_h((v16u8)vec0, (v16u8)vec0);
    vec2 = __msa_hadd_u_h((v16u8)vec2, (v16u8)vec2);
    vec4 = __msa_hadd_u_h((v16u8)vec4, (v16u8)vec4);
    vec6 = (v8u16)__msa_slli_h((v8i16)vec0, 1);
    vec6 |= (v8u16)__msa_srai_h((v8i16)vec0, 6);
    vec0 = (v8u16)__msa_slli_h((v8i16)vec2, 1);
    vec0 |= (v8u16)__msa_srai_h((v8i16)vec2, 6);
    vec2 = (v8u16)__msa_slli_h((v8i16)vec4, 1);
    vec2 |= (v8u16)__msa_srai_h((v8i16)vec4, 6);
    reg0 = vec6 * const_0x70;
    reg1 = vec0 * const_0x4A;
    reg2 = vec2 * const_0x70;
    reg3 = vec0 * const_0x5E;
    reg0 += const_0x8080;
    reg1 += vec2 * const_0x26;
    reg2 += const_0x8080;
    reg3 += vec6 * const_0x12;
    reg0 -= reg1;
    reg2 -= reg3;
    reg0 = (v8u16)__msa_srai_h((v8i16)reg0, 8);
    reg2 = (v8u16)__msa_srai_h((v8i16)reg2, 8);
    dst0 = (v16u8)__msa_pckev_b((v16i8)reg2, (v16i8)reg0);
    res0 = __msa_copy_u_d((v2i64)dst0, 0);
    res1 = __msa_copy_u_d((v2i64)dst0, 1);
    SD(res0, dst_u);
    SD(res1, dst_v);
    s += 16;
    t += 16;
    dst_u += 8;
    dst_v += 8;
  }
}

void RGB565ToUVRow_MSA(const uint8* src_rgb565,
                       int src_stride_rgb565,
                       uint8* dst_u,
                       uint8* dst_v,
                       int width) {
  int x;
  const uint16* s = (const uint16*)src_rgb565;
  const uint16* t = (const uint16*)(src_rgb565 + src_stride_rgb565);
  int64_t res0, res1;
  v8u16 src0, src1, src2, src3, reg0, reg1, reg2, reg3;
  v8u16 vec0, vec1, vec2, vec3, vec4, vec5;
  v16u8 dst0;
  v8u16 const_0x70 = (v8u16)__msa_ldi_h(0x70);
  v8u16 const_0x4A = (v8u16)__msa_ldi_h(0x4A);
  v8u16 const_0x26 = (v8u16)__msa_ldi_h(0x26);
  v8u16 const_0x5E = (v8u16)__msa_ldi_h(0x5E);
  v8u16 const_0x12 = (v8u16)__msa_ldi_h(0x12);
  v8u16 const_32896 = (v8u16)__msa_fill_h(0x8080);
  v8u16 const_0x1F = (v8u16)__msa_ldi_h(0x1F);
  v8u16 const_0x3F = (v8u16)__msa_fill_h(0x3F);

  for (x = 0; x < width; x += 16) {
    src0 = (v8u16)__msa_ld_b((v8i16*)s, 0);
    src1 = (v8u16)__msa_ld_b((v8i16*)s, 16);
    src2 = (v8u16)__msa_ld_b((v8i16*)t, 0);
    src3 = (v8u16)__msa_ld_b((v8i16*)t, 16);
    vec0 = src0 & const_0x1F;
    vec1 = src1 & const_0x1F;
    vec0 += src2 & const_0x1F;
    vec1 += src3 & const_0x1F;
    vec0 = (v8u16)__msa_pckev_b((v16i8)vec1, (v16i8)vec0);
    src0 = (v8u16)__msa_srai_h((v8i16)src0, 5);
    src1 = (v8u16)__msa_srai_h((v8i16)src1, 5);
    src2 = (v8u16)__msa_srai_h((v8i16)src2, 5);
    src3 = (v8u16)__msa_srai_h((v8i16)src3, 5);
    vec2 = src0 & const_0x3F;
    vec3 = src1 & const_0x3F;
    vec2 += src2 & const_0x3F;
    vec3 += src3 & const_0x3F;
    vec1 = (v8u16)__msa_pckev_b((v16i8)vec3, (v16i8)vec2);
    src0 = (v8u16)__msa_srai_h((v8i16)src0, 6);
    src1 = (v8u16)__msa_srai_h((v8i16)src1, 6);
    src2 = (v8u16)__msa_srai_h((v8i16)src2, 6);
    src3 = (v8u16)__msa_srai_h((v8i16)src3, 6);
    vec4 = src0 & const_0x1F;
    vec5 = src1 & const_0x1F;
    vec4 += src2 & const_0x1F;
    vec5 += src3 & const_0x1F;
    vec2 = (v8u16)__msa_pckev_b((v16i8)vec5, (v16i8)vec4);
    vec0 = __msa_hadd_u_h((v16u8)vec0, (v16u8)vec0);
    vec1 = __msa_hadd_u_h((v16u8)vec1, (v16u8)vec1);
    vec2 = __msa_hadd_u_h((v16u8)vec2, (v16u8)vec2);
    vec3 = (v8u16)__msa_slli_h((v8i16)vec0, 1);
    vec3 |= (v8u16)__msa_srai_h((v8i16)vec0, 6);
    vec4 = (v8u16)__msa_slli_h((v8i16)vec2, 1);
    vec4 |= (v8u16)__msa_srai_h((v8i16)vec2, 6);
    reg0 = vec3 * const_0x70;
    reg1 = vec1 * const_0x4A;
    reg2 = vec4 * const_0x70;
    reg3 = vec1 * const_0x5E;
    reg0 += const_32896;
    reg1 += vec4 * const_0x26;
    reg2 += const_32896;
    reg3 += vec3 * const_0x12;
    reg0 -= reg1;
    reg2 -= reg3;
    reg0 = (v8u16)__msa_srai_h((v8i16)reg0, 8);
    reg2 = (v8u16)__msa_srai_h((v8i16)reg2, 8);
    dst0 = (v16u8)__msa_pckev_b((v16i8)reg2, (v16i8)reg0);
    res0 = __msa_copy_u_d((v2i64)dst0, 0);
    res1 = __msa_copy_u_d((v2i64)dst0, 1);
    SD(res0, dst_u);
    SD(res1, dst_v);
    s += 16;
    t += 16;
    dst_u += 8;
    dst_v += 8;
  }
}

void RGB24ToUVRow_MSA(const uint8* src_rgb0,
                      int src_stride_rgb,
                      uint8* dst_u,
                      uint8* dst_v,
                      int width) {
  int x;
  const uint8* s = src_rgb0;
  const uint8* t = src_rgb0 + src_stride_rgb;
  int64 res0, res1;
  v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
  v16u8 inp0, inp1, inp2, inp3, inp4, inp5;
  v8u16 vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7;
  v8i16 reg0, reg1, reg2, reg3;
  v16u8 dst0;
  v8u16 const_0x70 = (v8u16)__msa_fill_h(0x70);
  v8u16 const_0x4A = (v8u16)__msa_fill_h(0x4A);
  v8u16 const_0x26 = (v8u16)__msa_fill_h(0x26);
  v8u16 const_0x5E = (v8u16)__msa_fill_h(0x5E);
  v8u16 const_0x12 = (v8u16)__msa_fill_h(0x12);
  v8u16 const_0x8080 = (v8u16)__msa_fill_h(0x8080);
  v16i8 mask = {0, 1, 2, 16, 3, 4, 5, 17, 6, 7, 8, 18, 9, 10, 11, 19};
  v16i8 zero = {0};

  for (x = 0; x < width; x += 16) {
    inp0 = (v16u8)__msa_ld_b((v16i8*)s, 0);
    inp1 = (v16u8)__msa_ld_b((v16i8*)s, 16);
    inp2 = (v16u8)__msa_ld_b((v16i8*)s, 32);
    inp3 = (v16u8)__msa_ld_b((v16i8*)t, 0);
    inp4 = (v16u8)__msa_ld_b((v16i8*)t, 16);
    inp5 = (v16u8)__msa_ld_b((v16i8*)t, 32);
    src1 = (v16u8)__msa_sldi_b((v16i8)inp1, (v16i8)inp0, 12);
    src5 = (v16u8)__msa_sldi_b((v16i8)inp4, (v16i8)inp3, 12);
    src2 = (v16u8)__msa_sldi_b((v16i8)inp2, (v16i8)inp1, 8);
    src6 = (v16u8)__msa_sldi_b((v16i8)inp5, (v16i8)inp4, 8);
    src3 = (v16u8)__msa_sldi_b((v16i8)inp2, (v16i8)inp2, 4);
    src7 = (v16u8)__msa_sldi_b((v16i8)inp5, (v16i8)inp5, 4);
    src0 = (v16u8)__msa_vshf_b(mask, (v16i8)zero, (v16i8)inp0);
    src1 = (v16u8)__msa_vshf_b(mask, (v16i8)zero, (v16i8)src1);
    src2 = (v16u8)__msa_vshf_b(mask, (v16i8)zero, (v16i8)src2);
    src3 = (v16u8)__msa_vshf_b(mask, (v16i8)zero, (v16i8)src3);
    src4 = (v16u8)__msa_vshf_b(mask, (v16i8)zero, (v16i8)inp3);
    src5 = (v16u8)__msa_vshf_b(mask, (v16i8)zero, (v16i8)src5);
    src6 = (v16u8)__msa_vshf_b(mask, (v16i8)zero, (v16i8)src6);
    src7 = (v16u8)__msa_vshf_b(mask, (v16i8)zero, (v16i8)src7);
    vec0 = (v8u16)__msa_ilvr_b((v16i8)src4, (v16i8)src0);
    vec1 = (v8u16)__msa_ilvl_b((v16i8)src4, (v16i8)src0);
    vec2 = (v8u16)__msa_ilvr_b((v16i8)src5, (v16i8)src1);
    vec3 = (v8u16)__msa_ilvl_b((v16i8)src5, (v16i8)src1);
    vec4 = (v8u16)__msa_ilvr_b((v16i8)src6, (v16i8)src2);
    vec5 = (v8u16)__msa_ilvl_b((v16i8)src6, (v16i8)src2);
    vec6 = (v8u16)__msa_ilvr_b((v16i8)src7, (v16i8)src3);
    vec7 = (v8u16)__msa_ilvl_b((v16i8)src7, (v16i8)src3);
    vec0 = (v8u16)__msa_hadd_u_h((v16u8)vec0, (v16u8)vec0);
    vec1 = (v8u16)__msa_hadd_u_h((v16u8)vec1, (v16u8)vec1);
    vec2 = (v8u16)__msa_hadd_u_h((v16u8)vec2, (v16u8)vec2);
    vec3 = (v8u16)__msa_hadd_u_h((v16u8)vec3, (v16u8)vec3);
    vec4 = (v8u16)__msa_hadd_u_h((v16u8)vec4, (v16u8)vec4);
    vec5 = (v8u16)__msa_hadd_u_h((v16u8)vec5, (v16u8)vec5);
    vec6 = (v8u16)__msa_hadd_u_h((v16u8)vec6, (v16u8)vec6);
    vec7 = (v8u16)__msa_hadd_u_h((v16u8)vec7, (v16u8)vec7);
    reg0 = (v8i16)__msa_pckev_d((v2i64)vec1, (v2i64)vec0);
    reg1 = (v8i16)__msa_pckev_d((v2i64)vec3, (v2i64)vec2);
    reg2 = (v8i16)__msa_pckev_d((v2i64)vec5, (v2i64)vec4);
    reg3 = (v8i16)__msa_pckev_d((v2i64)vec7, (v2i64)vec6);
    reg0 += (v8i16)__msa_pckod_d((v2i64)vec1, (v2i64)vec0);
    reg1 += (v8i16)__msa_pckod_d((v2i64)vec3, (v2i64)vec2);
    reg2 += (v8i16)__msa_pckod_d((v2i64)vec5, (v2i64)vec4);
    reg3 += (v8i16)__msa_pckod_d((v2i64)vec7, (v2i64)vec6);
    reg0 = __msa_srai_h((v8i16)reg0, 2);
    reg1 = __msa_srai_h((v8i16)reg1, 2);
    reg2 = __msa_srai_h((v8i16)reg2, 2);
    reg3 = __msa_srai_h((v8i16)reg3, 2);
    vec4 = (v8u16)__msa_pckev_h(reg1, reg0);
    vec5 = (v8u16)__msa_pckev_h(reg3, reg2);
    vec6 = (v8u16)__msa_pckod_h(reg1, reg0);
    vec7 = (v8u16)__msa_pckod_h(reg3, reg2);
    vec0 = (v8u16)__msa_pckev_h((v8i16)vec5, (v8i16)vec4);
    vec1 = (v8u16)__msa_pckev_h((v8i16)vec7, (v8i16)vec6);
    vec2 = (v8u16)__msa_pckod_h((v8i16)vec5, (v8i16)vec4);
    vec3 = vec0 * const_0x70;
    vec4 = vec1 * const_0x4A;
    vec5 = vec2 * const_0x26;
    vec2 *= const_0x70;
    vec1 *= const_0x5E;
    vec0 *= const_0x12;
    reg0 = __msa_subv_h((v8i16)vec3, (v8i16)vec4);
    reg1 = __msa_subv_h((v8i16)const_0x8080, (v8i16)vec5);
    reg2 = __msa_subv_h((v8i16)vec2, (v8i16)vec1);
    reg3 = __msa_subv_h((v8i16)const_0x8080, (v8i16)vec0);
    reg0 += reg1;
    reg2 += reg3;
    reg0 = __msa_srai_h(reg0, 8);
    reg2 = __msa_srai_h(reg2, 8);
    dst0 = (v16u8)__msa_pckev_b((v16i8)reg2, (v16i8)reg0);
    res0 = __msa_copy_u_d((v2i64)dst0, 0);
    res1 = __msa_copy_u_d((v2i64)dst0, 1);
    SD(res0, dst_u);
    SD(res1, dst_v);
    t += 48;
    s += 48;
    dst_u += 8;
    dst_v += 8;
  }
}

void RAWToUVRow_MSA(const uint8* src_rgb0,
                    int src_stride_rgb,
                    uint8* dst_u,
                    uint8* dst_v,
                    int width) {
  int x;
  const uint8* s = src_rgb0;
  const uint8* t = src_rgb0 + src_stride_rgb;
  int64 res0, res1;
  v16u8 inp0, inp1, inp2, inp3, inp4, inp5;
  v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
  v8u16 vec0, vec1, vec2, vec3, vec4, vec5, vec6, vec7;
  v8i16 reg0, reg1, reg2, reg3;
  v16u8 dst0;
  v8u16 const_0x70 = (v8u16)__msa_fill_h(0x70);
  v8u16 const_0x4A = (v8u16)__msa_fill_h(0x4A);
  v8u16 const_0x26 = (v8u16)__msa_fill_h(0x26);
  v8u16 const_0x5E = (v8u16)__msa_fill_h(0x5E);
  v8u16 const_0x12 = (v8u16)__msa_fill_h(0x12);
  v8u16 const_0x8080 = (v8u16)__msa_fill_h(0x8080);
  v16i8 mask = {0, 1, 2, 16, 3, 4, 5, 17, 6, 7, 8, 18, 9, 10, 11, 19};
  v16i8 zero = {0};

  for (x = 0; x < width; x += 16) {
    inp0 = (v16u8)__msa_ld_b((v16i8*)s, 0);
    inp1 = (v16u8)__msa_ld_b((v16i8*)s, 16);
    inp2 = (v16u8)__msa_ld_b((v16i8*)s, 32);
    inp3 = (v16u8)__msa_ld_b((v16i8*)t, 0);
    inp4 = (v16u8)__msa_ld_b((v16i8*)t, 16);
    inp5 = (v16u8)__msa_ld_b((v16i8*)t, 32);
    src1 = (v16u8)__msa_sldi_b((v16i8)inp1, (v16i8)inp0, 12);
    src5 = (v16u8)__msa_sldi_b((v16i8)inp4, (v16i8)inp3, 12);
    src2 = (v16u8)__msa_sldi_b((v16i8)inp2, (v16i8)inp1, 8);
    src6 = (v16u8)__msa_sldi_b((v16i8)inp5, (v16i8)inp4, 8);
    src3 = (v16u8)__msa_sldi_b((v16i8)inp2, (v16i8)inp2, 4);
    src7 = (v16u8)__msa_sldi_b((v16i8)inp5, (v16i8)inp5, 4);
    src0 = (v16u8)__msa_vshf_b(mask, (v16i8)zero, (v16i8)inp0);
    src1 = (v16u8)__msa_vshf_b(mask, (v16i8)zero, (v16i8)src1);
    src2 = (v16u8)__msa_vshf_b(mask, (v16i8)zero, (v16i8)src2);
    src3 = (v16u8)__msa_vshf_b(mask, (v16i8)zero, (v16i8)src3);
    src4 = (v16u8)__msa_vshf_b(mask, (v16i8)zero, (v16i8)inp3);
    src5 = (v16u8)__msa_vshf_b(mask, (v16i8)zero, (v16i8)src5);
    src6 = (v16u8)__msa_vshf_b(mask, (v16i8)zero, (v16i8)src6);
    src7 = (v16u8)__msa_vshf_b(mask, (v16i8)zero, (v16i8)src7);
    vec0 = (v8u16)__msa_ilvr_b((v16i8)src4, (v16i8)src0);
    vec1 = (v8u16)__msa_ilvl_b((v16i8)src4, (v16i8)src0);
    vec2 = (v8u16)__msa_ilvr_b((v16i8)src5, (v16i8)src1);
    vec3 = (v8u16)__msa_ilvl_b((v16i8)src5, (v16i8)src1);
    vec4 = (v8u16)__msa_ilvr_b((v16i8)src6, (v16i8)src2);
    vec5 = (v8u16)__msa_ilvl_b((v16i8)src6, (v16i8)src2);
    vec6 = (v8u16)__msa_ilvr_b((v16i8)src7, (v16i8)src3);
    vec7 = (v8u16)__msa_ilvl_b((v16i8)src7, (v16i8)src3);
    vec0 = (v8u16)__msa_hadd_u_h((v16u8)vec0, (v16u8)vec0);
    vec1 = (v8u16)__msa_hadd_u_h((v16u8)vec1, (v16u8)vec1);
    vec2 = (v8u16)__msa_hadd_u_h((v16u8)vec2, (v16u8)vec2);
    vec3 = (v8u16)__msa_hadd_u_h((v16u8)vec3, (v16u8)vec3);
    vec4 = (v8u16)__msa_hadd_u_h((v16u8)vec4, (v16u8)vec4);
    vec5 = (v8u16)__msa_hadd_u_h((v16u8)vec5, (v16u8)vec5);
    vec6 = (v8u16)__msa_hadd_u_h((v16u8)vec6, (v16u8)vec6);
    vec7 = (v8u16)__msa_hadd_u_h((v16u8)vec7, (v16u8)vec7);
    reg0 = (v8i16)__msa_pckev_d((v2i64)vec1, (v2i64)vec0);
    reg1 = (v8i16)__msa_pckev_d((v2i64)vec3, (v2i64)vec2);
    reg2 = (v8i16)__msa_pckev_d((v2i64)vec5, (v2i64)vec4);
    reg3 = (v8i16)__msa_pckev_d((v2i64)vec7, (v2i64)vec6);
    reg0 += (v8i16)__msa_pckod_d((v2i64)vec1, (v2i64)vec0);
    reg1 += (v8i16)__msa_pckod_d((v2i64)vec3, (v2i64)vec2);
    reg2 += (v8i16)__msa_pckod_d((v2i64)vec5, (v2i64)vec4);
    reg3 += (v8i16)__msa_pckod_d((v2i64)vec7, (v2i64)vec6);
    reg0 = __msa_srai_h(reg0, 2);
    reg1 = __msa_srai_h(reg1, 2);
    reg2 = __msa_srai_h(reg2, 2);
    reg3 = __msa_srai_h(reg3, 2);
    vec4 = (v8u16)__msa_pckev_h((v8i16)reg1, (v8i16)reg0);
    vec5 = (v8u16)__msa_pckev_h((v8i16)reg3, (v8i16)reg2);
    vec6 = (v8u16)__msa_pckod_h((v8i16)reg1, (v8i16)reg0);
    vec7 = (v8u16)__msa_pckod_h((v8i16)reg3, (v8i16)reg2);
    vec0 = (v8u16)__msa_pckod_h((v8i16)vec5, (v8i16)vec4);
    vec1 = (v8u16)__msa_pckev_h((v8i16)vec7, (v8i16)vec6);
    vec2 = (v8u16)__msa_pckev_h((v8i16)vec5, (v8i16)vec4);
    vec3 = vec0 * const_0x70;
    vec4 = vec1 * const_0x4A;
    vec5 = vec2 * const_0x26;
    vec2 *= const_0x70;
    vec1 *= const_0x5E;
    vec0 *= const_0x12;
    reg0 = __msa_subv_h((v8i16)vec3, (v8i16)vec4);
    reg1 = __msa_subv_h((v8i16)const_0x8080, (v8i16)vec5);
    reg2 = __msa_subv_h((v8i16)vec2, (v8i16)vec1);
    reg3 = __msa_subv_h((v8i16)const_0x8080, (v8i16)vec0);
    reg0 += reg1;
    reg2 += reg3;
    reg0 = __msa_srai_h(reg0, 8);
    reg2 = __msa_srai_h(reg2, 8);
    dst0 = (v16u8)__msa_pckev_b((v16i8)reg2, (v16i8)reg0);
    res0 = __msa_copy_u_d((v2i64)dst0, 0);
    res1 = __msa_copy_u_d((v2i64)dst0, 1);
    SD(res0, dst_u);
    SD(res1, dst_v);
    t += 48;
    s += 48;
    dst_u += 8;
    dst_v += 8;
  }
}

void NV12ToARGBRow_MSA(const uint8* src_y,
                       const uint8* src_uv,
                       uint8* rgb_buf,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  int x;
  uint64 val0, val1;
  v16u8 src0, src1, res0, res1, dst0, dst1;
  v8i16 vec0, vec1, vec2;
  v4i32 vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg, vec_br, vec_yg;
  v4i32 vec_ubvr, vec_ugvg;
  v16u8 zero = {0};
  v16u8 const_255 = (v16u8)__msa_ldi_b(255);

  YUVTORGB_SETUP(yuvconstants, vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg,
                 vec_br, vec_yg);
  vec_ubvr = __msa_ilvr_w(vec_vr, vec_ub);
  vec_ugvg = (v4i32)__msa_ilvev_h((v8i16)vec_vg, (v8i16)vec_ug);

  for (x = 0; x < width; x += 8) {
    val0 = LD(src_y);
    val1 = LD(src_uv);
    src0 = (v16u8)__msa_insert_d((v2i64)zero, 0, val0);
    src1 = (v16u8)__msa_insert_d((v2i64)zero, 0, val1);
    YUVTORGB(src0, src1, vec_ubvr, vec_ugvg, vec_bb, vec_bg, vec_br, vec_yg,
             vec0, vec1, vec2);
    res0 = (v16u8)__msa_ilvev_b((v16i8)vec2, (v16i8)vec0);
    res1 = (v16u8)__msa_ilvev_b((v16i8)const_255, (v16i8)vec1);
    dst0 = (v16u8)__msa_ilvr_b((v16i8)res1, (v16i8)res0);
    dst1 = (v16u8)__msa_ilvl_b((v16i8)res1, (v16i8)res0);
    ST_UB2(dst0, dst1, rgb_buf, 16);
    src_y += 8;
    src_uv += 8;
    rgb_buf += 32;
  }
}

void NV12ToRGB565Row_MSA(const uint8* src_y,
                         const uint8* src_uv,
                         uint8* rgb_buf,
                         const struct YuvConstants* yuvconstants,
                         int width) {
  int x;
  uint64 val0, val1;
  v16u8 src0, src1, dst0;
  v8i16 vec0, vec1, vec2;
  v4i32 vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg, vec_br, vec_yg;
  v4i32 vec_ubvr, vec_ugvg;
  v16u8 zero = {0};

  YUVTORGB_SETUP(yuvconstants, vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg,
                 vec_br, vec_yg);
  vec_ubvr = __msa_ilvr_w(vec_vr, vec_ub);
  vec_ugvg = (v4i32)__msa_ilvev_h((v8i16)vec_vg, (v8i16)vec_ug);

  for (x = 0; x < width; x += 8) {
    val0 = LD(src_y);
    val1 = LD(src_uv);
    src0 = (v16u8)__msa_insert_d((v2i64)zero, 0, val0);
    src1 = (v16u8)__msa_insert_d((v2i64)zero, 0, val1);
    YUVTORGB(src0, src1, vec_ubvr, vec_ugvg, vec_bb, vec_bg, vec_br, vec_yg,
             vec0, vec1, vec2);
    vec0 = vec0 >> 3;
    vec1 = (vec1 >> 2) << 5;
    vec2 = (vec2 >> 3) << 11;
    dst0 = (v16u8)(vec0 | vec1 | vec2);
    ST_UB(dst0, rgb_buf);
    src_y += 8;
    src_uv += 8;
    rgb_buf += 16;
  }
}

void NV21ToARGBRow_MSA(const uint8* src_y,
                       const uint8* src_vu,
                       uint8* rgb_buf,
                       const struct YuvConstants* yuvconstants,
                       int width) {
  int x;
  uint64 val0, val1;
  v16u8 src0, src1, res0, res1, dst0, dst1;
  v8i16 vec0, vec1, vec2;
  v4i32 vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg, vec_br, vec_yg;
  v4i32 vec_ubvr, vec_ugvg;
  v16u8 const_255 = (v16u8)__msa_ldi_b(255);
  v16u8 zero = {0};
  v16i8 shuffler = {1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14};

  YUVTORGB_SETUP(yuvconstants, vec_ub, vec_vr, vec_ug, vec_vg, vec_bb, vec_bg,
                 vec_br, vec_yg);
  vec_ubvr = __msa_ilvr_w(vec_vr, vec_ub);
  vec_ugvg = (v4i32)__msa_ilvev_h((v8i16)vec_vg, (v8i16)vec_ug);

  for (x = 0; x < width; x += 8) {
    val0 = LD(src_y);
    val1 = LD(src_vu);
    src0 = (v16u8)__msa_insert_d((v2i64)zero, 0, val0);
    src1 = (v16u8)__msa_insert_d((v2i64)zero, 0, val1);
    src1 = (v16u8)__msa_vshf_b(shuffler, (v16i8)src1, (v16i8)src1);
    YUVTORGB(src0, src1, vec_ubvr, vec_ugvg, vec_bb, vec_bg, vec_br, vec_yg,
             vec0, vec1, vec2);
    res0 = (v16u8)__msa_ilvev_b((v16i8)vec2, (v16i8)vec0);
    res1 = (v16u8)__msa_ilvev_b((v16i8)const_255, (v16i8)vec1);
    dst0 = (v16u8)__msa_ilvr_b((v16i8)res1, (v16i8)res0);
    dst1 = (v16u8)__msa_ilvl_b((v16i8)res1, (v16i8)res0);
    ST_UB2(dst0, dst1, rgb_buf, 16);
    src_y += 8;
    src_vu += 8;
    rgb_buf += 32;
  }
}

void SobelRow_MSA(const uint8* src_sobelx,
                  const uint8* src_sobely,
                  uint8* dst_argb,
                  int width) {
  int x;
  v16u8 src0, src1, vec0, dst0, dst1, dst2, dst3;
  v16i8 mask0 = {0, 0, 0, 16, 1, 1, 1, 16, 2, 2, 2, 16, 3, 3, 3, 16};
  v16i8 const_0x4 = __msa_ldi_b(0x4);
  v16i8 mask1 = mask0 + const_0x4;
  v16i8 mask2 = mask1 + const_0x4;
  v16i8 mask3 = mask2 + const_0x4;
  v16u8 const_0xFF = (v16u8)__msa_ldi_b(0xFF);

  for (x = 0; x < width; x += 16) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_sobelx, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_sobely, 0);
    vec0 = __msa_adds_u_b(src0, src1);
    dst0 = (v16u8)__msa_vshf_b(mask0, (v16i8)const_0xFF, (v16i8)vec0);
    dst1 = (v16u8)__msa_vshf_b(mask1, (v16i8)const_0xFF, (v16i8)vec0);
    dst2 = (v16u8)__msa_vshf_b(mask2, (v16i8)const_0xFF, (v16i8)vec0);
    dst3 = (v16u8)__msa_vshf_b(mask3, (v16i8)const_0xFF, (v16i8)vec0);
    ST_UB4(dst0, dst1, dst2, dst3, dst_argb, 16);
    src_sobelx += 16;
    src_sobely += 16;
    dst_argb += 64;
  }
}

void SobelToPlaneRow_MSA(const uint8* src_sobelx,
                         const uint8* src_sobely,
                         uint8* dst_y,
                         int width) {
  int x;
  v16u8 src0, src1, src2, src3, dst0, dst1;

  for (x = 0; x < width; x += 32) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_sobelx, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_sobelx, 16);
    src2 = (v16u8)__msa_ld_b((v16i8*)src_sobely, 0);
    src3 = (v16u8)__msa_ld_b((v16i8*)src_sobely, 16);
    dst0 = __msa_adds_u_b(src0, src2);
    dst1 = __msa_adds_u_b(src1, src3);
    ST_UB2(dst0, dst1, dst_y, 16);
    src_sobelx += 32;
    src_sobely += 32;
    dst_y += 32;
  }
}

void SobelXYRow_MSA(const uint8* src_sobelx,
                    const uint8* src_sobely,
                    uint8* dst_argb,
                    int width) {
  int x;
  v16u8 src0, src1, vec0, vec1, vec2;
  v16u8 reg0, reg1, dst0, dst1, dst2, dst3;
  v16u8 const_0xFF = (v16u8)__msa_ldi_b(0xFF);

  for (x = 0; x < width; x += 16) {
    src0 = (v16u8)__msa_ld_b((v16i8*)src_sobelx, 0);
    src1 = (v16u8)__msa_ld_b((v16i8*)src_sobely, 0);
    vec0 = __msa_adds_u_b(src0, src1);
    vec1 = (v16u8)__msa_ilvr_b((v16i8)src0, (v16i8)src1);
    vec2 = (v16u8)__msa_ilvl_b((v16i8)src0, (v16i8)src1);
    reg0 = (v16u8)__msa_ilvr_b((v16i8)const_0xFF, (v16i8)vec0);
    reg1 = (v16u8)__msa_ilvl_b((v16i8)const_0xFF, (v16i8)vec0);
    dst0 = (v16u8)__msa_ilvr_b((v16i8)reg0, (v16i8)vec1);
    dst1 = (v16u8)__msa_ilvl_b((v16i8)reg0, (v16i8)vec1);
    dst2 = (v16u8)__msa_ilvr_b((v16i8)reg1, (v16i8)vec2);
    dst3 = (v16u8)__msa_ilvl_b((v16i8)reg1, (v16i8)vec2);
    ST_UB4(dst0, dst1, dst2, dst3, dst_argb, 16);
    src_sobelx += 16;
    src_sobely += 16;
    dst_argb += 64;
  }
}

#ifdef __cplusplus
}  // extern "C"
}  // namespace libyuv
#endif

#endif  // !defined(LIBYUV_DISABLE_MSA) && defined(__mips_msa)
