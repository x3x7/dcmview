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
#include <cstring>
#include "img.h"
#include "tr.h"

QRgb img::_rgb_min[_rgb_sz];
QRgb img::_rgb_max[_rgb_sz];

void
img::rgb_init( ){
    const int r0=24,  g0=96,  b0=0;   // limit values at 0
    const int r1=144, g1=56,  b1=56;  // limit values at max
    int i, r,g,b;
    for( i=0; i<_rgb_sz; ++i ){
        r=(r0*(_rgb_sz-i-1))/(_rgb_sz-1);
        g=(g0*(_rgb_sz-i-1))/(_rgb_sz-1);
        b=(b0*(_rgb_sz-i-1))/(_rgb_sz-1);
        _rgb_min[i]=qRgb(r,g,b);
    }
    for( i=0; i<_rgb_sz; ++i ){
        r=(255*(_rgb_sz-i-2)+r1*(i+1))/(_rgb_sz-1);
        g=(255*(_rgb_sz-i-2)+g1*(i+1))/(_rgb_sz-1);
        b=(255*(_rgb_sz-i-2)+b1*(i+1))/(_rgb_sz-1);
        _rgb_max[i]=qRgb(r,g,b);
    }
}

void
img::create( int32_t x, int32_t y, px_t *px, int32_t pad ){
    if( x<0 || y<0 || pad < 0 )
        E( "unexpected params" );

    empty();
    _x=x;
    _y=y;
    _pad=pad;
    if( px!=NULL ){
        _px=px;
        _haspx=false;
    } else {
        _px=new px_t[(_x+_pad)*(_y+_pad)];
        _haspx=true;
    }    
}

img::img( int32_t x, int32_t y, px_t *px, int32_t pad ){
    if( x<0 || y<0 || pad < 0 )
        E( "unexpected params" );
    _x=x;
    _y=y;
    _pad=pad;
    if( px!=NULL ){
        _px=px;
        _haspx=false;
    } else {
        _px=new px_t[(_x+_pad)*(_y+_pad)];
        _haspx=true;
    }
}

void img::get( const int16_t *px, int min ){
    if( _px!=NULL ){
        int j;
        px_t *d=_px;
        for( j=0; j<_y; ++j ){
            for( int i=0; i<_x; ++i )
                d[i]=px[i]-min;
            d+=_x;
            memset( d, 0, _pad*sizeof(_px[0]) );
            d+=_pad;
            px+=_x;
        }
        memset( d, 0, _pad*(_x+_pad)*sizeof(_px[0]) );
    } else {
        E(" empty dest" );
    }
}

void img::get( const uint16_t *px, int min ){
    if( _px!=NULL ){
        int j;
        px_t *d=_px;
        for( j=0; j<_y; ++j ){
            for( int i=0; i<_x; ++i )
                d[i]=px[i]-min;
            d+=_x;
            memset( d, 0, _pad*sizeof(_px[0]) );
            d+=_pad;
            px+=_x;
        }
        memset( d, 0, _pad*(_x+_pad)*sizeof(_px[0]) );
    } else {
        E(" empty dest" );
    }
}
void 
img::clear( ){
    if( _px )
        memset( _px, 0, (_x+_pad)*(_y+_pad)*sizeof(_px[0]) );
}

void 
img::test1( px_t val ){
    if( _px ){
        clear();
        // set just one pixel
        int i=_x/3,j=_y/3;
        T( "test px %d %d", i, j );
        _px[ j*(_x+_pad)+i ]=val;
    }
}

void 
img::test01( px_t val ){
    if( _px ){
        clear();
        int i,j;
        for( j=0;j<_y; ++j )
            for( i=0;i<_x; ++i )
                if( (i+j)%2 ) 
                    _px[ j*(_x+_pad)+i ]=val;
    }
}
void 
img::test10( px_t val ){
    if( _px ){
        clear();
        int i,j;
        for( j=0;j<_y; ++j )
            for( i=0;i<_x; ++i )
                if( (i+j)%2==0 ) 
                    _px[ j*(_x+_pad)+i ]=val;
    }
}

