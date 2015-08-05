#ifndef G3D_WIDGET_HPP
#define G3D_WIDGET_HPP

#include <memory>

#include <QtWidgets/QWidget>

#include <GLG3D/OSWindow.h>

#include <SDL/SDL.h>
#ifdef main
#undef main
#endif

namespace G3D
{
class GApp;
class RenderDevice;
}

namespace mojo
{

class G3DWidgetOpenGLContext;

class G3DWidget : public QWidget, public G3D::OSWindow
{
    Q_OBJECT

public:
    G3DWidget(
        std::shared_ptr<G3DWidgetOpenGLContext> g3dWidgetOpenGLContext,
        std::shared_ptr<G3D::RenderDevice>      renderDevice,
        QWidget* parent = NULL);

    virtual ~G3DWidget();

    virtual void initialize();
    virtual void update();
    virtual void terminate();

    virtual QPaintEngine* paintEngine() const;

    virtual bool requiresMainLoop() const;
    virtual void pushLoopBody(void (*body)(void*), void* arg);
    virtual void pushLoopBody(G3D::GApp* app);
    virtual void popLoopBody();
    virtual void reallyMakeCurrent() const;
    virtual void swapGLBuffers();

    virtual void getSettings(G3D::OSWindow::Settings& settings) const;

    virtual G3D::Rect2D fullRect() const;
    virtual void setFullRect(const G3D::Rect2D&);

    virtual G3D::Rect2D clientRect() const;
    virtual void setClientRect(const G3D::Rect2D&);

    virtual int width() const;
    virtual int height() const;

    virtual bool hasFocus() const;

    virtual void getRelativeMouseState(G3D::Vector2& position, G3D::uint8& mouseButtons) const;
    virtual void getRelativeMouseState(double& x, double& y, G3D::uint8& mouseButtons) const;
    virtual void getRelativeMouseState(int& x, int& y, G3D::uint8& mouseButtons) const;
    virtual void setRelativeMousePosition(double x, double y);
    virtual void setRelativeMousePosition(const G3D::Vector2& v);
    virtual void setMouseVisible(bool);

    virtual int numJoysticks() const;
    virtual G3D::String joystickName(unsigned int sticknum) const;
    virtual void getJoystickState(unsigned int stickNum, G3D::Array<float>& axis, G3D::Array<bool>& button) const;

    virtual G3D::String caption();
    virtual void setCaption(const G3D::String&);

    virtual G3D::String getAPIVersion() const;
    virtual G3D::String getAPIName() const;
    virtual G3D::String className() const;

    virtual void getDroppedFilenames(G3D::Array<G3D::String>& files);

    virtual void setInputCapture(bool);
    virtual void setFullPosition(int, int);
    virtual void setClientPosition(int, int);    
    virtual void setGammaRamp(const G3D::Array<G3D::uint16>& gammaRamp);

protected:
    virtual void paintEvent(QPaintEvent*);
    virtual void resizeEvent(QResizeEvent* e);
    virtual void enterEvent(QEvent*);
    virtual void leaveEvent(QEvent*);
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void dragEnterEvent(QDragEnterEvent* e);
    virtual void dropEvent(QDropEvent* e);
    virtual void keyPressEvent(QKeyEvent* e);
    virtual void keyReleaseEvent(QKeyEvent* e);

    virtual G3D::String _clipboardText() const;
    virtual void _setClipboardText(const G3D::String&) const;

private:
    G3D::uint8 getG3DMouseButtonPressedFlags(Qt::MouseButtons mouseButtons) const;
    G3D::uint8 getG3DMouseButtonPressedIndex(Qt::MouseButtons mouseButtons) const;

    void keyEvent(QKeyEvent* keyEvent, G3D::GEvent& e);

    std::shared_ptr<G3DWidgetOpenGLContext> m_g3dWidgetOpenGLContext;
    bool                                    m_initialized;
    QPoint                                  m_mousePrevPos;
    Qt::MouseButtons                        m_mousePressEventButtons;
    bool                                    m_mouseVisible;
    G3D::Array<SDL_Joystick*>               m_joy;
    G3D::Array<G3D::String>                 m_dropFileList;
    bool                                    m_previouslyActive;
    qreal                                   m_devicePixelRatio;
    G3D::GApp*                              m_GApp;
};

}

#endif
