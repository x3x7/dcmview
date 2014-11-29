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
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include "dicom.h"
#include "tr.h"

dicom::del_t dicom::tab[]={
#include "dicom.tab"
};
int dicom::tabsz=sizeof(dicom::tab)/sizeof(dicom::tab[0]);

void
roi_seq_t::show() const {
    //printf( "%.*s %d\n", name.len, name.str, nelm );
    unsigned i;
    for( i=0; i<ne; ++i ){
        printf( "\t%3d [%2d] \"%.*s\"/%d\n", 
                i, e[i].ncp, e[i].iuid.len, e[i].iuid.str, e[i].iuid.len );
    }
}

void
dcmf::clear(){
    if( _dat != NULL ){
        munmap( (void*)_dat, _len );
        _dat=NULL;
        _len=0;
        if( _e != NULL ){
            free( _e );
            _e=NULL;
            _esz=0;
        }
    }
}

int
dcmf::load( const char *fname ){
    clear();
    int fd=open( fname, O_RDONLY );
    if( fd < 0 ){
        E( "open %s", fname );
        return -1;
    }
    _len=lseek( fd, 0, SEEK_END );	
    _dat=(const u8_t*)mmap( NULL, _len, PROT_READ, MAP_PRIVATE|MAP_NORESERVE|MAP_POPULATE, fd, 0 );
    close( fd );
    if( _dat==(void *) -1 ){
        T( "mmap '%s': %s", fname, strerror(errno) );
        _dat=NULL;
        return -1;
    }
    if( parse()== -1 ){
        T( "'%s' not a DICOM file!", fname );
        clear();
        return -1;
    }
    get_info();
    return 0;
}

int
dcmf::seq( int lvl, const void *dat, uint32_t size ){
    const u8_t *d=(const u8_t*)dat, *end=d+size;
    bool hasvr=false;
    while( d<end ){
        uint16_t gn, en, vr;
        gn=getu16( d );
        d+=2;
        en=getu16( d );
        d+=2;
        uint32_t len=0;
        if( hasvr ){
            vr=getvr( d );
            d+=2;
            switch( vr ){
                case vr_OB:
                case vr_OF:
                case vr_OW:
                case vr_SQ:
                case vr_UN:
                case vr_UT:
                    d+=2;
                    len=getu32(d);
                    d+=4;
                    break;
                case vr_AE:
                case vr_AS:
                case vr_AT:
                case vr_CS:
                case vr_DA:
                case vr_DS:
                case vr_DT:
                case vr_FD:
                case vr_FL:
                case vr_IS:
                case vr_LO:
                case vr_LT:
                case vr_PN:
                case vr_SH:
                case vr_SL:
                case vr_SS:
                case vr_ST:
                case vr_TM:
                case vr_UI:
                case vr_UL:
                case vr_US:
                    len=getu16(d);                
                    d+=2;
                    break;
                default:
                    if( gn==0x8 && en==0x5 ){ // data bug?!
                        d-=2;
                        len=getu32(d);
                        d+=4;
                        hasvr=false; // value reps are no longer included
                    } else {
                        T( "unknown vr: >%02x %02x<", (vr>>8), vr&0xff );
                        return -1;
                    }
                    break;
            }
        } else {
            len=getu32(d);
            d+=4;
        }
        if( len+1==0 )
            E("undefined length not yet handled!" );

        int i=dicom::del( gn, en );
        if( i==-1 )
            E( "unknown element (%4x,%4x)", gn, en );
        if( !hasvr )
            vr=dicom::tab[i].vr;

#if(1)
        uint32_t pos=d-(const u8_t*)dat;
        for( int l=0; l<lvl; ++l ) putchar( '\t' );
        printf( " %4d 0x%06x %4x %4x '%c%c'%c [%6d] %s ", 
                i, pos, gn, en, (vr>>8), vr&0xff, hasvr?' ':'?', len, dicom::tab[i].nm );
        switch( vr){
            case vr_AS:
            case vr_CS:
            case vr_DA:
            case vr_DS:
            case vr_DT:
            case vr_IS:
            case vr_LO:
            case vr_LT:
            case vr_PN:
            case vr_SH:
            case vr_TM:
            case vr_UI:
                printf( "\"%.*s\"\n", len , (const char*)d );
                break;
            case vr_US:
                printf( " %d\n", getu16( d ) );
                break;                
            case vr_SS:
                printf( " %d\n", gets16( d ) );
                break;
            case vr_SQ:
                printf( "\n" );
                seq( lvl+1, d, len );
                break;
            default:
                printf( "\n" );
                seq( lvl+1, d, len );
                printf( "[%.*s]\n", len, (const char*)d );
                break;
        }
#endif
        d+=len;
    }
    return 0;
}

