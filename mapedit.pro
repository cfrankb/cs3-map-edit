QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets opengl

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += shared/
DEFINES += USE_QFILE=1

SOURCES += \
    data.cpp \
    #font.cpp \
    level.cpp \
    main.cpp \
    mainwindow.cpp \
    mapfile.cpp \
    mapscroll.cpp \
    mapwidget.cpp \
    shared/DotArray.cpp \
    shared/FileWrap.cpp \
    shared/Frame.cpp \
    shared/FrameSet.cpp \
    shared/PngMagic.cpp \
    shared/helper.cpp \
    shared/map.cpp \
    shared/qtgui/qfilewrap.cpp \
    shared/qtgui/qthelper.cpp \
    tilesdata.cpp

HEADERS += \
    data.h \
    #font.h \
    level.h \
    mainwindow.h \
    mapfile.h \
    mapscroll.h \
    mapwidget.h \
    shared/DotArray.h \
    shared/FileWrap.h \
    shared/Frame.h \
    shared/FrameSet.h \
    shared/PngMagic.h \
    shared/helper.h \
    shared/map.h \
    shared/qtgui/cheat.h \
    shared/qtgui/qfilewrap.h \
    shared/qtgui/qthelper.h \
    tilesdata.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    mapedit.qrc

unix:LIBS += -lz
