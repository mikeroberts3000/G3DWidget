/**
 \file GLG3D.lib/source/RenderDevice.cpp
 
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 \created 2001-07-08
 \edited  2013-07-12
 */

#include "G3D/platform.h"
#include "G3D/Image.h"
#include "G3D/Log.h"
#include "G3D/FileSystem.h"
#include "G3D/fileutils.h"
#include "G3D/CPUPixelTransferBuffer.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Texture.h"
#include "GLG3D/getOpenGLState.h"
#include "GLG3D/VertexBuffer.h"
#include "GLG3D/AttributeArray.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/ShadowMap.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/Draw.h"
#include "GLG3D/GApp.h" // for screenPrintf
#include "GLG3D/Shader.h"
#include "GLG3D/Args.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/Milestone.h"
#include "GLG3D/GLPixelTransferBuffer.h"

namespace G3D {

__thread RenderDevice* RenderDevice::current = NULL;
std::string RenderDevice::dummyString;

static GLenum toGLBlendFunc(RenderDevice::BlendFunc b) {
    debugAssert(b != RenderDevice::BLEND_CURRENT);
    return GLenum(b);
}


static void _glViewport(double a, double b, double c, double d) {
    glViewport(iRound(a), iRound(b), 
           iRound(a + c) - iRound(a), iRound(b + d) - iRound(b));
}


static GLenum primitiveToGLenum(PrimitiveType primitive) {
    return GLenum(primitive);
}


const std::string& RenderDevice::getCardDescription() const {
    return cardDescription;
}


void RenderDevice::beginOpenGL() {
    debugAssert(! m_inRawOpenGL);

    beforePrimitive();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    debugAssertGLOk();

    m_inRawOpenGL = true;
}


void RenderDevice::endOpenGL() {
    debugAssert(m_inRawOpenGL);
    m_inRawOpenGL = false;

    glPopClientAttrib();
    glPopAttrib();

    afterPrimitive();    
}


RenderDevice::RenderDevice() :
    m_window(NULL), m_deleteWindow(false),  m_minLineWidth(0),
    m_inRawOpenGL(false), m_inIndexedPrimitive(false) {
    m_initialized = false;
    m_cleanedup = false;
    m_inPrimitive = false;
    m_numTextureUnits = 0;
    m_numTextures = 0;
    m_numTextureCoords = 0;
    m_lastTime = System::time();

    memset(currentlyBoundTextures, 0, sizeof(currentlyBoundTextures));

    current = this;
}


RenderDevice::~RenderDevice() {
    debugAssertM(m_cleanedup || !initialized(), "You deleted an initialized RenderDevice without calling RenderDevice::cleanup()");
}

/*
static void APIENTRY debugCallback(GLenum source,
                    GLenum type,
                    GLuint id,
                    GLenum severity,
                    GLsizei length,
                    const GLchar* message,
                    void* userParam) {
    debugPrintf("-----GL DEBUG INFO-----\nSource: %s\nType: %s\nID: %d\nSeverity: %s\nMessage: %s\n-----------------------\n", 
        GLenumToString(source), GLenumToString(type), id, GLenumToString(severity), message);
}

void RenderDevice::setDebugOutput(bool b) {
     if (b) {
        glEnable(GL_DEBUG_OUTPUT);
        //glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
        glDebugMessageCallback(debugCallback, NULL);
    } else {
        glDisable(GL_DEBUG_OUTPUT);
    }
 }
*/


/**
 Used by RenderDevice::init.
 */
static const char* isOk(bool x) {
    return x ? "ok" : "UNSUPPORTED";
}

/**
 Used by RenderDevice::init.
 */
static const char* isOk(void* x) {
    // GCC incorrectly claims this function is not called.
    return isOk(x != NULL);
}


void RenderDevice::init
   (const OSWindow::Settings&      _settings) {

    m_deleteWindow = true;
    init(OSWindow::create(_settings));
}


OSWindow* RenderDevice::window() const {
    return m_window;
}

void RenderDevice::setWindow(OSWindow* window) {
    debugAssert(initialized());
    debugAssert(window);
    debugAssert(window->m_renderDevice == this);

    m_window = window;
}

void RenderDevice::init(OSWindow* window) {
    debugAssert(! initialized());
    debugAssert(window);
    debugAssertM(glGetInteger(GL_PIXEL_PACK_BUFFER_BINDING) == GL_NONE, "GL_PIXEL_PACK_BUFFER unexpectedly bound");

    m_swapBuffersAutomatically = true;
    m_swapGLBuffersPending = false;
    m_window = window;

    OSWindow::Settings settings;
    window->getSettings(settings);

    m_beginEndFrame = 0;

    // Under Windows, reset the last error so that our debug box
    // gives the correct results
    #ifdef G3D_WINDOWS
        SetLastError(0);
    #endif

    const int minimumDepthBits    = iMin(16, settings.depthBits);
    const int desiredDepthBits    = settings.depthBits;

    const int minimumStencilBits  = settings.stencilBits;
    const int desiredStencilBits  = settings.stencilBits;

    // Don't use more texture units than allowed at compile time.
    m_numTextureUnits  = iMin(GLCaps::numTextureUnits(), MAX_TRACKED_TEXTURE_UNITS);
    m_numTextureCoords = iMin(GLCaps::numTextureCoords(), MAX_TRACKED_TEXTURE_UNITS);
    m_numTextures      = iMin(GLCaps::numTextures(), MAX_TRACKED_TEXTURE_IMAGE_UNITS);

    debugAssertGLOk();

    logPrintf("Setting video mode\n");

    setVideoMode();

    if (! strcmp((char*)glGetString(GL_RENDERER), "GDI Generic")) {
        logPrintf(
         "\n*********************************************************\n"
           "* WARNING: This computer does not have correctly        *\n"
           "*          installed graphics drivers and is using      *\n"
           "*          the default Microsoft OpenGL implementation. *\n"
           "*          Most graphics capabilities are disabled.  To *\n"
           "*          correct this problem, download and install   *\n"
           "*          the latest drivers for the graphics card.    *\n"
           "*********************************************************\n\n");
    }

    glViewport(0, 0, width(), height());
    int depthBits, stencilBits, redBits, greenBits, blueBits, alphaBits;
    depthBits       = glGetInteger(GL_DEPTH_BITS);
    stencilBits     = glGetInteger(GL_STENCIL_BITS);
    redBits         = glGetInteger(GL_RED_BITS);
    greenBits       = glGetInteger(GL_GREEN_BITS);
    blueBits        = glGetInteger(GL_BLUE_BITS);
    alphaBits       = glGetInteger(GL_ALPHA_BITS);
    debugAssertGLOk();

    bool depthOk   = depthBits >= minimumDepthBits;
    bool stencilOk = stencilBits >= minimumStencilBits;

    cardDescription = GLCaps::renderer() + " " + GLCaps::driverVersion();

    {
        if (GLCaps::supports_GL_ARB_multitexture()) {
            glGetInteger(GL_MAX_TEXTURE_UNITS_ARB);
        }

        if (GLCaps::supports_GL_ARB_fragment_program()) {
            glGetInteger(GL_MAX_TEXTURE_IMAGE_UNITS_ARB);
        }
                
        // Test which texture and render buffer formats are supported by this card
        logLazyPrintf("Supported Formats:\n");
        logLazyPrintf("%20s  %s %s %s\n", "Format", "Texture", "RenderBuffer", "Can bind Texture as render target");
        for (int code = 0; code < ImageFormat::CODE_NUM; ++code) {
            if ((code == ImageFormat::CODE_DEPTH24_STENCIL8) && 
                (GLCaps::enumVendor() == GLCaps::MESA)) {
                // Mesa crashes on this format
                continue;
            }

            const ImageFormat* fmt = 
                ImageFormat::fromCode((ImageFormat::Code)code);
            
            if (fmt) {
                bool t = GLCaps::supportsTexture(fmt);
                bool r = GLCaps::supportsRenderBuffer(fmt);
                bool d = GLCaps::supportsTextureDrawBuffer(fmt); //TODO Mike
                logLazyPrintf("%20s  %s       %s         %s\n", 
                              fmt->name().c_str(), 
                              t ? "Yes" : "No ", 
                              r ? "Yes" : "No ", 
                              d ? "Yes" : "No ");
            }
        }
        logLazyPrintf("\n");
    
        OSWindow::Settings actualSettings;
        window->getSettings(actualSettings);

        // This call is here to make GCC realize that isOk is used.
        (void)isOk(false);
        (void)isOk((void*)NULL);

        logLazyPrintf(
                 "Capability    Minimum   Desired   Received  Ok?\n"
                 "-------------------------------------------------\n"
                 "* RENDER DEVICE \n"
                 "Depth       %4d bits %4d bits %4d bits   %s\n"
                 "Stencil     %4d bits %4d bits %4d bits   %s\n"
                 "Alpha                           %4d bits   %s\n"
                 "Red                             %4d bits   %s\n"
                 "Green                           %4d bits   %s\n"
                 "Blue                            %4d bits   %s\n"
                 "FSAA                      %2d    %2d    %s\n"

                 "Width             %8d pixels           %s\n"
                 "Height            %8d pixels           %s\n"
                 "Mode                 %10s             %s\n\n",

                 minimumDepthBits, desiredDepthBits, depthBits, isOk(depthOk),
                 minimumStencilBits, desiredStencilBits, stencilBits, isOk(stencilOk),

                 alphaBits, "ok",
                 redBits, "ok", 
                 greenBits, "ok", 
                 blueBits, "ok", 

                 settings.msaaSamples, actualSettings.msaaSamples,
                 isOk(settings.msaaSamples == actualSettings.msaaSamples),

                 settings.width, "ok",
                 settings.height, "ok",
                 (settings.fullScreen ? "Fullscreen" : "Windowed"), "ok"
                 );

        std::string e;
        bool s = GLCaps::supportsG3D9(e);
        logLazyPrintf("This driver will%s support G3D 9.00:\n%s\n\n",
                      s ? "" : " NOT", e.c_str());
        logPrintf("Done initializing RenderDevice.\n"); 
    }
    m_initialized = true;

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    m_window->m_renderDevice = this;
    debugAssertGLOk();
}


void RenderDevice::describeSystem
    (std::string&        s) {
    
    TextOutput t;
    describeSystem(t);
    t.commitString(s);
}


bool RenderDevice::initialized() const {
    return m_initialized;
}


#ifdef G3D_WINDOWS

HDC RenderDevice::getWindowHDC() const {
    return wglGetCurrentDC();
}

#endif


void RenderDevice::setVideoMode() {

    debugAssertM(m_stateStack.size() == 0, 
                 "Cannot call setVideoMode between pushState and popState");
    debugAssertM(m_beginEndFrame == 0, 
                 "Cannot call setVideoMode between beginFrame and endFrame");

    // Reset all state

    OSWindow::Settings settings;
    m_window->getSettings(settings);

    // Set the refresh rate
#   ifdef G3D_WINDOWS
        if (settings.asynchronous) {
            logLazyPrintf("wglSwapIntervalEXT(0);\n");
            wglSwapIntervalEXT(0);
        } else {
            logLazyPrintf("wglSwapIntervalEXT(1);\n");
            wglSwapIntervalEXT(1);
        }
#   endif

    // Enable proper specular lighting
    if (GLCaps::supports("GL_EXT_separate_specular_color")) {
        logLazyPrintf("Enabling separate specular lighting.\n");
        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT, 
                      GL_SEPARATE_SPECULAR_COLOR_EXT);
        debugAssertGLOk();
    } else {
        logLazyPrintf("Cannot enable separate specular lighting, extension not supported.\n");
    }

    // Make sure we use good interpolation.
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    // Smoothing by default on NVIDIA cards.  On ATI cards
    // there is a bug that causes shaders to generate incorrect
    // results and run at 0 fps, so we can't enable this.
    if (! beginsWith(GLCaps::vendor(), "ATI")) {
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_POINT_SMOOTH);
    }
    
    // glHint(GL_GENERATE_MIPMAP_HINT_EXT, GL_NICEST);
    if (GLCaps::supports("GL_ARB_multisample")) {
        glEnable(GL_MULTISAMPLE_ARB);
    }

    debugAssertGLOk();
    if (GLCaps::supports("GL_NV_multisample_filter_hint")) {
        glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
    }

    resetState();

    logPrintf("Done setting initial state.\n");
}