const char*
dcmf::get_dbl( const char* str, const char *eptr, double d[], int &max ){
    int n=0;
    while( str<eptr && n<max ){
        char *end;
        d[n]=strtod( str, &end );
        if( str==end )
            break;
        ++n;
        if( end[0]=='\\' ) ++end;
        str=end;
    }
    max=n;
    while( n<max ) d[n++]=0;
    return str;
}

const char*
dcmf::get_flt( const char* str, const char *eptr, float d[], int &max ){
    int n=0;
    while( str<eptr && n<max ){
        char *end;
        d[n]=strtof( str, &end );
        if( str==end )
            break;
        ++n;
        if( end[0]=='\\' ) ++end;
        str=end;
    }
    max=n;
    while( n<max ) d[n++]=0;
    return str;
}

const char*
dcmf::get_int( const char* str, const char *eptr, int d[], int &max ){
    int n=0;
    while( str<eptr && n<max ){
        char *end;
        d[n]=strtol( str, &end, 10 );
        if( str==end )
            break;
        ++n;
        if( end[0]=='\\' ) ++end;
        str=end;
    }
    max=n;
    while( n<max ) d[n++]=0;
    return str;
}

void
dcmf::pxstats(){
    _pxmin=_pxmax=0;
    if( _pxr==0 ){    // unsigned data
        const u16_t *ip=d16u();
        if( ip!=NULL ){
            i32_t min=1<<16, max=0;
            int i;
            for( i=0; i<_nx*_ny; ++i ){
                u16_t pv=ip[i];
                if( pv<min )
                    min=pv;
                if( pv>max )
                    max=pv;
            }
            _pxmin=min;
            _pxmax=max;
        }
    } else {                // signed data
        const i16_t *ip=d16s();
        if( ip!=NULL ){
            i32_t min=1<<16, max=0;
            int i;
            for( i=0; i<_nx*_ny; ++i ){
                i16_t pv=ip[i];
                if( pv<min )
                    min=pv;
                if( pv>max )
                    max=pv;
            }
            _pxmin=min;
            _pxmax=max;
        }
    }
    //T("%d,%d", _pxmin, _pxmax );
}

