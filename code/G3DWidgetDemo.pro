#-------------------------------------------------
#
# Project created by QtCreator 2015-08-04T19:03:57
#
#-------------------------------------------------

QT += core widgets webkitwidgets

TARGET = G3DWidgetDemo

CONFIG += console
CONFIG -= app_bundle
CONFIG += c++11

TEMPLATE = app

QMAKE_CXXFLAGS_WARN_ON  = ""
QMAKE_CXXFLAGS         += -msse4.1 -Wno-unknown-pragmas

QMAKE_OBJECTIVE_CFLAGS_WARN_ON  = ""
QMAKE_OBJECTIVE_CFLAGS         += -msse4.1 -Wno-unknown-pragmas

INCLUDEPATH +=              \
    /opt/local/include      \
    ${G3D10DATA}/../include \

LIBS +=                      \
    -F"/Library/Frameworks/" \
    -L"${G3D10DATA}/../lib/" \
    -framework Cocoa         \
    -framework CoreVideo     \
    -framework OpenGL        \
    -framework SDL           \
    -lavcodec.56             \
    -lavformat.56            \
    -lavutil.54              \
    -lfmod                   \
    -lfreeimage              \
    -lswscale.3              \
    -lz                      \

CONFIG(debug,   release|debug):LIBS += -lG3Dd -lGLG3Dd -lassimpd -lcivetwebd -lenetd -lglewd -lglfwd -lnfdd -lzipd
CONFIG(release, release|debug):LIBS += -lG3D  -lGLG3D  -lassimp  -lcivetweb  -lenet  -lglew  -lglfw  -lnfd  -lzip

HEADERS +=                     \
    Assert.hpp                 \
    Printf.hpp                 \
    ToString.hpp               \
    QtUtil.hpp                 \
    G3DWidgetOpenGLContext.hpp \
    G3DWidget.hpp              \
    PixelShaderApp.hpp         \
    StarterApp.hpp             \
    MainWindow.hpp             \

SOURCES +=             \
    Printf.cpp         \
    G3DWidget.cpp      \
    PixelShaderApp.cpp \
    StarterApp.cpp     \
    MainWindow.cpp     \
    Main.cpp           \

OBJECTIVE_SOURCES +=          \
    G3DWidgetOpenGLContext.mm \

FORMS += \
    MainWindow.ui

QMAKE_POST_LINK +=               \
    cp ../bin/*     $$OUT_PWD && \
    cp ../shaders/* $$OUT_PWD && \
    install_name_tool -change "@rpath/libfmod.dylib" "@loader_path/libfmod.dylib" $$OUT_PWD/${TARGET} \