int RenderDevice::width() const {
    if (isNull(m_state.drawFramebuffer)) {
        return m_window->width();
    } else {
        return m_state.drawFramebuffer->width();
    }
}


int RenderDevice::height() const {
    if (isNull(m_state.drawFramebuffer)) {
        return m_window->height();
    } else {
        return m_state.drawFramebuffer->height();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////


Vector4 RenderDevice::project(const Vector3& v) const {
    return project(Vector4(v, 1));
}


Vector4 RenderDevice::project(const Vector4& v) const {
    Matrix4 M = modelViewProjectionMatrix();

    Vector4 result = M * v;
    
    const Rect2D& view = viewport();

    // Homogeneous divide
    const double rhw = 1.0 / result.w;

    const float depthRange[2] = {0.0f, 1.0f};

    return Vector4(
        (1.0f + result.x * (float)rhw) * view.width() / 2.0f + view.x0(),
        (1.0f + result.y * (float)rhw) * view.height() / 2.0f + view.y0(),
        (result.z * (float)rhw) * (depthRange[1] - depthRange[0]) + depthRange[0],
        (float)rhw);
}


void RenderDevice::cleanup() {
    debugAssert(initialized());

    logLazyPrintf("Shutting down RenderDevice.\n");

    logPrintf("Freeing all AttributeArray memory\n");

    if (m_deleteWindow) {
        logPrintf("Deleting window.\n");
        VertexBuffer::cleanupAllVertexBuffers();
        delete m_window;
        m_window = NULL;
    }

    m_cleanedup = true;
}


void RenderDevice::push2D() {
    push2D(viewport());
}


void RenderDevice::push2D(const Rect2D& viewport) {
    push2D(m_state.drawFramebuffer, viewport);
}


void RenderDevice::push2D(const shared_ptr<Framebuffer>& fb) {
    const Rect2D& viewport = 
        (fb && (fb->width() > 0)) ? 
            fb->rect2DBounds() : 
            Rect2D::xywh(0.0f, 0.0f, (float)m_window->width(), (float)m_window->height());
    push2D(fb, viewport);
}


void RenderDevice::push2D(const shared_ptr<Framebuffer>& fb, const Rect2D& viewport) {
    pushState(fb);
    setDepthWrite(false);
    setDepthTest(DEPTH_ALWAYS_PASS);
    setCullFace(CullFace::NONE);
    setViewport(viewport);
    setObjectToWorldMatrix(CoordinateFrame());
    setCameraToWorldMatrix(CoordinateFrame());

    setProjectionMatrix(Matrix4::orthogonalProjection(viewport.x0(), viewport.x0() + viewport.width(), 
                                                      viewport.y0() + viewport.height(), viewport.y0(), -1, 1));

    // Workaround for a bug where setting the draw buffer alone is not
    // enough, or where the order of setting causes problems.
    setFramebuffer(fb);
}


void RenderDevice::pop2D() {
    popState();
}

////////////////////////////////////////////////////////////////////////////////////////////////
RenderDevice::RenderState::RenderState(int width, int height, int htutc) :

    // WARNING: this must be kept in sync with the initialization code
    // in init();
    viewport(Rect2D::xywh(0.0f, 0.0f, (float)width, (float)height)),
    clip2D(Rect2D::inf()),
    useClip2D(false),

    depthWrite(true),
    colorWrite(true),
    alphaWrite(true),

    depthTest(DEPTH_LEQUAL),
    alphaTest(ALPHA_ALWAYS_PASS),
    alphaReference(0.0),
    
    sRGBConversion(false) {

    drawFramebuffer.reset();
    readFramebuffer.reset();

    srcBlendFunc                = BLEND_ONE;
    dstBlendFunc                = BLEND_ZERO;
    blendEq                     = BLENDEQ_ADD;

    drawBuffer                  = DRAW_BACK;
    readBuffer                  = READ_BACK;

    stencil.stencilTest         = STENCIL_ALWAYS_PASS;
    stencil.stencilReference    = 0;
    stencil.frontStencilFail    = STENCIL_KEEP;
    stencil.frontStencilZFail   = STENCIL_KEEP;
    stencil.frontStencilZPass   = STENCIL_KEEP;
    stencil.backStencilFail     = STENCIL_KEEP;
    stencil.backStencilZFail    = STENCIL_KEEP;
    stencil.backStencilZPass    = STENCIL_KEEP;

    logicOp                     = LOGIC_COPY;

    polygonOffset               = 0;
    lineWidth                   = 1;
    pointSize                   = 1;

    renderMode                  = RenderDevice::RENDER_SOLID;

    shininess                   = 15;
    specular                    = Color3::white() * 0.8f;

    color                       = Color4(1,1,1,1);
    normal                      = Vector3(0,0,0);

    // Note: texture units and lights initialize themselves

    matrices.objectToWorldMatrix         = CoordinateFrame();
    matrices.cameraToWorldMatrix         = CoordinateFrame();
    matrices.cameraToWorldMatrixInverse  = CoordinateFrame();
    matrices.invertY                     = true;

    stencil.stencilClear        = 0;
    depthClear                  = 1;
    colorClear                  = Color4(0,0,0,1);

    shadeMode                   = SHADE_FLAT;

    // Set projection matrix
    const double aspect = (double)viewport.width() / viewport.height();

    matrices.projectionMatrix = Matrix4::perspectiveProjection(-aspect, aspect, -1, 1, 0.1, 100.0);

    cullFace                    = CullFace::BACK;
    
    lowDepthRange               = 0;
    highDepthRange              = 1;

    highestTextureUnitThatChanged = htutc;
}


RenderDevice::RenderState::TextureUnit::TextureUnit() : LODBias(0) {
    texCoord        = Vector4(0,0,0,1);
    combineMode     = TEX_MODULATE;

    // Identity texture matrix
    memset(textureMatrix, 0, sizeof(float) * 16);
    for (int i = 0; i < 4; ++i) {
        textureMatrix[i + i * 4] = (float)1.0;
    }
}


RenderDevice::RenderState::TextureImageUnit::TextureImageUnit() {
}


void RenderDevice::resetState() {
    m_state = RenderState(width(), height());

    glClearDepth(1.0);

    glEnable(GL_NORMALIZE);

    debugAssertGLOk();
    if (GLCaps::supports_GL_EXT_stencil_two_side()) {
        glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
    }

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    // Compute specular term correctly
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    debugAssertGLOk();

    logPrintf("Setting initial rendering state.\n");
    glDisable(GL_LIGHT0);
    debugAssertGLOk();
    {
        // WARNING: this must be kept in sync with the 
        // RenderState constructor
        m_state = RenderState(width(), height(), iMax(MAX_TRACKED_TEXTURE_UNITS, MAX_TRACKED_TEXTURE_IMAGE_UNITS) - 1);

        _glViewport(m_state.viewport.x0(), m_state.viewport.y0(), m_state.viewport.width(), m_state.viewport.height());
        glDepthMask(GL_TRUE);
        glColorMask(1,1,1,1);

        if (GLCaps::supports_GL_EXT_stencil_two_side()) {
            glActiveStencilFaceEXT(GL_BACK);
        }
        for (int i = 0; i < 2; ++i) {
            glStencilMask((GLuint)~0);
            glDisable(GL_STENCIL_TEST);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glStencilFunc(GL_ALWAYS, 0, 0xFFFFFFFF);
            glDisable(GL_ALPHA_TEST);
            if (GLCaps::supports_GL_EXT_stencil_two_side()) {
                glActiveStencilFaceEXT(GL_FRONT);
            }
        }

        glLogicOp(GL_COPY);

        glDepthFunc(GL_LEQUAL);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);

        glDisable(GL_BLEND);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glLineWidth(1);
        glPointSize(1);

        glDisable(GL_LIGHTING);

        glDrawBuffer(GL_BACK);
        glReadBuffer(GL_BACK);

        glColor4d(1, 1, 1, 1);
        glNormal3d(0, 0, 0);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        setShininess(m_state.shininess);
        setGlossyCoefficient(m_state.specular);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glShadeModel(GL_FLAT);

        glClearStencil(0);
        glClearDepth(1);
        glClearColor(0,0,0,1);
        glMatrixMode(GL_PROJECTION);
        glLoadMatrix(m_state.matrices.projectionMatrix);
        glMatrixMode(GL_MODELVIEW);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glDisable(GL_FRAMEBUFFER_SRGB);

        glDepthRange(0, 1);

        // Set up the texture units.
        if (glMultiTexCoord4fvARB != NULL) {
            for (int t = m_numTextureCoords - 1; t >= 0; --t) {
                float f[] = {0,0,0,1};
                glMultiTexCoord4fvARB(GL_TEXTURE0_ARB + t, f);
            }
        } else if (m_numTextureCoords > 0) {
            glTexCoord(Vector4(0,0,0,1));
        }

        if (glActiveTextureARB != NULL) {
            glActiveTextureARB(GL_TEXTURE0_ARB);
        }
    }
    debugAssertGLOk();

}


bool RenderDevice::RenderState::Stencil::operator==(const Stencil& other) const {
    return 
        (stencilTest == other.stencilTest) &&
        (stencilReference == other.stencilReference) &&
        (stencilClear == other.stencilClear) &&
        (frontStencilFail == other.frontStencilFail) &&
        (frontStencilZFail == other.frontStencilZFail) &&
        (frontStencilZPass == other.frontStencilZPass) &&
        (backStencilFail == other.backStencilFail) &&
        (backStencilZFail == other.backStencilZFail) &&
        (backStencilZPass == other.backStencilZPass);
}


bool RenderDevice::RenderState::Matrices::operator==(const Matrices& other) const {
    return
        (objectToWorldMatrix == other.objectToWorldMatrix) &&
        (cameraToWorldMatrix == other.cameraToWorldMatrix) &&
        (projectionMatrix == other.projectionMatrix) &&
        (invertY == other.invertY);
}


bool RenderDevice::invertY() const {
    return m_state.matrices.invertY;
}


const Matrix4& RenderDevice::invertYMatrix() const {
    if (invertY()) {
        // "Normal OpenGL"
        static Matrix4 M(1,  0, 0, 0,
                       0, -1, 0, 0,
                       0,  0, 1, 0,
                       0,  0, 0, 1); 
        return M;
    } else {
        // "G3D"
        return Matrix4::identity();
    }
}


void RenderDevice::setState(
    const RenderState&          newState) {
    // The state change checks inside the individual
    // methods will (for the most part) minimize
    // the state changes so we can set all of the
    // new state explicitly.
    
    // Set framebuffer first, since it can affect the viewport
    if (m_state.drawFramebuffer != newState.drawFramebuffer) {
        setDrawFramebuffer(newState.drawFramebuffer);
        
        // Intentionally corrupt the viewport, forcing renderdevice to
        // reset it below.
        m_state.viewport = Rect2D::xywh(-1, -1, -1, -1);
    }

    if (m_state.readFramebuffer != newState.readFramebuffer) {
        setReadFramebuffer(newState.readFramebuffer);
    }

    if (m_state.readFramebuffer != newState.readFramebuffer) {
        setReadFramebuffer(newState.readFramebuffer);
    }
    
    setViewport(newState.viewport);

    if (newState.useClip2D) {
        setClip2D(newState.clip2D);
    } else {
        setClip2D(Rect2D::inf());
    }
    
    setDepthWrite(newState.depthWrite);
    setColorWrite(newState.colorWrite);
    setAlphaWrite(newState.alphaWrite);

    setDrawBuffer(newState.drawBuffer);
    setReadBuffer(newState.readBuffer);

    setShadeMode(newState.shadeMode);
    setDepthTest(newState.depthTest);

    if (newState.stencil != m_state.stencil) {
        setStencilConstant(newState.stencil.stencilReference);

        setStencilTest(newState.stencil.stencilTest);

        setStencilOp(
            newState.stencil.frontStencilFail, newState.stencil.frontStencilZFail, newState.stencil.frontStencilZPass,
            newState.stencil.backStencilFail, newState.stencil.backStencilZFail, newState.stencil.backStencilZPass);

        setStencilClearValue(newState.stencil.stencilClear);
    }

    setDepthClearValue(newState.depthClear);

    setColorClearValue(newState.colorClear);

    setAlphaTest(newState.alphaTest, newState.alphaReference);

    setBlendFunc(newState.srcBlendFunc, newState.dstBlendFunc, newState.blendEq);

    setRenderMode(newState.renderMode);
    setPolygonOffset(newState.polygonOffset);
    setLineWidth(newState.lineWidth);
    setPointSize(newState.pointSize);

    setGlossyCoefficient(newState.specular);
    setShininess(newState.shininess);
    
    setColor(newState.color);
    setNormal(newState.normal);

    for (int u = m_state.highestTextureUnitThatChanged; u >= 0; --u) {

        if (u < m_numTextures) {

            if (newState.textureImageUnits[u] != m_state.textureImageUnits[u]) {
                setTexture(u, newState.textureImageUnits[u].texture);
            }
        }

        if (u < MAX_TRACKED_TEXTURE_UNITS) {

            if (newState.textureUnits[u] != m_state.textureUnits[u]) {

                // Only revert valid texture units
                if (u < m_numTextureUnits) {
                    setTextureCombineMode(u, newState.textureUnits[u].combineMode);
                    setTextureMatrix(u, newState.textureUnits[u].textureMatrix);

                    setTextureLODBias(u, newState.textureUnits[u].LODBias);
                }

                // Only revert valid texture coords
                if (u < m_numTextureCoords) {
                    setTexCoord(u, newState.textureUnits[u].texCoord);            
                }
            }
        }
    }

    setCullFace(newState.cullFace);

    setSRGBConversion(newState.sRGBConversion);

    setDepthRange(newState.lowDepthRange, newState.highDepthRange);

    if (m_state.matrices.changed) { //(newState.matrices != m_state.matrices) { 
        if (newState.matrices.cameraToWorldMatrix != m_state.matrices.cameraToWorldMatrix) {
            setCameraToWorldMatrix(newState.matrices.cameraToWorldMatrix);
        }

        if (newState.matrices.objectToWorldMatrix != m_state.matrices.objectToWorldMatrix) {
            setObjectToWorldMatrix(newState.matrices.objectToWorldMatrix);
        }

        setProjectionMatrix(newState.matrices.projectionMatrix);
    }

    
    // Adopt the popped state's deltas relative the state that it replaced.
    m_state.highestTextureUnitThatChanged = newState.highestTextureUnitThatChanged;
    m_state.matrices.changed = newState.matrices.changed;
}


void RenderDevice::syncDrawBuffer(bool alreadyBound) {
    if (isNull(m_state.drawFramebuffer)) {
        return;
    }

    if (m_state.drawFramebuffer->bind(alreadyBound, Framebuffer::MODE_DRAW)) {
       
        // Apply the bindings from this framebuffer
        const Array<GLenum>& array = m_state.drawFramebuffer->openGLDrawArray();
        if (array.size() > 0) {
            debugAssertM(array.size() <= glGetInteger(GL_MAX_DRAW_BUFFERS),
                         format("This graphics card only supports %d draw buffers.",
                                glGetInteger(GL_MAX_DRAW_BUFFERS)));
            
            glDrawBuffersARB(array.size(), array.getCArray());
        } else {
            // May be only depth or stencil; don't need a draw buffer.
            
            // Some drivers crash when providing NULL or an actual
            // zero-element array for a zero-element array, so make a fake
            // array.
            const GLenum noColorBuffers[] = { GL_NONE };
            glDrawBuffersARB(1, noColorBuffers);
        }
    }
}


static GLenum toFBOReadBuffer(RenderDevice::ReadBuffer b, const shared_ptr<Framebuffer>& fbo) {

    switch (b) {
    case RenderDevice::READ_FRONT:
    case RenderDevice::READ_BACK:
    case RenderDevice::READ_FRONT_LEFT:
    case RenderDevice::READ_FRONT_RIGHT:
    case RenderDevice::READ_BACK_LEFT:
    case RenderDevice::READ_BACK_RIGHT:
    case RenderDevice::READ_LEFT:
    case RenderDevice::READ_RIGHT:
        if (fbo->has(Framebuffer::COLOR0)) {
            return GL_COLOR_ATTACHMENT0;
        } else {
            return GL_NONE;
        }
        
    default:
        // The specification and various pieces of documentation are
        // unclear on what arguments glReadBuffer may have.
        // http://www.opengl.org/wiki/Framebuffer_Object#Multiple_Render_Targets
        // and the extension specification indicate that COLOR0 is a valid argument
        // even though the OpenGL 4.0 spec does not.
        if (fbo->has(Framebuffer::AttachmentPoint(b))) {
            return GLenum(b);
        } else {
            return GL_NONE;
        }
    }
}


void RenderDevice::syncReadBuffer(bool alreadyBound) {
    if (isNull(m_state.readFramebuffer)) {
        return;
    }

	// The return value only tells us if we had to sync attachment points, tells us nothing about
	// the read buffer.
    m_state.readFramebuffer->bind(alreadyBound, Framebuffer::MODE_READ);
    if ((m_state.readFramebuffer->numAttachments() == 1) &&
        (m_state.readFramebuffer->get(Framebuffer::DEPTH))) {
        // OpenGL spec requires that readbuffer be none if there is no color buffer
        glReadBuffer(GL_NONE);
    } else {
        glReadBuffer(toFBOReadBuffer(m_state.readBuffer, m_state.readFramebuffer));
    }
    
}


void RenderDevice::beforePrimitive() {
    debugAssertM(! m_inRawOpenGL, "Cannot make RenderDevice calls while inside beginOpenGL...endOpenGL");

    syncDrawBuffer(true);
    syncReadBuffer(true);

}


void RenderDevice::afterPrimitive() {

}

GLenum RenderDevice::applyWinding(GLenum f) const {
    if (! invertY()) {
        if (f == GL_FRONT) {
            return GL_BACK;
        } else if (f == GL_BACK) {
            return GL_FRONT;
        }
    }

    // Pass all other values (like GL_ALWAYS) through
    return f;
}


void RenderDevice::setGlossyCoefficient(const Color3& c) {
    minStateChange();
    if (m_state.specular != c) {
        m_state.specular = c;

        static float spec[4];
        spec[0] = c[0];
        spec[1] = c[1];
        spec[2] = c[2];
        spec[3] = 1.0f;

        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
        minGLStateChange();
    }
}


void RenderDevice::setGlossyCoefficient(float s) {
    setGlossyCoefficient(s * Color3::white());
}


void RenderDevice::setShininess(float s) {
    minStateChange();
    if (m_state.shininess != s) {
        m_state.shininess = s;
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, clamp(s, 0.0f, 128.0f));
        minGLStateChange();
    }
}


