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
#include <QOpenGLFunctions_3_1>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLPaintDevice>
#include <QOpenGLShaderProgram>
#include <QWindow>
#include <QWidget>
#include <QSurfaceFormat>
#include <QMatrix4x4>
#include <QShowEvent>
#include <cmath>
#include "tr.h"
#include "dicom.h"
#include <limits>

class mimx {                    // min/max stats
    float _min;
    float _max;
public:
    mimx( float val0 ) : _min(val0), _max(val0) {}
    mimx( ){ clear(); }
    void add( float val ){
        if( _min > val ) _min=val;
        if( _max < val ) _max=val;
    }
    void clear( float val ){ 
        _min=val;
        _max=val;
    }
    void clear( ){
        _min=std::numeric_limits<float>::max();
        _max=-std::numeric_limits<float>::max();
    }
    float min() const { return _min; }
    float max() const { return _max; }
};

class GLWindow : public QWindow, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT        // must include this if you use Qt signals/slots
    QWidget *_parent;
    int _w, _h;
    QOpenGLShaderProgram *_prog;
    QOpenGLShaderProgram *_prog1;
    QOpenGLContext *_ctxt;
    QOpenGLPaintDevice *_dev;
    bool _pending;
    int _first;
    float _ang0, _ang1, _ang2;
    float _zs, _dz;
    static const float deg2rad=M_PI/180.0f;
    static const float eps=1.0e-5; // zero range
    
    float *_roid;                // data values (seq of 3d vectors)
    u32_t *_roii;                // indexes
    u32_t _roisz;                // number of slices
    int32_t _a;
    int32_t _sf;                // scale factor;

    void trafo( float v4[], float t[] );
    void rot( float ang, float u, float v, float w, float r[] );

    QPoint pos0(){
        if( _parent ){
            QRect pfg=_parent->frameGeometry();
            QRect pg=_parent->geometry();
            QRect fg=frameGeometry();

            int x=pg.x()+pfg.width();
            int y=pg.y()+pfg.height();
            if( fg.height()==0 )
                y-=minimumHeight()+pfg.height()-pg.height();
            else
                y-=fg.height();
            //T( "p0 %d %d", x, y );
            return QPoint( x, y );
        }
        else
            return position();
    }

signals:
    void toggled( bool val );   // to notify callers of state change
public:
    GLWindow(QWidget *parent=0) : QWindow(), _parent(parent), 
                                  _dev(NULL), _pending(false), _first(0) {
        setSurfaceType(QWindow::OpenGLSurface);
        //setGeometry(QRect(10, 10, 640, 480));
        setMinimumHeight(480);
        setMinimumWidth(640);
        _ang0=0;
        _ang1=-45;
        _ang2=0;
        _a=-1;
        _sf=1;
        _dz=0;
        QSurfaceFormat fmt;
        fmt.setSamples(4);    // Set the number of samples used for multisampling
        setFormat(fmt);
        create();
        initializeGL(fmt);
    }
    void set_roi( const roi_seq_t *rs );
    void set_slice( int n ){
        if( n>=0 && n<=int(_roisz) ){
            _a=n;
            renderLater();
        } else {
            if( _a!=-1 ){
                _a=-1;
                renderLater();
            }
        }
    }

    virtual void render();
    virtual void initialize();
                             
public slots:
    void renderLater();
    void renderNow();
    void parent_pos( ){
        QPoint p0=pos0();
        if( p0!=position() )
            setPosition( pos0() );
    }
    
protected:
    void exposeEvent( QExposeEvent *event );

    void showEvent( QShowEvent * ev ){
        if( _first==0 ) setPosition( pos0() );
        QWindow::showEvent( ev );
        ev->accept();
    }
    void moveEvent( QMoveEvent *ev ){
        if( _first<3 ){         // try a few moves
            QPoint p0=pos0();
            //T(" %d %d [%d]", position().x(), position().y(), _first );
            if( position()!=p0 )     // try a few times to hit the ideal position
                setPosition( p0 );
            else
                _first=3;
            ++_first;
        }
        ev->accept();
    }

    bool event( QEvent *event );
    void keyPressEvent(QKeyEvent *ev );

    void initializeGL(const QSurfaceFormat &fmt);

};
