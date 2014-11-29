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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "words.h"
#include "tr.h"

momap words::cls;
str_t words::bem={.str="[<@>]",.len=0,.val=0};
str_t words::sep={.str="]<@>[",.len=0,.val=0};
str_t words::num={.str="[###]",.len=0,.val=0};
str_t words::curr={.str="[$$$]",.len=0,.val=0};
str_t words::http={.str="[http:]",.len=0,.val=0};
str_t words::email={.str="[@mail]",.len=0,.val=0};

static inline 
int iswordc( int c ){ // words can contain these characters
    if( isalnum( c )
//        || c=='\''
//        || c=='/'
//        || c=='&'
        || c=='_'
//	|| c==':'
//        || c=='-'
        )
        return 1;
    return 0;
}

static inline 
uint32_t hfun( uint32_t i ){ return i*1812433253 + 12345; }

uint32_t
momap::pos( uint32_t i ) const {
    uint32_t p=hfun( i )%_sz;
    for(;;) {
        if( _p[p].i == i || _p[p].i==0 ) return p;
        ++p;
        if( p==_sz ) p=0;
    }
    return _sz;                 // never
}

void
momap::alloc( uint32_t sz ){
    if( _p != NULL )
        free( _p );
    _sz=sz;
    if( _sz )
        _p=(pair_t*) calloc( _sz, sizeof( _p[0] ) );
    else
        _p=NULL;
}

static const char*
chk_url( const char *txt, const char *eptr ){
    while( txt<eptr && !isspace(txt[0]) ) ++txt;
    return txt;
}
static const char*
chk_eml( const char *txt, const char *eptr ){
    while( txt<eptr && !isspace(txt[0]) ) ++txt;
    return txt;
}

static const char*
chk_num( const char *txt, const char *eptr ){
    int dots=0;
    const char *ptx=txt;
    while( txt<eptr ){
        if( ptx[0]=='.' )
            ++dots;
        else
            if( (ptx[0] < '0' || ptx[0] > '9' || dots > 1)
                &&  (ptx[0]!=',' ) )
                break;
        ++ptx;
    }
    // trim dots sequences at end
    if( dots > 1 ){
        while( ptx > txt && ptx[-1]=='.' ){
            --ptx;
            --dots;
        }
    }
    if( ptx[-1]=='.' ) // remove end dot, perhaps end of sentence
        return ptx-1;
    else
        return ptx;
}

static const char*
get_num( str_t *w, const char *sptr, const char *ptr, const char *eptr ){
    if( isdigit(w->str[0] ) ){
        const char *p=chk_num( w->str, eptr );
        if( p>=ptr ){
            if( w->str > sptr && w->str[-1]=='$' ){
                --w->str;
                w->val=words::curr.val;
            } else
                w->val=words::num.val;
            w->len=p-w->str;
        }
        return p;
    }
    return w->str;
}

static const char*
chk_abbrev( const char *txt, const char *eptr ){
    int dots=0;
    const char *ptx=txt;
    if( isalnum(ptx[0]) ){
        ++ptx;
        while( ptx<eptr && (isalnum(ptx[0])||ptx[0]=='.') ){
            if( ptx[0]=='.' ) ++dots;
            ++ptx;
        }
    }
    // trim dots sequences at end
    if( dots > 1 ){
        while( ptx > txt && ptx[-1]=='.' ){
            --ptx;
            --dots;
        }
    }
    if( dots==1 && ptx[-1]=='.' ) // just one dot, perhaps just end of sentence
        return ptx-1;
    else
        return ptx;
}

static const char*
chk_space( const char *txt, const char *eptr, int &nl ){
    nl=0;
    while( txt<eptr && !iswordc(txt[0]) ){
        if( txt[0]=='\n' ) ++nl;  
        ++txt;
    }
    return txt;
}

