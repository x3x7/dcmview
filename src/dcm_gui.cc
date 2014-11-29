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
#include <QtGui>
#include <QDesktopWidget>
#include <QDirIterator>
#include <QFileInfo>
#include <QPainterPath>
#include <QGraphicsPathItem>
#include <QGraphicsSceneMouseEvent>
#include <getopt.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include "dcm_gui.h"
#include "bmp.h"
#include "tr.h"

#include "dcm_gui_xpm.cc"

static inline void
rscale( QRect &r, int dpi, int xdpi, int ydpi ){
    int x,y,w,h;
    x=r.x();
    y=r.y();
    w=r.width();
    h=r.height();

    r.setX( (x*xdpi)/dpi );
    r.setWidth( (w*xdpi)/dpi );
    r.setY( (y*ydpi)/dpi );
    r.setHeight( (h*ydpi)/dpi );    
}


void
dcm_gui::scaleUI(){
    QDesktopWidget *desk = QApplication::desktop();
    const int dpi=101;         // dpi the UI was designed at
    int 
        xdpi=desk->physicalDpiX(),
        ydpi=desk->physicalDpiY();
    T( "X: %ddpi %dmm %dpx", xdpi, desk->widthMM(), desk->width() );
    T( "Y: %ddpi %dmm %dpx", ydpi, desk->heightMM(), desk->height() );
    if( xdpi != dpi || ydpi != dpi ){
        QRect r1, r2;
        //this->resize( (560*xdpi)/dpi, (950*ydpi)/dpi );
    }
}


dcm_gui::dcm_gui( int argc, char *argv[], QWidget *parent ) 
    : QMainWindow(parent), _hg(NULL), _an(NULL), _i2r(NULL), _im(NULL),_imgs(parent), _nsq(0) {
    img::rgb_init();
    _x=_y=0;
    _x0=_y0=0;
    _xc=_yc=0;
    _zf=2;
    _cidx=-1;
    _cidx2=-1;

    _sn=-1;
    _ssq=NSQ;
    _esq=0;

    _proi=NULL;
    _proi2=NULL;

    int i;
    for( i=0; i<NSQ; ++i )
        _sq[i]=NULL;

    _ui.setupUi(this);
    _hw=new HWidget(this);
    _hui.setupUi(_hw);
    setWindowTitle( QString("dcmView:\t2-D") );
    _hw->setWindowTitle( QString("dcmView:\tHistogram") );
    _3dw=new GLWindow(this);
    _3dw->setTitle( QString("dcmView:\t3-D") );
    _ui.gv->setScene( &_imgs );
    _hui.hv->setScene( &_hgs );
    _hui.hv->scale( 1, -1 );

    _pxmi=NULL;
    _pxmi2=NULL;
    _pxmicol=NULL;
    _crsri=NULL;
    _crsri2=NULL;
    _rgb=NULL;
    _rgb_map=NULL;
    _gray0=8;
    _gray1=252;
    _im=NULL;
    _im2x=NULL;
    _imcol=NULL;
    _hpxsz=128+64;

    scaleUI();
    reset_stats();

    _ui.l1->setText("");
    _ui.lxyz->setText("");
    QPixmap *crsr=new QPixmap( xpm_crsr[0] );    
    QIcon ButtonIcon(crsr[0]);
    _ui.bcrsr->setIcon(ButtonIcon);
    _ui.bcrsr->setIconSize(crsr->rect().size());
    delete crsr;
    //button->setFixedSize(pixmap.rect().size());
    _helpw=new QWebView( );
    _helpw->setUrl( QUrl("qrc:/help.html") );
    _helpw->setWindowTitle( QString( "dcmView:\tHelp" ) );
    connect( _ui.s, SIGNAL(valueChanged(int)),
             this, SLOT(show_pic(int)), Qt::DirectConnection );
    connect( _ui.sbn, SIGNAL(valueChanged(int)),
             this, SLOT(show_pic(int)), Qt::DirectConnection );
    connect( _ui.sbz, SIGNAL(valueChanged(double)),
             this, SLOT(refresh_pic(void)), Qt::DirectConnection );
    connect( _ui.bh, SIGNAL(toggled(bool)),
             this, SLOT(show_hist(bool)), Qt::DirectConnection );
    connect( _ui.b3D, SIGNAL(toggled(bool)),
             this, SLOT(show_3d(bool)), Qt::DirectConnection );
    connect( _ui.bROI, SIGNAL(toggled(bool)),
             this, SLOT(show_roi(bool)), Qt::DirectConnection );
    connect( _ui.bx2, SIGNAL(toggled(bool)),
             this, SLOT(x2(bool)), Qt::DirectConnection );
    connect( _ui.bcrsr, SIGNAL(toggled(bool)),
             this, SLOT(show_cursor(bool)), Qt::DirectConnection );

    connect( _ui.sbx, SIGNAL(valueChanged(int)),
             this, SLOT(move_cursor(int)), Qt::DirectConnection );
    connect( _ui.sby, SIGNAL(valueChanged(int)),
             this, SLOT(move_cursor(int)), Qt::DirectConnection );

    // hist win controls
    connect( _hw, SIGNAL(toggled(bool)),
             _ui.bh, SLOT(setChecked(bool)), Qt::DirectConnection );

    // _dmin & _dmax bounds
    connect( _hui.sbm, SIGNAL(valueChanged(int)),
             this, SLOT(set_bound(int)), Qt::DirectConnection );
    connect( _hui.sbM, SIGNAL(valueChanged(int)),
             this, SLOT(set_bound(int)), Qt::DirectConnection );
    // gray levels
    connect( _hui.srgb0, SIGNAL(valueChanged(int)),
             this, SLOT(set_bound(int)), Qt::DirectConnection );
    connect( _hui.srgb1, SIGNAL(valueChanged(int)),
             this, SLOT(set_bound(int)), Qt::DirectConnection );
    // 3d win controls
    connect( _3dw, SIGNAL(toggled(bool)),
             _ui.b3D, SLOT(setChecked(bool)), Qt::DirectConnection );

    _ui.gv->installEventFilter(this);
    _ui.sbx->installEventFilter(this);
    _ui.sby->installEventFilter(this);
    _ui.sbz->installEventFilter(this);
    _ui.sbn->installEventFilter(this);
    _helpw->installEventFilter(this);
    _hw->installEventFilter(this);
    _3dw->installEventFilter(this);
    _hui.sbm->installEventFilter(this);
    _hui.sbM->installEventFilter(this);
    _hui.srgb0->installEventFilter(this);
    _hui.srgb1->installEventFilter(this);
    
    int opt;
    while( (opt=getopt(argc, argv, "d:"))!=-1 ){
        switch( opt ){
            case 'd':
                load_dir(optarg);
                break;
            default:
                break;
        }
    }
#if(1)
    while( optind < argc ){
        load_dir(argv[optind]);
        ++optind;
    }
#else
    argv+=optind;
    argc-=optind;
    load_files( argc, (const char**)argv );
#endif
    show();
    //_helpw->show();
}

