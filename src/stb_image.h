/*
 *  Copyright (C) 2021 Steward Fu
 *  Copyright (C) 2001 Peponas Mathieu
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __STB_IMAGE_H__
#define __STB_IMAGE_H__

#include <stdio.h>
#include <stdint.h>

#define STBI_VERSION 1

enum {
  STBI_default = 0, // only used for req_comp

  STBI_grey       = 1,
  STBI_grey_alpha = 2,
  STBI_rgb        = 3,
  STBI_rgb_alpha  = 4,
};

typedef unsigned char stbi_uc;

#ifdef __cplusplus
extern "C" {
#endif

// WRITING API

#if !defined(STBI_NO_WRITE) && !defined(STBI_NO_STDIO)
// write a BMP/TGA file given tightly packed 'comp' channels (no padding, nor bmp-stride-padding)
// (you must include the appropriate extension in the filename).
// returns TRUE on success, FALSE if couldn't open file, error writing file
extern int      stbi_write_bmp(char const *filename,     int x, int y, int comp, void *data);
extern int      stbi_write_tga(char const *filename,     int x, int y, int comp, void *data);
#endif

// PRIMARY API - works on images of any type

// load image by filename, open file, or memory buffer
#ifndef STBI_NO_STDIO
extern stbi_uc *stbi_load(char const *filename,     int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi_load_from_file(FILE *f,                  int *x, int *y, int *comp, int req_comp);
extern int      stbi_info_from_file(FILE *f,                  int *x, int *y, int *comp);
#endif
extern stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
// for stbi_load_from_file, file pointer is left pointing immediately after image

#ifndef STBI_NO_HDR
#ifndef STBI_NO_STDIO
extern float *stbi_loadf(char const *filename,     int *x, int *y, int *comp, int req_comp);
extern float *stbi_loadf_from_file(FILE *f,                  int *x, int *y, int *comp, int req_comp);
#endif
extern float *stbi_loadf_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);

extern void   stbi_hdr_to_ldr_gamma(float gamma);
extern void   stbi_hdr_to_ldr_scale(float scale);

extern void   stbi_ldr_to_hdr_gamma(float gamma);
extern void   stbi_ldr_to_hdr_scale(float scale);

#endif // STBI_NO_HDR

// get a VERY brief reason for failure
// NOT THREADSAFE
extern char    *stbi_failure_reason(void);

// free the loaded image -- this is just free()
extern void     stbi_image_free(void *retval_from_stbi_load);

// get image dimensions & components without fully decoding
extern int      stbi_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp);
extern int      stbi_is_hdr_from_memory(stbi_uc const *buffer, int len);
#ifndef STBI_NO_STDIO
extern int      stbi_info(char const *filename,     int *x, int *y, int *comp);
extern int      stbi_is_hdr(char const *filename);
extern int      stbi_is_hdr_from_file(FILE *f);
#endif

// ZLIB client - used by PNG, available for other purposes

extern char *stbi_zlib_decode_malloc_guesssize(const char *buffer, int len, int initial_size, int *outlen);
extern char *stbi_zlib_decode_malloc(const char *buffer, int len, int *outlen);
extern int   stbi_zlib_decode_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);

extern char *stbi_zlib_decode_noheader_malloc(const char *buffer, int len, int *outlen);
extern int   stbi_zlib_decode_noheader_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);

// TYPE-SPECIFIC ACCESS

// is it a jpeg?
extern int      stbi_jpeg_test_memory(stbi_uc const *buffer, int len);
extern stbi_uc *stbi_jpeg_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
extern int      stbi_jpeg_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp);

#ifndef STBI_NO_STDIO
extern stbi_uc *stbi_jpeg_load(char const *filename,     int *x, int *y, int *comp, int req_comp);
extern int      stbi_jpeg_test_file(FILE *f);
extern stbi_uc *stbi_jpeg_load_from_file(FILE *f,                  int *x, int *y, int *comp, int req_comp);

extern int      stbi_jpeg_info(char const *filename,     int *x, int *y, int *comp);
extern int      stbi_jpeg_info_from_file(FILE *f,                  int *x, int *y, int *comp);
#endif

// is it a png?
extern int      stbi_png_test_memory(stbi_uc const *buffer, int len);
extern stbi_uc *stbi_png_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
extern int      stbi_png_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp);

#ifndef STBI_NO_STDIO
extern stbi_uc *stbi_png_load(char const *filename,     int *x, int *y, int *comp, int req_comp);
extern int      stbi_png_info(char const *filename,     int *x, int *y, int *comp);
extern int      stbi_png_test_file(FILE *f);
extern stbi_uc *stbi_png_load_from_file(FILE *f,                  int *x, int *y, int *comp, int req_comp);
extern int      stbi_png_info_from_file(FILE *f,                  int *x, int *y, int *comp);
#endif

// is it a bmp?
extern int      stbi_bmp_test_memory(stbi_uc const *buffer, int len);

extern stbi_uc *stbi_bmp_load(char const *filename,     int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi_bmp_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
#ifndef STBI_NO_STDIO
extern int      stbi_bmp_test_file(FILE *f);
extern stbi_uc *stbi_bmp_load_from_file(FILE *f,                  int *x, int *y, int *comp, int req_comp);
#endif

// is it a tga?
extern int      stbi_tga_test_memory(stbi_uc const *buffer, int len);

extern stbi_uc *stbi_tga_load(char const *filename,     int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi_tga_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
#ifndef STBI_NO_STDIO
extern int      stbi_tga_test_file(FILE *f);
extern stbi_uc *stbi_tga_load_from_file(FILE *f,                  int *x, int *y, int *comp, int req_comp);
#endif

// is it a psd?
extern int      stbi_psd_test_memory(stbi_uc const *buffer, int len);

extern stbi_uc *stbi_psd_load(char const *filename,     int *x, int *y, int *comp, int req_comp);
extern stbi_uc *stbi_psd_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
#ifndef STBI_NO_STDIO
extern int      stbi_psd_test_file(FILE *f);
extern stbi_uc *stbi_psd_load_from_file(FILE *f,                  int *x, int *y, int *comp, int req_comp);
#endif

// is it an hdr?
extern int      stbi_hdr_test_memory(stbi_uc const *buffer, int len);

extern float   *stbi_hdr_load(char const *filename,     int *x, int *y, int *comp, int req_comp);
extern float   *stbi_hdr_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
#ifndef STBI_NO_STDIO
extern int      stbi_hdr_test_file(FILE *f);
extern float   *stbi_hdr_load_from_file(FILE *f,                  int *x, int *y, int *comp, int req_comp);
#endif

// define new loaders
typedef struct {
  int (*test_memory)(stbi_uc const *buffer, int len);
  stbi_uc *(*load_from_memory)(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
#ifndef STBI_NO_STDIO
  int (*test_file)(FILE *f);
  stbi_uc *(*load_from_file)(FILE *f, int *x, int *y, int *comp, int req_comp);
#endif
} stbi_loader;

// register a loader by filling out the above structure (you must defined ALL functions)
// returns 1 if added or already added, 0 if not added (too many loaders)
// NOT THREADSAFE
extern int stbi_register_loader(stbi_loader *loader);

// define faster low-level operations (typically SIMD support)
#if STBI_SIMD
typedef void (*stbi_idct_8x8)(uint8 *out, int out_stride, short data[64], unsigned short *dequantize);
// compute an integer IDCT on "input"
//     input[x] = data[x] * dequantize[x]
//     write results to 'out': 64 samples, each run of 8 spaced by 'out_stride'
//                             CLAMP results to 0..255
typedef void (*stbi_YCbCr_to_RGB_run)(uint8 *output, uint8 const *y, uint8 const *cb, uint8 const *cr, int count, int step);
// compute a conversion from YCbCr to RGB
//     'count' pixels
//     write pixels to 'output'; each pixel is 'step' bytes (either 3 or 4; if 4, write '255' as 4th), order R,G,B
//     y: Y input channel
//     cb: Cb input channel; scale/biased to be 0..255
//     cr: Cr input channel; scale/biased to be 0..255

extern void stbi_install_idct(stbi_idct_8x8 func);
extern void stbi_install_YCbCr_to_RGB(stbi_YCbCr_to_RGB_run func);
#endif // STBI_SIMD

#ifdef __cplusplus
}
#endif

//
//
////   end header file   /////////////////////////////////////////////////////
#endif // STBI_INCLUDE_STB_IMAGE_H
