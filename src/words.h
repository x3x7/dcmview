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
#ifndef _WORDS_H_
#define _WORDS_H_
#include <stdint.h>
#include <stdlib.h>

typedef uint32_t chksum_t;

typedef struct {
    const char *str;
    uint32_t len;
    chksum_t val;
} str_t;

void 
initxl_ci();

void 
initxl_cd();

uint32_t
crc32( const uint8_t *data, size_t len  );

uint32_t
crc32_inc( const uint32_t *data, uint32_t &crc32tmp );

uint32_t 
crc32( const char *data );

inline chksum_t
crc( const char *data, size_t len  ) { return crc32( (const uint8_t *)data, len ); }

inline chksum_t
crc( const str_t &str ) { return crc32( (const uint8_t *)str.str, str.len ); }

inline chksum_t 
crc( const char *data ){  return crc32( data ); }

inline bool
streq( const str_t& a, const str_t& b ){ return a.val==b.val; }

typedef struct {
    str_t wd;                   // string
    uint32_t sz;                // crnt size
    uint32_t mx;                // max size
    uint32_t d[1];              // data
} smatch_t;

class momap {                   // many to one map
    typedef struct {
        uint32_t i;             // input value
        uint32_t o;             // output value
    } pair_t;

    uint32_t _sz;
    pair_t *_p;
    uint32_t pos( uint32_t i ) const;
public:
    momap( uint32_t sz=0 ) :_sz(sz), _p(NULL){ alloc( sz ); }
    ~momap(){ if( _p ) free( _p ); }

    void alloc( uint32_t sz );
    uint32_t get( uint32_t i ) const { return _p[pos(i)].o; }
    uint32_t get( const char *str ) const { return get( crc(str) ); }
    uint32_t get( const str_t &s ) const { return get( crc(s.str,s.len) ); }

    void set( uint32_t i, uint32_t o ){
        uint32_t p=pos( i );
        if( _p[p].i==0 ) _p[p].i=i;
        _p[p].o=o;
    }
    void set( const char *str, uint32_t o ){ set( crc(str), o ); }
    void set( const str_t &s, uint32_t o ){ set( crc(s.str,s.len), o ); }
    
};

class strh {
    // hashes words to list of matches
    uint32_t  _iz;              // items
    uint32_t  _sz;              // total size
    smatch_t  **_m;
    uint32_t pos( chksum_t ck ) const;
    uint32_t pos( const str_t &wd ) const { return pos(wd.val); };
public:
    strh( uint32_t sz );
    ~strh( );
    void add( const str_t &wd, uint32_t val );
    const smatch_t *get( const str_t &wd ) const { return _m[pos(wd)]; }
    const smatch_t *get( chksum_t ck ) const { return _m[pos(ck)]; }
    void quality() const;
    uint32_t items() const { return _iz; }
    smatch_t** all( uint32_t *isz );
};

typedef struct {
    uint32_t d;                 // index/pos usage
    uint32_t m;                 // word mask
} ngd_t;

typedef struct {
    chksum_t cs;                // checksum
    uint32_t sz;                // crnt size
    uint32_t mx;                // max size
    ngd_t d[1];                 // data
} gmatch_t;

class ngh {                     // n-gram hash
    // hashes n-grams to list of matches
    uint32_t  _iz;              // items
    uint32_t  _mk;              // mask (for modulo)
    gmatch_t  **_m;
    inline uint32_t pos( chksum_t cs ) const {
        uint32_t p=cs&_mk;
        for(;;) {
            if( _m[p] == NULL ) return p;
            if( _m[p]->cs==cs )
                return p;
            if( p==_mk ) 
                p=0;
            else 
                ++p;
        }
        return _mk+1;           // never
    }
public:
    ngh( ) :_iz(0), _m(NULL) {}
    ~ngh( );
    void alloc( uint32_t sz, uint32_t mul );
    void add( chksum_t cs, ngd_t val );
    const gmatch_t *get( chksum_t cs ) const { return _m[pos(cs)]; }
    void quality() const;
    uint32_t items() const { return _iz; }
    gmatch_t** all( uint32_t *isz );
};


class words {                   // a hash of words
    uint32_t _iz;
    uint32_t _sz;
    const char **_w;
    chksum_t *_c;

    uint32_t pos( const str_t &wd ) const;
    uint32_t pos( const char* w ) const;

public:
    static momap cls;           // classes are taken into account
    // some helper lexical tokens
    static str_t bem;           // begin/end marker
    static str_t sep;           // sentence/paragraph separators.
    static str_t num;           // number (123 or 12.3)
    static str_t curr;          // money amount
    static str_t http;          // http, https URL
    static str_t email;         // email
    
    static uint32_t getv( const char* str ){
        uint32_t v=crc32( str );
        uint32_t o=cls.get(v);
        if( o ) return o;
        return v;
    }

    static void setv( str_t &s ){      // class based crc
        uint32_t v=crc( s.str, s.len );
        uint32_t o=cls.get(v);
        if( o )
            s.val=o;
        else
            s.val=v;
    }

    static uint32_t split( const char *txt, const char *eptr, 
                           str_t *wd, uint32_t max, bool sep=false );

    static inline               // l >= 2 !
    chksum_t chksum( const str_t w[], int l ){
        chksum_t c[l];              // use NGL if compiler complains
        int i; 
        for( i=0; i<l; ++i ) c[i]=w[i].val;
        return crc32( (const uint8_t*)c, sizeof(c) );
    }
    
    static inline               // l >= 2
    chksum_t chksum( const char *w[], int l ){
        chksum_t c[l];
        int i;
        for( i=0; i<l; ++i ) c[i]=words::getv( w[i] );
        return crc32( (const uint8_t*)c, sizeof(c) );
    }
    static inline
    chksum_t chksum_add( const str_t&w, chksum_t &chk ){
        return crc32_inc( &w.val, chk );
    }
    static inline
    chksum_t chksum_add( const char*str, chksum_t &chk ){
        uint32_t val=getv(str);
        return crc32_inc( &val, chk );
    }
    static inline               // l >= 2
    chksum_t chksum( const chksum_t c[], int l ){
        return crc32( (const uint8_t*)c, l*sizeof(c[0]) );
    }

    words( uint32_t sz=0);
    ~words();
    bool empty(){ return _sz==0 || _iz==0; }
    void alloc( uint32_t sz );

    bool add( const char *w );
    bool add( const str_t &w );
    bool has( const str_t &wd ) const { return wd.len ? _w[pos(wd)]!=NULL:false; }
    bool has( const char *w ) const { return _w[pos(w)]!=NULL; }
    void quality() const;
};

#endif