dcm_gui::~dcm_gui(){
    int i;
    for( i=0; i<NSQ; ++i )
        if( _sq[i]!=NULL ) delete _sq[i];

    if( _pxmi ) delete _pxmi;
    if( _crsri ) delete _crsri;
    if( _pxmi2 ) delete _pxmi2;
    if( _crsri2 ) delete _crsri2;
    if( _pxmicol ) delete _pxmicol;

    if( _hg != NULL )
        free( _hg );
    if( _im != NULL )
        delete _im;
    if( _im2x != NULL )
        delete _im2x;
    if( _imcol != NULL )
        delete _imcol;

    if( _rgb != NULL )
        delete[] _rgb;

    if( _rgb_map != NULL )
        delete[] _rgb_map;
    if( _rgb_col != NULL )
        delete[] _rgb_col;

    if( _an != NULL )
        delete _an;
    if( _i2r != NULL )
        delete _i2r;
    if( _3dw != NULL ){
        delete _3dw;
        _3dw=NULL;
    }
    if( _helpw!=NULL ){
        delete _helpw;
        _helpw=NULL;
    }
}

void
dcm_gui::load_dcm( const char *fname ){
    dcmf *dcm=new dcmf( fname );
    if( !dcm->valid() ){
        T("invalid file %s", fname );
        delete dcm;
        return;
    }
    if( dcm->x() == 0 ){
        _an=dcm;
        _roi=dcm->roi_seq();
        if( _roi ){
            if( _i2r ) delete _i2r;
            _i2r=new strh( 4*_roi->ne+1 );
            unsigned i;
            for( i=0; i<_roi->ne; ++i ){
                _i2r->add( _roi->e[i].iuid, i );
            }
            _3dw->set_roi( _roi );
        }
        return;
    } else {
        if( _x==0 ){
            _x=dcm->x();
            _y=dcm->y();
        } else {
            if( _x!=dcm->x() || _y!=dcm->y() ){
                E("image size diff x:%d!=%d or y:%d!=%d for %s",
                  _x, dcm->x(), _y, dcm->y(), fname );
            }
        }
        if( _sn==-1 ) 
            _sn=dcm->sn();
    }
    int in=dcm->in();
    if( _sn==dcm->sn() && in < NSQ && _sq[in]==NULL ){ 
        _sq[in]=dcm;
        if( _ssq > in )
            _ssq=in;
        if( _esq < in )
            _esq=in;
        ++_nsq;
        dcm->pxstats();
        int min=dcm->pxmin();
        int max=dcm->pxmax();
        if( min < _min ) _min=min;
        if( max > _max ) _max=max;
    } else {
        T( "wrong data s%d[=?%d] i%d[<%d]", dcm->sn(), _sn, dcm->in(), NSQ );
        delete dcm;
    }
}

