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
#include <QPainter>
#include <QCoreApplication>
#include "glw.h"

void 
GLWindow::set_roi( const roi_seq_t *rs ){
    // count number of data points
    if( rs!=NULL && rs->ne ){
        // compute data offsets
        u32_t i, ttl;
        _roisz=rs->ne;
        _roii=new u32_t[rs->ne+1];
        for( i=ttl=0; i<rs->ne; ++i ){
            _roii[i]=ttl;
            ttl+=rs->e[i].ncp;
        }
        _roii[i]=ttl;

        _roid = new float[3*ttl];
        // store the data
        for( i=0; i<rs->ne; ++i ){
            const str_t *s=&rs->e[i].dta;
            int ncp=3*rs->e[i].ncp;
            //printf( "%d %.*s\n", ncp, s->len, s->str );
            dcmf::get_flt( s->str, s->str+s->len, _roid+3*_roii[i], ncp );
            if( ncp != int(3*rs->e[i].ncp) ){
                T( "%d!=%d", ncp, rs->e[i].ncp );
            }
        }
        // get bounds
        mimx sx( _roid[0] ), sy( _roid[1] ), sz( _roid[2] );
        for( i=3; i<3*ttl; i+=3 ){
            sx.add( _roid[i+0] );
            sy.add( _roid[i+1] );
            sz.add( _roid[i+2] );
        }
#if(0)
        T( "[%.2f %.2f] [%.2f %.2f] [%.2f %.2f]",
           sx.min(), sx.max(),
           sy.min(), sy.max(),
           sz.min(), sz.max() );
        
        T( "[%.2f %.2f %.2f]",
           sx.max() - sx.min(),
           sy.max() - sy.min(),
           sz.max() - sz.min() );
#endif
        // center and scale
        float xd, xs, yd, ys, zd, zs;
        xd=0.5f*(sx.min()+sx.max());
        xs=sx.max() - sx.min();
        if( xs > eps ) xs=1.9f/xs;
        yd=0.5f*(sy.min()+sy.max());
        ys=sy.max() - sy.min();
        if( ys > eps ) ys=1.9f/ys;
        zd=0.5f*(sz.min()+sz.max());
        zs=sz.max() - sz.min();
        if( zs > eps ) zs=1.9f/zs;
#if(0)        
        T( "d= [%.3f %.3f %.3f]", xd, yd, zd );
        T( "s= [%.3f %.3f %.3f]", xs, ys, zs );
#endif
        // pick smallest scale
        float mins=zs;
        
        if( mins > xs ) mins=xs;
        if( mins > ys ) mins=ys;

        xs=ys=zs=mins;
        _zs=zs;

        for( i=0; i<3*ttl; i+=3 ){
            _roid[i+0]=(_roid[i+0]-xd)*xs;
            _roid[i+1]=(_roid[i+1]-yd)*ys;
            _roid[i+2]=(_roid[i+2]-zd)*zs;
        }
        // compute the new bounds
        sx.clear(_roid[0]); sy.clear(_roid[1]); sz.clear(_roid[2]);
        for( i=3; i<3*ttl; i+=3 ){
            sx.add( _roid[i+0] );
            sy.add( _roid[i+1] );
            sz.add( _roid[i+2] );
        }
        T( "[%.2f %.2f] [%.2f %.2f] [%.2f %.2f]",
           sx.min(), sx.max(),
           sy.min(), sy.max(),
           sz.min(), sz.max() );
    }
}

void 
GLWindow::trafo( float v4[], float t[] ){
    float r4[4];
    for( int p=0; p<4; ++p ){
        int o=p<<2;
        r4[p]=v4[o]*t[o]+v4[o+1]*t[o+1]+v4[o+2]*t[o+2]+v4[o+3]*t[o+3];
    }
    memcpy( v4, r4, sizeof( r4 ) );
}

