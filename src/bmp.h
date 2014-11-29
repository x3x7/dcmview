/*
  Copyright (c) 2014, x3x7apps(@gmail.com)
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer. 
  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef _BMP_H_
#define _BMP_H_
#include <stdint.h>
#ifdef __cplusplus
#define C_DEC_BEGIN extern "C" {
#define C_DEC_END }
#else
#define C_DEC_BEGIN
#define C_DEC_END
#endif

C_DEC_BEGIN


typedef enum {
    BMP_RGB = 0,
    BMP_RLE8,
    BMP_RLE4,
    BMP_BITFIELDS,
    BMP_JPEG,
    BMP_PNG,
} bmp_compress_t;

typedef struct {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
} bmp_rgba_t;

typedef struct __attribute__((packed)) {
    uint8_t magic[2];     /* e.g. 0x42 0x4D is hex code of 'B' and 'M'
                             BM - Windows bitmap
                             BA - OS/2 Bitmap Array
                             ... more can be used.
                          */
    uint32_t fsize;           /* file size in bytes */
    uint16_t rsvd[2];         /* reserved */
    uint32_t offset;          /* offset where bitmap data is placed */

} bmp_header_t;

typedef struct __attribute__((packed)) {
    uint32_t hsize;          /* this header's size (40 bytes) */
    uint32_t width;          /* width in pixels */
    uint32_t height;         /* height in pixels */
    uint16_t nplanes;        /* the number of color planes being used (should be 1) */
    uint16_t depth;          /* bits per pixel (color depth) */
    uint32_t compress;       /* one of bmp_compress_t */
    uint32_t dsize;          /* the image data size */
    uint32_t wres;           /* x px/m */
    uint32_t hres;           /* y px/m */
    uint32_t tcolors;        /* # colors in the color table */
    uint32_t icolors;        /* # of important colors used (0 ->all important) */
} bmp_dib3_t;


C_DEC_END

#endif