void
dcm_gui::update_rgb_map() {
    int i;
    const int r0= 32, g0=128,  b0=  0;  // limit values at 0
    const int r1=144, g1= 56,  b1= 56;  // limit values at max

    int sz=_dmin-_min;
    // below _dmin, use min colormap
    for( i=_min; i<_dmin; ++i ){
        int r,g,b, p=(i-_min);
        r=(r0*(sz-p)+_gray0*p)/sz;
        g=(g0*(sz-p)+_gray0*p)/sz;
        b=(b0*(sz-p)+_gray0*p)/sz;
        _rgb_map[i-_min]=qRgb(r,g,b);
    }
    // b&w range
    if( _dmin<_dmax )
        for( i=_dmin; i<=_dmax; ++i ){
            int v=_gray0+((i-_dmin)*(_gray1-_gray0))/(_dmax-_dmin);
            _rgb_map[i-_min]=qRgb( v, v, v );
        }
    else
        if( _dmin==_dmax ){
            int v=(_gray0+_gray1)>>1;
            _rgb_map[_dmin-_min]=qRgb( v, v, v );
        }
    // above _dmax, use max colormap
    sz=_max-_dmax;
    for( i=_dmax+1; i<=_max; ++i ){
        int r,g,b, p=(i-_dmax);
        r=(_gray1*(sz-p)+r1*p)/sz;
        g=(_gray1*(sz-p)+g1*p)/sz;
        b=(_gray1*(sz-p)+b1*p)/sz;        
        _rgb_map[i-_min]=qRgb(r,g,b);
    }
    // update rgb buff
    QRgb *p=_rgb_col;
    for( i=0; i<(_max-_min+1)>>_scol; ++i ){
        int j;
        QRgb rgb=_rgb_map[(i<<_scol)];
        for( j=0; j<_hpxsz; ++j ) p[j]=rgb;
        p+=_hpxsz;
    }
    _pxmcol.convertFromImage( _imcol[0] );
    _pxmicol->setPixmap( _pxmcol );
}

void
dcm_gui::hist( int n, u32_t *hg ){
    dcmf *dcm=_sq[n];
    if( dcm==NULL ) {
        T( "seq gap at %d?!", n );
        return;
    }
    if( dcm->pxr()==0 ){    // unsigned data
        const u16_t *ip=dcm->d16u();
        if( ip!=NULL )
            for( int i=0; i<dcm->x()*dcm->y(); ++i )
                ++hg[ip[i]-_min];
    } else {                // signed data
        const i16_t *ip=dcm->d16s();
        if( ip!=NULL )
            for( int i=0; i<dcm->x()*dcm->y(); ++i )
                ++hg[ip[i]-_min];
    }

}

class DotItem : public QGraphicsItem {
public:
    QRectF boundingRect() const{
        return QRectF( 0, 0, 16, 16 );
    }

    void paint(QPainter *pt, const QStyleOptionGraphicsItem *option,
               QWidget *widget) {
        Q_UNUSED( option );
        Q_UNUSED( widget );
        pt->drawRoundedRect( 0, 0, 16, 16, 2, 2);
        //pt->fillRect( p.x()+3, p.y()+4, 8, 8, Qt::blue );        
    }
#if(0)
    void mousePressEvent(QGraphicsSceneMouseEvent *event){
        QGraphicsItem::mousePressEvent(event);
        update();
        event->accept();
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event){
        QGraphicsItem::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event){
        QGraphicsItem::mouseReleaseEvent(event);
        update();
    }
#endif
};