void RenderDevice::setRenderMode(RenderMode m) {
    minStateChange();

    if (m == RENDER_CURRENT) {
        return;
    }

    if (m_state.renderMode != m) {
        minGLStateChange();
        m_state.renderMode = m;
        switch (m) {
        case RENDER_SOLID:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;

        case RENDER_WIREFRAME:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;

        case RENDER_POINTS:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;

        case RENDER_CURRENT:
            return;
            break;
        }
    }
}


RenderDevice::StencilTest RenderDevice::stencilTest() const {
    return m_state.stencil.stencilTest;
}


RenderDevice::RenderMode RenderDevice::renderMode() const {
    return m_state.renderMode;
}


void RenderDevice::setDrawBuffer(DrawBuffer b) {
    minStateChange();

    if (b == DRAW_CURRENT) {
        return;
    }

    if (isNull(m_state.drawFramebuffer)) {
        alwaysAssertM
            (!( (b >= DRAW_COLOR0) && (b <= DRAW_COLOR15)), 
             "Drawing to a color buffer is only supported by application-created framebuffers!");
    }

    if (b != m_state.drawBuffer) {
        minGLStateChange();
        m_state.drawBuffer = b;
        if (isNull(m_state.drawFramebuffer)) {
            // Only update the GL state if there is no framebuffer bound.
            glDrawBuffer(GLenum(m_state.drawBuffer));
        }
    }
}


void RenderDevice::setReadBuffer(ReadBuffer b) {
    minStateChange();

    if (b == READ_CURRENT) {
        return;
    }

    if (b != m_state.readBuffer) {
        minGLStateChange();
        m_state.readBuffer = b;
        if (m_state.readFramebuffer) {
            glReadBuffer(toFBOReadBuffer(m_state.readBuffer, m_state.readFramebuffer));
        } else {
            glReadBuffer(GLenum(m_state.readBuffer));
        }
    }
}


void RenderDevice::forceSetCullFace(CullFace f) {
    minGLStateChange();
    if (f == CullFace::NONE) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace(applyWinding(GLenum(f.value)));
    }

    m_state.cullFace = f;
}


void RenderDevice::setCullFace(CullFace f) {
    minStateChange();
    if ((f != m_state.cullFace) && (f != CullFace::CURRENT)) {
        forceSetCullFace(f);
    }
}


void RenderDevice::setSRGBConversion(bool b) {
    debugAssert(! m_inPrimitive);
    minStateChange();
    if (m_state.sRGBConversion != b) {
        m_state.sRGBConversion = b;
        minGLStateChange();
        if (b) {
            glEnable(GL_FRAMEBUFFER_SRGB);
        } else {
            glDisable(GL_FRAMEBUFFER_SRGB);
        }
    }
}


void RenderDevice::pushState() {
    debugAssert(! m_inPrimitive);

    // texgen enables
    glPushAttrib(GL_TEXTURE_BIT);
    m_stateStack.push(m_state);

    // Record that that the lights and matrices are unchanged since the previous state.
    // This allows popState to restore the lighting environment efficiently.

    m_state.matrices.changed = false;
    m_state.highestTextureUnitThatChanged = -1;

    m_stats.pushStates += 1;
}


void RenderDevice::popState() {
    debugAssert(! m_inPrimitive);
    debugAssertM(m_stateStack.size() > 0, "More calls to RenderDevice::pushState() than RenderDevice::popState().");
    setState(m_stateStack.last());
    m_stateStack.popDiscard();
    // texgen enables
    glPopAttrib();
}


void RenderDevice::clear(bool clearColor, bool clearDepth, bool clearStencil) {
    debugAssert(! m_inPrimitive);
    syncDrawBuffer(true);
    syncReadBuffer(true);

#   ifdef G3D_DEBUG
    {
        std::string why;
        debugAssertM(currentDrawFramebufferComplete(why), why);
    }
#   endif
    majStateChange();
    majGLStateChange();

    GLint mask = 0;

    bool oldColorWrite = colorWrite();
    if (clearColor) {
        mask |= GL_COLOR_BUFFER_BIT;
        setColorWrite(true);
    }

    bool oldDepthWrite = depthWrite();
    if (clearDepth) {
        mask |= GL_DEPTH_BUFFER_BIT;
        setDepthWrite(true);
    }

    if (clearStencil) {
        mask |= GL_STENCIL_BUFFER_BIT;
        minGLStateChange();
        minStateChange();
    }

    glClear(mask);
    minGLStateChange();
    minStateChange();
    setColorWrite(oldColorWrite);
    setDepthWrite(oldDepthWrite);
}


void RenderDevice::beginFrame() {
    if (m_swapGLBuffersPending) {
        swapBuffers();
    }

    m_stats.reset();

    ++m_beginEndFrame;
    debugAssertM(m_beginEndFrame == 1, "Mismatched calls to beginFrame/endFrame");
}


void RenderDevice::swapBuffers() {

    // Process the pending swap buffers call
    m_swapTimer.tick();
    m_window->swapGLBuffers();
    m_swapTimer.tock();
    m_swapGLBuffersPending = false;
}


void RenderDevice::setSwapBuffersAutomatically(bool b) {
    if (b == m_swapBuffersAutomatically) {
        // Setting to current state; nothing to do.
        return;
    }

    if (m_swapGLBuffersPending) {
        swapBuffers();
    }

    m_swapBuffersAutomatically = b;
}


