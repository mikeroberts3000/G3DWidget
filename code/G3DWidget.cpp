#include <boost/type_traits.hpp>

#include "G3DWidget.hpp"

#include <QtCore/QUrl>
#include <QtGui/QResizeEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QClipboard>
#include <QtGui/QApplication>

#include <GLG3D/GLCaps.h>
#include <GLG3D/RenderDevice.h>

#include "Assert.hpp"
#include "Printf.hpp"
#include "G3DWidgetOpenGLContext.hpp"

namespace mojo
{

G3DWidget::G3DWidget(
    std::tr1::shared_ptr<G3DWidgetOpenGLContext> g3dWidgetOpenGLContext,
    std::tr1::shared_ptr<G3D::RenderDevice>      renderDevice,
    QWidget* parent) :
    QWidget                 (parent),
    m_g3dWidgetOpenGLContext(g3dWidgetOpenGLContext),
    m_initialized           (false),
    m_mousePressEventButtons(Qt::NoButton),
    m_mouseVisible          (true),
    m_previouslyActive      (false) {

    MOJO_RELEASE_ASSERT(g3dWidgetOpenGLContext);
    MOJO_RELEASE_ASSERT(renderDevice);

    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(true);
    setMouseTracking(true);
    setAcceptDrops(true);

    m_renderDevice = renderDevice.get();
    m_g3dWidgetOpenGLContext->getSettings(m_settings);
}

G3DWidget::~G3DWidget() {
}

void G3DWidget::initialize() {

    // Check for joysticks
    int j = SDL_NumJoysticks();
    if ((j < 0) || (j > 10)) {
        // If there is no joystick adapter on Win32,
        // SDL returns ridiculous numbers.
        j = 0;
    }

    if (j > 0) {
        SDL_JoystickEventState(SDL_ENABLE);
        // Turn on the joysticks

        m_joy.resize(j);
        for (int i = 0; i < j; ++i) {
            m_joy[i] = SDL_JoystickOpen(i);
            debugAssert(m_joy[i]);
        }
    }

    m_settings.x      = 0;
    m_settings.y      = 0;
    m_settings.width  = QWidget::width();
    m_settings.height = QWidget::height();
    m_initialized     = true;

    // make this G3DWidget the current rendering target
    OSWindow::makeCurrent();

    // initializing OpenGL extensions requires the G3DWidget to be current
    G3D::GLCaps::init();
}

void G3DWidget::update() {
    MOJO_RELEASE_ASSERT(m_initialized);

    OSWindow::makeCurrent();

    // construct a G3D::GEventType::FOCUS event
    bool currentlyActive = QApplication::activeWindow() != NULL;
    if (currentlyActive != m_previouslyActive) {
        G3D::GEvent e;
        e.type           = G3D::GEventType::FOCUS;
        e.focus.hasFocus = currentlyActive;

        m_previouslyActive = currentlyActive;

        fireEvent(e);
    }

    // execute the G3D::GApp loop
    executeLoopBody();

    // swap buffers explicitly
    m_renderDevice->swapBuffers();
}

void G3DWidget::terminate() {
    MOJO_RELEASE_ASSERT(m_initialized);

    popLoopBody();

    // Close joysticks, if opened
    for (int j = 0; j < m_joy.size(); ++j) {
        SDL_JoystickClose(m_joy[j]);
    }

    m_joy.clear();
}

QPaintEngine* G3DWidget::paintEngine() const {
    return NULL;
}

bool G3DWidget::requiresMainLoop() const {
    MOJO_RELEASE_ASSERT(m_initialized);
    return true;
}

void G3DWidget::pushLoopBody(void (*body)(void*), void* arg) {
    MOJO_RELEASE_ASSERT(m_initialized);
    MOJO_RELEASE_ASSERT(body           != NULL);
    MOJO_RELEASE_ASSERT(arg            != NULL);
    MOJO_RELEASE_ASSERT(m_renderDevice != NULL);

    OSWindow::makeCurrent();
    m_renderDevice->setSwapBuffersAutomatically(false);
    OSWindow::pushLoopBody(body, arg);
}

void G3DWidget::pushLoopBody(G3D::GApp* app) {
    MOJO_RELEASE_ASSERT(m_initialized);
    MOJO_RELEASE_ASSERT(app            != NULL);
    MOJO_RELEASE_ASSERT(m_renderDevice != NULL);

    OSWindow::makeCurrent();
    m_renderDevice->setSwapBuffersAutomatically(false);
    OSWindow::pushLoopBody(app);
}

void G3DWidget::popLoopBody() {
    MOJO_RELEASE_ASSERT(m_initialized);
    OSWindow::popLoopBody();
}

void G3DWidget::reallyMakeCurrent() const {
    MOJO_RELEASE_ASSERT(m_initialized);

    m_g3dWidgetOpenGLContext->makeCurrent();
    m_g3dWidgetOpenGLContext->setView(winId());

    if (m_renderDevice->initialized()) {
        G3D::Rect2D newViewport = G3D::Rect2D::xywh(0.0f, 0.0f, (float)QWidget::width(), (float)QWidget::height());
        m_renderDevice->setWindow((G3D::OSWindow*)this);
        m_renderDevice->setViewport(newViewport);
    }
}

void G3DWidget::swapGLBuffers() {
    MOJO_RELEASE_ASSERT(m_initialized);
    m_g3dWidgetOpenGLContext->flushBuffer();
}

void G3DWidget::getSettings(G3D::OSWindow::Settings& settings) const {
    MOJO_RELEASE_ASSERT(m_initialized);
    settings = m_settings;
}

G3D::Rect2D G3DWidget::fullRect() const {
    MOJO_RELEASE_ASSERT(m_initialized);
    return G3D::Rect2D::xywh(0, 0, width(), height());
}

void G3DWidget::setFullRect(const G3D::Rect2D&) {
    MOJO_RELEASE_ASSERT(m_initialized);
}

G3D::Rect2D G3DWidget::clientRect() const {
    MOJO_RELEASE_ASSERT(m_initialized);
    return G3D::Rect2D::xywh(0, 0, width(), height());
}

void G3DWidget::setClientRect(const G3D::Rect2D&) {
    MOJO_RELEASE_ASSERT(m_initialized);
}

int G3DWidget::width() const {
    MOJO_RELEASE_ASSERT(m_initialized);
    return QWidget::width();
}

int G3DWidget::height() const {
    MOJO_RELEASE_ASSERT(m_initialized);
    return QWidget::height();
}

bool G3DWidget::hasFocus() const {
    MOJO_RELEASE_ASSERT(m_initialized);
    return QApplication::activeWindow() != NULL;
}

void G3DWidget::getRelativeMouseState(G3D::Vector2& position, G3D::uint8& mouseButtons) const {
    MOJO_RELEASE_ASSERT(m_initialized);

    int xx, yy;
    getRelativeMouseState(xx, yy, mouseButtons);
    position.x = xx;
    position.y = yy;
}

void G3DWidget::getRelativeMouseState(double& x, double& y, G3D::uint8& mouseButtons) const {
    MOJO_RELEASE_ASSERT(m_initialized);

    int xx, yy;
    getRelativeMouseState(xx, yy, mouseButtons);
    x = (double)xx;
    y = (double)yy;
}

void G3DWidget::getRelativeMouseState(int& x, int& y, G3D::uint8& mouseButtons) const {
    MOJO_RELEASE_ASSERT(m_initialized);

    QPoint p     = QWidget::mapFromGlobal(QCursor::pos());
    x            = p.x();
    y            = p.y();
    mouseButtons = 0;

    if (underMouse()) {
        mouseButtons = getG3DMouseButtonPressedFlags(m_mousePressEventButtons);
    }
}

void G3DWidget::setRelativeMousePosition(double x, double y) {
    MOJO_RELEASE_ASSERT(m_initialized);
    QCursor::setPos(QWidget::mapToGlobal(QPoint((int)x, (int)y)));
}

void G3DWidget::setRelativeMousePosition(const G3D::Vector2& v) {
    MOJO_RELEASE_ASSERT(m_initialized);
    QCursor::setPos(QWidget::mapToGlobal(QPoint((int)v.x, (int)v.y)));
}

void G3DWidget::setMouseVisible(bool b) {
    MOJO_RELEASE_ASSERT(m_initialized);

    if (underMouse()) {
        if (b) {
            setCursor(QCursor(Qt::ArrowCursor));
        } else {
            setCursor(QCursor(Qt::BlankCursor));
        }
    }
    m_mouseVisible = b;
}

int G3DWidget::numJoysticks() const {
    MOJO_RELEASE_ASSERT(m_initialized);
    return m_joy.size();
}

std::string G3DWidget::joystickName(unsigned int sticknum) const {
    MOJO_RELEASE_ASSERT(m_initialized);
    MOJO_ASSERT(sticknum < ((unsigned int) m_joy.size()));
    return SDL_JoystickName(sticknum);
}

void G3DWidget::getJoystickState(unsigned int stickNum, G3D::Array<float>& axis, G3D::Array<bool>& button) const {
    MOJO_ASSERT(stickNum < ((unsigned int) m_joy.size()));

    ::SDL_Joystick* sdlstick = m_joy[stickNum];

    axis.resize(SDL_JoystickNumAxes(sdlstick), G3D::DONT_SHRINK_UNDERLYING_ARRAY);

    for (int a = 0; a < axis.size(); ++a) {
        axis[a] = SDL_JoystickGetAxis(sdlstick, a) / 32768.0;
    }

    button.resize(SDL_JoystickNumButtons(sdlstick), G3D::DONT_SHRINK_UNDERLYING_ARRAY);

    for (int b = 0; b < button.size(); ++b) {
        button[b] = (SDL_JoystickGetButton(sdlstick, b) != 0);
    }
}

std::string G3DWidget::caption() {
    return m_settings.caption;
}

void G3DWidget::setCaption(const std::string&) {
}

std::string G3DWidget::getAPIVersion() const {
    return QT_VERSION_STR;
}

std::string G3DWidget::getAPIName() const {
    return "Qt";
}

std::string G3DWidget::className() const {
    return "G3DWidget";
}

void G3DWidget::getDroppedFilenames(G3D::Array<std::string>& files) {
    files.clear();
    if (m_dropFileList.size() > 0) {
        files.append(m_dropFileList);
    }
}

void G3DWidget::setInputCapture(bool) {
    MOJO_ASSERT(0);
}

void G3DWidget::setFullPosition(int, int) {
    MOJO_ASSERT(0);
}

void G3DWidget::setClientPosition(int, int) {
    MOJO_ASSERT(0);
}

void G3DWidget::setGammaRamp(const G3D::Array<G3D::uint16>& gammaRamp) {
    MOJO_ASSERT(gammaRamp.size() >= 256 && "Gamma ramp must have at least 256 entries");

    uint16* ptr = const_cast<uint16*>(gammaRamp.getCArray());
    bool success = (SDL_SetGammaRamp(ptr, ptr, ptr) != -1);

    MOJO_ASSERT(success);
}

void G3DWidget::paintEvent(QPaintEvent*) {
}

void G3DWidget::resizeEvent(QResizeEvent* e) {
    if (m_initialized) {
        OSWindow::makeCurrent();
        m_g3dWidgetOpenGLContext->update();

        if (m_renderDevice != NULL) {
            handleResize(e->size().width(), e->size().height());
        }
    }
}

void G3DWidget::enterEvent(QEvent*) {
    if (m_mouseVisible) {
        setCursor(QCursor(Qt::ArrowCursor));
    } else {
        setCursor(QCursor(Qt::BlankCursor));
    }

    setFocus(Qt::OtherFocusReason);
}

void G3DWidget::leaveEvent(QEvent*) {
    setCursor(QCursor(Qt::ArrowCursor));
}

void G3DWidget::mouseMoveEvent(QMouseEvent* mouseEvent) {
    if (m_initialized) {
        G3D::GEvent e;
        e.motion.type  = G3D::GEventType::MOUSE_MOTION;
        e.motion.which = 0;
        e.motion.state = getG3DMouseButtonPressedFlags(mouseEvent->buttons());
        e.motion.x     = mouseEvent->x();
        e.motion.y     = mouseEvent->y();
        e.motion.xrel  = (mouseEvent->pos() - m_mousePrevPos).x();
        e.motion.yrel  = (mouseEvent->pos() - m_mousePrevPos).y();

        m_mousePrevPos = mouseEvent->pos();

        fireEvent(e);
    }
}

void G3DWidget::mousePressEvent(QMouseEvent* mouseEvent) {
    if (m_initialized) {
        G3D::GEvent e;
        e.button.type   = G3D::GEventType::MOUSE_BUTTON_DOWN;
        e.button.which  = 0;
        e.button.state  = G3D::GButtonState::PRESSED;
        e.button.x      = mouseEvent->x();
        e.button.y      = mouseEvent->y();
        e.button.button = getG3DMouseButtonPressedIndex(mouseEvent->buttons());

        m_mousePressEventButtons = mouseEvent->buttons();

        fireEvent(e);
    }
}

void G3DWidget::mouseReleaseEvent(QMouseEvent* mouseEvent) {
    if (m_initialized) {
        G3D::GEvent e;
        e.button.type   = G3D::GEventType::MOUSE_BUTTON_UP;
        e.button.which  = 0;
        e.button.state  = G3D::GButtonState::RELEASED;
        e.button.x      = mouseEvent->x();
        e.button.y      = mouseEvent->y();
        e.button.button = getG3DMouseButtonPressedIndex(m_mousePressEventButtons);

        m_mousePressEventButtons = Qt::NoButton;

        fireEvent(e);

        e.type             = G3D::GEventType::MOUSE_BUTTON_CLICK;
        e.button.numClicks = 1;

        fireEvent(e);
    }
}

void G3DWidget::dragEnterEvent(QDragEnterEvent* dragEnterEvent) {
    if (m_initialized) {
        if (dragEnterEvent->mimeData()->hasUrls()) {
            dragEnterEvent->acceptProposedAction();
        }
    }
}

void G3DWidget::dropEvent(QDropEvent* dropEvent) {
    if (m_initialized) {
        m_dropFileList.clear();
        Q_FOREACH(QUrl url, dropEvent->mimeData()->urls()) {
            m_dropFileList.append(url.toLocalFile().toStdString());
        }

        dropEvent->acceptProposedAction();

        G3D::GEvent e;

        e.drop.type = G3D::GEventType::FILE_DROP;
        e.drop.x    = dropEvent->pos().x();
        e.drop.y    = dropEvent->pos().y();

        fireEvent(e);
    }
}

void G3DWidget::keyPressEvent(QKeyEvent* k) {
    G3D::GEvent e;
    e.key.which = 0; //All keyboard events map to 0 currently
    e.key.type  = k->isAutoRepeat() ? G3D::GEventType::KEY_REPEAT : G3D::GEventType::KEY_DOWN;
    e.key.state = G3D::GButtonState::PRESSED;

    keyEvent(k, e);
    fireEvent(e);

    if(k->key() >= Qt::Key_Exclam && k->key() <= Qt::Key_AsciiTilde) {
        G3D::GEvent e;
        e.type = G3D::GEventType::CHAR_INPUT;

        if (k->text().toAscii().length()) {
            e.character.unicode = k->text().toAscii().at(0);
        } else {
            e.character.unicode = 0;
        }

        fireEvent(e);
    }
}

void G3DWidget::keyReleaseEvent(QKeyEvent* k) {
    if (!k->isAutoRepeat()) {
        G3D::GEvent e;
        e.key.which = 0; //All keyboard events map to 0 currently
        e.key.type  = G3D::GEventType::KEY_UP;
        e.key.state = G3D::GButtonState::RELEASED;

        keyEvent(k, e);

        fireEvent(e);
    }
}

std::string G3DWidget::_clipboardText() const {
    MOJO_RELEASE_ASSERT(m_initialized);
    return QApplication::clipboard()->text().toStdString();
}

void G3DWidget::_setClipboardText(const std::string& text) const {
    MOJO_RELEASE_ASSERT(m_initialized);
    QApplication::clipboard()->setText(text.c_str());
}

G3D::uint8 G3DWidget::getG3DMouseButtonPressedFlags(Qt::MouseButtons qtMouseButtons) const {
    G3D::uint8 g3dMouseButtonPressedFlags = 0;

    if (qtMouseButtons & Qt::LeftButton)
        g3dMouseButtonPressedFlags |= 1 << 0;
    if (qtMouseButtons & Qt::RightButton)
        g3dMouseButtonPressedFlags |= 1 << 2;
    if (qtMouseButtons & Qt::MiddleButton)
        g3dMouseButtonPressedFlags |= 1 << 1;
    if (qtMouseButtons & Qt::XButton1)
        g3dMouseButtonPressedFlags |= 1 << 3;
    if (qtMouseButtons & Qt::XButton2)
        g3dMouseButtonPressedFlags |= 1 << 4;

    return g3dMouseButtonPressedFlags;
}

G3D::uint8 G3DWidget::getG3DMouseButtonPressedIndex(Qt::MouseButtons qtMouseButtons) const {
    if (qtMouseButtons & Qt::LeftButton)
        return 0;
    if (qtMouseButtons & Qt::RightButton)
        return 2;
    if (qtMouseButtons & Qt::MiddleButton)
        return 1;
    if (qtMouseButtons & Qt::XButton1)
        return 3;
    if (qtMouseButtons & Qt::XButton2)
        return 4;

    MOJO_ASSERT(0 && "No mouse buttons have been pressed.");
    return 255;
}

void G3DWidget::keyEvent(QKeyEvent* keyEvent, G3D::GEvent& e) {

    switch(keyEvent->key()) {
    case Qt::Key_Escape:        e.key.keysym.sym = G3D::GKey::ESCAPE;     break;
    case Qt::Key_Enter:         e.key.keysym.sym = G3D::GKey::RETURN;     break;
    case Qt::Key_Return:        e.key.keysym.sym = G3D::GKey::RETURN;     break;
    case Qt::Key_Tab:           e.key.keysym.sym = G3D::GKey::TAB;        break;
    case Qt::Key_Backspace:     e.key.keysym.sym = G3D::GKey::BACKSPACE;  break;
    case Qt::Key_Insert:        e.key.keysym.sym = G3D::GKey::INSERT;     break;
    case Qt::Key_Delete:        e.key.keysym.sym = G3D::GKey::DELETE;     break;
    case Qt::Key_Right:         e.key.keysym.sym = G3D::GKey::RIGHT;      break;
    case Qt::Key_Left:          e.key.keysym.sym = G3D::GKey::LEFT;       break;
    case Qt::Key_Down:          e.key.keysym.sym = G3D::GKey::DOWN;       break;
    case Qt::Key_Up:            e.key.keysym.sym = G3D::GKey::UP;         break;
    case Qt::Key_PageUp:        e.key.keysym.sym = G3D::GKey::PAGEUP;     break;
    case Qt::Key_PageDown:      e.key.keysym.sym = G3D::GKey::PAGEDOWN;   break;
    case Qt::Key_Home:          e.key.keysym.sym = G3D::GKey::HOME;       break;
    case Qt::Key_End:           e.key.keysym.sym = G3D::GKey::END;        break;
    case Qt::Key_CapsLock:      e.key.keysym.sym = G3D::GKey::CAPSLOCK;   break;
    case Qt::Key_ScrollLock:    e.key.keysym.sym = G3D::GKey::SCROLLOCK;  break;
    case Qt::Key_NumLock:       e.key.keysym.sym = G3D::GKey::NUMLOCK;    break;
    case Qt::Key_Print:         e.key.keysym.sym = G3D::GKey::PRINT;      break;
    case Qt::Key_Pause:         e.key.keysym.sym = G3D::GKey::PAUSE;      break;
    case Qt::Key_Shift:         e.key.keysym.sym = G3D::GKey::LSHIFT;     break;
    case Qt::Key_Control:       e.key.keysym.sym = G3D::GKey::LCTRL;      break;
    case Qt::Key_Meta:          e.key.keysym.sym = G3D::GKey::LCTRL;      break;
    case Qt::Key_Alt:           e.key.keysym.sym = G3D::GKey::LALT;       break;
    case Qt::Key_Super_L:       e.key.keysym.sym = G3D::GKey::LSUPER;     break;
    case Qt::Key_Super_R:       e.key.keysym.sym = G3D::GKey::RSUPER;     break;
    case Qt::Key_Menu:          e.key.keysym.sym = G3D::GKey::MENU;       break;
    default:
        if(keyEvent->key() >= Qt::Key_A && keyEvent->key() <= Qt::Key_Z) {
            e.key.keysym.sym = (G3D::GKey::Value)(keyEvent->key() + 'a' - 'A');
        } else if(keyEvent->key() >= Qt::Key_Exclam && keyEvent->key() <= Qt::Key_AsciiTilde) {
            e.key.keysym.sym = (G3D::GKey::Value)keyEvent->key();
        } else if (keyEvent->key() >= Qt::Key_F1 && keyEvent->key() <= Qt::Key_F15) {
            e.key.keysym.sym = (G3D::GKey::Value)(keyEvent->key() - Qt::Key_F1 + G3D::GKey::F1);
        } else {
            MOJO_ASSERT(0 && "Unsupported key.");
        }
    }

    if (keyEvent->modifiers() & Qt::KeypadModifier) {
        switch(keyEvent->key()) {
        case Qt::Key_Right:         e.key.keysym.sym = G3D::GKey::RIGHT;       break;
        case Qt::Key_Left:          e.key.keysym.sym = G3D::GKey::LEFT;        break;
        case Qt::Key_Down:          e.key.keysym.sym = G3D::GKey::DOWN;        break;
        case Qt::Key_Up:            e.key.keysym.sym = G3D::GKey::UP;          break;
        case Qt::Key_Period:        e.key.keysym.sym = G3D::GKey::KP_PERIOD;   break;
        case Qt::Key_Slash:         e.key.keysym.sym = G3D::GKey::KP_DIVIDE;   break;
        case Qt::Key_Asterisk:      e.key.keysym.sym = G3D::GKey::KP_MULTIPLY; break;
        case Qt::Key_Minus:         e.key.keysym.sym = G3D::GKey::KP_MINUS;    break;
        case Qt::Key_Plus:          e.key.keysym.sym = G3D::GKey::KP_PLUS;     break;
        case Qt::Key_Enter:         e.key.keysym.sym = G3D::GKey::KP_ENTER;    break;
        case Qt::Key_Equal:         e.key.keysym.sym = G3D::GKey::KP_EQUALS;   break;
        default:
            if (keyEvent->key() >= Qt::Key_0 && keyEvent->key() <= Qt::Key_9) {
                e.key.keysym.sym = (G3D::GKey::Value)((keyEvent->key() - Qt::Key_0) + G3D::GKey::KP0);
            }
        }
    }

    G3D::GKeyMod::Value mod = (G3D::GKeyMod::Value)0;
    if (keyEvent->modifiers() & Qt::ShiftModifier) {
        mod = (G3D::GKeyMod::Value)(mod | G3D::GKeyMod::LSHIFT);
    }
    if (keyEvent->modifiers() & Qt::ControlModifier) {
        mod = (G3D::GKeyMod::Value)(mod | G3D::GKeyMod::LCTRL);
    }
    if (keyEvent->modifiers() & Qt::AltModifier) {
        mod = (G3D::GKeyMod::Value)(mod | G3D::GKeyMod::LALT);
    }
    e.key.keysym.mod = mod;

    //TODO: Properly set e.key.keysym.unicode
    e.key.keysym.unicode = keyEvent->key();

    //TODO: Properly set e.key.keysym.scancode
    e.key.keysym.scancode = keyEvent->key();
}
}
