SOURCES += dicom.cc words.cc
SOURCES += dcm_gui.cc glw.cc img.cc
SOURCES += main.cc

RESOURCES = dcmview.qrc
FORMS += dcm.ui hist.ui

HEADERS += dcm_gui.h glw.h img.h dcm_gui_xpm.cc
QMAKE_CXXFLAGS+= -g3

LIBS += -g

INCLUDEPATH += ..

QT += core gui widgets webkit
QT += webkitwidgets
#QT += opengl
CONFIG += qt 
CONFIG += gui

CONFIG += release
#CONFIG += debug

CONFIG(debug, debug|release) { 
    DESTDIR = .dbg
}

CONFIG(release, release|debug) { 
    DESTDIR = .rel
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui
