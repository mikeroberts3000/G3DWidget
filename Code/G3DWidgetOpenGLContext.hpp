#ifndef G3D_WIDGET_OPENGL_CONTEXT_H
#define G3D_WIDGET_OPENGL_CONTEXT_H

#include <GLG3D/OSWindow.h>

#ifdef __OBJC__
@class NSOpenGLContext;
@class NSView;
#else
class NSOpenGLContext;
class NSView;
#endif

#ifdef Q_WS_MAC32
typedef int WId;
#else
typedef long WId;
#endif

namespace mojo
{

class G3DWidgetOpenGLContext
{
public:
    G3DWidgetOpenGLContext(const G3D::OSWindow::Settings& settings);
    ~G3DWidgetOpenGLContext();

    void getSettings(G3D::OSWindow::Settings& settings) const;

    void makeCurrent();
    void setView(WId winId);
    void update();
    void flushBuffer();

private:
    NSOpenGLContext*        m_nsOpenGLContext;
    G3D::OSWindow::Settings m_settings;
};

}

#endif