void
dcm_gui::hist(){
    if( _max >= _min ){
        if( _hg!=NULL ) free( _hg );
        _hg=(u32_t*)calloc( _max - _min + 1, sizeof( _hg[0] ) );
    } else
        return;
    for( int n=_ssq; n<=_esq; ++n )
        hist( n, _hg );

    int i;
    u32_t hmax=0;
    // ignore 1st bin ('0' level )
    for( i=1; i<=_max-_min; ++i ){
        if( _hg[i] > hmax ) hmax=_hg[i];
    }
    if( hmax==0 ) ++hmax;
    _scol=0;
    while( ( (_max-_min+1)>>_scol ) > 512 )
        ++_scol;

    QPainterPath *pp=new QPainterPath();
    pp->setFillRule( Qt::WindingFill );
    pp->moveTo(  0, _min>>_scol );
    for( i=1; i<=(_max-_min); i+=4 ){
        pp->lineTo( (_hg[i]*_hpxsz)/hmax, (i+_min)>>_scol );
    }
    pp->closeSubpath();
    
    QGraphicsPathItem *pi=new QGraphicsPathItem( pp[0] );
    pi->setPen( QPen(QColor(79, 106, 25), 1, Qt::SolidLine,
                     Qt::FlatCap, Qt::MiterJoin) );
    pi->setBrush(QColor(122, 163, 39));

    _hgs.setSceneRect( 0, _min>>_scol, _hpxsz, (_max-_min+1)>>_scol );
    // create background color image
    _rgb_col=new QRgb[ ((_max-_min+1)*_hpxsz)>>_scol ];
    _imcol=new QImage( (uchar*)_rgb_col, _hpxsz, (_max-_min+1)>>_scol, QImage::Format_RGB32 );
    _pxmcol=QPixmap::fromImage( _imcol[0] );
    _pxmicol=new QGraphicsPixmapItem( _pxmcol );
    _pxmicol->setY( _min>>_scol );
    _hgs.addItem( _pxmicol );
    _hgs.addItem( pi );    
    // set display limits
    u32_t wttl, wmax;
    _dmin=_min; wttl=0;
    if( _hg[1]== 0 ){           // we likely have artificial "black-level"
        wmax=(_x*_y*_nsq)-_hg[0];
        for( i=2; i<=_max-_min; ++i ){
            wttl+=_hg[i];
            if( wttl*1000 > 7*wmax ){
                _dmin=_min+i;
                break;
            }
        }
    }
    wmax=(_x*_y*_nsq);
    _dmax=_max; wttl=0;
    for( i=_max-_min; i>0; --i ){
        wttl+=_hg[i];
        if( wttl*1000 > 4*wmax ){
            _dmax=_min+i;
            break;
        }
    }
    if( _rgb_map!=NULL ) delete[] _rgb_map;
    _rgb_map=new QRgb[_max-_min+1];
    T( "rgb[%d]", _max-_min+1 );
#if(0)
    DotItem *dot=new DotItem( );
    dot->setFlag( QGraphicsItem::ItemIsMovable, true );
    dot->setPos( _dmax>>3, _hpxsz );
    _hgs.addItem( dot );
#endif

    _hui.sbm->setMinimum( _min );
    _hui.sbm->setMaximum( _max );
    _hui.sbm->setValue( _dmin );

    _hui.sbM->setMinimum( _min );
    _hui.sbM->setMaximum( _max );
    _hui.sbM->setValue( _dmax );
    QString str;
    str.sprintf( "%4d", _min );
    _hui.lmin->setText( str );
    str.sprintf( "%4d", _max );
    _hui.lmax->setText( str );
}
void
dcm_gui::init_imgs(){
    if( _ssq < _esq ){          // data found
        QString str;
        _csq=0;

        hist();
        _x=_sq[_ssq]->x();
        _y=_sq[_ssq]->y();
        _x0=0;
        _y0=0;
        _ui.sbx->setMaximum( _x-1 );
        _ui.sby->setMaximum( _y-1 );

        T( "resize for %dx%d", _x, _y  );
        //_ui.gv->setBackgroundBrush(QBrush(Qt::red, Qt::SolidPattern));
        //_ui.gv->setStyleSheet( "QGraphicsView { border-style: none; }" ); 
        _ui.gv->setMinimumSize( _bord+_x, _bord+_y );
        _ui.gv->setMaximumSize( _bord+_x, _bord+_y );
        //_ui.gv->updateGeometry();
        // init graphic state
        _imgs.setSceneRect( 0, 0, _x, _y );
        _imgs2.setSceneRect( 0, 0, _x<<1, _y<<1 );
        
        // pixel buffers
        _img.create( _x, _y );
        T( "_img created" );
        _imgz2.create( _x<<1, _y<<1 );
        _imgz.create( _x, _y, _imgz.px() ); // share pixels with 2x
        
        // rgb and images
        _rgb=new QRgb[ (_x*_y)<<2 ];
        _im=new QImage( (const uchar *)_rgb, _x, _y, QImage::Format_RGB32 );
        _im2x=new QImage( (const uchar *)_rgb, _x<<1, _y<<1, QImage::Format_RGB32 );
        
        _ui.s->setMinimum( _ssq );
        _ui.s->setMaximum( _esq );        

        _ui.sbn->setMinimum( _ssq );
        _ui.sbn->setMaximum( _esq );
        
        show_pic( _ssq );
    }
}

void
dcm_gui::load_dir( const char *dname ){
    QDirIterator idir(dname);//QDirIterator::Subdirectories
    while (idir.hasNext()) {
        idir.next();
        QFileInfo fi(idir.filePath());
        if ( fi.isFile()
             && (fi.suffix() == "dcm") ){
            load_dcm(fi.filePath().toUtf8().constData());
        }
    }
    init_imgs();
    //dump_bmp( "/tmp/aseg" );
}

void
dcm_gui::load_files( int argc, const char *argv[] ){
    int i;
    for( i=0; i<argc; ++i )
        load_dcm(argv[i]);

    init_imgs();
}

