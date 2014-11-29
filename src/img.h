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
#ifndef _IMG_H_
#define _IMG_H_
#include <stdint.h>
#include <QColor>
#include "tr.h"

typedef uint16_t px_t;          // pixel data


class img {

public:
    img( int32_t x, int32_t y, px_t *px=NULL, int32_t pad=16 );
    img() : _x(0),_y(0),_px(NULL),_haspx(false){ }
    ~img(){ if( _haspx && _px!=NULL ) delete[] _px; }
    int32_t x() const { return _x; }
    int32_t y() const { return _y; }
    int32_t pad() const { return _pad; }
    int32_t px( int32_t x, int32_t y ) const { 
        if( _px!=NULL && x<_x && y<_y ) 
            return _px[y*(_x+_pad)+x];
        return -1;
    }
    void create( int32_t x, int32_t y, px_t *px=NULL, int32_t pad=16 );
    px_t* px() const { return _px; };
    void get( const int16_t *px, int min=0 );
    void get( const uint16_t *px, int min=0 );

    static inline int px255( int px, int range ){
        if( px<0 ) return 0;
        if( px>=range ) return 255;
        return (px<<8)/range;
    }
    static inline QRgb px2rgb( int px, int min, int max, int Max ){
        if( px<min )
            return _rgb_min[(px<<6)/min];

        if( px>max )
            return _rgb_max[((px-max)<<6)/(Max-max+1)];

        int v=((px-min)<<8)/(max-min+1);
        return qRgb( v, v, v );
    }

    static const int _rgb_sz=1<<6;
    static QRgb _rgb_min[_rgb_sz];
    static QRgb _rgb_max[_rgb_sz];

    static void rgb_init( );

    void rgb( QRgb *dest, int min, int max, int Max ) const {
        int i,j;
        for( j=0; j<_y; ++j )
            for( i=0; i<_x; ++i )
                *dest++=img::px2rgb( _px[i+j*(_x+_pad)], min, max, Max );
    }
    void rgb( QRgb *dest, const QRgb *map ) const {
        int i,j;
        for( j=0; j<_y; ++j )
            for( i=0; i<_x; ++i )
                *dest++=map[ _px[i+j*(_x+_pad)] ];
    }

    // convert to rgb while zooming 2 times
    void rgbx2( QRgb *dest, int min, int max, int Max ) const {
        int i,j;
        QRgb *rgb0=dest, *rgb1=rgb0+(_x<<1);
        for( j=0; j<_y; ++j ){
            for( i=0; i<_x; ++i ){
                QRgb rgb=img::px2rgb( _px[i+j*(_x+_pad)], min, max, Max );
                rgb0[(i<<1)+1]=rgb0[i<<1]=rgb;
                rgb1[(i<<1)+1]=rgb1[i<<1]=rgb;
            }
            rgb0=rgb1+(_x<<1);
            rgb1=rgb0+(_x<<1);                
        }
    }
    void rgbx2( QRgb *dest, const QRgb *map ) const {
        int i,j;
        QRgb *rgb0=dest, *rgb1=rgb0+(_x<<1);
        for( j=0; j<_y; ++j ){
            for( i=0; i<_x; ++i ){
                QRgb rgb=map[ _px[i+j*(_x+_pad)] ];
                rgb0[(i<<1)+1]=rgb0[i<<1]=rgb;
                rgb1[(i<<1)+1]=rgb1[i<<1]=rgb;
            }
            rgb0=rgb1+(_x<<1);
            rgb1=rgb0+(_x<<1);                
        }
    }

    void clear( );
    // fill current with scaled up data from 'src'
    void zoom( const img &src, int& x0, int& y0, int zf );
    void test1( px_t val );
    void test01( px_t val );
    void test10( px_t val );

private:
    int32_t _x;
    int32_t _y;
    int32_t _pad;
    px_t *_px;
    bool _haspx;                // owns pixel data
    void empty(){
        if( _haspx && _px!=NULL ) delete[] _px;
        _px=NULL;
        _haspx=false;
        _x=_y=_pad=0;
    }
};


#endif