int
dcmf::parse(){
    const u8_t *d=(const u8_t*)_dat, *end=d+_len;
    if( _len < 132 )
        return -1;
    if( _e!=NULL ) free( _e );
    _e=NULL;
    _esz=0;
    
    d+=128;
    if( d[0]!='D' || d[1]!='I' || d[2]!='C' || d[3]!='M' )
        return -1;
    d+=4;
    bool hasvr=true;
    while( d<end ){
        uint16_t gn, en, vr;
        gn=getu16( d );
        d+=2;
        en=getu16( d );
        d+=2;
        uint32_t len=0;
        if( hasvr ){
            vr=getvr( d );
            d+=2;
            switch( vr ){
                case vr_OB:
                case vr_OF:
                case vr_OW:
                case vr_SQ:
                case vr_UN:
                case vr_UT:
                    d+=2;
                    len=getu32(d);
                    d+=4;
                    break;
                case vr_AE:
                case vr_AS:
                case vr_AT:
                case vr_CS:
                case vr_DA:
                case vr_DS:
                case vr_DT:
                case vr_FD:
                case vr_FL:
                case vr_IS:
                case vr_LO:
                case vr_LT:
                case vr_PN:
                case vr_SH:
                case vr_SL:
                case vr_SS:
                case vr_ST:
                case vr_TM:
                case vr_UI:
                case vr_UL:
                case vr_US:
                    len=getu16(d);                
                    d+=2;
                    break;
                default:
                    if( 1 ){// gn==0x8 && en==0x5 ){ // data bug?!
                        d-=2;
                        len=getu32(d);
                        d+=4;
                        hasvr=false; // value reps are no longer included!?
                    } else {
                        int i=dicom::del( gn, en );
                        const char *nm;
                        if( i==-1 )
                            nm="?unknown?";
                        else
                            nm=dicom::tab[i].nm;
                        T( "%s (%4x,%4x): unknown vr >%02x %02x<", 
                           nm, gn, en, int(vr>>8), int(vr&0xff) );
                        //return -1;
                    }
                    break;
            }
        } else {
            len=getu32(d);
            d+=4;
        }
        if( len+1==0 )
            E("undefined length not yet handled!" );

        int i=dicom::del( gn, en );
        if( i==-1 )
            E( "unknown element (%4x,%4x)", gn, en );
        if( !hasvr )
            vr=dicom::tab[i].vr;

        if( (_esz%128)==0 )     // increment size by 128 elements
            _e=(fel_t*)realloc( _e, (_esz+128)*sizeof(_e[0]) );
        
        _e[_esz].ptr=d;
        _e[_esz].len=len;
        _e[_esz].ti=i;
        ++_esz;
        if( _debug ){
            uint32_t pos=d-(const u8_t*)_dat;
            printf( "%2d %4d 0x%06x %4x %4x '%c%c'%c [%6d] %s ", 
                    _esz-1, i, pos, gn, en, (vr>>8), vr&0xff, hasvr?' ':'?', len, dicom::tab[i].nm );
            
            switch( vr ){
                case vr_AS:
                case vr_CS:
                case vr_DA:
                case vr_DS:
                case vr_DT:
                case vr_IS:
                case vr_LO:
                case vr_LT:
                case vr_PN:
                case vr_SH:
                case vr_TM:
                case vr_UI:
                    printf( "\"%.*s\"\n", len , (const char*)d );
                    break;
                case vr_US:
                    printf( " %d\n", getu16( d ) );
                    break;                
                case vr_SS:
                    printf( " %d\n", gets16( d ) );
                    break;
                case vr_SQ:
                    printf( "\n" );
                    //seq( 1, d, len );
                    break;
                default:
                    printf( "\n" );
                    break;
            }
        }
        d+=len;
    }
    return 0;
}

