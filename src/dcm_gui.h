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
#ifndef _DCM_GUI_H_
#define _DCM_GUI_H_
#include <QtCore/QObject>
#include <QWidget>
#include <QDockWidget>
#include <QDialog>
#include <QWheelEvent>
#include <QCloseEvent>
#include <QShortcut>
#include <QImage>
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <QWebView>

#include <stdint.h>
#include "ui_dcm.h"
#include "ui_hist.h"
#include "glw.h"
#include "dicom.h"
#include "img.h"

class HWidget : public QWidget {
    Q_OBJECT
    int _first;
    QPoint pos0(){
        QWidget *par=parentWidget();
        if( par ){
            QRect pfg=par->frameGeometry(), pg=par->geometry();
            QRect fg=frameGeometry(), g=geometry();
            int x,y;
            x=pfg.x()-fg.width();
            y=pfg.y()+pfg.height()-fg.height();
            // de we have a frame?
            if( fg.height()==g.height() ){
                //T( "no frame" );
                // apply corrections, asuming a similar frame is added
                x+=2*(pfg.x()-pg.x());
                y+=(pfg.y()-pg.y())+pfg.x()-pg.x();
                // use parent's frame sizes
            }
            //T( "p0 %d %d", x, y );
            return QPoint( x, y );
        }
        return pos();
    }

signals:
    void toggled( bool val );   // to notify callers of state change

public slots:
    void parent_pos( ){
        QPoint p0=pos0();
        if( p0!=pos() )  move( p0 );
    }

public:
    HWidget(QWidget *parent=0): QWidget( parent, Qt::Window ), _first(0) { }
    void closeEvent( QCloseEvent * event ){
        event->accept();
        emit toggled( false ); 
    }

protected:
    bool event( QEvent *ev ){
        if(ev->type() == QEvent::LayoutRequest) {
            setFixedSize(sizeHint());
        }
        return QWidget::event(ev);
    }
    void showEvent( QShowEvent * ev ){
        //QSize sh=sizeHint();
        //T(" %d %d %dx%d", pos().x(), pos().y(), sh.width(), sh.height() );
        if( _first==0 ) move( pos0() );
        QWidget::showEvent( ev );
        ev->accept();
    }
    void moveEvent( QMoveEvent *ev ){
        if( _first<3 ){         // try a few moves
            QPoint p0=pos0();
            //T(" %d %d [%d]", pos().x(), pos().y(), _first );
            if( pos()!=p0 )     // try a few times to hit the ideal position
                move( p0 );
            else
                _first=3;
            ++_first;
        }
        ev->accept();
    }
};

class dcm_gui : public QMainWindow
{
    Q_OBJECT

public:
    dcm_gui(int argc, char *argv[], QWidget *parent=NULL);
    ~dcm_gui();
protected:
    void closeEvent( QCloseEvent * ev ){
        if( _3dw != NULL ){
            delete _3dw;
            _3dw=NULL;
        }
        if( _hw!=NULL ){
            delete _hw;
            _hw=NULL;
        }
        if( _helpw!=NULL ){
            delete _helpw;
            _helpw=NULL;
        }
        ev->accept();
    }
    void wheelEvent( QWheelEvent *ev );

    void mousePressEvent( QMouseEvent *ev );

    void keyPressEvent( QKeyEvent *ev );
    bool event( QEvent *ev ){
        if(ev->type() == QEvent::LayoutRequest) {
            setFixedSize(sizeHint());
        }
        return QMainWindow::event(ev);
    }

    bool eventFilter( QObject *obj, QEvent *ev );

public slots:
    void step(bool fwd, int step=1);
    void x2(bool is_x2 );
    void next_pic( int step=1 );
    void prev_pic( int step=1 );

signals:
    void toggleOK();
    void movedBy( QPoint dta );

private slots:
    // display 'n'th image
    void show_pic( int n, bool force=false );
    void refresh_pic( );

    void move_cursor( int pos );

    void show_cursor( bool show );

    void show_hist( bool show );

    void show_roi( bool show );

    void show_3d( bool show );
    void set_bound( int val );  // _dmin or _dmax

private:
    inline 
    bool is_x2() const {
        return _ui.bx2->isChecked();
    }
    void refresh_cursors();
    void position_cursors();
    void show_coords();

    void init_imgs( );          // setup ui after dcm's loaded
    void load_dcm( const char *fname );
    void load_dir( const char *dname );
    void load_files( int argc, const char *argv[] );
    void dump_bmp( const char *dname );

    void reset_stats(){ 
        _min=1<<16; _max=0;
        if( _hg != NULL ){ 
            free( _hg );
            _hg=NULL;
        }
    }
    static const int NSQ=1024;

    u32_t *_hg;
    dcmf *_sq[NSQ];             // image sequence
    dcmf *_an;                  // annotations file
    roi_seq_t *_roi;
    strh *_i2r;                 // map of images -> roi

    // Zooming data
    int _x, _y;                 // image size
    int _x0, _y0;               // start point (in original image)
    int _xc, _yc;               // cursor point (in original image)
    int _zf;                    // zoom factor
    
    img _img;                   // original active image
    img _imgz;                  // zoomed image
    img _imgz2;                 // zoomed image, x2

    QRgb *_rgb;                 // RGB pixels
    QRgb *_rgb_map;             // RGB colormap
    int _gray0;                 // blak level
    int _gray1;                 // white level
    void update_rgb_map();

    QImage *_im;                // used as temporary buffer only    
    QImage *_im2x;              // used as temporary buffer only
    QRgb   *_rgb_col;           // color data for image below
    QImage *_imcol;             // color scale image
    QPixmap _pxmcol;            // color pixmap
    QGraphicsPixmapItem *_pxmicol;
    int _hpxsz;                 // histogram x-size, in pizels
    int _scol;                  // decimation shift (_max-_min+1)>>_scol is y-size

    i32_t  _min, _max;          // absolute min/max gray levels in all series
    i32_t _dmin,_dmax;          // active display  min max
    void hist( int n, u32_t *hg ); // image 'n' in sequence
    void hist();                   // all

    static const int _bord=16;  // extra pixels for graphics view
    static const int _zf_max=10;
    static const int _zf_min=2;
    int _cidx;          // current cursor index ( ==_zf-2 if in sync)
    int _cidx2;         // current 2x cursor index ( ==_zf-2 if in sync)

    QGraphicsPathItem *_proi;   // ROI path
    QPixmap _pxm;
    QGraphicsPixmapItem *_pxmi;
    QGraphicsPixmapItem *_crsri;
    QGraphicsScene _imgs;       // images

    QGraphicsPathItem *_proi2;  // ROI path x2
    QPixmap _pxm2;
    QGraphicsPixmapItem *_crsri2;
    QGraphicsPixmapItem *_pxmi2;
    QGraphicsScene _imgs2;      // images x2

    QGraphicsScene _hgs;        // histogram

    int _csq;                   // current shown
    int _nsq;                   // loaded images from sequence
    int _ssq;                   // start idx
    int _esq;                   // end idx
    int _sn;                    // sequence number

    QPainterPath *roi_path( const dcmf *dcm, const roi_elm_t *e, int zf=2 );

    void scaleUI();             // compensate screen dpi variations
    Ui::gui _ui;                // main UI
    HWidget *_hw;               // histogram view
    Ui::hist _hui;              // histogram ui
    GLWindow *_3dw;             // 3D view
    QWebView *_helpw;           // Help view
};


#endif