void 
img::zoom( const img &src, int& x0, int& y0, int zf ){

    // ensure whole target area covered
    if( zf*(src._x-x0)<2*_x )
        x0=(zf*src._x-2*_x)/zf;
    if( zf*(src._y-y0)<2*_y )
        y0=(zf*src._y-2*_y)/zf;
    if( x0 < 0 ) x0=0;
    if( y0 < 0 ) y0=0;

    int x1, y1;                 // lower-right corner
    // ensure we don't overflow the destination
    x1=x0+(2*_x)/zf+1;
    y1=y0+(2*_y)/zf+1;
    // ensure we don't overread the source (the padding guards the loops below)
    if( x1>src._x ) x1=src._x;
    if( y1>src._y ) y1=src._y;

    //T( "zf %d %d,%d -> %d,%d px %d,%d ", zf, x0, y0, x1, y1, (x1-x0)*zf/2, (y1-y0)*zf/2 );

    int dlsz=_x+_pad;
    int slsz=src._x+src._pad;
    int i,j;
    px_t *d0, *d1, *d2, *d3, *d4, *d5, *d6, *d7, *d8, *d9;

    if( zf&1 ){            // odd scale
        const px_t *pl0=src._px+y0*slsz+x0;
        const px_t *pl1=pl0+slsz;
        switch( zf ){
            case 3:         // 2px -> 3px 
                for( j=0; j<y1-y0; j+=2 ){
                    d0=_px+((3*j*dlsz)>>1);
                    d1=d0+dlsz;
                    d2=d1+dlsz;
                    for( i=0; i<x1-x0; i+=2 ){
                        d0[0]=pl0[i];
                        d0[1]=(pl0[i]+pl0[i+1])>>1;
                        d0[2]=pl0[i+1];

                        d2[0]=pl1[i];
                        d2[1]=(pl1[i]+pl1[i+1])>>1;
                        d2[2]=pl1[i+1];

                        d1[0]=(d0[0]+d2[0])>>1;
                        d1[1]=(d0[1]+d2[1])>>1;
                        d1[2]=(d0[2]+d2[2])>>1;

                        d0+=3;
                        d1+=3;
                        d2+=3;
                    }
                    pl0=pl1+slsz;
                    pl1=pl0+slsz;
                }
                d0=d2;                    
                break;
            case 5:         // 2px -> 5px
                for( j=0; j<y1-y0; j+=2 ){
                    d0=_px+((5*j*dlsz)>>1);
                    d1=d0+dlsz;
                    d2=d1+dlsz;
                    d3=d2+dlsz;
                    d4=d3+dlsz;
                    for( i=0; i<x1-x0; i+=2 ){
                        px_t p00=pl0[i], p01=pl0[i+1];
                        px_t p10=pl1[i], p11=pl1[i+1];

                        d0[0]=d0[1]=p00;
                        d1[0]=d1[1]=p00;
                        d0[2]=d1[2]=(p00+p01)>>1;
                        d0[3]=d0[4]=p01;
                        d1[3]=d1[4]=p01;
                        
                        d3[0]=d3[1]=p10;
                        d4[0]=d4[1]=p10;
                        d3[2]=d4[2]=(p10+p11)>>1;
                        d3[3]=d3[4]=p11;
                        d4[3]=d4[4]=p11;

                        d2[0]=(d1[0]+d3[0])>>1;
                        d2[1]=(d1[1]+d3[1])>>1;
                        d2[2]=(d1[2]+d3[2])>>1;
                        d2[3]=(d1[3]+d3[3])>>1;
                        d2[4]=(d1[4]+d3[4])>>1;

                        d0+=5;
                        d1+=5;
                        d2+=5;
                        d3+=5;
                        d4+=5;
                    }
                    pl0=pl1+slsz;
                    pl1=pl0+slsz;
                }
                d0=d4;
                break;
            case 7:         // 2px -> 7px
                for( j=0; j<y1-y0; j+=2 ){
                    d0=_px+((7*j*dlsz)>>1);
                    d1=d0+dlsz;
                    d2=d1+dlsz;
                    d3=d2+dlsz;
                    d4=d3+dlsz;
                    d5=d4+dlsz;
                    d6=d5+dlsz;
                    for( i=0; i<x1-x0; i+=2 ){
                        px_t p00=pl0[i], p01=pl0[i+1];
                        px_t p10=pl1[i], p11=pl1[i+1];

                        d0[0]=d0[1]=d0[2]=p00;
                        d1[0]=d1[1]=d1[2]=p00;
                        d2[0]=d2[1]=d2[2]=p00;
                        d0[3]=d1[3]=d2[3]=(p00+p01)>>1;

                        d0[4]=d0[5]=d0[6]=p01;
                        d1[4]=d1[5]=d1[6]=p01;
                        d2[4]=d2[5]=d2[6]=p01;
                        
                        d4[0]=d4[1]=d4[2]=p10;
                        d5[0]=d5[1]=d5[2]=p10;
                        d6[0]=d6[1]=d6[2]=p10;
                        d4[3]=d5[3]=d6[3]=(p10+p11)>>1;
                        d4[4]=d4[5]=d4[6]=p11;
                        d5[4]=d5[5]=d5[6]=p11;
                        d6[4]=d6[5]=d6[6]=p11;

                        d3[0]=(d2[0]+d4[0])>>1;
                        d3[1]=(d2[1]+d4[1])>>1;
                        d3[2]=(d2[2]+d4[2])>>1;
                        d3[3]=(d2[3]+d4[3])>>1;
                        d3[4]=(d2[4]+d4[4])>>1;
                        d3[5]=(d2[5]+d4[5])>>1;
                        d3[6]=(d2[6]+d4[6])>>1;

                        d0+=7;
                        d1+=7;
                        d2+=7;
                        d3+=7;
                        d4+=7;
                        d5+=7;
                        d6+=7;
                    }
                    pl0=pl1+slsz;
                    pl1=pl0+slsz;
                }
                d0=d6;
                break;
            case 9:         // 2px -> 9px
                for( j=0; j<y1-y0; j+=2 ){
                    d0=_px+((9*j*dlsz)>>1);
                    d1=d0+dlsz;
                    d2=d1+dlsz;
                    d3=d2+dlsz;
                    d4=d3+dlsz;
                    d5=d4+dlsz;
                    d6=d5+dlsz;
                    d7=d6+dlsz;
                    d8=d7+dlsz;
                    for( i=0; i<x1-x0; i+=2 ){
                        px_t p00=pl0[i], p01=pl0[i+1];
                        px_t p10=pl1[i], p11=pl1[i+1];

                        d0[0]=d0[1]=d0[2]=d0[3]=p00;
                        d1[0]=d1[1]=d1[2]=d1[3]=p00;
                        d2[0]=d2[1]=d2[2]=d2[3]=p00;
                        d3[0]=d3[1]=d3[2]=d3[3]=p00;

                        d0[4]=d1[4]=d2[4]=d3[4]=(p00+p01)>>1;

                        d0[5]=d0[6]=d0[7]=d0[8]=p01;
                        d1[5]=d1[6]=d1[7]=d1[8]=p01;
                        d2[5]=d2[6]=d2[7]=d2[8]=p01;
                        d3[5]=d3[6]=d3[7]=d3[8]=p01;
                        

                        d5[0]=d5[1]=d5[2]=d5[3]=p10;
                        d6[0]=d6[1]=d6[2]=d6[3]=p10;
                        d7[0]=d7[1]=d7[2]=d7[3]=p10;
                        d8[0]=d8[1]=d8[2]=d8[3]=p10;

                        d5[4]=d6[4]=d7[4]=d8[4]=(p10+p11)>>1;

                        d5[5]=d5[6]=d5[7]=d5[8]=p11;
                        d6[5]=d6[6]=d6[7]=d6[8]=p11;
                        d7[5]=d7[6]=d7[7]=d7[8]=p11;
                        d8[5]=d8[6]=d8[7]=d8[8]=p11;

                        d4[0]=(d3[0]+d5[0])>>1;
                        d4[1]=(d3[1]+d5[1])>>1;
                        d4[2]=(d3[2]+d5[2])>>1;
                        d4[3]=(d3[3]+d5[3])>>1;
                        d4[4]=(d3[4]+d5[4])>>1;
                        d4[5]=(d3[5]+d5[5])>>1;
                        d4[6]=(d3[6]+d5[6])>>1;
                        d4[7]=(d3[7]+d5[7])>>1;
                        d4[8]=(d3[8]+d5[8])>>1;

                        d0+=9;
                        d1+=9;
                        d2+=9;
                        d3+=9;
                        d4+=9;
                        d5+=9;
                        d6+=9;
                        d7+=9;
                        d8+=9;
                    }
                    pl0=pl1+slsz;
                    pl1=pl0+slsz;
                }
                d0=d8;
                break;
            default:
                T( "unexpected zoom factor %d", zf );
                break;
        }

    } else {                // even scale

#define CPYTO( c, n ) for( int k=0; k<n; ++k ) d##c[k]=pi; d##c+=n
 
        const px_t *pl=src._px+y0*slsz+x0;
        switch( zf ){
            case 4:         // 1px -> 2px
                for( j=0; j<y1-y0; ++j ){
                    d0=_px+2*j*dlsz;
                    d1=d0+dlsz;
                    for( i=0; i<x1-x0; ++i ){
                        px_t pi=pl[i];
                        CPYTO( 0, 2 );
                        CPYTO( 1, 2 );
                    }
                    pl+=slsz;
                }
                d0=d1;
                break;
            case 6:         // 1px -> 3px
                for( j=0; j<y1-y0; ++j ){
                    d0=_px+3*j*dlsz;
                    d1=d0+dlsz;
                    d2=d1+dlsz;

                    for( i=0; i<x1-x0; ++i ){
                        px_t pi=pl[i];
                        CPYTO( 0, 3 );
                        CPYTO( 1, 3 );
                        CPYTO( 2, 3 ); 
                    }
                    pl+=slsz;
                }
                d0=d2;
                break;
            case 8:         // 1px -> 4px
                for( j=0; j<y1-y0; ++j ){
                    d0=_px+4*j*dlsz;
                    d1=d0+dlsz;
                    d2=d1+dlsz;
                    d3=d2+dlsz;
                    for( i=0; i<x1-x0; ++i ){
                        px_t pi=pl[i];
                        CPYTO( 0, 4 );
                        CPYTO( 1, 4 );
                        CPYTO( 2, 4 ); 
                        CPYTO( 3, 4 ); 
                    }
                    pl+=slsz;
                }
                d0=d3;
                break;
            case 10:
                for( j=0; j<y1-y0; ++j ){
                    d0=_px+5*j*dlsz;
                    d1=d0+dlsz;
                    d2=d1+dlsz;
                    d3=d2+dlsz;
                    d4=d3+dlsz;
                    for( i=0; i<x1-x0; ++i ){
                        px_t pi=pl[i];
                        CPYTO( 0, 5 );
                        CPYTO( 1, 5 );
                        CPYTO( 2, 5 ); 
                        CPYTO( 3, 5 ); 
                        CPYTO( 4, 5 ); 
                    }
                    pl+=slsz;
                }
                d0=d4;
                break;
            case 12:
                for( j=0; j<y1-y0; ++j ){
                    d0=_px+6*j*dlsz;
                    d1=d0+dlsz;
                    d2=d1+dlsz;
                    d3=d2+dlsz;
                    d4=d3+dlsz;
                    d5=d4+dlsz;
                    for( i=0; i<x1-x0; ++i ){
                        px_t pi=pl[i];
                        CPYTO( 0, 6 );
                        CPYTO( 1, 6 );
                        CPYTO( 2, 6 ); 
                        CPYTO( 3, 6 ); 
                        CPYTO( 4, 6 ); 
                        CPYTO( 5, 6 ); 
                    }
                    pl+=slsz;
                }
                d0=d5;
                break;
            case 14:
                for( j=0; j<y1-y0; ++j ){
                    d0=_px+7*j*dlsz;
                    d1=d0+dlsz;
                    d2=d1+dlsz;
                    d3=d2+dlsz;
                    d4=d3+dlsz;
                    d5=d4+dlsz;
                    d6=d5+dlsz;
                    for( i=0; i<x1-x0; ++i ){
                        px_t pi=pl[i];
                        CPYTO( 0, 7 );
                        CPYTO( 1, 7 );
                        CPYTO( 2, 7 ); 
                        CPYTO( 3, 7 ); 
                        CPYTO( 4, 7 ); 
                        CPYTO( 5, 7 ); 
                        CPYTO( 6, 7 ); 
                    }
                    pl+=slsz;
                }
                d0=d6;
                break;
            case 16:
                for( j=0; j<y1-y0; ++j ){
                    d0=_px+8*j*dlsz;
                    d1=d0+dlsz;
                    d2=d1+dlsz;
                    d3=d2+dlsz;
                    d4=d3+dlsz;
                    d5=d4+dlsz;
                    d6=d5+dlsz;
                    d7=d6+dlsz;
                    for( i=0; i<x1-x0; ++i ){
                        px_t pi=pl[i];
                        CPYTO( 0, 8 );
                        CPYTO( 1, 8 );
                        CPYTO( 2, 8 ); 
                        CPYTO( 3, 8 ); 
                        CPYTO( 4, 8 ); 
                        CPYTO( 5, 8 ); 
                        CPYTO( 6, 8 ); 
                        CPYTO( 7, 8 ); 
                    }
                    pl+=slsz;
                }
                d0=d7;
                break;
            case 18:
                for( j=0; j<y1-y0; ++j ){
                    d0=_px+9*j*dlsz;
                    d1=d0+dlsz;
                    d2=d1+dlsz;
                    d3=d2+dlsz;
                    d4=d3+dlsz;
                    d5=d4+dlsz;
                    d6=d5+dlsz;
                    d7=d6+dlsz;
                    d8=d7+dlsz;
                    for( i=0; i<x1-x0; ++i ){
                        px_t pi=pl[i];
                        CPYTO( 0, 9 );
                        CPYTO( 1, 9 );
                        CPYTO( 2, 9 ); 
                        CPYTO( 3, 9 ); 
                        CPYTO( 4, 9 ); 
                        CPYTO( 5, 9 ); 
                        CPYTO( 6, 9 ); 
                        CPYTO( 7, 9 ); 
                        CPYTO( 8, 9 ); 
                    }
                    pl+=slsz;
                }
                d0=d8;
                break;
            case 20:
                for( j=0; j<y1-y0; ++j ){
                    d0=_px+10*j*dlsz;
                    d1=d0+dlsz;
                    d2=d1+dlsz;
                    d3=d2+dlsz;
                    d4=d3+dlsz;
                    d5=d4+dlsz;
                    d6=d5+dlsz;
                    d7=d6+dlsz;
                    d8=d7+dlsz;
                    d9=d8+dlsz;
                    for( i=0; i<x1-x0; ++i ){
                        px_t pi=pl[i];
                        CPYTO( 0, 10 );
                        CPYTO( 1, 10 );
                        CPYTO( 2, 10 ); 
                        CPYTO( 3, 10 ); 
                        CPYTO( 4, 10 ); 
                        CPYTO( 5, 10 ); 
                        CPYTO( 6, 10 ); 
                        CPYTO( 7, 10 ); 
                        CPYTO( 8, 10 ); 
                        CPYTO( 9, 10 );
                    }
                    pl+=slsz;
                }
                d0=d9;
                break;

            default:
                T( "unexpected zoom factor %d", zf );
                break;
        }
    }
    //T( "lines=%d rem %d (dlsz %d)", (d0-_px)/dlsz, (d0-_px)%dlsz, dlsz );
}
