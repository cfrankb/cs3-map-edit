QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += runtime/shared/
DEFINES += USE_QFILE=1
#DEFINES += USE_SDL_MIXER=1
#DEFINES += USE_HUNSPELL=1

SOURCES += \
    runtime/ai_path.cpp \
    runtime/bossdata.cpp \
    runtime/actor.cpp \
    runtime/randomz.cpp \
    runtime/animator.cpp \
    runtime/game.cpp \
    runtime/game_ai.cpp \
    runtime/gameui.cpp \
    runtime/gamemixin.cpp \
    runtime/level.cpp \
    runtime/maparch.cpp \
    runtime/shared/DotArray.cpp \
    runtime/shared/FileWrap.cpp \
    runtime/shared/FileMem.cpp \
    runtime/shared/Frame.cpp \
    runtime/shared/FrameSet.cpp \
    runtime/shared/PngMagic.cpp \
    runtime/shared/helper.cpp \
    runtime/map.cpp \
    runtime/shared/qtgui/qfilewrap.cpp \
    runtime/shared/qtgui/qthelper.cpp \
    runtime/tilesdata.cpp \
    runtime/tilesdebug.cpp \
    runtime/states.cpp \
    runtime/statedata.cpp \
    runtime/chars.cpp \
    runtime/recorder.cpp \
    runtime/gamestats.cpp \
    runtime/colormap.cpp \
    runtime/strhelper.cpp \
    runtime/boss.cpp \
    dlgattr.cpp \
    dlgmsgs.cpp \
    dlgresize.cpp \
    dlgabout.cpp \
    dlgselect.cpp \
    dlgstat.cpp \
    dlgtest.cpp \
    tilebox.cpp \
    keyvaluedialog.cpp \
    main.cpp \
    mainwindow.cpp \
    mapfile.cpp \
    mapscroll.cpp \
    mapwidget.cpp \
    report.cpp \
    mapprops.cpp


HEADERS += \
    runtime/ai_path.h \
    runtime/bossdata.h  \
    runtime/build.h  \
    runtime/color.h  \
    runtime/filemacros.h  \
    runtime/randomz.h  \
    runtime/actor.h \
    runtime/anniedata.h \
    runtime/animzdata.h \
    runtime/animator.h \
    runtime/game.h \
    runtime/gameui.h \
    runtime/gamemixin.h \
    runtime/level.h \
    runtime/maparch.h \
    runtime/shared/DotArray.h \
    runtime/shared/FileWrap.h \
    runtime/shared/FileMem.h \
    runtime/shared/Frame.h \
    runtime/shared/FrameSet.h \
    runtime/shared/PngMagic.h \
    runtime/shared/helper.h \
    runtime/map.h \
    runtime/shared/qtgui/cheat.h \
    runtime/shared/qtgui/qfilewrap.h \
    runtime/shared/qtgui/qthelper.h \
    runtime/sprtypes.h \
    runtime/tilesdata.h \
    runtime/tilesdebug.h \
    runtime/sounds.h \
    runtime/states.h \
    runtime/statedata.h \
    runtime/skills.h \
    runtime/events.h \
    runtime/chars.h \
    runtime/recorder.h \
    runtime/gamesfx.h \
    runtime/gamestats.h \
    runtime/colormap.h \
    runtime/strhelper.h \
    runtime/attr.h \
    runtime/logger.h \
    runtime/boss.h \
    runtime/rect.h \
    runtime/gamesfx.h \
    runtime/joyaim.h \
    app_version.h \
    dlgattr.h \
    dlgmsgs.h \
    dlgresize.h \
    dlgabout.h \
    dlgselect.h \
    dlgstat.h \
    dlgtest.h \
    mainwindow.h \
    mapfile.h \
    mapscroll.h \
    mapwidget.h \
    report.h \
    tilebox.h \
    keyvaluedialog.h \
    mapprops.h

FORMS += \
    dlgattr.ui \
    dlgmsgs.ui \
    dlgresize.ui \
    dlgabout.ui \
    dlgselect.ui \
    dlgstat.ui \
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
contains(DEFINES, USE_SDL_MIXER=1){
    unix:LIBS += -lSDL2_mixer -lSDL2
    HEADERS += shared/implementers/sn_sdl.h \
        shared/interfaces/ISound.h
    SOURCES +=  shared/implementers/sn_sdl.cpp
}

QMAKE_CXXFLAGS_RELEASE += -std=c++20 -O3
QMAKE_CXXFLAGS_DEBUG += -std=c++20 -g3

contains(DEFINES, USE_HUNSPELL=1){
    INCLUDEPATH += ../external/hunspell/src/hunspell
    LIBS += -Lpath/to/hunspell/lib -lhunspell
    HEADERS += SpellHighlighter.h \
        SpellTextEdit.h
}