uint32_t
words::split( const char *txt, const char *eptr, 
              str_t *wd, uint32_t wmax,  bool sep ){
    str_t *w=wd, *we=w+wmax;
    const char *ptx=txt;
    w->str=NULL;
    while( ptx<eptr ){
        if( iswordc( ptx[0] ) ){ // word character
            if( w->str==NULL )
                w->str=ptx;     // start new word
            ++ptx;
        } else {
            // word separator
            if( w->str ){      // and a word is ending
                const char *p;
                // is it a number or currency value
                p=get_num( w, txt, ptx, eptr );
                if( p>=ptx ){
                    ptx=p;
                    goto wnext;
                }
                switch( ptx[0] ){
                    case '.':   // abbrev?
                        p=chk_abbrev( w->str, eptr );
                        if( p>=ptx )
                            ptx=p;
                        break;
                        // handle 's and `s endings as same
                    case '\'':
                    case '`':
                        if( (ptx[1]=='s'||ptx[1]=='S') && isspace( ptx[2] )){
                                w->len=ptx-w->str;
                                setv(w[0]);
                                ++w;
                                if( w==we )
                                    break;
                                w->str=ptx;
                                w->len=2;
                                w->val=crc("'s");
                                ptx+=2;
                                goto wnext;
                        }
                        break;
                    case '@':
                        p=chk_eml( w->str, eptr );
                        if( p > ptx ){
                            w->len=p-w->str;
                            w->val=words::email.val;
                            ptx=p;
                            goto wnext;
                        }
                        break;
                    case ';': // xml escapes?
                        if( w->str[-1]=='&' ){
                            --w->str;
                            ++ptx;
                        }
                        break;
                    case ':':
                        if( strncasecmp( w->str, "http", 4 )==0 ){
                            // we seem to be in an url
                            ptx=chk_url( w->str, eptr );
                            w->val=words::http.val;
                            w->len=ptx-w->str;
                            goto wnext;
                        }
                        break;
                    case '/':
                        if( w->str==ptx-1 ){
                            ++ptx; // allow short seq like 'a/s'
                            continue;
                        }
                    default:
                        break;
                }
                // end the word
                w->len=ptx-w->str;
                setv(w[0]);
                // '.' as end of sentence is unreliable, we check for '\n'
                int nl;
                ptx=chk_space( ptx, eptr, nl );
                if( nl > 1 && sep ){
                    ++w;
                    if( w==we )
                        break;
                    w->len=0;
                    w->str=w[-1].str+w[-1].len;
                    w->val=words::sep.val;
                }
              wnext:
                //T( "%d[%.*s]", w-wd, w->len, w->str );
                ++w;
                if( w==we )
                    break;
                w->str=NULL;                
            } else              // just advance
                ++ptx;
        }
    }
    if( w<we && w->str ){
        if( get_num( w, txt, ptx, eptr )<eptr ){
            w->len=ptx-w->str;
            setv(w[0]);
        }
        ++w;        
    }
    //T( "nw %d eptr-ptx %d", w-wd, eptr-ptx );
    if( ptx==eptr )
        return w-wd;
    else
        return wmax+1;          // signal incomplete parse
}

strh::strh( uint32_t sz ) : _iz(0), _sz(sz) {
    _m=(smatch_t **)calloc( _sz, sizeof( _m[0] ) );
}

strh::~strh( ){
    uint32_t n;
    for( n=0; n<_sz; ++n )
        if( _m[n]!=NULL )
            free( _m[n] );
    free( _m );
}

uint32_t
strh::pos( chksum_t val ) const {
    uint32_t p=val%_sz;
    for(;;) {
        if( _m[p] == NULL ) return p;
        if( _m[p]->wd.val==val )
            return p;
        ++p;
        if( p==_sz ) p=0;
    }
    return _sz;                   // never
}

void 
strh::add( const str_t &wd, uint32_t val ){
    uint32_t p=pos( wd );
    if( _m[p] ){
        smatch_t *m=_m[p];
        if( m->sz==m->mx ){
            m->mx+=8;
            m=(smatch_t*)realloc( m, sizeof( smatch_t )+(m->mx-1)*sizeof(m->d[0]));
            _m[p]=m;
        }
        m->d[m->sz++]=val;
        
    } else {
        smatch_t *m=(smatch_t*)malloc( sizeof( smatch_t ) );
        m->sz=m->mx=1;
        m->wd=wd;
        m->d[0]=val;
        _m[p]=m;
        ++_iz;
    }
}

smatch_t**
strh::all( uint32_t *riz ){
    uint32_t iz=items();
    *riz=iz;
    if( iz ){
        smatch_t **v=(smatch_t **)malloc( iz*sizeof(v[0]) );
        uint32_t i, p;
        for( p=i=0; i<_sz; ++i ){
            if( _m[i] )
                v[p++]=_m[i];
        }
        return v;
    } else 
        return NULL;
}


void
strh::quality() const {
    if( _iz ){
        uint32_t n, sd, mx; 
        // search stats for hash elements
        sd=0;
        mx=0;
        for( n=0; n<_sz; ++n ){
            if( _m[n] ){
                uint32_t d, p;
//                p=hfun( _m[n]->wd.str, _m[n]->wd.len )%_sz;
                p=_m[n]->wd.val%_sz;
                d=1+(((n+_sz)-p)%_sz);
                if( d > mx ) mx=d;
                sd+=d;
            }
        }
        // bucket distribution (for reject speed)
        uint32_t c, ns, bl, sbl, bmx;
        c=n=0;
        while( n<_sz && _m[n] ) ++n;     // start at 1st empty position
        if( n < _sz ){
            ns=n; sbl=bl=0; bmx=0;
            do {
                if( _m[n] ) 
                    ++bl;
            else
                if( bl ){
                    sbl+=bl;
                    if( bl > bmx ) bmx=bl;
                    bl=0;
                    ++c;
                }
                ++n;
                if( n==_sz ) n=0;
            } while( n!=ns );
        } else {                // full hash!!?
            c=1;
            sbl=_sz;
            bmx=_sz;
        }

        T( "size: %d/%d avg: %3.1f max: %d\t bkts:%d avgl: %3.1f maxl: %d",
           _iz, _sz, double(sd)/double(_iz), mx,
           c, double(sbl)/double(c), bmx );
    } else {
        T( "empty hash" );
    }
}