void RenderDevice::endFrame() {
    --m_beginEndFrame;
    debugAssertM(m_beginEndFrame == 0, "Mismatched calls to beginFrame/endFrame");

    // Schedule a swap buffer iff we are handling them automatically.
    m_swapGLBuffersPending = m_swapBuffersAutomatically;

    debugAssertM(m_stateStack.size() == 0, "Missing RenderDevice::popState or RenderDevice::pop2D.");

    double now = System::time();
    double dt = now - m_lastTime;
    if (dt <= 0) {
        dt = 0.0001;
    }

    m_stats.frameRate = 1.0f / (float)dt;
    m_stats.triangleRate = m_stats.triangles * dt;

    {
        // small inter-frame time: A (interpolation parameter) is small
        // large inter-frame time: A is big
        double A = clamp(dt * 0.6, 0.001, 1.0);
        if (abs(m_stats.smoothFrameRate - m_stats.frameRate) / max(m_stats.smoothFrameRate, m_stats.frameRate) > 0.18) {
            // There's a huge discrepancy--something major just changed in the way we're rendering
            // so we should jump to the new value.
            A = 1.0;
        }
    
        m_stats.smoothFrameRate     = lerp(m_stats.smoothFrameRate, m_stats.frameRate, (float)A);
        m_stats.smoothTriangleRate  = lerp(m_stats.smoothTriangleRate, m_stats.triangleRate, A);
        m_stats.smoothTriangles     = lerp(m_stats.smoothTriangles, m_stats.triangles, A);
    }

    if ((m_stats.smoothFrameRate == finf()) || (isNaN(m_stats.smoothFrameRate))) {
        m_stats.smoothFrameRate = 1000000;
    } else if (m_stats.smoothFrameRate < 0) {
        m_stats.smoothFrameRate = 0;
    }

    if ((m_stats.smoothTriangleRate == finf()) || isNaN(m_stats.smoothTriangleRate)) {
        m_stats.smoothTriangleRate = 1e20;
    } else if (m_stats.smoothTriangleRate < 0) {
        m_stats.smoothTriangleRate = 0;
    }

    if ((m_stats.smoothTriangles == finf()) || isNaN(m_stats.smoothTriangles)) {
        m_stats.smoothTriangles = 1e20;
    } else if (m_stats.smoothTriangles < 0) {
        m_stats.smoothTriangles = 0;
    }

    m_lastTime = now;
}


bool RenderDevice::alphaWrite() const {
    return m_state.alphaWrite;
}


bool RenderDevice::depthWrite() const {
    return m_state.depthWrite;
}


bool RenderDevice::colorWrite() const {
    return m_state.colorWrite;
}


void RenderDevice::setStencilClearValue(int s) {
    debugAssert(! m_inPrimitive);
    minStateChange();
    if (m_state.stencil.stencilClear != s) {
        minGLStateChange();
        glClearStencil(s);
        m_state.stencil.stencilClear = s;
    }
}


void RenderDevice::setDepthClearValue(float d) {
    minStateChange();
    debugAssert(! m_inPrimitive);
    if (m_state.depthClear != d) {
        minGLStateChange();
        glClearDepth(d);
        m_state.depthClear = d;
    }
}


void RenderDevice::setColorClearValue(const Color4& c) {
    debugAssert(! m_inPrimitive);
    minStateChange();
    if (m_state.colorClear != c) {
        minGLStateChange();
        glClearColor(c.r, c.g, c.b, c.a);
        m_state.colorClear = c;
    }
}


void RenderDevice::setViewport(const Rect2D& v) {
    minStateChange();
    if (m_state.viewport != v) {
        forceSetViewport(v);
    }
}


void RenderDevice::forceSetViewport(const Rect2D& v) {
    // Flip to OpenGL y-axis
    float x = v.x0();
    float y;
    if (invertY()) {
        y = height() - v.y1();
    } else { 
        y = v.y0();
    }
    _glViewport(x, y, v.width(), v.height());
    m_state.viewport = v;
    minGLStateChange();
}


void RenderDevice::setClip2D(const Rect2D& clip) {
    minStateChange();

    if (clip.isFinite() || clip.isEmpty()) {
        m_state.clip2D = clip;

        Rect2D r = clip;
        if (clip.isEmpty()) {
            r = Rect2D::xywh(0,0,0,0);
        }

        // set the new clip Rect2D
        minGLStateChange();

        int clipX0 = iFloor(r.x0());
        int clipY0 = iFloor(r.y0());
        int clipX1 = iCeil(r.x1());
        int clipY1 = iCeil(r.y1());

        int y = 0;
        if (invertY()) {
            y = height() - clipY1;
        } else {
            y = clipY0;
        }
        glScissor(clipX0, y, clipX1 - clipX0, clipY1 - clipY0);

        if (clip.area() == 0) {
            // On some graphics cards a clip region that is zero without being (0,0,0,0) 
            // fails to actually clip everything.
            glScissor(0,0,0,0);
            glEnable(GL_SCISSOR_TEST);
        }

        // enable scissor test itself if not 
        // just adjusting the clip region
        if (! m_state.useClip2D) {
            glEnable(GL_SCISSOR_TEST);
            minStateChange();
            minGLStateChange();
            m_state.useClip2D = true;
        }
    } else if (m_state.useClip2D) {
        // disable scissor test if not already disabled
        minGLStateChange();
        glDisable(GL_SCISSOR_TEST);
        m_state.useClip2D = false;
    }
}


Rect2D RenderDevice::clip2D() const {
    if (m_state.useClip2D) {
        return m_state.clip2D;
    } else {
        return m_state.viewport;
    }
}


void RenderDevice::setProjectionAndCameraMatrix(const Projection& P, const CFrame& C) {
    setProjectionMatrix(P);
    setCameraToWorldMatrix(C);
}


const Rect2D& RenderDevice::viewport() const {
    return m_state.viewport;
}


void RenderDevice::pushState(const shared_ptr<Framebuffer>& fb) {
    pushState();

    if (fb) {
        setFramebuffer(fb);

        // When we change framebuffers, we almost certainly don't want to use the old clipping region
        setClip2D(Rect2D::inf());
        setViewport(fb->rect2DBounds());
    }
}


void RenderDevice::setReadFramebuffer(const shared_ptr<Framebuffer>& fbo) {
    if (fbo != m_state.readFramebuffer) {
        majGLStateChange();

        // Set Framebuffer
        if (isNull(fbo)) {
            m_state.readFramebuffer.reset();
            Framebuffer::bindWindowBuffer(Framebuffer::MODE_READ);

            // Restore the buffer that was in use before the framebuffer was attached
            glReadBuffer(GLenum(m_state.readBuffer));
        } else {
            debugAssertM(GLCaps::supports_GL_ARB_framebuffer_object() || GLCaps::supports_GL_EXT_framebuffer_object(), 
                "Framebuffer Object not supported!");
            m_state.readFramebuffer = fbo;
            syncReadBuffer(false);

            // The read enables for this framebuffer will be set during beforePrimitive()            
        }
    }
}


void RenderDevice::setDrawFramebuffer(const shared_ptr<Framebuffer>& fbo) {
    if (fbo != m_state.drawFramebuffer) {
        majGLStateChange();

        // Set Framebuffer
        if (isNull(fbo)) {
            m_state.drawFramebuffer.reset();
            Framebuffer::bindWindowBuffer(Framebuffer::MODE_DRAW);

            // Restore the buffer that was in use before the framebuffer was attached
            glDrawBuffer(GLenum(m_state.drawBuffer));
        } else {
            debugAssertM(GLCaps::supports_GL_ARB_framebuffer_object() || GLCaps::supports_GL_EXT_framebuffer_object(), 
                "Framebuffer Object not supported!");
            m_state.drawFramebuffer = fbo;
            syncDrawBuffer(false);

            // The draw enables for this framebuffer will be set during beforePrimitive()            
        }

        const bool newInvertY = isNull(m_state.drawFramebuffer);
        const bool invertYChanged = (m_state.matrices.invertY != newInvertY);
        if (invertYChanged) {
            m_state.matrices.invertY = newInvertY;
            // Force projection matrix change
            forceSetProjectionMatrix(projectionMatrix());

            // TODO: avoid if full-screen
            forceSetViewport(Rect2D(viewport()));

            // TODO: avoid if no-ops
            forceSetCullFace(m_state.cullFace);

            // TODO: avoid if no-ops
            forceSetStencilOp
                (m_state.stencil.frontStencilFail, m_state.stencil.frontStencilZFail, m_state.stencil.frontStencilZPass,
                 m_state.stencil.backStencilFail,  m_state.stencil.backStencilZFail, m_state.stencil.backStencilZPass);
        }
    }
}


void RenderDevice::setDepthTest(DepthTest test) {
    debugAssert(! m_inPrimitive);

    minStateChange();

    // On ALWAYS_PASS we must force a change because under OpenGL
    // depthWrite and depth test are dependent.
    if ((test == DEPTH_CURRENT) && (test != DEPTH_ALWAYS_PASS)) {
        return;
    }
    
    if ((m_state.depthTest != test) || (test == DEPTH_ALWAYS_PASS)) {
        minGLStateChange();
        if ((test == DEPTH_ALWAYS_PASS) && (m_state.depthWrite == false)) {
            // http://www.opengl.org/sdk/docs/man/xhtml/glDepthFunc.xml
            // "Even if the depth buffer exists and the depth mask is non-zero, the
            // depth buffer is not updated if the depth test is disabled."
            glDisable(GL_DEPTH_TEST);
        } else {
            minStateChange();
            minGLStateChange();
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GLenum(test));
        }

        m_state.depthTest = test;
    }
}

static GLenum toGLEnum(RenderDevice::StencilTest t) {
    debugAssert(t != RenderDevice::STENCIL_CURRENT);
    return GLenum(t);
}


void RenderDevice::_setStencilTest(RenderDevice::StencilTest test, int reference) {

    if (test == RenderDevice::STENCIL_CURRENT) {
        return;
    }

    const GLenum t = toGLEnum(test);

    if (GLCaps::supports_GL_EXT_stencil_two_side()) {
        // NVIDIA/EXT
        glActiveStencilFaceEXT(GL_BACK);
        glStencilFunc(t, reference, 0xFFFFFFFF);
        glActiveStencilFaceEXT(GL_FRONT);
        glStencilFunc(t, reference, 0xFFFFFFFF);
        minGLStateChange(4);
    } else if (GLCaps::supports_GL_ATI_separate_stencil()) {
        // ATI
        glStencilFuncSeparateATI(t, t, reference, 0xFFFFFFFF);
        minGLStateChange(1);
    } else {
        // Default OpenGL
        glStencilFunc(t, reference, 0xFFFFFFFF);
        minGLStateChange(1);
    }
}


void RenderDevice::setStencilConstant(int reference) {
    minStateChange();

    debugAssert(! m_inPrimitive);
    if (m_state.stencil.stencilReference != reference) {

        m_state.stencil.stencilReference = reference;
        _setStencilTest(m_state.stencil.stencilTest, reference);
        minStateChange();

    }
}


void RenderDevice::setStencilTest(StencilTest test) {
    minStateChange();

    if (test == STENCIL_CURRENT) {
        return;
    }

    debugAssert(! m_inPrimitive);

    if (m_state.stencil.stencilTest != test) {
        glEnable(GL_STENCIL_TEST);

        if (test == STENCIL_ALWAYS_PASS) {

            // Can't actually disable if the stencil op is using the test as well
            // due to an OpenGL limitation.
            if ((m_state.stencil.frontStencilFail   == STENCIL_KEEP) &&
                (m_state.stencil.frontStencilZFail  == STENCIL_KEEP) &&
                (m_state.stencil.frontStencilZPass  == STENCIL_KEEP) &&
                (! GLCaps::supports_GL_EXT_stencil_two_side() ||
                 ((m_state.stencil.backStencilFail  == STENCIL_KEEP) &&
                  (m_state.stencil.backStencilZFail == STENCIL_KEEP) &&
                  (m_state.stencil.backStencilZPass == STENCIL_KEEP)))) {
                minGLStateChange();
                glDisable(GL_STENCIL_TEST);
            }

        } else {

            _setStencilTest(test, m_state.stencil.stencilReference);
        }

        m_state.stencil.stencilTest = test;

    }
}

 void RenderDevice::setLogicOp(const LogicOp op) {
     debugAssert(! m_inPrimitive);
     minStateChange();
 
     if (op == LOGICOP_CURRENT) {
         return;
     }

     if (m_state.logicOp != op) {
         minGLStateChange();
         if (op == LOGIC_COPY) {
              ///< Colour index mode. Redundant?
             glDisable(GL_LOGIC_OP);
             glDisable(GL_COLOR_LOGIC_OP);
         } else {
             ///< Colour index mode. Redundant?
             glEnable(GL_LOGIC_OP); 
             glEnable(GL_COLOR_LOGIC_OP);
             glLogicOp(op);
         }
         m_state.logicOp = op;
     }
 }