void
dcm_gui::dump_bmp( const char *dname ){
    if( _nsq==0 ) return;
    // fill up structs
    const int aoff=2;           // alignment offset
    bmp_header_t bh;
    bmp_dib3_t bd;
    bh.magic[0]='B';bh.magic[1]='M';
    bh.rsvd[0]=bh.rsvd[1]=0;
    bh.offset=sizeof(bh)+sizeof(bd)+256*sizeof(bmp_rgba_t)+aoff;

    bd.hsize=sizeof(bd);
    bd.width=_sq[_ssq]->x();
    bd.height=_sq[_ssq]->y();
    bd.nplanes=1;
    bd.depth=8;
    bd.compress=BMP_RGB;
    bd.dsize=bd.width*bd.height;
    bd.tcolors=256; bd.icolors=0;
    bd.wres=bd.hres=3937;       // 100dpi (use proper DICOM data?)

    bmp_rgba_t bc[256];
    int n;
    for( n=0;n<256;++n){
        bc[n].r=n;
        bc[n].g=n;
        bc[n].b=n;
        bc[n].a=0;
    }

    bh.fsize=bh.offset+bd.dsize;
    T( "data offset %d fsize %d", bh.offset, bh.fsize );
    uint8_t *px=(uint8_t*)malloc( bd.width*bd.height );
    for( n=_ssq; n<=_esq; ++n ){
        char fnm[256];
        sprintf( fnm, "%s/%d_%03d.bmp", dname, _sn, n );
        int fd=open( fnm, O_CREAT|O_TRUNC|O_WRONLY, 0644 );
        if( fd==-1 ){ T( "err for %s", fnm );
            return;
        }
        write( fd, &bh, sizeof(bh) );
        write( fd, &bd, sizeof(bd) );
        write( fd, bc, sizeof(bc) );
        i32_t min, max;
        min=_dmin;
        max=_dmax;
        if( max==min ) max=min+1;
        dcmf *dcm=_sq[n];
        // FIXME: pad lines to multiple of 4-bytes
        if( dcm->pxr()==0 ){
            const u16_t *ip=dcm->d16u();
            int i,j;
            for( j=0; j<dcm->y(); ++j ){
                const u16_t *il=ip+j*dcm->x(); // scanline
                uint8_t *pl=px+(dcm->y()-j-1)*dcm->x();
                for( i=0; i<dcm->x(); ++i ){
                    u16_t iv=il[i];
                    int v;
                    if( iv <= min ) v=0;
                    else
                        if( iv >= max ) v=255;
                        else
                            v=((iv-min)*255)/(max-min);
                    pl[i]=(uint8_t)v;
                }
            }
        } else {
            const i16_t *ip=dcm->d16s();
            int i,j;
            for( j=0; j<dcm->y(); ++j ){
                const i16_t *il=ip+j*dcm->x(); // scanline
                uint8_t *pl=px+(dcm->y()-j-1)*dcm->x();
                for( i=0; i<dcm->x(); ++i ){
                    i16_t iv=il[i];
                    int v;
                    if( iv <= min ) v=0;
                    else
                        if( iv >= max ) v=255;
                        else
                            v=((iv-min)*255)/(max-min);
                    pl[i]=(uint8_t)v;
                }
            }
        }
        /* compensate bad alignment from header */
        if( aoff ) lseek( fd, aoff, SEEK_CUR );
        write( fd, px, bd.dsize );
        ::close( fd );
    }
    free( px );
}

QPainterPath*
dcm_gui::roi_path( const dcmf *dcm, const roi_elm_t *e, int zf ){
    int i, n=e->ncp*3;
    double *c=new double[n];
    dcmf::get_dbl( e->dta.str, e->dta.str+e->dta.len, c, n );
    if( n<2 ){
        delete[] c;
        return NULL;
    }
    if( n!=3*int(e->ncp) ){
        T( "unexpected # of points (%d != %d)", n, 3*e->ncp );
        delete[] c;
        return NULL;
    }
        
    QPainterPath *pp=new QPainterPath();
    double dx=dcm->dx(), dy=dcm->dy(); // *2 to compensate zf
    const double *pc=dcm->ipp();
    float x,y;
    i=0;
    x=(((c[i]-pc[0])/dx-_x0+1)*zf)*.5f-1;
    y=(((c[i+1]-pc[1])/dy-_y0+1)*zf)*.5f-1;
    pp->moveTo( x, y );
    for( i=3; i<n; i+=3 ){
        x=(((c[i]-pc[0])/dx-_x0+1)*zf)*.5f-1;
        y=(((c[i+1]-pc[1])/dy-_y0+1)*zf)*.5f-1;
        pp->lineTo( x, y );
    }
    pp->closeSubpath();
    delete[] c;
    return pp;
}

void
dcm_gui::x2( bool is_x2 ){
    if( _ssq < _esq ){          // data found
        QRect geo=geometry();
        if( is_x2 ){
            //T( "resize for %dx%d", _x<<1, _y<<1  );       
            _ui.gv->setMinimumSize( _bord+(_x<<1), _bord+(_y<<1) );
            _ui.gv->setMaximumSize( _bord+(_x<<1), _bord+(_y<<1) );
            _ui.gv->setScene( &_imgs2 );
            geo.adjust( 0, -_y, _x, _y );
        }else{
            //T( "resize for %dx%d", _x, _y );
            _ui.gv->setMinimumSize( _bord+_x, _bord+_y );
            _ui.gv->setMaximumSize( _bord+_x, _bord+_y );
            _ui.gv->setScene( &_imgs );
            geo.adjust( 0, _y, -_x, -_y );
        }
        // compensate size change to keep lower border fixed
        setGeometry( geo );
        show_pic( _csq, true );
    }
}