ngh::~ngh( ){
    uint32_t n;
    for( n=0; n<=_mk; ++n )
        if( _m[n]!=NULL )
            free( _m[n] );
    free( _m );
}

void
ngh::alloc( uint32_t sz, uint32_t mul ){
    if( _m != NULL )
        free( _m );

    _mk=1;
    while( _mk <= sz*mul )
        _mk<<=1;

    _mk-=1;
    
    if( _mk )
        _m=(gmatch_t **) calloc( _mk+1, sizeof( _m[0] ) );
    else
        _m=NULL;
    _iz=0;
}

void 
ngh::add( chksum_t cs, ngd_t val ){
    uint32_t p=pos( cs );
    if( _m[p] ){
        gmatch_t *m=_m[p];
        if( m->sz==m->mx ){
            m->mx+=8;
            m=(gmatch_t*)realloc( m, sizeof( gmatch_t )+(m->mx-1)*sizeof(m->d[0]));
            _m[p]=m;
        }
        m->d[m->sz++]=val;
        
    } else {
        gmatch_t *m=(gmatch_t*)malloc( sizeof( gmatch_t ) );
        m->sz=m->mx=1;
        m->cs=cs;
        m->d[0]=val;
        _m[p]=m;
        ++_iz;
    }
}

gmatch_t**
ngh::all( uint32_t *riz ){
    uint32_t iz=items();
    *riz=iz;
    if( iz ){
        gmatch_t **v=(gmatch_t **)malloc( iz*sizeof(v[0]) );
        uint32_t i, p;
        for( p=i=0; i<=_mk; ++i ){
            if( _m[i] )
                v[p++]=_m[i];
        }
        return v;
    } else 
        return NULL;
}


void
ngh::quality() const {
    if( _iz ){
        uint32_t n, sd, mx; 
        // search stats for hash elements
        sd=0;
        mx=0;
        for( n=0; n<=_mk; ++n ){
            if( _m[n] ){
                uint32_t d, p;
                p=_m[n]->cs&_mk;
                d=1+(((n+_mk+1)-p)&_mk);
                if( d > mx ) mx=d;
                sd+=d;
            }
        }
        // bucket distribution (for reject speed)
        uint32_t c, ns, bl, sbl, bmx;
        c=n=0;
        while( n<=_mk && _m[n] ) ++n;     // start at 1st empty position
        if( n <= _mk ){
            ns=n; sbl=bl=0; bmx=0;
            do {
                if( _m[n] ) 
                    ++bl;
            else
                if( bl ){
                    sbl+=bl;
                    if( bl > bmx ) bmx=bl;
                    bl=0;
                    ++c;
                }
                if( n==_mk ) n=0;
                else ++n;
            } while( n!=ns );
        } else {                // full hash!!?
            c=1;
            sbl=_mk+1;
            bmx=_mk+1;
        }

        T( "size: %d/%d avg: %3.1f max: %d\t bkts:%d avgl: %3.1f maxl: %d",
           _iz, _mk+1, double(sd)/double(_iz), mx,
           c, double(sbl)/double(c), bmx );
    } else {
        T( "empty hash" );
    }
}


words::words(uint32_t sz) : _iz(0), _sz(sz), _w(NULL) {
    if( _sz )
        _w=(const char**)calloc( _sz, sizeof( _w[0] ) );
    else
        _w=NULL;
}

void
words::alloc( uint32_t sz ){
    if( _w != NULL ){           // _w and _c are in same state
        free( _w ); free( _c );
    }
    _sz=sz;
    _iz=0;
    if( _sz ){
        _w=(const char**)calloc( _sz, sizeof( _w[0] ) );
        _c=(chksum_t*)malloc( _sz*sizeof( _c[0] ) );
    } else
        _w=NULL;
}

words::~words(){
    if( _sz ){
#if(0)
        uint32_t i;
        for( i=0; i<_sz; ++i )
            if( _w[i] ) free( (void*)_w[i] );
#endif
        free( _c );
        free( _w );
    }
}


uint32_t
words::pos( const str_t &wd ) const {
    chksum_t c=wd.val;
    uint32_t p=c%_sz;
    for(;;) {
        if( _w[p] == NULL ){ _c[p]=c; return p; }
        if( _c[p] ==c ) return p;
        ++p;
        if( p==_sz ) p=0;
    }
    return 0;                   // never
}

