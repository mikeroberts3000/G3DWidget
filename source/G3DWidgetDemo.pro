#-------------------------------------------------
#
# Project created by QtCreator 2013-03-29T02:37:26
#
#-------------------------------------------------

TARGET   = G3DWidgetDemo
TEMPLATE = app

QT     += core gui webkit
CONFIG -= app_bundle
CONFIG += console

CONFIG(debug,   debug|release): DEFINES += MOJO_DEBUG
CONFIG(release, debug|release): DEFINES += MOJO_RELEASE

QMAKE_CXXFLAGS_WARN_ON += "-Wno-unused-parameter"
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O4 -g
QMAKE_CXXFLAGS_DEBUG   += -fasm-blocks
QMAKE_LFLAGS_RELEASE   -= -O2
QMAKE_LFLAGS_RELEASE   += -O4 -g

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.8

INCLUDEPATH +=              \
    /opt/local/include      \
    /usr/X11/include        \
    $(G3D9DATA)/../include/ \

LIBS +=                     \
    -framework Cocoa        \
    -framework OpenGL       \
    -framework IOKit        \
    -framework SDL          \
    -lz                     \
    -L"$(G3D9DATA)/../lib/" \
    -lassimp                \
    -lavcodec.54            \
    -lavformat.54           \
    -lavutil.51             \
    -lfreeimage             \
    -lglfw                  \
    -lswscale.2             \
    -lzip                   \
    -lenet                  \

CONFIG(debug,   release|debug):LIBS += -lG3Dd -lGLG3Dd
CONFIG(release, release|debug):LIBS += -lG3D  -lGLG3D

FORMS += \
    MainWindow.ui

HEADERS +=                     \
    Assert.hpp                 \
    G3DWidget.hpp              \
    G3DWidgetOpenGLContext.hpp \
    MainWindow.hpp             \
    PixelShaderApp.hpp         \
    Printf.hpp                 \
    QtUtil.hpp                 \
    StarterApp.hpp             \
    ToString.hpp               \

SOURCES +=             \
    Main.cpp           \
    MainWindow.cpp     \
    PixelShaderApp.cpp \
    StarterApp.cpp     \
    Printf.cpp         \
    G3DWidget.cpp      \

OBJECTIVE_SOURCES +=          \
    G3DWidgetOpenGLContext.mm \

QMAKE_POST_LINK +=              \
    cp ../bin/*    $$OUT_PWD && \
    cp ../shader/* $$OUT_PWD    \