void dcmf::get_info() {
    int i;
    char buf[128];
    _sn=_in=-1;
    _nx=_ny=0;
    _d=NULL;
    _nb=_nbe=0;
    _pv=0;
    _pxr=0;
    i=fel( 0x8, 0x18 );       // SOPInstanceUID
    if( i!=-1 ){
        _iid.str=(const char*)_e[i].ptr;
        _iid.len=_e[i].len;
        _iid.val=crc(_iid.str, _iid.len);
    } else {
        _iid.str=NULL;
        _iid.len=0;
        _iid.val=0;
    }

    i=fel( 0x20, 0xd );       // StudyInstanceUID
    if( i!=-1 ){
        _stid.str=(const char*)_e[i].ptr;
        _stid.len=_e[i].len;
        _stid.val=crc(_stid.str, _stid.len);
    } else {
        _stid.str=NULL;
        _stid.len=0;
        _stid.val=0;
    }

    i=fel( 0x20, 0xe );       // SeriesInstanceUID
    if( i!=-1 ){
        _seid.str=(const char*)_e[i].ptr;
        _seid.len=_e[i].len;
        _seid.val=crc(_seid.str, _seid.len);
    } else {
        _seid.str=NULL;
        _seid.len=0;
        _seid.val=0;
    }

    i=fel( 0x20, 0x11 );       // series number
    if( i!=-1 ){
        strncpy( buf, (const char*)_e[i].ptr, _e[i].len );
        buf[_e[i].len]='\0';
        _sn=strtol( buf, NULL, 10 );
    }

    i=fel( 0x20, 0x13 );       // instance number
    if( i!=-1 ){
        strncpy( buf, (const char*)_e[i].ptr, _e[i].len );
        buf[_e[i].len]='\0';
        _in=strtol( buf, NULL, 10 );
    }
    i=fel( 0x20, 0x32 );        // ImagePositionPatient
    if( i!=-1 ){
        const char *str=(const char*)_e[i].ptr;
        int n=3;
        get_dbl( str, str+_e[i].len, _c, n );
    } else 
        _c[0]=_c[1]=_c[2]=0;

    i=fel( 0x28, 0x10 );
    if( i!=-1 ){
        switch ( _e[i].len ){
            case 2:
                _ny=getu16( _e[i].ptr );
                break;
            case 4:
                _ny=getu32( _e[i].ptr );
                break;
            default:
                _ny=0;
                break;
        }
    } else
        _ny=0;

    i=fel( 0x28, 0x11 );
    if( i!=-1 ){
        switch ( _e[i].len ){
            case 2:
                _nx=getu16( _e[i].ptr );
                break;
            case 4:
                _nx=getu32( _e[i].ptr );
                break;
            default:
                _nx=0;
                break;
        }
    } else
        _nx=0;

    i=fel( 0x28, 0x30 );        // pixel spacing
    if( i!=-1 ){
        const char *str=(const char*)_e[i].ptr;
        int n=2;
        double c[2];
        get_dbl( str, str+_e[i].len, c, n );
        _dx=c[0];
        _dy=c[1];
    } else 
        _dx=_dy=0;

    i=fel( 0x28, 0x100 );       // bits allocated
    if( i!=-1 ) _nb=getu16( _e[i].ptr );
    else _nb=0;

    i=fel( 0x28, 0x101 );       // bits effective
    if( i!=-1 ) _nbe=getu16( _e[i].ptr );
    else _nbe=0;

    i=fel( 0x28, 0x103 );      // pixel representation
    if( i!=-1 ) _pxr=getu16( _e[i].ptr );
    else _pxr=0;

    i=fel( 0x28, 0x120 );      // pixel padding value
    if( i!=-1 ) _pv=gets16( _e[i].ptr );
    else _pv=0;

    i=fel( 0x7fe0, 0x10 );      // pixel data
    if( i!=-1 ){
        if( _nb==16 )
            _d=_e[i].ptr;
        if( _nb==8 ){
            E( "8bit is not supported" );
        }
    } else
        _d=NULL;
}