uint32_t
words::pos( const char *w ) const {
    chksum_t c=words::getv( w );
    uint32_t p=c%_sz;
    for(;;) {
        if( _w[p] == NULL ){ _c[p]=c; return p; }
        if( _c[p] == c ) return p;
        ++p;
        if( p==_sz ) p=0;
    }
    return 0;                   // never
}

bool 
words::add( const char *w ){ 
    uint32_t p=pos(w);
    if( _w[p]==NULL ){
        _w[p]=w;                // all strings are const's no need for 'strdup'
        // _c[p] updated by 'pos'
        ++_iz;
        return true;
    }
    return false;
}

bool 
words::add( const str_t &wd ){ 
    uint32_t p=pos(wd);
    E( "FIXME: memory issues here!" );
    if( _w[p]==NULL ){
        _w[p]=strndup(wd.str, wd.len);
        ++_iz;
        return true;
    }
    return false;
}

void
words::quality() const {
    if( _iz ){
        uint32_t n, sd, mx; 
        // search stats for hash elements
        sd=0;
        mx=0;
        for( n=0; n<_sz; ++n ){
            if( _w[n] ){
                uint32_t d, p=crc( _w[n] )%_sz;
                d=1+(((n+_sz)-p)%_sz);
                if( d > mx ) mx=d;
                sd+=d;
            }
        }
        // bucket distribution (for reject speed)
        uint32_t c, ns, bl, sbl, bmx;
        c=n=0;
        while( n<_sz && _w[n] ) ++n;     // start at 1st empty position
        if( n < _sz ){
            ns=n; sbl=bl=0; bmx=0;
            do {
                if( _w[n] ) 
                    ++bl;
            else
                if( bl ){
                    sbl+=bl;
                    if( bl > bmx ) bmx=bl;
                    bl=0;
                    ++c;
                }
                ++n;
                if( n==_sz ) n=0;
            } while( n!=ns );
        } else {                // full hash!!?
            c=1;
            sbl=_sz;
            bmx=_sz;
        }

        T( "size: %d/%d avg: %3.1f max: %d\t bkts:%d avgl: %3.1f maxl: %d",
           _iz, _sz, double(sd)/double(_iz), mx,
           c, double(sbl)/double(c), bmx );
    } else {
        T( "empty hash" );
    }
}



static const
uint32_t tab_crc32[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419,
    0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4,
    0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
    0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
    0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856,
    0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
    0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
    0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a,
    0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599,
    0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190,
    0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
    0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e,
    0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed,
    0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3,
    0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
    0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
    0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
    0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010,
    0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17,
    0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
    0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
    0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344,
    0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a,
    0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
    0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c,
    0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
    0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe,
    0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
    0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
    0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b,
    0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1,
    0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
    0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
    0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
    0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66,
    0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
    0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8,
    0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
    0x2d02ef8d
};

// case independent trafo
static 
uint8_t xl[256];

uint32_t
crc32( const uint8_t *data, size_t len  )
{
    uint32_t crc32tmp=0;
    size_t i;
    for (i = 0;  i < len;  ++i)
        crc32tmp = tab_crc32[(crc32tmp ^ xl[data[i]]) & 0xff] ^ (crc32tmp >> 8);
    if( crc32tmp )
        return crc32tmp;
    else {
        E("zero");
        return 1;               // 0 not an option
    }
}

uint32_t                        // used only for n-grams, no need for xl
crc32_inc( const uint32_t *d32, uint32_t &crc32tmp  )
{
    const uint8_t *data=(const uint8_t*)d32;
    size_t i;
    for (i = 0;  i < sizeof(uint32_t);  ++i)
        crc32tmp = tab_crc32[(crc32tmp ^ data[i]) & 0xff] ^ (crc32tmp >> 8);
    if( crc32tmp )
        return crc32tmp;
    else {
        E("zero");
        return 1;               // 0 not an option
    }
}

uint32_t
crc32( const char *cdata  )
{
    const uint8_t *data=(const uint8_t*)cdata;
    uint32_t crc32tmp=0;
    while( data[0] ){
        crc32tmp = tab_crc32[(crc32tmp ^ xl[data[0]]) & 0xff] ^ (crc32tmp >> 8);
        ++data;
    }
    if( crc32tmp )
        return crc32tmp;
    else {
        E("zero");
        return 1;
    }
}

void 
initxl_cd(){
    int i;
    for( i=0; i<256; ++i )
        xl[i]=i;
}

void 
initxl_ci(){
    initxl_cd();
    int i;
    for( i='a'; i<='z'; ++i )
        xl[i]='A'+(i-'a');
}