RenderDevice::AlphaTest RenderDevice::alphaTest() const {
    return m_state.alphaTest;
}


float RenderDevice::alphaTestReference() const {
    return m_state.alphaReference;
}


void RenderDevice::setAlphaTest(AlphaTest test, float reference) {
    debugAssert(! m_inPrimitive);

    minStateChange();

    if (test == ALPHA_CURRENT) {
        return;
    }
    
    if ((m_state.alphaTest != test) || (m_state.alphaReference != reference)) {
        minGLStateChange();
        if (test == ALPHA_ALWAYS_PASS) {
            
            glDisable(GL_ALPHA_TEST);

        } else {

            glEnable(GL_ALPHA_TEST);
            switch (test) {
            case ALPHA_LESS:
                glAlphaFunc(GL_LESS, reference);
                break;

            case ALPHA_LEQUAL:
                glAlphaFunc(GL_LEQUAL, reference);
                break;

            case ALPHA_GREATER:
                glAlphaFunc(GL_GREATER, reference);
                break;

            case ALPHA_GEQUAL:
                glAlphaFunc(GL_GEQUAL, reference);
                break;

            case ALPHA_EQUAL:
                glAlphaFunc(GL_EQUAL, reference);
                break;

            case ALPHA_NOTEQUAL:
                glAlphaFunc(GL_NOTEQUAL, reference);
                break;

            case ALPHA_NEVER_PASS:
                glAlphaFunc(GL_NEVER, reference);
                break;

            default:
                debugAssertM(false, "Fell through switch");
            }
        }

        m_state.alphaTest = test;
        m_state.alphaReference = reference;
    }
}


GLint RenderDevice::toGLStencilOp(RenderDevice::StencilOp op) const {
    debugAssert(op != STENCILOP_CURRENT);
    switch (op) {
    case RenderDevice::STENCIL_INCR_WRAP:
        if (GLCaps::supports_GL_EXT_stencil_wrap()) {
            return GL_INCR_WRAP_EXT;
        } else {
            return GL_INCR;
        }

    case RenderDevice::STENCIL_DECR_WRAP:
        if (GLCaps::supports_GL_EXT_stencil_wrap()) {
            return GL_DECR_WRAP_EXT;
        } else {
            return GL_DECR;
        }
    default:
        return GLenum(op);
    }
}






void RenderDevice::copyTextureFromScreen(const shared_ptr<Texture>& texture, const Rect2D& rect, const ImageFormat* format, int mipLevel, CubeFace face) {  
    if (format == NULL) {
        format = texture->format();
    }

    const bool invertY = readFramebuffer();

    int y = 0;
    if (invertY) {
        y = iRound(viewport().height() - rect.y1());
    } else {
        y = iRound(rect.y0());
    }

    texture->copyFromScreen(Rect2D::xywh(rect.x0(), (float)y, rect.width(), rect.height()), format);
}




void RenderDevice::forceSetStencilOp(
    StencilOp                       frontStencilFail,
    StencilOp                       frontZFail,
    StencilOp                       frontZPass,
    StencilOp                       backStencilFail,
    StencilOp                       backZFail,
    StencilOp                       backZPass) {

    if (! invertY()) {
        std::swap(frontStencilFail, backStencilFail);
        std::swap(frontZFail, backZFail);
        std::swap(frontZPass, backZPass);
    }

    if (GLCaps::supports_GL_EXT_stencil_two_side()) {
        // NVIDIA

        // Set back face operation
        glActiveStencilFaceEXT(GL_BACK);
        glStencilOp(
            toGLStencilOp(backStencilFail),
            toGLStencilOp(backZFail),
            toGLStencilOp(backZPass));
        
        // Set front face operation
        glActiveStencilFaceEXT(GL_FRONT);
        glStencilOp(
            toGLStencilOp(frontStencilFail),
            toGLStencilOp(frontZFail),
            toGLStencilOp(frontZPass));

        minGLStateChange(4);

    } else if (GLCaps::supports_GL_ATI_separate_stencil()) {
        // ATI
        minGLStateChange(2);
        glStencilOpSeparateATI(GL_FRONT, 
            toGLStencilOp(frontStencilFail),
            toGLStencilOp(frontZFail),
            toGLStencilOp(frontZPass));

        glStencilOpSeparateATI(GL_BACK, 
            toGLStencilOp(backStencilFail),
            toGLStencilOp(backZFail),
            toGLStencilOp(backZPass));

    } else {
        // Generic OpenGL

        // Set front face operation
        glStencilOp(
            toGLStencilOp(frontStencilFail),
            toGLStencilOp(frontZFail),
            toGLStencilOp(frontZPass));

        minGLStateChange(1);
    }
}

void RenderDevice::setStencilOp(
    StencilOp                       frontStencilFail,
    StencilOp                       frontZFail,
    StencilOp                       frontZPass,
    StencilOp                       backStencilFail,
    StencilOp                       backZFail,
    StencilOp                       backZPass) {

    minStateChange();

    if (frontStencilFail == STENCILOP_CURRENT) {
        frontStencilFail = m_state.stencil.frontStencilFail;
    }
    
    if (frontZFail == STENCILOP_CURRENT) {
        frontZFail = m_state.stencil.frontStencilZFail;
    }
    
    if (frontZPass == STENCILOP_CURRENT) {
        frontZPass = m_state.stencil.frontStencilZPass;
    }

    if (backStencilFail == STENCILOP_CURRENT) {
        backStencilFail = m_state.stencil.backStencilFail;
    }
    
    if (backZFail == STENCILOP_CURRENT) {
        backZFail = m_state.stencil.backStencilZFail;
    }
    
    if (backZPass == STENCILOP_CURRENT) {
        backZPass = m_state.stencil.backStencilZPass;
    }
    
    if ((frontStencilFail  != m_state.stencil.frontStencilFail) ||
        (frontZFail        != m_state.stencil.frontStencilZFail) ||
        (frontZPass        != m_state.stencil.frontStencilZPass) || 
        (GLCaps::supports_two_sided_stencil() && 
        ((backStencilFail  != m_state.stencil.backStencilFail) ||
         (backZFail        != m_state.stencil.backStencilZFail) ||
         (backZPass        != m_state.stencil.backStencilZPass)))) { 

        forceSetStencilOp(frontStencilFail, frontZFail, frontZPass,
                 backStencilFail, backZFail, backZPass);


        // Need to manage the mask as well

        if ((frontStencilFail  == STENCIL_KEEP) &&
            (frontZPass        == STENCIL_KEEP) && 
            (frontZFail        == STENCIL_KEEP) &&
            (! GLCaps::supports_two_sided_stencil() ||
            ((backStencilFail  == STENCIL_KEEP) &&
             (backZPass        == STENCIL_KEEP) &&
             (backZFail        == STENCIL_KEEP)))) {

            // Turn off writing.  May need to turn off the stencil test.
            if (m_state.stencil.stencilTest == STENCIL_ALWAYS_PASS) {
                // Test doesn't need to be on
                glDisable(GL_STENCIL_TEST);
            }

        } else {
#           ifdef G3D_DEBUG
            {
                int stencilBits = 0;

                if (isNull(drawFramebuffer())) {
                    stencilBits = glGetInteger(GL_STENCIL_BITS);
                } else {
                    stencilBits = drawFramebuffer()->stencilBits();
                }
                debugAssertM(stencilBits > 0,
                    "Allocate nonzero OSWindow.stencilBits in GApp constructor or RenderDevice::init before using the stencil buffer.");
            }
#           endif
            // Turn on writing.  We also need to turn on the
            // stencil test in this case.

            if (m_state.stencil.stencilTest == STENCIL_ALWAYS_PASS) {
                // Test is not already on
                glEnable(GL_STENCIL_TEST);
                _setStencilTest(m_state.stencil.stencilTest, m_state.stencil.stencilReference);
            }
        }

        m_state.stencil.frontStencilFail  = frontStencilFail;
        m_state.stencil.frontStencilZFail = frontZFail;
        m_state.stencil.frontStencilZPass = frontZPass;
        m_state.stencil.backStencilFail   = backStencilFail;
        m_state.stencil.backStencilZFail  = backZFail;
        m_state.stencil.backStencilZPass  = backZPass;
    }
}


void RenderDevice::setStencilOp(
    StencilOp           fail,
    StencilOp           zfail,
    StencilOp           zpass) {
    debugAssert(! m_inPrimitive);

    setStencilOp(fail, zfail, zpass, fail, zfail, zpass);
}


static GLenum toGLBlendEq(RenderDevice::BlendEq e) {
    switch (e) {
    case RenderDevice::BLENDEQ_MIN:
        debugAssert(GLCaps::supports("GL_EXT_blend_minmax"));
        return GL_MIN;

    case RenderDevice::BLENDEQ_MAX:
        debugAssert(GLCaps::supports("GL_EXT_blend_minmax"));
        return GL_MAX;

    case RenderDevice::BLENDEQ_ADD:
        return GL_FUNC_ADD;

    case RenderDevice::BLENDEQ_SUBTRACT:
        debugAssert(GLCaps::supports("GL_EXT_blend_subtract"));
        return GL_FUNC_SUBTRACT;

    case RenderDevice::BLENDEQ_REVERSE_SUBTRACT:
        debugAssert(GLCaps::supports("GL_EXT_blend_subtract"));
        return GL_FUNC_REVERSE_SUBTRACT;

    default:
        debugAssertM(false, "Fell through switch");
        return GL_ZERO;
    }
}


void RenderDevice::setBlendFunc(
    BlendFunc src,
    BlendFunc dst,
    BlendEq   eq) {
    debugAssert(! m_inPrimitive);

    minStateChange();
    if (src == BLEND_CURRENT) {
        src = m_state.srcBlendFunc;
    }
    
    if (dst == BLEND_CURRENT) {
        dst = m_state.dstBlendFunc;
    }

    if (eq == BLENDEQ_CURRENT) {
        eq = m_state.blendEq;
    }

    if ((m_state.dstBlendFunc != dst) ||
        (m_state.srcBlendFunc != src) ||
        (m_state.blendEq != eq)) {

        minGLStateChange();

        if ((dst == BLEND_ZERO) && (src == BLEND_ONE) && 
            ((eq == BLENDEQ_ADD) || (eq == BLENDEQ_SUBTRACT))) {
            glDisable(GL_BLEND);
        } else {
            glEnable(GL_BLEND);
            glBlendFunc(toGLBlendFunc(src), toGLBlendFunc(dst));

            debugAssert(eq == BLENDEQ_ADD ||
                GLCaps::supports("GL_EXT_blend_minmax") ||
                GLCaps::supports("GL_EXT_blend_subtract"));

            static bool supportsBlendEq = GLCaps::supports("GL_EXT_blend_minmax");

            if (supportsBlendEq && (glBlendEquationEXT != 0)) {
                glBlendEquationEXT(toGLBlendEq(eq));
            }
        }
        m_state.dstBlendFunc = dst;
        m_state.srcBlendFunc = src;
        m_state.blendEq = eq;
    }
}


void RenderDevice::setLineWidth
   (float               width) {

    debugAssert(! m_inPrimitive);
    minStateChange();
    if (m_state.lineWidth != width) {
        glLineWidth(max(m_minLineWidth, width));
        minGLStateChange();
        m_state.lineWidth = width;
    }
}


void RenderDevice::setPointSize(
    float               width) {
    debugAssert(! m_inPrimitive);
    minStateChange();
    if (m_state.pointSize != width) {
        glPointSize(width);
        minGLStateChange();
        m_state.pointSize = width;
    }
}