void
dcm_gui::show_pic( int n, bool force ){
//    T( "csq=%d n=%d %s", _csq, n, force?"force":"" );
    if( ( n!=_csq || force )  && _x>0 && _y>0 && _sq[n]!=NULL ){
        _csq=n;
        // update sequence numbers
        QString str;
        _ui.l1->setText( str.sprintf( "%3d/%3d", n, _esq-_ssq+1 ) );
        _ui.s->setValue( n );
        _ui.sbn->setValue( n );
        // fetch image
        dcmf *dcm=_sq[n];
#if(1)
        if( _sq[n]->pxr()==0 )
            _img.get( dcm->d16u(), _min );
        else
            _img.get( dcm->d16s(), _min );
#else
//        _img.test1( (_dmax-_dmin)*3/4 );
        _img.test01( (_dmax-_dmin)*3/4 );
#endif

        // get zoom range
        int zf=int(round(2*_ui.sbz->value()));
#if(0)                          // manual boundary checks
        if( zf < _zf_min ){
            _zf=_zf_min;
        } else {
            if( zf > _zf_max ){
                _zf=_zf_max;
            } else {
                _zf=zf;
                _ui.sbz->setValue( _zf*0.5 );
            }
        }
        _ui.sbz->setValue( _zf*0.5 );
#else  // rely on Qt for boundary checks
        _zf=zf;
#endif

        // center on cursor, if possible
        _x0=_xc-_x/_zf;
        if( _x0<0 ) _x0=0;
        _y0=_yc-_y/_zf;
        if( _y0<0 ) _y0=0;
        //T( "zero (zf %d) pos %d, %d  cur %d %d", _zf, _x0, _y0, _xc, _yc );

        // zoom and RGB conversion
        if( is_x2() ){ // scale 2x
            //_imgz2.clear();
            _imgz2.zoom( _img, _x0, _y0, 2*_zf );
            //_imgz2.rgb( _rgb, _dmin-_min, _dmax-_min, _max-_min );
            _imgz2.rgb( _rgb, _rgb_map );

            if( _pxmi2 ){
                _pxm2.convertFromImage( _im2x[0] );
                _pxmi2->setPixmap( _pxm2 );
                position_cursors();
            } else {
                // create 2x scene elements
                _pxm2=QPixmap::fromImage( _im2x[0] );
                _pxmi2=new QGraphicsPixmapItem( _pxm2 );
                _imgs2.addItem( _pxmi2 );
                _crsri2=new QGraphicsPixmapItem( );
                refresh_cursors();
                _imgs2.addItem( _crsri2 );
                _pxmi2->setVisible( true );
                _crsri2->setVisible( _ui.bcrsr->isChecked() );
            }
        } else {
            if( _zf > 2 ){
                //_imgz.clear();
                _imgz.zoom( _img, _x0, _y0, _zf );
                //_imgz.rgb( _rgb, _dmin-_min, _dmax-_min, _max-_min );
                _imgz.rgb( _rgb, _rgb_map );
            } else {
                _x0=0; _y0=0;
                //_img.rgb( _rgb, _dmin-_min, _dmax-_min, _max-_min );
                _img.rgb( _rgb, _rgb_map );
            }
            if( _pxmi ){
                _pxm.convertFromImage( _im[0] );
                _pxmi->setPixmap( _pxm );
                position_cursors();
            } else {
                // create 1x scene elements
                _pxm=QPixmap::fromImage( _im[0] );
                _pxmi=new QGraphicsPixmapItem( _pxm );
                _crsri=new QGraphicsPixmapItem( );
                refresh_cursors();
                _imgs.addItem( _pxmi );
                _imgs.addItem( _crsri );
                _pxmi->setVisible( true );
                _crsri->setVisible( _ui.bcrsr->isChecked() );
            }
        }
        
        // get and show ROI, if any 
        const smatch_t *sm=NULL;
        if( _i2r ) sm=_i2r->get( dcm->iid() );
        if( sm != NULL ){
            _ui.bROI->setText( "ROI" );
            // show ROI
            if( is_x2() ){ // scale 2x
                QPainterPath *p=roi_path( dcm, _roi->e+sm->d[0], _zf*2 );
                if( _proi2 != NULL )
                    _proi2->setPath( p[0] );
                else {
                    _proi2=new QGraphicsPathItem( p[0] );
                    _imgs2.addItem( _proi2 );
                }
                _proi2->hide();
                _proi2->setPen(QPen(QColor(QColor( _roi->rgb )), 1, Qt::SolidLine,
                                    Qt::FlatCap, Qt::MiterJoin) );                
            } else {
                QPainterPath *p=roi_path( dcm, _roi->e+sm->d[0], _zf );
                if( _proi != NULL )
                    _proi->setPath( p[0] );
                else {
                    _proi=new QGraphicsPathItem( p[0] );
                    _imgs.addItem( _proi );
                }
                _proi->hide();
                _proi->setPen(QPen(QColor(QColor( _roi->rgb )), 1, Qt::SolidLine,
                                   Qt::FlatCap, Qt::MiterJoin) );
            }
            _ui.bROI->setEnabled( true );

            if( _ui.bROI->isChecked() ){
                if( _proi ) 
                    _proi->show();
                if( _proi2 ) 
                    _proi2->show();
                _3dw->set_slice( sm->d[0] );
            }
        } else {
            _3dw->set_slice( -1 ); // disable 3d slice highlight
            _ui.bROI->setEnabled( false );
            if( _proi )
                _proi->hide();
            if( _proi2 )
                _proi2->hide();
        }
        show_coords();
    }
}

void
dcm_gui::show_coords() {
    // display the 'real' coordinates
    if( _ssq < _esq ){
        dcmf *dcm=_sq[_csq];
        if( dcm ){
            const double *pc=dcm->ipp();
            QString xyz;
            xyz.sprintf( "%+6.1f %+6.1f %+6.1f = %5d",
                         pc[0]+_xc*dcm->dx(), 
                         pc[1]+_yc*dcm->dy(),
                         pc[2],
                         _min+_img.px(_xc,_yc) );
            _ui.lxyz->setText( xyz );
        }
    }
}