void 
GLWindow::rot( float ang, float u, float v, float w, float r[] ){
    float c=cosf( ang*deg2rad ), s=sinf( ang*deg2rad );
    r[0]=u*u+(1-u*u)*c;  r[1]=u*v*(1-c)-w*s; r[2]=u*w*(1-c)+v*s; r[3]=0;
    r[4]=u*v*(1-c)+w*s; r[4]=v*v+(1-v*v)*c;  r[5]=v*w*(1-c)-u*s; r[7]=0;
    r[8]=u*w*(1-c)-v*s; r[7]=v*w*(1-c)+u*s; r[8]=w*w+(1-w*w)*c;  r[11]=0;
    r[12]=0; r[13]=0; r[14]=0; r[15]=0;
}

void 
GLWindow::render(){
    if( _sf==0 ) {
        _sf=1;
        return;
    }
    if( height()==0 )
        return;
#if(0)
    float c0=cosf( _ang0*deg2rad ), s0=sinf( _ang0*deg2rad );
    float c1=cosf( _ang1*deg2rad ), s1=sinf( _ang1*deg2rad );
    float c2=cosf( _ang2*deg2rad ), s2=sinf( _ang2*deg2rad );
    float u=1,v=0,w=0;

    float r0v[]={               // around x
        1,  0,  0, 0,
        0, c0,-s0, 0,
        0, s0, c0, 0,
        0,  0,  0, 1
    };

    float r1v[]={               // around y
       c1,  0,-s1, 0,
        0,  1,  0, 0,
       s1,  0, c1, 0,
        0,  0,  0, 1
    };

    float r2v[]={               // around z
        c2,-s2, 0, 0,
        s2, c2, 0, 0,
         0,  0, 1, 0,
         0,  0, 0, 1 
    };

    float ruvw[]={
        u*u+(1-u*u)*c1, u*v*(1-c1)-w*s1, u*w*(1-c1)+v*s1, 0,
        u*v*(1-c1)+w*s1, v*v+(1-v*v)*c1, v*w*(1-c1)-u*s1, 0,
        u*w*(1-c1)-v*s1, v*w*(1-c1)+u*s1, w*w+(1-w*w)*c1, 0,
        0, 0, 0, 1
    };
#endif

    QMatrix4x4 tr;
    if( _ang1!=0 )
        tr.rotate( _ang1, 0, 1, 0 ); // rotate on y
    if( _ang2!=0 )
        tr.rotate( _ang2, 0, 0, 1 ); // rotate on orig z axis
    if( _ang0!=0 )
        tr.rotate( _ang0, 1, 0, 0 ); // rotate on orig x axis
    
    if( _dz )
        tr.translate( 0, 0, _dz*_zs ); // translate on z

    if( _sf > 1 )
        tr.scale(powf( 1.01f, _sf ));

    
    //tr.translate( 0, 0, 1.0f ); // translate on z
    //tr.perspective( 30.0f, float(width())/float(height()), -1.0f, 3.0f );
    //T("slice %d dz %f zs %f _sf %f (%dx%d)", _a, _dz, _zs, _sf, width(),height());

    glViewport(0, 0, (GLint)width(), (GLint)height());
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    _prog->bind();
    _prog->enableAttributeArray(0);
    _prog->setAttributeArray(0, GL_FLOAT, _roid, 3);

    _prog->setUniformValue("trafo", tr);
    _prog->setUniformValue("lpos", 0.8f, 0.8f, 0);
    _prog->setUniformValue("color", 1, 1, 0);
    _prog->setUniformValue("t", (float) 0.2f);
    // draw all slices
    int32_t i;
    for( i=0; i<int32_t(_roisz); ++i )        
        glDrawArrays(GL_LINE_LOOP, _roii[i], _roii[i+1]-_roii[i]);

    if( _a >=0 && _a<int(_roisz) ){
        // fill the active one
        _prog->setUniformValue("color", 0, 1, 0);
        glFrontFace(GL_CW);
        glDrawArrays(GL_TRIANGLE_FAN, _roii[_a], _roii[_a+1]-_roii[_a]);
        glFrontFace(GL_CCW);
        glDrawArrays(GL_TRIANGLE_FAN, _roii[_a], _roii[_a+1]-_roii[_a]);
    }
    _prog->disableAttributeArray(0);


    _prog->release();
}