void RenderDevice::setObjectToWorldMatrix(
    const CoordinateFrame& cFrame) {
    
    minStateChange();
    debugAssert(! m_inPrimitive);

    // No test to see if it is already equal; this is called frequently and is
    // usually different.
    m_state.matrices.changed = true;
    m_state.matrices.objectToWorldMatrix = cFrame;
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrix(m_state.matrices.cameraToWorldMatrixInverse * m_state.matrices.objectToWorldMatrix);
    minGLStateChange();
}


const CoordinateFrame& RenderDevice::objectToWorldMatrix() const {
    return m_state.matrices.objectToWorldMatrix;
}


void RenderDevice::setCameraToWorldMatrix(
    const CoordinateFrame& cFrame) {

    debugAssert(! m_inPrimitive);
    majStateChange();
    majGLStateChange();
    
    m_state.matrices.changed = true;
    m_state.matrices.cameraToWorldMatrix = cFrame;
    m_state.matrices.cameraToWorldMatrixInverse = cFrame.inverse();

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrix(m_state.matrices.cameraToWorldMatrixInverse * m_state.matrices.objectToWorldMatrix);
}


const CoordinateFrame& RenderDevice::cameraToWorldMatrix() const {
    return m_state.matrices.cameraToWorldMatrix;
}


Matrix4 RenderDevice::projectionMatrix() const {
    return m_state.matrices.projectionMatrix;
}


CoordinateFrame RenderDevice::modelViewMatrix() const {
    return m_state.matrices.cameraToWorldMatrixInverse * objectToWorldMatrix();
}


Matrix4 RenderDevice::modelViewProjectionMatrix() const {
    return projectionMatrix() * modelViewMatrix();
}


void RenderDevice::forceSetProjectionMatrix(const Matrix4& P) {
    m_state.matrices.projectionMatrix = P;
    m_state.matrices.changed = true;
    glMatrixMode(GL_PROJECTION);
    glLoadMatrix(invertYMatrix() * P);
    glMatrixMode(GL_MODELVIEW);
    minGLStateChange();
}


void RenderDevice::setProjectionMatrix(const Matrix4& P) {
    minStateChange();
    if (m_state.matrices.projectionMatrix != P) {
        forceSetProjectionMatrix(P);
    }
}


void RenderDevice::setProjectionMatrix(const Projection& P) {
    Matrix4 M;
    P.getProjectUnitMatrix(viewport(), M);
    setProjectionMatrix(M);
}


void RenderDevice::forceSetTextureMatrix(int unit, const double* m) {
    float f[16];
    for (int i = 0; i < 16; ++i) {
        f[i] = (float)m[i];
    }

    forceSetTextureMatrix(unit, f);
}


void RenderDevice::forceSetTextureMatrix(int unit, const float* m) {
    minStateChange();
    minGLStateChange();

    m_state.textureUnitModified(unit);
    memcpy(m_state.textureUnits[unit].textureMatrix, m, sizeof(float)*16);
    if (GLCaps::supports_GL_ARB_multitexture()) {
        glActiveTextureARB(GL_TEXTURE0_ARB + unit);
    }

    // Transpose the texture matrix
    float tt[16];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            tt[i + j * 4] = m[j + i * 4];
        }
    }
    glMatrixMode(GL_TEXTURE);
    glLoadMatrixf(tt);
}


Matrix4 RenderDevice::getTextureMatrix(int unit) {
    debugAssertM(unit < m_numTextureCoords,
        format("Attempted to access texture matrix %d on a device with %d matrices.",
        unit, m_numTextureCoords));

    const float* M = m_state.textureUnits[unit].textureMatrix;

    return Matrix4(M[0], M[4], M[8],  M[12],
                   M[1], M[5], M[9],  M[13],
                   M[2], M[6], M[10], M[14],
                   M[3], M[7], M[11], M[15]);
}



void RenderDevice::setTextureMatrix(
    int                  unit,
    const Matrix4&         m) {

    float f[16];
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            f[r * 4 + c] = m[r][c];
        }
    }
    
    setTextureMatrix(unit, f);
}


void RenderDevice::setTextureMatrix(
    int                  unit,
    const double*        m) {

    debugAssert(! m_inPrimitive);
    debugAssertM(unit <  m_numTextureCoords,
        format("Attempted to access texture matrix %d on a device with %d matrices.",
        unit, m_numTextureCoords));

    forceSetTextureMatrix(unit, m);
}


void RenderDevice::setTextureMatrix(
    int                 unit,
    const float*        m) {

    debugAssert(! m_inPrimitive);
    debugAssertM(unit <  m_numTextureCoords,
        format("Attempted to access texture matrix %d on a device with %d matrices.",
        unit, m_numTextureCoords));

    if (memcmp(m, m_state.textureUnits[unit].textureMatrix, sizeof(float)*16)) {
        forceSetTextureMatrix(unit, m);
    }
}


void RenderDevice::setTextureMatrix(
    int                     unit,
    const CoordinateFrame&  c) {

    float m[16] = 
    {c.rotation[0][0], c.rotation[0][1], c.rotation[0][2], c.translation.x,
     c.rotation[1][0], c.rotation[1][1], c.rotation[1][2], c.translation.y,
     c.rotation[2][0], c.rotation[2][1], c.rotation[2][2], c.translation.z,
                    0,                0,                0,               1};

    setTextureMatrix(unit, m);
}


const ImageFormat* RenderDevice::colorFormat() const {
    shared_ptr<Framebuffer> fbo = drawFramebuffer();
    if (isNull(fbo)) {
        OSWindow::Settings settings;
        window()->getSettings(settings);
        return settings.colorFormat();
    } else {
        shared_ptr<Framebuffer::Attachment> screen = fbo->get(Framebuffer::COLOR0);
        if (isNull(screen)) {
            return NULL;
        }
        return screen->format();
    }
}


void RenderDevice::setTextureLODBias(
    int                     unit,
    float                   bias) {

    minStateChange();
    if (m_state.textureUnits[unit].LODBias != bias) {
        m_state.textureUnitModified(unit);

        if (GLCaps::supports_GL_ARB_multitexture()) {
            glActiveTextureARB(GL_TEXTURE0_ARB + unit);
        }

        m_state.textureUnits[unit].LODBias = bias;

        minGLStateChange();
        glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, bias);
    }
}


void RenderDevice::setTextureCombineMode(
    int                     unit,
    const CombineMode       mode) {

    minStateChange();
    if (mode == TEX_CURRENT) {
        return;
    }

    debugAssertM(unit < m_numTextureUnits,
        format("Attempted to access texture unit %d when only %d units supported.",
        unit, m_numTextureUnits));

    if ((m_state.textureUnits[unit].combineMode != mode)) {
        minGLStateChange();
        m_state.textureUnitModified(unit);

        m_state.textureUnits[unit].combineMode = mode;

        if (GLCaps::supports_GL_ARB_multitexture()) {
            glActiveTextureARB(GL_TEXTURE0_ARB + unit);
        }

        static const bool hasAdd = GLCaps::supports("GL_EXT_texture_env_add");
        static const bool hasCombine = GLCaps::supports("GL_ARB_texture_env_combine");
        static const bool hasDot3 = GLCaps::supports("GL_ARB_texture_env_dot3");

        switch (mode) {
        case TEX_REPLACE:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            break;

        case TEX_BLEND:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
            break;

        case TEX_MODULATE:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            break;

        case TEX_INTERPOLATE:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
            break;

        case TEX_ADD:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, hasCombine ? GL_ADD : GL_BLEND);
            break;

        case TEX_SUBTRACT:
            // (add and subtract are in the same extension)
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, hasAdd ? GL_SUBTRACT_ARB : GL_BLEND);
            break;

        case TEX_ADD_SIGNED:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, hasAdd ? GL_ADD_SIGNED_ARB : GL_BLEND);
            break;
            
        case TEX_DOT3_RGB:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, hasDot3 ? GL_DOT3_RGB_ARB : GL_BLEND);
            break;
             
        case TEX_DOT3_RGBA:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, hasDot3 ? GL_DOT3_RGBA_ARB : GL_BLEND);
            break;

        default:
            debugAssertM(false, "Unrecognized texture combine mode");
        }
    }
}


void RenderDevice::resetTextureUnit(
    int                    unit) {
    debugAssertM(unit < m_numTextureUnits,
        format("Attempted to access texture unit %d when only %d units supported.",
        unit, m_numTextureUnits));

    RenderState newState(m_state);
    m_state.textureUnits[unit] = RenderState::TextureUnit();
    m_state.textureImageUnits[unit] = RenderState::TextureImageUnit();
    m_state.textureUnitModified(unit);
    setState(newState);
}


void RenderDevice::setPolygonOffset(
    float              offset) {
    debugAssert(! m_inPrimitive);

    minStateChange();

    if (m_state.polygonOffset != offset) {
        minGLStateChange();
        if (offset != 0) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glEnable(GL_POLYGON_OFFSET_LINE);
            glEnable(GL_POLYGON_OFFSET_POINT);
            glPolygonOffset(offset, sign(offset) * 2.0f);
        } else {
            glDisable(GL_POLYGON_OFFSET_POINT);
            glDisable(GL_POLYGON_OFFSET_FILL);
            glDisable(GL_POLYGON_OFFSET_LINE);
        }
        m_state.polygonOffset = offset;
    }
}


void RenderDevice::setNormal(const Vector3& normal) {
    m_state.normal = normal;
    glNormal(normal);
    minStateChange();
    minGLStateChange();
}


void RenderDevice::setTexCoord(int unit, const Vector4& texCoord) {
    debugAssertM(unit < m_numTextureCoords,
        format("Attempted to access texture coordinate %d when only %d coordinates supported.",
               unit, m_numTextureCoords));

    m_state.textureUnits[unit].texCoord = texCoord;
    if (GLCaps::supports_GL_ARB_multitexture()) {
        glMultiTexCoord(GL_TEXTURE0_ARB + unit, texCoord);
    } else {
        debugAssertM(unit == 0, "This machine has only one texture unit");
        glTexCoord(texCoord);
    }
    m_state.textureUnitModified(unit);
    minStateChange();
    minGLStateChange();
}


void RenderDevice::setTexCoord(int unit, const Vector3& texCoord) {
    setTexCoord(unit, Vector4(texCoord, 1));
}


void RenderDevice::setTexCoord(int unit, const Vector3int16& texCoord) {
    setTexCoord(unit, Vector4(texCoord.x, texCoord.y, texCoord.z, 1));
}


void RenderDevice::setTexCoord(int unit, const Vector2& texCoord) {
    setTexCoord(unit, Vector4(texCoord, 0, 1));
}


void RenderDevice::setTexCoord(int unit, const Vector2int16& texCoord) {
    setTexCoord(unit, Vector4(texCoord.x, texCoord.y, 0, 1));
}


void RenderDevice::setTexCoord(int unit, double texCoord) {
    setTexCoord(unit, Vector4((float)texCoord, 0, 0, 1));
}


void RenderDevice::sendVertex(const Vector2& vertex) {
    debugAssertM(m_inPrimitive, "Can only be called inside beginPrimitive()...endPrimitive()");
    glVertex(vertex);
    ++m_currentPrimitiveVertexCount;
}


void RenderDevice::sendVertex(const Vector3& vertex) {
    debugAssertM(m_inPrimitive, "Can only be called inside beginPrimitive()...endPrimitive()");
    glVertex(vertex);
    ++m_currentPrimitiveVertexCount;
}


void RenderDevice::sendVertex(const Vector4& vertex) {
    debugAssertM(m_inPrimitive, "Can only be called inside beginPrimitive()...endPrimitive()");
    glVertex(vertex);
    ++m_currentPrimitiveVertexCount;
}


void RenderDevice::beginPrimitive(PrimitiveType p) {
    debugAssertM(! m_inPrimitive, "Already inside a primitive");
    std::string why;
    debugAssertM( currentDrawFramebufferComplete(why), why);

    beforePrimitive();
    
    m_inPrimitive = true;
    m_currentPrimitiveVertexCount = 0;
    m_currentPrimitive = p;

    debugAssertGLOk();

    glBegin(primitiveToGLenum(p));
}