int
dcmf::roi_elm(const void *dat, uint32_t len, roi_elm_t *roie ) const {
    const u8_t *d=(const u8_t*)dat, *end=d+len;
    while( d<end ){
        uint16_t gn, en;
        gn=getu16( d );
        d+=2;
        en=getu16( d );
        d+=2;
        len=getu32(d);
        d+=4;
        if( gn==0x3006 && en==0x16 ){
            gn=getu16( d );
            d+=2;
            en=getu16( d );
            d+=2;
            len=getu32(d);
            d+=4;               // gn&en must be Item elem
            const u8_t *ei=d+len;
            while( d<ei ){
                gn=getu16( d );
                d+=2;
                en=getu16( d );
                d+=2;
                len=getu32(d);
                d+=4;
                if( gn==0x8 ){
                    if( en==0x1150 ){ // ReferencedSOPClassUID
                        str_t st;
                        st.str=(const char*)d;
                        st.len=len;
                        st.val=crc(st.str, st.len);
                        roie->cuid=st;
                    }
                    if( en==0x1155 ){ // ReferencedSOPInstanceUID
                        str_t st;
                        st.str=(const char*)d;
                        st.len=len;
                        st.val=crc(st.str, st.len);
                        roie->iuid=st;
                    }
                } else          // unexpected element?
                    break;
                d+=len;
            }
            d=ei;
            continue;
        }
        if( gn==0x3006 ){
            //T( "%p %p l%d", gn, en, len );
            if( en==0x42 ){       // ContourGeometricType
                str_t st;
                st.str=(const char*)d;
                st.len=len;
                st.val=crc(st.str, st.len);
                roie->gtyp=st;                
            }
            if( en==0x46 ){       // NumberOfContourPoints
                roie->ncp=strtol( (const char*)d, NULL, 10 );
            }
            if( en==0x50 ){       // ContourData
                str_t st;
                st.str=(const char*)d;
                st.len=len;
                st.val=crc(st.str, st.len);
                roie->dta=st;                
            }
        }
        d+=len;
    }
    
    return 0;
}

int
dcmf::cont_seq(const void *dat, uint32_t len, roi_elm_t *roie ) const{
    const u8_t *d=(const u8_t*)dat, *end=d+len;
    if( roie!=NULL ){           // get data
        int ni=0;
        while( d<end ){
            uint16_t gn, en;
            gn=getu16( d );
            d+=2;
            en=getu16( d );
            d+=2;
            len=getu32(d);
            d+=4;
            if( gn!=0xfffe || en!=0xe000 ) // Item expected here
                return -1;
            roi_elm( d, len, roie+ni );
            ++ni;
            d+=len;
        }
        return ni;
    } else {                    // count items
        int ni=0;
        while( d<end ){
            uint16_t gn, en;
            gn=getu16( d );
            d+=2;
            en=getu16( d );
            d+=2;
            len=getu32(d);
            d+=4;
            //T( "%p %p l%d", gn, en, len );
            if( gn!=0xfffe || en!=0xe000 ) // Item expected here
                return -1;
            ++ni;
            d+=len;
        }
        return ni;
    }
    return 0;
}

roi_seq_t*
dcmf::roi_seq( bool hasvr ) const{
    int n=fel( 0x3006, 0x39 );  // ROIContourSequence

    if( n==-1 ) return NULL;

    const u8_t *d=(const u8_t*)_e[n].ptr, *end=d+_e[n].len;
    uint16_t gn, en;// vr;
    gn=getu16( d );
    d+=2;
    en=getu16( d );
    d+=2;
    uint32_t len=0;
    if( hasvr ){
        E( "fixme: hasvr is not implemented" );
        //vr=getvr( d );
        d+=2;
    } else {
        len=getu32(d);
        d+=4;
    }
    if( gn!=0xfffe || en!=0xe000 ) // Item expected here
        return NULL;

    roi_seq_t *rs=NULL;
    uint32_t rgb=0;
    while( d < end ){
        gn=getu16( d );
        d+=2;
        en=getu16( d );
        d+=2;
        len=getu32(d);
        d+=4;        
        if( gn==0x3006 && en==0x2a ){ // ROIDisplayColor
            int v[3], n=3;
            get_int( (const char*)d, (const char*)(d+len), v, n );
            rgb=v[0]<<16|v[1]<<8|v[2];
            // get rgb values
            T( "rgb's, %d", n );
            d+=len;
            continue;
        }
        if( gn==0x3006 && en==0x40 ){ // ContourSequence
            int nc=cont_seq( d, len, NULL );
            T( "found %d contours", nc );
            if( nc <= 0 ) return NULL;
            rs=(roi_seq_t*)malloc( sizeof(rs[0])+(nc-1)*sizeof(rs->e[0]) );
            rs->ne=cont_seq( d, len, rs->e );
            d+=len;
        }
    }
    if( rs ) rs->rgb=rgb;
    return rs;
}
