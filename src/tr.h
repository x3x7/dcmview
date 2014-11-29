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
#ifndef _TR_H_
#define _TR_H_

#ifndef NDEBUG
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

typedef struct {
    uint64_t _t0;
    uint32_t _khz;
    int _fd;
    int _on;
} TR;


static inline uint64_t rdtsc( void ) {
    uint32_t l, h;
//    return 0;
    asm volatile (
     "cpuid\n rdtsc"
//        "rdtscp" 
        : "=a"(l), "=d"(h)         /* outputs */
        : "a"(0)                   /* inputs */
        : "%ebx", "%ecx");         /* clobbers*/
    return ((uint64_t)l) | (((uint64_t)h) << 32);
}

static inline uint32_t tr_td( TR *t) { 
    uint32_t v=(rdtsc()-t->_t0)/t->_khz; 
    return v;
}

static inline void tr_init( TR *t, int fd, uint32_t mhz ) {
    t->_khz=mhz*1000;
    t->_fd=fd;
    t->_t0=rdtsc();
    t->_on=1;
}

static inline void tr_t0( TR *t ){
        t->_t0=rdtsc();       
}

static void tr_msg( TR *t, int ext, const char *file, int line, const char *fun, const char *fmt, ... ) __attribute__ ((__format__ (__printf__, 5, 0)));

static inline void tr_msg( TR *t, int ext, const char *file, int line, const char *fun, const char *fmt, ... ) {
    if( t->_on ){
        char buf[4*4096], *bufp=buf;
        bufp+=snprintf( bufp, sizeof(buf)-1,
                        "[%c%5u %s:%d %s> ",
                        ext?'!':'-', tr_td(t), file, line, fun );
        va_list ap;
        va_start( ap, fmt );
        bufp+=vsnprintf( bufp, sizeof(buf)-(bufp-buf)-1, fmt, ap );
        va_end( ap );
        bufp[0]='\n';
        write( t->_fd, buf, bufp-buf+1 );// fsync( t->_fd );
    }
//    if( ext ) abort();
    if( ext ) exit(-1);
}

extern TR _tr;



#define TR_INIT( fd, mhz ) __attribute__ ((__constructor__)) \
static void tr_setup(void) { tr_init(&_tr, fd, mhz);}\
TR _tr
#define T0() tr_t0(&_tr)

#define Toff() do{_tr._on=0;}while(0)
#define Ton() do{_tr._on=1;}while(0)

#ifndef __FUNCTION__
#define __FUNCTION__    __PRETTY_FUNCTION__
#endif
#define T( ... ) tr_msg( &_tr, 0, __FILE__,__LINE__,__FUNCTION__, __VA_ARGS__ )
#define E( ... ) tr_msg( &_tr, 1, __FILE__,__LINE__,__FUNCTION__, __VA_ARGS__ )

#else

#define T( ... ) 
#define E( ... ) exit(-1)

#endif

#endif
