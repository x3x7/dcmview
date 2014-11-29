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
#ifndef _DICOM_H_
#define _DICOM_H_
#include <stddef.h>
#include <stdint.h>
#include "tr.h"
#include "words.h"
typedef  uint8_t u8_t;
typedef uint16_t u16_t;
typedef  int16_t i16_t;
typedef uint32_t u32_t;
typedef  int32_t i32_t;

class dcmf;
enum vr_e {
    vr_00= 0,
    vr_AE= (('A'<<8)|'E'),
    vr_AS= (('A'<<8)|'S'),
    vr_AT= (('A'<<8)|'T'),
    vr_CS= (('C'<<8)|'S'),
    vr_DA= (('D'<<8)|'A'),
    vr_DS= (('D'<<8)|'S'),
    vr_DT= (('D'<<8)|'T'),
    vr_FD= (('F'<<8)|'D'),
    vr_FL= (('F'<<8)|'L'),
    vr_IS= (('I'<<8)|'S'),
    vr_LO= (('L'<<8)|'O'),
    vr_LT= (('L'<<8)|'T'),
    vr_OB= (('O'<<8)|'B'),
    vr_OF= (('O'<<8)|'F'),
    vr_OW= (('O'<<8)|'W'),
    vr_PN= (('P'<<8)|'N'),
    vr_SH= (('S'<<8)|'H'),
    vr_SL= (('S'<<8)|'L'),
    vr_SQ= (('S'<<8)|'Q'),
    vr_SS= (('S'<<8)|'S'),
    vr_ST= (('S'<<8)|'T'),
    vr_TM= (('T'<<8)|'M'),
    vr_UI= (('U'<<8)|'I'),
    vr_UL= (('U'<<8)|'L'),
    vr_US= (('U'<<8)|'S'),
    vr_UN= (('U'<<8)|'N'),
    vr_UT= (('U'<<8)|'T'),
};

class dicom {                   // DICOM params
    friend class dcmf;
public:
    typedef struct {
        uint16_t gn;            // group number
        uint16_t en;            // element number
        uint16_t vr;            // value representation
        uint16_t mk;            // mask for gn
//        uint16_t fl;            // flags
        const char *nm;         // name
    } del_t;                    // data elements
    // get table index for gn:en
    static int del( uint16_t gn, uint16_t en ) {
        int i=0;
        while( i<tabsz ){
            if( (tab[i].gn==(gn&tab[i].mk))
                && (tab[i].en==en) )
                return i;
            if( gn < tab[i].gn )
                return -1;
            ++i;
        }
        return -1;
    }
private:
    static del_t tab[];
    static int   tabsz;
};


typedef struct {
    str_t cuid; // ReferencedSOPClassUID
    str_t iuid; // ReferencedSOPInstanceUID
    str_t gtyp; // ContourGeometricType
    str_t dta;  // ContourData
    u32_t ncp;  // NumberOfContourPoints
} roi_elm_t;

typedef struct {
    str_t name;                   // ROIName
    str_t algo;                   // ROIGenerationAlgorithm
    u32_t rgb;                    // ROIDisplayColor
    u32_t nr;                     // ROINumber
    u32_t ne;                     // # of elements
    roi_elm_t e[1];               // elements data
    void show() const;
} roi_seq_t;

class dcmf {                    // dcm file
    typedef struct {
        const void *ptr;        // data pointer
        u32_t len;              // length
        u32_t ti;               // index in elements table
    } fel_t;                    // file data element

    // locate file element with 'gn' and 'en'
    int fel( uint16_t gn, uint16_t en ) const{
        int i=0;
        while( i<_esz ){
            int j=_e[i].ti;
            if( (dicom::tab[j].gn==(gn&dicom::tab[j].mk))
                && (dicom::tab[j].en==en) )
                return i;
            if( gn < dicom::tab[j].gn )
                return -1;
            ++i;
        }
        return -1;              // not found
    }
    int _debug;                // show debug traces
    const void *_dat;           // data file
    size_t _len;                // file size