void 
GLWindow::renderNow(){
    if( isVisible() ){
        //T("");
        _ctxt->makeCurrent( this );
        render();
        _ctxt->swapBuffers( this );
        _ctxt->doneCurrent();
    }
}

void
GLWindow::initialize(){
}

bool
GLWindow::event( QEvent *event ){

    switch (event->type()) {
        case QEvent::UpdateRequest:
            _pending=false;
            renderNow();
            return true;
        case QEvent::Close:
            emit toggled( false );
        default:
            return QWindow::event(event);
    }
}

void
GLWindow::exposeEvent( QExposeEvent *event ){

    Q_UNUSED(event);
    if (isExposed())
        renderNow();
}

void
GLWindow::renderLater(){
    if (!_pending) {
        _pending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

const char *fsh_old=
    "uniform lowp float t;"
    "varying highp vec3 coords;"
    "void main() {"
    "    lowp float i = 1. - (pow(abs(coords.x), 4.) + pow(abs(coords.y), 4.));"
    "    i = smoothstep(t - 0.8, t + 0.8, i);"
    "    i = floor(i * 20.) / 20.;"
    "    gl_FragColor = vec4(coords * .5 + .5, i);"
    "}";


const char *vsh=
    "uniform highp mat4 trafo;"
    "attribute highp vec4 vertices;"
    "varying highp vec3 coords;"
    "void main() {"
    "    gl_Position = trafo*vertices;"
    "    coords = gl_Position.xyz;"
    "}";

const char *fsh=
    "uniform lowp float t;"
    "uniform highp vec3 lpos;"
    "uniform highp vec3 color;"
    "varying highp vec3 coords;"
    "void main() {"
    "    highp float d = distance(coords, lpos);"
    "    d = (8-d*d)/8;"
    "    gl_FragColor = vec4(color * d, 1);"
    "}";



void
GLWindow::initializeGL(const QSurfaceFormat &fmt){
    _ctxt = new QOpenGLContext;
    _ctxt->setFormat( fmt );
    _ctxt->create();
    _ctxt->makeCurrent( this );
    initializeOpenGLFunctions();
    T( "%s", glGetString( GL_VERSION ) );
#if(0)
    GLint nExtensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &nExtensions);
    for( int i = 0; i < nExtensions; i++ )
        T("%s", glGetStringi( GL_EXTENSIONS, i ) );
#endif
    // Set up the rendering context, define display lists etc.:
    //glDisable(GL_DEPTH_TEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    //glEnable(GL_LIGHTING);
    //glEnable(GL_LIGHT0);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);

    // Shaders
    _prog = new QOpenGLShaderProgram();
    _prog->addShaderFromSourceCode(QOpenGLShader::Vertex, vsh );
    _prog->addShaderFromSourceCode(QOpenGLShader::Fragment, fsh );
    _prog->bindAttributeLocation("vertices", 0);

    _prog->link();
    _ctxt->doneCurrent();
}

void 
GLWindow::keyPressEvent(QKeyEvent *ev ){
    bool acc=true;
    switch( ev->key() ){
        case Qt::Key_Up:
            if( ev->modifiers() & Qt::ShiftModifier )
                _ang0+=2;   // x-axis
            else
                _ang2+=2;
            break;
        case Qt::Key_Down:
            if( ev->modifiers() & Qt::ShiftModifier )
                _ang0-=2;   // x-axis
            else
                _ang2-=2;   // z-axis
            break;
        case Qt::Key_Left:  // y-axis
            if( ev->modifiers() & Qt::ShiftModifier )
                _dz-=1;     // z-shift
            else
                _ang1-=2;
            break;
        case Qt::Key_Right:
            if( ev->modifiers() & Qt::ShiftModifier )
                _dz+=1;     // z-shift
            else
                _ang1+=2;
            break;
        case Qt::Key_Z:
            _sf+=1;
            break;
        case Qt::Key_X:
            _sf-=1;
            break;
        case Qt::Key_Home:
            _ang0=0;
            _ang1=-45;
            _ang2=0;
            _sf=1;
            _dz=0;
            break;
        default:
            acc=false;
            break;
    }
    if( acc ){
        ev->accept();
        renderNow();
    }
}
