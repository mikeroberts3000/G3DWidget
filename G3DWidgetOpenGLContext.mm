#import "G3DWidgetOpenGLContext.hpp"

#import <Cocoa/Cocoa.h>

#import <OpenGL/gl.h>

#include <SDL/SDL.h>
#ifdef main
#undef main
#endif

#import "Assert.hpp"

namespace mojo
{

G3DWidgetOpenGLContext::G3DWidgetOpenGLContext(const G3D::OSWindow::Settings& settings) :
    m_nsOpenGLContext(nil) {
    G3D::Array<NSOpenGLPixelFormatAttribute> nsOpenGLPixelFormatAttributes;

    nsOpenGLPixelFormatAttributes.append(NSOpenGLPFADoubleBuffer);

    if (settings.stereo) {
        nsOpenGLPixelFormatAttributes.append(NSOpenGLPFAStereo);
    }

    nsOpenGLPixelFormatAttributes.append(NSOpenGLPFAColorSize,   settings.rgbBits + settings.alphaBits);
    nsOpenGLPixelFormatAttributes.append(NSOpenGLPFAAlphaSize,   settings.alphaBits);
    nsOpenGLPixelFormatAttributes.append(NSOpenGLPFADepthSize,   settings.depthBits);
    nsOpenGLPixelFormatAttributes.append(NSOpenGLPFAStencilSize, settings.stencilBits);

    if (settings.msaaSamples > 1) {
        nsOpenGLPixelFormatAttributes.append(NSOpenGLPFASampleBuffers, 1);
        nsOpenGLPixelFormatAttributes.append(NSOpenGLPFASamples, settings.msaaSamples);
    }

    nsOpenGLPixelFormatAttributes.append(NSOpenGLPFANoRecovery);

    if (settings.hardware) {
        nsOpenGLPixelFormatAttributes.append(NSOpenGLPFAAccelerated);
    }

    nsOpenGLPixelFormatAttributes.append(NSOpenGLPFAWindow);
    nsOpenGLPixelFormatAttributes.append(0);

    NSOpenGLPixelFormat* nsOpenGLPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes: nsOpenGLPixelFormatAttributes.getCArray()];
    MOJO_RELEASE_ASSERT(nsOpenGLPixelFormat != nil);

    m_nsOpenGLContext = [[NSOpenGLContext alloc] initWithFormat: nsOpenGLPixelFormat shareContext: nil];
    MOJO_RELEASE_ASSERT(m_nsOpenGLContext != nil);

    [nsOpenGLPixelFormat release];
    nsOpenGLPixelFormat = nil;

    GLint swapInterval = settings.asynchronous ? 0 : 1;
    [m_nsOpenGLContext setValues: &swapInterval forParameter:NSOpenGLCPSwapInterval];

    int depthBits, stencilBits, redBits, greenBits, blueBits, alphaBits;
    glGetIntegerv(GL_DEPTH_BITS,   &depthBits);
    glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
    glGetIntegerv(GL_RED_BITS,     &redBits);
    glGetIntegerv(GL_GREEN_BITS,   &greenBits);
    glGetIntegerv(GL_BLUE_BITS,    &blueBits);
    glGetIntegerv(GL_ALPHA_BITS,   &alphaBits);

    m_settings.allowMaximize       = false;
    m_settings.alphaBits           = alphaBits;
    m_settings.asynchronous        = settings.asynchronous;
    m_settings.caption             = "";
    m_settings.center              = false;
    m_settings.defaultIconFilename = "";
    m_settings.depthBits           = depthBits;
    m_settings.framed              = false;
    m_settings.fullScreen          = false;
    m_settings.hardware            = settings.hardware;
    m_settings.height              = -1;
    m_settings.msaaSamples         = settings.msaaSamples > 1 ? settings.msaaSamples : 0;
    m_settings.refreshRate         = -1;
    m_settings.resizable           = true;
    m_settings.rgbBits             = redBits + greenBits + blueBits;
    m_settings.sharedContext       = false;
    m_settings.stencilBits         = stencilBits;
    m_settings.stereo              = settings.stereo;
    m_settings.visible             = false;
    m_settings.width               = -1;
    m_settings.x                   = -1;
    m_settings.y                   = -1;

    int error = SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_VIDEO);
    MOJO_RELEASE_ASSERT(error == 0);
}

G3DWidgetOpenGLContext::~G3DWidgetOpenGLContext() {
    MOJO_RELEASE_ASSERT(m_nsOpenGLContext != nil);

    SDL_Quit();

    [m_nsOpenGLContext release];
    m_nsOpenGLContext = nil;
}

void G3DWidgetOpenGLContext::getSettings(G3D::OSWindow::Settings &settings) const {
    settings = m_settings;
}

void G3DWidgetOpenGLContext::setView(WId winId) {
    MOJO_RELEASE_ASSERT(m_nsOpenGLContext != nil);

    [m_nsOpenGLContext setView: (NSView*)winId];
}

void G3DWidgetOpenGLContext::update() {
    MOJO_RELEASE_ASSERT(m_nsOpenGLContext != nil);

    [m_nsOpenGLContext update];
}

void G3DWidgetOpenGLContext::makeCurrent() {
    MOJO_RELEASE_ASSERT(m_nsOpenGLContext != nil);

    [m_nsOpenGLContext makeCurrentContext];
}

void G3DWidgetOpenGLContext::flushBuffer() {
    MOJO_RELEASE_ASSERT(m_nsOpenGLContext != nil);

    [m_nsOpenGLContext flushBuffer];
}

}