    fel_t *_e;                  // parsed elements
    int32_t _esz;
    
    int32_t _sn;                // series #
    int32_t _in;                // instance #
    
    str_t _iid;                 // SOPInstanceUID
    str_t _stid;                // StudyInstanceUID
    str_t _seid;                // SeriesInstanceUID

    int32_t _nb;                // allocated bits
    int32_t _nbe;               // effective bits (<= _nb );

    i16_t _pv;          // padding value
    u16_t _pxr;         // pixel representation (0 unsigned, 1 signed)
    const void *_d;     // image data
    i32_t _pxmin;       // image stats
    i32_t _pxmax;

    int32_t _nx, _ny;           // size x (Columns), y (Rows)
    // PixelSpacing
    double _dx, _dy;
    // ImagePositionPatient
    double _c[3];               // (x,y,z)

    int parse();                // parse data
    void get_info();            // read selected elements
    // read a ContourImageSequence
    int roi_elm(const void *dat, uint32_t len, roi_elm_t *roie ) const;
    // scan/read a ContourSequence, return # of ContourImageSequence elems
    int cont_seq(const void *d, uint32_t len, roi_elm_t *roie ) const;
    int seq( int lvl, const void *d, uint32_t len );
public:
    dcmf()
        :_debug(0), _dat(NULL), _len(0), _e(NULL), _esz(0) {};
    dcmf( const char *fname, int dbg=0 ) 
        :_debug(dbg),_dat(NULL), _len(0), _e(NULL), _esz(0) { load(fname); }
    ~dcmf(){ clear(); }    
    int load( const char*fname );
    void pxstats();
    void clear();
    bool valid() const { return _dat!=NULL; }
    int x() const { return _nx; }
    int y() const { return _ny; }
    int b() const { return _nb; }
    int be() const { return _nbe; }
    int in() const { return _in; }
    int sn() const { return _sn; }
    int pxr() const { return _pxr; }
    int pv() const { return _pv; }
    i32_t pxmin() const { return _pxmin; }
    i32_t pxmax() const { return _pxmax; }
    const double *ipp() const { return _c; }
    double dx() const { return _dx; }
    double dy() const { return _dy; }
    const str_t& iid() const { return _iid; }
    const u16_t *d16u() const { 
        if( _pxr==0 ) return (const u16_t*)_d;
        else return NULL;
    }

    const i16_t *d16s() const { 
        if( _pxr==1 ) return (const i16_t*)_d;
        else return NULL;
    }
    
    roi_seq_t* roi_seq( bool hasvr=false ) const;

    static u16_t getu16( const void *dv ){ 
        const u8_t*d=(const u8_t*)dv;
        return (d[1]<<8)|d[0];
    }
    static u16_t getu16s( const void *dv ){ // swapped
        const u8_t*d=(const u8_t*)dv;
        return (d[0]<<8)|d[1];
    }
    static i16_t gets16( const void *dv ){ 
        const u8_t*d=(const u8_t*)dv;
        return (d[1]<<8)|d[0];
    }
    static i16_t gets16s( const void *dv ){ 
        const u8_t*d=(const u8_t*)dv;
        return (d[0]<<8)|d[1];
    }
    static u16_t getvr( const void *dv ){
        const u8_t*d=(const u8_t*)dv;
        return (d[0]<<8)|d[1];
    }
    static u32_t getu32( const void *dv ){ 
        const u8_t*d=(const u8_t*)dv;
        return (getu16(d+2)<<16)|getu16(d);
    }
    static u32_t getu32s( const void *dv ){ 
        const u8_t*d=(const u8_t*)dv;
        return (getu16s(d)<<16)|getu16s(d+2);
    }
    static const char* 
    get_dbl( const char* str, const char *eptr, double d[], int &max );
    static const char* 
    get_flt( const char* str, const char *eptr, float d[], int &max );
    static const char* 
    get_int( const char* str, const char *eptr, int d[], int &max );
};

#endif