void RenderDevice::endPrimitive() {
    

    debugAssertM(m_inPrimitive, "Call to endPrimitive() without matching beginPrimitive()");

    minStateChange(m_currentPrimitiveVertexCount);
    minGLStateChange(m_currentPrimitiveVertexCount);
    countTriangles(m_currentPrimitive, m_currentPrimitiveVertexCount);
    
    glEnd();
    debugAssertGLOk();
    m_inPrimitive = false;

    afterPrimitive();
}


void RenderDevice::countTriangles(PrimitiveType primitive, int numVertices) {
    switch (primitive) {
    case PrimitiveType::LINES:
        m_stats.triangles += (numVertices / 2);
        break;

    case PrimitiveType::LINE_STRIP:
        m_stats.triangles += (numVertices - 1);
        break;

    case PrimitiveType::TRIANGLES:
        m_stats.triangles += (numVertices / 3);
        break;

    case PrimitiveType::TRIANGLE_STRIP:
    case PrimitiveType::TRIANGLE_FAN:
        m_stats.triangles += (numVertices - 2);
        break;

    case PrimitiveType::QUADS:
        m_stats.triangles += ((numVertices / 4) * 2);
        break;

    case PrimitiveType::QUAD_STRIP:
        m_stats.triangles += (((numVertices / 2) - 1) * 2);
        break;

    case PrimitiveType::POINTS:
        m_stats.triangles += numVertices;
        break;
    }
}


void RenderDevice::setTexture(
    int                     unit,
    const shared_ptr<Texture>&     texture) {

    // NVIDIA cards have more textures than texture units.
    // "fixedFunction" textures have an associated unit 
    // and must be glEnabled.  Programmable units *cannot*
    // be enabled.
    bool fixedFunction = (unit < m_numTextureUnits);

    debugAssertM(! m_inPrimitive, 
                 "Can't change textures while rendering a primitive.");

    debugAssertM(unit < m_numTextures,
        format("Attempted to access texture %d when only %d textures supported.",
               unit, m_numTextures));

    // early-return if the texture is already set
    if (m_state.textureImageUnits[unit].texture == texture) {
        return;
    }

    majStateChange();
    majGLStateChange();

    // cache old texture for texture matrix check below
    shared_ptr<Texture> oldTexture = m_state.textureImageUnits[unit].texture;

    // assign new texture
    m_state.textureImageUnits[unit].texture = texture;
    m_state.textureUnitModified(unit);

    if (GLCaps::supports_GL_ARB_multitexture()) {
        glActiveTextureARB(GL_TEXTURE0_ARB + unit);
    }

    // Turn off whatever was on previously if this is a fixed function unit
    if (fixedFunction) {
        glDisableAllTextures();
    }

    if (texture) {
        GLint id = texture->openGLID();
        GLint target = texture->openGLTextureTarget();

        if ((GLint)currentlyBoundTextures[unit] != id) {
            glBindTexture(target, id);
            currentlyBoundTextures[unit] = id;
        }

        if (fixedFunction) {
            glEnable(target);
        }
    } else {
        // Disabled texture unit
        currentlyBoundTextures[unit] = 0;
    }
}


double RenderDevice::getDepthBufferValue(
    int                     x,
    int                     y) const {

    GLfloat depth = 0;
    debugAssertGLOk();

    if (m_state.readFramebuffer) {
        debugAssertM(m_state.readFramebuffer->has(Framebuffer::DEPTH),
            "No depth attachment");
    }

    glReadPixels(x,
             (height() - 1) - y,
                 1, 1,
                 GL_DEPTH_COMPONENT,
             GL_FLOAT,
             &depth);

    debugAssertM(glGetError() != GL_INVALID_OPERATION, 
        "getDepthBufferValue failed, probably because you did not allocate a depth buffer.");

    return depth;
}


shared_ptr<Image> RenderDevice::screenshotPic(bool getAlpha, bool invertY) const {
    shared_ptr<CPUPixelTransferBuffer> imageBuffer = CPUPixelTransferBuffer::create(width(), height(), getAlpha ? ImageFormat::RGBA8() : ImageFormat::RGB8());

    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT); {
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        debugAssertM(glGetInteger(GL_PIXEL_PACK_BUFFER_BINDING) == 0, "GL_PIXEL_PACK_BUFFER bound during glReadPixels");
        debugAssert(glGetInteger(GL_READ_FRAMEBUFFER_BINDING) == 0);
        glReadPixels(0, 0, width(), height(), imageBuffer->format()->openGLBaseFormat, imageBuffer->format()->openGLDataFormat, imageBuffer->buffer());
        debugAssertGLOk();
    } glPopClientAttrib();

    const shared_ptr<Image>& image = Image::fromPixelTransferBuffer(imageBuffer);

    // Flip right side up
    if (invertY) {
        image->flipVertical();
    }

    return image;
}


std::string RenderDevice::screenshot(const std::string& filepath) const {
    const std::string filename(FilePath::concat(filepath, generateFilenameBase("", "_" + System::appName()) + ".jpg"));

    const shared_ptr<Image> screen(screenshotPic());
    if (screen) {
        screen->save(filename);
    }

    return filename;
}


void RenderDevice::beginIndexedPrimitives() {
    debugAssert(! m_inPrimitive);
    debugAssert(! m_inIndexedPrimitive);

    // TODO: can we avoid this push?
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT); 

    m_inIndexedPrimitive = true;
}


void RenderDevice::endIndexedPrimitives() {
    debugAssert(! m_inPrimitive);
    debugAssert(m_inIndexedPrimitive);

    // Allow garbage collection of VARs
    m_tempVAR.fastClear();
    
    if (GLCaps::supports_GL_ARB_vertex_buffer_object()) {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }

    glPopClientAttrib();
    m_inIndexedPrimitive = false;
    m_currentVertexBuffer.reset();
}


void RenderDevice::setVARAreaFromVAR(const class AttributeArray& v) {
    debugAssert(m_inIndexedPrimitive);
    debugAssert(! m_inPrimitive);
    alwaysAssertM(isNull(m_currentVertexBuffer) || (v.area() == m_currentVertexBuffer), 
        "All vertex arrays used within a single begin/endIndexedPrimitive"
                  " block must share the same VertexBuffer.");

    majStateChange();

    if (v.area() != m_currentVertexBuffer) {
        m_currentVertexBuffer = const_cast<AttributeArray&>(v).area();

        // Bind the buffer (for MAIN_MEMORY, we need do nothing)
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_currentVertexBuffer->m_glbuffer);
        majGLStateChange();
    }
}


void RenderDevice::setVARs
(const class AttributeArray&  vertex, 
 const class AttributeArray&  normal, 
 const class AttributeArray&  color,
 const Array<AttributeArray>& texCoord) {

    // Wipe old VertexBuffer
    m_currentVertexBuffer.reset();

    // Disable anything that is not about to be set
    debugAssertM((m_varState.highestEnabledTexCoord == 0) || GLCaps::supports_GL_ARB_multitexture(),
                 "Graphics card does not support multitexture");
    for (int i = texCoord.size(); i <= m_varState.highestEnabledTexCoord; ++i) {
        if (GLCaps::supports_GL_ARB_multitexture()) {
            glClientActiveTextureARB(GL_TEXTURE0_ARB + i);
        }
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    // Bind new
    setVertexArray(vertex);

    if (normal.size() > 0) {
        setNormalArray(normal);
    } else {
        glDisableClientState(GL_NORMAL_ARRAY);
    }

    if (color.size() > 0) {
        setColorArray(color);
    } else {
        glDisableClientState(GL_COLOR_ARRAY);
    }

    for (int i = 0; i < texCoord.size(); ++i) {
        setTexCoordArray(i, texCoord[i]);
        if (texCoord[i].size() > 0) {
            m_varState.highestEnabledTexCoord = i;
        }
    }
}


void RenderDevice::setVARs(const class AttributeArray& vertex, const class AttributeArray& normal, const class AttributeArray& texCoord0,
                           const class AttributeArray& texCoord1) {
    m_tempVAR.fastClear();
    if ((texCoord0.size() > 0) || (texCoord1.size() > 0)) {
        m_tempVAR.append(texCoord0, texCoord1);
    }
    setVARs(vertex, normal, AttributeArray(), m_tempVAR);
}



void RenderDevice::setVertexArray(const class AttributeArray& v) {
    setVARAreaFromVAR(v);
    v.vertexPointer();
}


void RenderDevice::setVertexAttribArray(unsigned int attribNum, const class AttributeArray& v) {
    setVARAreaFromVAR(v);
    v.vertexAttribPointer(attribNum);
}


void RenderDevice::setNormalArray(const class AttributeArray& v) {
    setVARAreaFromVAR(v);
    v.normalPointer();
}


void RenderDevice::setColorArray(const class AttributeArray& v) {
    setVARAreaFromVAR(v);
    v.colorPointer();
}


void RenderDevice::setTexCoordArray(unsigned int unit, const class AttributeArray& v) {
    if (v.size() == 0) {
        debugAssertM(GLCaps::supports_GL_ARB_multitexture() || (unit == 0),
            "Graphics card does not support multitexture");

        if (GLCaps::supports_GL_ARB_multitexture()) {
            glClientActiveTextureARB(GL_TEXTURE0_ARB + unit);
        }
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        if (GLCaps::supports_GL_ARB_multitexture()) {
            glClientActiveTextureARB(GL_TEXTURE0_ARB);
        }
    } else {
        setVARAreaFromVAR(v);
        v.texCoordPointer(unit);
    }
}


void RenderDevice::configureShadowMap
   (int                   unit,
    const ShadowMap::Ref& shadowMap) {
    configureShadowMap(unit, shadowMap->lightMVP(), shadowMap->depthTexture());
}


void RenderDevice::configureShadowMap
   (int                 unit,
    const Matrix4&      lightMVP,
    const shared_ptr<Texture>& shadowMap) {

    minStateChange();
    minGLStateChange();

    // http://www.nvidia.com/dev_content/nvopenglspecs/GL_ARB_shadow.txt

    debugAssertM(shadowMap->format()->openGLBaseFormat == GL_DEPTH_COMPONENT,
        "Can only configure shadow maps from depth textures");

    debugAssertM(shadowMap->settings().depthReadMode != Texture::DEPTH_NORMAL,
        "Shadow maps must be configured for either Texture::DEPTH_LEQUAL"
        " or Texture::DEPTH_GEQUAL comparisions.");

    debugAssertM(GLCaps::supports_GL_ARB_shadow(),
        "The device does not support shadow maps");


    // Texture coordinate generation will use the current MV matrix
    // to determine OpenGL "eye" space.  We want OpenGL "eye" space to
    // be our world space, so set the object to world matrix to be the
    // identity.
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrix(m_state.matrices.cameraToWorldMatrixInverse);

    setTexture(unit, shadowMap);
    
    if (GLCaps::supports_GL_ARB_multitexture()) {
        glActiveTextureARB(GL_TEXTURE0_ARB + unit);
    }
    
    const Matrix4& textureMatrix = m_state.textureUnits[unit].textureMatrix;

    const Matrix4& textureProjectionMatrix2D =
        textureMatrix  * lightMVP;

    // Set up tex coord generation - all 4 coordinates required
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_S, GL_EYE_PLANE, textureProjectionMatrix2D[0]);
    glEnable(GL_TEXTURE_GEN_S);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_T, GL_EYE_PLANE, textureProjectionMatrix2D[1]);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_R, GL_EYE_PLANE, textureProjectionMatrix2D[2]);
    glEnable(GL_TEXTURE_GEN_R);
    glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_Q, GL_EYE_PLANE, textureProjectionMatrix2D[3]);
    glEnable(GL_TEXTURE_GEN_Q);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}


