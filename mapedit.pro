QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += shared/ headers/
DEFINES += USE_QFILE=1

SOURCES += \
    actor.cpp \
    animator.cpp \
    dlgattr.cpp \
    dlgresize.cpp \
    dlgabout.cpp \
    dlgselect.cpp \
    dlgtest.cpp \
    game.cpp \
    gamemixin.cpp \
    level.cpp \
    main.cpp \
    mainwindow.cpp \
    maparch.cpp \
    mapfile.cpp \
    mapscroll.cpp \
    mapwidgetgdi.cpp \
    shared/DotArray.cpp \
    shared/FileWrap.cpp \
    shared/Frame.cpp \
    shared/FrameSet.cpp \
    shared/PngMagic.cpp \
    shared/helper.cpp \
    map.cpp \
    shared/qtgui/qfilewrap.cpp \
    shared/qtgui/qthelper.cpp \
    tilebox.cpp \
    tilesdata.cpp

HEADERS += \
    actor.h \
    animator.h \
    dlgattr.h \
    dlgresize.h \
    dlgabout.h \
    dlgselect.h \
    dlgtest.h \
    game.h \
    gamemixin.h \
    level.h \
    mainwindow.h \
    maparch.h \
    mapfile.h \
    mapscroll.h \
    mapwidgetgdi.h \
    shared/DotArray.h \
    shared/FileWrap.h \
    shared/Frame.h \
    shared/FrameSet.h \
    shared/PngMagic.h \
    shared/helper.h \
    map.h \
    shared/qtgui/cheat.h \
    shared/qtgui/qfilewrap.h \
    shared/qtgui/qthelper.h \
    tilebox.h \
    tilesdata.h

FORMS += \
    dlgattr.ui \
    dlgresize.ui \
    dlgabout.ui \
    dlgselect.ui \
    dlgtest.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    mapedit.qrc

unix:LIBS += -lz
win32:LIBS += -L"libs" -lzlib

QMAKE_CXXFLAGS_RELEASE += -std=c++17 -O3
QMAKE_CXXFLAGS_DEBUG += -std=c++17 -g3