void
dcm_gui::show_hist(bool show){
    //T( "%s", show?"true":"false" );
    if( show )
        _hw->show();
    else
        _hw->hide();
}

void
dcm_gui::show_cursor(bool show){
    //T( "%s", show?"true":"false" );
    if( _crsri ) _crsri->setVisible( show );
    if( _crsri2 ) _crsri2->setVisible( show );
}

void
dcm_gui::move_cursor(int pos){
    if( sender()==(QObject*)_ui.sbx ){
        if( _xc != pos ){
            _xc=pos;
            position_cursors();
        }
        return;
    }
    if( sender()==(QObject*)_ui.sby ){
        if( _yc != pos ){
            _yc=pos;
            position_cursors();
        }
        return;
    }
}

void
dcm_gui::set_bound(int val){
    if( sender()==(QObject*)_hui.sbm )
        _dmin=val;

    if( sender()==(QObject*)_hui.sbM )
        _dmax=val;

    if( sender()==(QObject*)_hui.srgb0 )
        _gray0=val;

    if( sender()==(QObject*)_hui.srgb1 )
        _gray1=val;

    update_rgb_map();
    refresh_pic();
}

void
dcm_gui::position_cursors( ){
    if( _crsri ){
        int xo=_zf&1?(_xc-_x0)&1:0; // adjustment to hit the "full" pixels
        int yo=_zf&1?(_yc-_y0)&1:0;
        _crsri->setOffset( (_zf*(_xc-_x0)>>1)-C_OFF+xo, (_zf*(_yc-_y0)>>1)-C_OFF+yo );
    }
    if( _crsri2 )
        _crsri2->setOffset( _zf*(_xc-_x0)-C2_OFF, _zf*(_yc-_y0)-C2_OFF );

    show_coords();
}

void
dcm_gui::refresh_cursors( ){
    if( _crsri && _cidx != _zf-2 ){
        QPixmap *pxm = new QPixmap( xpm_crsr[_zf-2] );
        _crsri->setPixmap( pxm[0] );
        delete pxm;
        _cidx=_zf-2;
    }
    if( _crsri2 && _cidx2 != _zf-2 ){
        QPixmap *pxm = new QPixmap( xpm_crsr2[_zf-2] );
        _crsri2->setPixmap( pxm[0] );
        delete pxm;
        _cidx2=_zf-2;
    }
    position_cursors();
};

void 
dcm_gui::refresh_pic( ){ 
    show_pic( _csq, true );
    refresh_cursors( );
}


void
dcm_gui::show_3d(bool show){
    if( _ssq<_esq ){
        if( show )
            _3dw->show();
        else
            _3dw->hide();
    }
}

void
dcm_gui::show_roi(bool show){
    //T( "%s", show?"true":"false" );
    if( _proi ){
        if( show )
            _proi->show();
        else
            _proi->hide();
    }
    if( _proi2 ){
        if( show )
            _proi2->show();
        else
            _proi2->hide();
    }
}

void
dcm_gui::step(bool fwd, int step){
    if( fwd )
        next_pic( step );
    else
        prev_pic( step );
}

void
dcm_gui::next_pic( int step ){
    int n=_csq+step;
    if( n > _esq )
        n=_ssq;
    show_pic( n );
}

void
dcm_gui::prev_pic( int step ){
    int n=_csq-step;
    if( n < _ssq )
        n=_esq;
    show_pic( n );
}

bool
dcm_gui::eventFilter( QObject *obj, QEvent *ev ){
    if( ev->type() != QEvent::KeyPress ) return false;
    
    QKeyEvent *e=static_cast<QKeyEvent *>(ev);
    if( obj==_ui.gv ) {
        // get arrow events of Graphics View from main window
        switch( e->key() ){
            case Qt::Key_Up:
            case Qt::Key_Down:
            case Qt::Key_Left:
            case Qt::Key_Right:
            case Qt::Key_Home:
                // forward
                keyPressEvent( e );
                return true;
            default:
                return false;
        }
    }
    if( obj==_ui.sbx || obj==_ui.sby || obj==_ui.sbz || obj==_ui.sbn ){
        // intercept main window's key controls
        switch( e->key() ){
            case Qt::Key_A:
            case Qt::Key_X:
            case Qt::Key_Z:
            case Qt::Key_R:
            case Qt::Key_D:
            case Qt::Key_Q:
            case Qt::Key_C:
            case Qt::Key_H:
            case Qt::Key_S:
            case Qt::Key_F1:
            case Qt::Key_Home:
                // forward
                keyPressEvent( e );
                return true;
            default:
                return false;
        }
    }
    if( obj==_helpw || obj==_3dw || obj==_hw
        || obj==_hui.srgb0 || obj==_hui.srgb1 || obj==_hui.sbm || obj==_hui.sbM ){
        switch( e->key() ){
            case Qt::Key_A:
            case Qt::Key_Q:
            case Qt::Key_H:
            case Qt::Key_S:
            case Qt::Key_F1:
                // forward
                keyPressEvent( e );
                return true;
            default:
                return false;
        }
    }
    return false;
}