void RenderDevice::configureReflectionMap
   (int                 textureUnit,
    shared_ptr<Texture>        reflectionTexture) {

    debugAssert(! GLCaps::hasBug_normalMapTexGen());
    debugAssert(reflectionTexture->dimension() == Texture::DIM_CUBE_MAP);

    // Texture coordinates will be generated in object space.
    // Set the texture matrix to transform them into camera space.
    CoordinateFrame cframe = cameraToWorldMatrix();

    // The environment map assumes we are always in the center,
    // so zero the translation.
    cframe.translation = Vector3::zero();

    // The environment map is in world space.  The reflection vector
    // will automatically be multiplied by the object->camera space 
    // transformation by hardware (just like any other vector) so we 
    // take it back from camera space to world space for the correct
    // vector.
    setTexture(textureUnit, reflectionTexture);

    setTextureMatrix(textureUnit, cframe);

    minStateChange();
    minGLStateChange();
    glActiveTextureARB(GL_TEXTURE0_ARB + textureUnit);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_GEN_R);
}


void RenderDevice::sendSequentialIndices
(PrimitiveType primitive, int numVertices, int start) {

    beforePrimitive();

    glDrawArrays(primitiveToGLenum(primitive), start, numVertices);

    countTriangles(primitive, numVertices);
    afterPrimitive();

    minStateChange();
    minGLStateChange();
}


void RenderDevice::sendSequentialIndicesInstanced
(PrimitiveType primitive, int numVertices, int numInstances) {

    beforePrimitive();

    glDrawArraysInstanced(primitiveToGLenum(primitive), 0, numVertices, numInstances);

    countTriangles(primitive, numVertices * numInstances);
    afterPrimitive();

    minStateChange();
    minGLStateChange();
}


void RenderDevice::sendIndices
(PrimitiveType primitive, const IndexStream& indexVAR) {
    sendIndices(primitive, indexVAR, 1, false);
}


void RenderDevice::sendIndicesInstanced
(PrimitiveType primitive, const IndexStream& indexVAR, int numInstances) {
    sendIndices(primitive, indexVAR, numInstances, true);
}


void RenderDevice::sendIndices
(PrimitiveType primitive, const IndexStream& indexVAR,
 int numInstances, bool useInstances) {

    std::string why;
    debugAssertM(currentDrawFramebufferComplete(why), why);

    if (indexVAR.m_numElements == 0) {
        // There's nothing in this index array, so don't bother rendering.
        return;
    }

    debugAssertM(indexVAR.area(), "Corrupt AttributeArray");

    // Calling glGetInteger triggers a stall on SLI GPUs
    //int old = glGetInteger(GL_ELEMENT_ARRAY_BUFFER_BINDING);

    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, indexVAR.area()->openGLVertexBufferObject());

    internalSendIndices(primitive, (int)indexVAR.m_elementSize, indexVAR.m_numElements, 
                        indexVAR.pointer(), numInstances, useInstances);
    
    countTriangles(primitive, indexVAR.m_numElements * numInstances);

    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void RenderDevice::internalSendIndices
(PrimitiveType           primitive,
 int                     indexSize, 
 int                     numIndices, 
 const void*             index,
 int                     numInstances,
 bool                    useInstances) {
    
    beforePrimitive();

    GLenum i;
    
    switch (indexSize) {
    case sizeof(uint32):
        i = GL_UNSIGNED_INT;
        break;
    
    case sizeof(uint16):
        i = GL_UNSIGNED_SHORT;
        break;
    
    case sizeof(uint8):
        i = GL_UNSIGNED_BYTE;
        break;
    
    default:
        debugAssertM(false, "Indices must be either 8, 16, or 32-bytes each.");
        i = 0;
    }
    
    GLenum p = primitiveToGLenum(primitive);
    
    if (useInstances) {
        glDrawElementsInstancedARB(p, numIndices, i, index, numInstances);
    } else {
        glDrawElements(p, numIndices, i, index);
    }
        
    afterPrimitive();
}


static bool checkFramebuffer(GLenum which, std::string& whyNot) {
    GLenum status = static_cast<GLenum>(glCheckFramebufferStatus(which));

    switch(status) {
    case GL_FRAMEBUFFER_COMPLETE:
        return true;

    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        whyNot = "Framebuffer Incomplete: Incomplete Attachment.";
        break;

    case GL_FRAMEBUFFER_UNSUPPORTED:
        whyNot = "Unsupported framebuffer format.";
        break;

    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        whyNot = "Framebuffer Incomplete: Missing attachment.";
        break;

    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        whyNot = "Framebuffer Incomplete: Missing draw buffer.";
        break;

    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        whyNot = "Framebuffer Incomplete: Missing read buffer.";
        break;

    default:
        whyNot = format("Framebuffer Incomplete: Unknown error. (0x%X)", status);
    }

    return false;    
}


bool RenderDevice::checkDrawFramebuffer(std::string& whyNot) const {
    return checkFramebuffer(GL_DRAW_FRAMEBUFFER, whyNot);
}


bool RenderDevice::checkReadFramebuffer(std::string& whyNot) const {
    return checkFramebuffer(GL_READ_FRAMEBUFFER, whyNot);
}


/////////////////////////////////////////////////////////////////////////////////////

RenderDevice::Stats::Stats() {
    smoothTriangles = 0;
    smoothTriangleRate = 0;
    smoothFrameRate = 0;
    reset();
}

void RenderDevice::Stats::reset() {
    minorStateChanges = 0;
    minorOpenGLStateChanges = 0;
    majorStateChanges = 0;
    majorOpenGLStateChanges = 0;
    pushStates = 0;
    primitives = 0;
    triangles = 0;
    swapbuffersTime = 0;
    frameRate = 0;
    triangleRate = 0;
}

/////////////////////////////////////////////////////////////////////////////////////


static void var(TextOutput& t, const std::string& name, const std::string& val) {
    t.writeSymbols(name,"=");
    t.writeString(val + ";");
    t.writeNewline();
}


static void var(TextOutput& t, const std::string& name, const bool val) {
    t.writeSymbols(name, "=", val ? "true;" : "false;");
    t.writeNewline();
}


static void var(TextOutput& t, const std::string& name, const int val) {
    t.writeSymbols(name, "=");
    t.writeNumber(val);
    t.printf(";");
    t.writeNewline();
}


void RenderDevice::describeSystem(TextOutput& t) {

    debugAssertGLOk();
    t.writeSymbols("GPU", "=", "{");
    t.writeNewline();
    t.pushIndent();
    {
        var(t, "Chipset", GLCaps::renderer());
        var(t, "Vendor", GLCaps::vendor());
        var(t, "Driver", GLCaps::driverVersion());
        var(t, "OpenGL version", GLCaps::glVersion());
        var(t, "Textures", GLCaps::numTextures());
        var(t, "Texture coordinates", GLCaps::numTextureCoords());
        var(t, "Texture units", GLCaps::numTextureUnits());
        var(t, "GL_MAX_TEXTURE_SIZE", glGetInteger(GL_MAX_TEXTURE_SIZE));
        var(t, "GL_MAX_CUBE_MAP_TEXTURE_SIZE", glGetInteger(GL_MAX_CUBE_MAP_TEXTURE_SIZE_EXT));
        if (GLCaps::supports_GL_ARB_framebuffer_object() || GLCaps::supports_GL_EXT_framebuffer_object()) {
            debugAssertGLOk();
            var(t, "GL_MAX_COLOR_ATTACHMENTS", glGetInteger(GL_MAX_COLOR_ATTACHMENTS));
            debugAssertGLOk();
        } else {
            var(t, "GL_MAX_COLOR_ATTACHMENTS", 0);
        }
    }
    t.popIndent();
    t.writeSymbols("}", ";");
    t.writeNewline();
    t.writeNewline();

    OSWindow* w = window();
    OSWindow::Settings settings;
    w->getSettings(settings);

    t.writeSymbols("Window", "=", "{");
    t.writeNewline();
    t.pushIndent();
        var(t, "API", w->getAPIName());
        var(t, "Version", w->getAPIVersion());
        t.writeNewline();

        var(t, "In focus", w->hasFocus());
        var(t, "Centered", settings.center);
        var(t, "Framed", settings.framed);
        var(t, "Visible", settings.visible);
        var(t, "Resizable", settings.resizable);
        var(t, "Full screen", settings.fullScreen);
        var(t, "Top", settings.y);
        var(t, "Left", settings.x);
        var(t, "Width", settings.width);
        var(t, "Height", settings.height);
        var(t, "Refresh rate", settings.refreshRate);
        t.writeNewline();

        var(t, "Alpha bits", settings.alphaBits);
        var(t, "Red bits", settings.rgbBits);
        var(t, "Green bits", settings.rgbBits);
        var(t, "Blue bits", settings.rgbBits);
        var(t, "Depth bits", settings.depthBits);
        var(t, "Stencil bits", settings.stencilBits);
        var(t, "Asynchronous", settings.asynchronous);
        var(t, "Stereo", settings.stereo);
        var(t, "FSAA samples", settings.msaaSamples);


        t.writeSymbols("GL extensions", "=", "[");
        t.pushIndent();
        std::string extStringCopy = (char*)glGetString(GL_EXTENSIONS);
        Array<std::string> ext = stringSplit(extStringCopy, ' ');
        std::string s = ",\n";
        for (int i = 0; i < ext.length(); ++i) {
            if (i == ext.length() - 1) { s = ""; } // skip the last comma
            t.writeSymbol(trimWhitespace(ext[i]) + s);
        }
        t.popIndent();
        t.writeSymbol("];");
        t.writeNewline();

    t.popIndent();
    t.writeSymbols("};");
    t.writeNewline();
    t.writeNewline();
}


void RenderDevice::apply(const shared_ptr<Shader>& s, const Args& args) {
    shared_ptr<Shader::ShaderProgram> program = s->compileAndBind(args, this);

    const Shader::DomainType domainType = Shader::domainType(s, args);
    switch ( domainType ) {

    case Shader::STANDARD_INDEXED_RENDERING_MODE:
    case Shader::STANDARD_NONINDEXED_RENDERING_MODE:
    case Shader::INDIRECT_RENDERING_MODE:
         beginIndexedPrimitives(); {
            s->bindStreamArgs(program, args, this);
            if ( domainType == Shader::STANDARD_INDEXED_RENDERING_MODE ) {
                sendIndicesInstanced(args.getPrimitiveType(), args.getIndexStream(), args.getNumInstances());
            } else if ( domainType == Shader::STANDARD_NONINDEXED_RENDERING_MODE ) {
                sendSequentialIndicesInstanced(args.getPrimitiveType(), args.numIndices(), args.getNumInstances());
            } else { // domainType == Shader::INDIRECT_RENDERING
                glBindBuffer(GL_DRAW_INDIRECT_BUFFER, args.indirectBuffer()->glBufferID());
                glDrawArraysIndirect(args.getPrimitiveType(), (const void*)args.indirectOffset());
                glBindBuffer(GL_DRAW_INDIRECT_BUFFER, GL_NONE);
            }
         } endIndexedPrimitives();
         break;

    case Shader::STANDARD_COMPUTE_MODE:
        {
            const Vector3int32& gridDim = args.computeGridDim;
            glDispatchCompute(gridDim.x, gridDim.y, gridDim.z);
        }
        break;
    case Shader::INDIRECT_COMPUTE_MODE:
        glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, args.indirectBuffer()->glBufferID());
        glDispatchComputeIndirect(args.indirectOffset());
        glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, GL_NONE);
        break;
    case Shader::IMMEDIATE_MODE:
        args.sendImmediateModePrimitives();
        break;

	case Shader::RECT_MODE:
		{
			debugAssertGLOk();
			const float zCoord = args.getRectZCoord();
			const Rect2D& r = args.getRect();
			beginPrimitive(PrimitiveType::QUADS); {
				glTexCoord2f(0, 0);
				glVertex(Vector3(r.x0y0(), zCoord));

				glTexCoord2f(0, 1);
				glVertex(Vector3(r.x0y1(), zCoord));

				glTexCoord2f(1, 1);
				glVertex(Vector3(r.x1y1(), zCoord));

				glTexCoord2f(1, 0);
				glVertex(Vector3(r.x1y0(), zCoord));
			} endPrimitive();
		}
		break;
    default:
        alwaysAssertM(false, "Invalid Shader/Args Configuration, no domain type is valid. This can be caused by either not specifying any computation, or by mixing modes");
    }
    debugAssertGLOk();
    glUseProgram(GL_NONE); 
}


} // namespace