void
dcm_gui::keyPressEvent(QKeyEvent *ev ){
    bool acc=true;
    switch( ev->key() ){
        case Qt::Key_Up:
            if( !(ev->modifiers() & Qt::ControlModifier) ){
                if( ev->modifiers() & Qt::ShiftModifier )
                    _ui.sbn->setValue(_csq-10);
                else
                    _ui.sbn->setValue(_csq-1);
    
            } else {
                if( ev->modifiers() & Qt::ShiftModifier )
                    _ui.sby->setValue(_yc-10);
                else
                    _ui.sby->setValue(_yc-1);
            }
            break;
        case Qt::Key_Down:
            if( !(ev->modifiers() & Qt::ControlModifier) ){
                if( ev->modifiers() & Qt::ShiftModifier )
                    _ui.sbn->setValue(_csq+10);
                else
                    _ui.sbn->setValue(_csq+1);
            } else {
                if( ev->modifiers() & Qt::ShiftModifier )
                    _ui.sby->setValue(_yc+10);
                else
                    _ui.sby->setValue(_yc+1);
            }
            break;
        case Qt::Key_Left:
            if( !(ev->modifiers() & Qt::ControlModifier) ){
                if( ev->modifiers() & Qt::ShiftModifier )
                    _ui.sbn->setValue(_csq-10);
                else
                    _ui.sbn->setValue(_csq-1);
    
            } else {
                if( ev->modifiers() & Qt::ShiftModifier )
                    _ui.sbx->setValue(_xc-10);
                else
                    _ui.sbx->setValue(_xc-1);
            }
            break;
        case Qt::Key_Right:
            if( !(ev->modifiers() & Qt::ControlModifier) ){
                if( ev->modifiers() & Qt::ShiftModifier )
                    _ui.sbn->setValue(_csq+10);
                else
                    _ui.sbn->setValue(_csq+1);
            } else {
                if( ev->modifiers() & Qt::ShiftModifier )
                    _ui.sbx->setValue(_xc+10);
                else
                    _ui.sbx->setValue(_xc+1);
            }
            break;
        case Qt::Key_Z:
            if( _zf < _zf_max ){
                _zf+=1;
                _ui.sbz->setValue( double(_zf)*0.5 );
            }
            break;
        case Qt::Key_X:
            if( _zf > _zf_min ){
                _zf-=1;
                _ui.sbz->setValue( double(_zf)*0.5 );
            }
            break;
        case Qt::Key_C:         // just center
            if( _ssq<_esq )
                refresh_pic( );
            break;
        case Qt::Key_R:         // ROI
            if( _ui.bROI->isEnabled() )
                _ui.bROI->toggle();
            break;
        case Qt::Key_D:         // double (x2)
            _ui.bx2->toggle();
            break;
        case Qt::Key_Home:      // reset zoom
            if( _zf!=2 ){
                _ui.sbz->setValue( 1.0 );
            }
            break;
        case Qt::Key_F1:
            if( _helpw!=NULL ){
                bool vis=_helpw->isVisible();
                _helpw->setVisible( !vis );
            }
            break;
        case Qt::Key_H:
        case Qt::Key_S:
            _ui.bh->toggle();
            break;
        case Qt::Key_Q:
            QCoreApplication::quit();
            break;
        case Qt::Key_A:         // align subwindows
            if( _hw != NULL ) _hw->parent_pos();
            if( _3dw != NULL ) _3dw->parent_pos();
            break;
        default:
            acc=false;
            break;
    }
    if( acc )
        ev->accept();    
}

void
dcm_gui::mousePressEvent(QMouseEvent *ev){

    //T( "mouse press %d %d", ev->x(), ev->y() );

    QPoint gp=ev->pos()-_ui.gv->pos();
    QPoint g0=_ui.gv->mapFromScene( QPointF( 0, 0 ) );

    QPoint cp=gp-g0;
    int xc=cp.x(), yc=cp.y();
    if( xc < 0 ) xc=0;
    if( yc < 0 ) yc=0;
    if( is_x2() ) {
        if( xc >= 2*_x ) xc=2*_y-1;
        if( yc >= 2*_y ) yc=2*_y-1;
        
        xc=xc>>1;
        yc=yc>>1;

    } else {
        if( xc >= _x ) xc=_x-1;
        if( yc >= _y ) yc=_y-1;
    }
    // now we need to compensate zoom and offsets
    xc=_x0+(2*xc)/_zf;
    yc=_y0+(2*yc)/_zf;
    if( xc != _xc || yc!=_yc ){
        _ui.sbx->setValue( xc );
        _ui.sby->setValue( yc );
    } else {                    // center image as, perhaps, a double click
        refresh_pic( );
    }
    ev->accept();
    //step(true);
}

void dcm_gui::wheelEvent ( QWheelEvent *ev ){
    ev->accept();
    if( ev->modifiers() & Qt::ControlModifier ){
        if( ev->delta() < 0 ) {
            if( _zf > _zf_min ){
                _zf-=1;
                _ui.sbz->setValue( double(_zf)*0.5 );
            }
        }  else {
            if( _zf < _zf_max ){
                _zf+=1;
                _ui.sbz->setValue( double(_zf)*0.5 );
            }
        }
    } else {
        if( ev->modifiers() & Qt::ShiftModifier ){
            step( ev->delta() < 0, 10 );            
        } else {
            step( ev->delta() < 0 );
        }
    }
}
