/**
 \file GApp.cpp
  
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 \created 2003-11-03
 \edited  2013-07-31
 */

#include "G3D/platform.h"

#include "GLG3D/Camera.h"
#include "G3D/fileutils.h"
#include "G3D/Log.h"
#include "G3D/units.h"
#include "G3D/NetworkDevice.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/GApp.h"
#include "GLG3D/FirstPersonManipulator.h"
#include "GLG3D/UserInput.h"
#include "GLG3D/OSWindow.h"
#include "GLG3D/Draw.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/VideoRecordDialog.h"
#include "G3D/ParseError.h"
#include "G3D/FileSystem.h"
#include "GLG3D/GuiTextureBox.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/Light.h"
#include "GLG3D/AmbientOcclusion.h"
#include "GLG3D/Shader.h"
#include "GLG3D/MotionBlur.h"
#include "GLG3D/DepthOfField.h"
#include "GLG3D/SceneEditorWindow.h"
#include "GLG3D/UprightSplineManipulator.h"
#include "GLG3D/CameraControlWindow.h"
#include "GLG3D/GLPixelTransferBuffer.h"
#include "G3D/PixelTransferBuffer.h"
#include "G3D/CPUPixelTransferBuffer.h"

#include <time.h>

namespace G3D {

static GApp* currentGApp = NULL;

/** Framerate when the app does not have focus.  Should be low, e.g., 4fps */
static const float BACKGROUND_FRAME_RATE = 4.0; // fps

// Forward reference
void initGLG3D(const G3DSpecification& spec = G3DSpecification());

GApp::Settings::Settings() : 
    dataDir("<AUTO>"), 
    debugFontName("console-small.fnt"), 
    logFilename("log.txt"), 
    useDeveloperTools(true), 
    writeLicenseFile(true),
    colorGuardBandThickness(0, 0),
    depthGuardBandThickness(0, 0) {
    initGLG3D();
}


GApp::Settings::Settings(int argc, const char* argv[]) {    
    *this = Settings();

    argArray.resize(argc);
    for (int i = 0; i < argc; ++i) {
        argArray[i] = argv[i];
    }
}


void screenPrintf(const char* fmt ...) {
    va_list argList;
    va_start(argList, fmt);
    if (currentGApp) {
        currentGApp->vscreenPrintf(fmt, argList);
    }
    va_end(argList);
}

void GApp::vscreenPrintf
(const char*                 fmt,
 va_list                     argPtr) {
    if (showDebugText) {
        std::string s = G3D::vformat(fmt, argPtr);
        m_debugTextMutex.lock();
        debugText.append(s);
        m_debugTextMutex.unlock();
    }
}


DebugID debugDraw
(const shared_ptr<Shape>& shape, 
 float             displayTime, 
 const Color4&     solidColor, 
 const Color4&     wireColor, 
 const CFrame&     frame) {

    if (currentGApp) {
        debugAssert(shape);
        GApp::DebugShape& s = currentGApp->debugShapeArray.next();
        s.shape             = shape;
        s.solidColor        = solidColor;
        s.wireColor         = wireColor;
        s.frame             = frame;
        if (displayTime == 0) {
            s.endTime = 0;
        } else {
            s.endTime       = System::time() + displayTime;
        }
        s.id                = currentGApp->m_lastDebugID++;
        return s.id;
    } else {
        return 0;
    }
}

DebugID debugDraw
(const Box& b,
 float             displayTime,
 const Color4&     solidColor, 
 const Color4&     wireColor, 
 const CoordinateFrame&     cframe){
     return debugDraw(shared_ptr<Shape>(new BoxShape(b)), displayTime, solidColor, wireColor, cframe);
}

DebugID debugDraw
(const Array<Vector3>& vertices,
 const Array<int>&     indices,
 float             displayTime,
 const Color4&     solidColor, 
 const Color4&     wireColor, 
 const CoordinateFrame&     cframe){
     return debugDraw(shared_ptr<Shape>(new MeshShape(vertices, indices)), displayTime, solidColor, wireColor, cframe);
}

DebugID debugDraw
(const CPUVertexArray& vertices,
 const Array<Tri>&     tris,
 float             displayTime,
 const Color4&     solidColor, 
 const Color4&     wireColor, 
 const CoordinateFrame&     cframe){
     return debugDraw(shared_ptr<Shape>(new MeshShape(vertices, tris)), displayTime, solidColor, wireColor, cframe);
}

DebugID debugDraw
(const Sphere& s,
 float             displayTime,
 const Color4&     solidColor, 
 const Color4&     wireColor, 
 const CoordinateFrame&     cframe){
     return debugDraw(shared_ptr<Shape>(new SphereShape(s)), displayTime, solidColor, wireColor, cframe);
}

DebugID debugDraw
(const CoordinateFrame& cf,
 float             displayTime,
 const Color4&     solidColor, 
 const Color4&     wireColor, 
 const CoordinateFrame&     cframe){
     return debugDraw(shared_ptr<Shape>(new AxesShape(cf)), displayTime, solidColor, wireColor, cframe);
}


DebugID debugDrawLabel
(const Point3& wsPos,
 const Vector3& csOffset,
 const GuiText text,
 float displayTime,
 float size,
 bool  sizeInPixels,
 const GFont::XAlign xalign,
 const GFont::YAlign yalign) {

    if (notNull(currentGApp)) {
        GApp::DebugLabel& L = currentGApp->debugLabelArray.next();
        L.text = text;
        
        L.wsPos= wsPos + currentGApp->activeCamera()->frame().vectorToWorldSpace(csOffset);
        if (sizeInPixels) {
            const float factor = -currentGApp->activeCamera()->imagePlanePixelsPerMeter(currentGApp->renderDevice->viewport());
            const float z = currentGApp->activeCamera()->frame().pointToObjectSpace(L.wsPos).z;
            L.size  = (size / factor) * (float)abs(z);
        } else {
            L.size = size;
        }
        L.xalign = xalign;
        L.yalign = yalign;

        if (displayTime == 0) {
            L.endTime = 0;
        } else {
            L.endTime = System::time() + displayTime;
        }

        L.id = currentGApp->m_lastDebugID;
        ++currentGApp->m_lastDebugID;
        return L.id;
    } else {
        return 0;
    }
}


DebugID debugDrawLabel
(const Point3& wsPos,
 const Vector3& csOffset,
 const std::string& text,
 const Color3& color,
 float displayTime,
 float size,
 bool  sizeInPixels,
 const GFont::XAlign xalign,
 const GFont::YAlign yalign) {
     return debugDrawLabel(wsPos, csOffset, GuiText(text, shared_ptr<GFont>(), -1.0f, color), displayTime, size, sizeInPixels, xalign, yalign);
}


/** Attempt to write license file */
static void writeLicense() {
    FILE* f = FileSystem::fopen("g3d-license.txt", "wt");
    if (f != NULL) {
        fprintf(f, "%s", license().c_str());
        FileSystem::fclose(f);
    }
}


GApp::GApp(const Settings& settings, OSWindow* window, RenderDevice* rd) :
    m_lastDebugID(0),
    m_activeVideoRecordDialog(NULL),
    m_settings(settings),
    m_renderPeriod(1),
    m_endProgram(false),
    m_exitCode(0),
    m_debugTextColor(Color3::black()),
    m_debugTextOutlineColor(Color3(0.7f)),
    m_lastFrameOverWait(0),
    debugPane(NULL),
    renderDevice(NULL),
    userInput(NULL),
    m_lastWaitTime(System::time()),
    m_wallClockTargetDuration(1.0f / 60.0f),
    m_lowerFrameRateInBackground(true),
    m_simTimeStep(MATCH_REAL_TIME_TARGET),
    m_simTimeScale(1.0f),
    m_previousSimTimeStep(1.0f / 60.0f),
    m_previousRealTimeStep(1.0f / 60.0f),
    m_realTime(0), 
    m_simTime(0) {

    currentGApp = this;
#   ifdef G3D_DEBUG
        // Let the debugger catch them
        catchCommonExceptions = false;
#   else
        catchCommonExceptions = true;
#   endif

    logLazyPrintf("\nEntering GApp::GApp()\n");
    char b[2048];
    (void)getcwd(b, 2048);
    logLazyPrintf("cwd = %s\n", b);
    
    if (settings.dataDir == "<AUTO>") {
        dataDir = FilePath::parent(System::currentProgramFilename());
    } else {
        dataDir = settings.dataDir;
    }
    logPrintf("System::setAppDataDir(\"%s\")\n", dataDir.c_str());
    System::setAppDataDir(dataDir);

    if (settings.writeLicenseFile && ! FileSystem::exists("g3d-license.txt")) {
        writeLicense();
    }

    if (m_settings.screenshotDirectory != "") {
        if (! isSlash(m_settings.screenshotDirectory[m_settings.screenshotDirectory.length() - 1])) {
            m_settings.screenshotDirectory += "/";
        }
        debugAssertM(FileSystem::exists(m_settings.screenshotDirectory), 
            "GApp::Settings.screenshotDirectory set to non-existent directory " + m_settings.screenshotDirectory);
    }

    if (rd != NULL) {
        debugAssertM(window != NULL,
            "If you pass in your own RenderDevice, then you must also pass in your own OSWindow when creating a GApp.");

        _hasUserCreatedRenderDevice = true;
        _hasUserCreatedWindow       = true;
        renderDevice = rd;
    }
    else {
        _hasUserCreatedRenderDevice = false;
        renderDevice = new RenderDevice();
        if (window != NULL) {
            _hasUserCreatedWindow = true;
            renderDevice->init(window);
        } else {
            _hasUserCreatedWindow = false;    
            renderDevice->init(settings.window);
        }
    }
    debugAssertGLOk();

    _window = renderDevice->window();
    _window->makeCurrent();
    debugAssertGLOk();

    m_widgetManager = WidgetManager::create(_window);
    userInput = new UserInput(_window);
    m_debugController = FirstPersonManipulator::create(userInput);

    {
        TextOutput t;

        t.writeSymbols("System","=", "{");
        t.pushIndent();
        t.writeNewline();
        System::describeSystem(t);
        if (renderDevice) {
            renderDevice->describeSystem(t);
        }

        NetworkDevice::instance()->describeSystem(t);
        t.writeNewline();
        t.writeSymbol("};");
        t.writeNewline();

        std::string s;
        t.commitString(s);
        logPrintf("%s\n", s.c_str());
    }
    m_debugCamera  = Camera::create("(Debug Camera)");
    m_activeCamera = m_debugCamera;

    debugAssertGLOk();
    loadFont(settings.debugFontName);
    debugAssertGLOk();

    if (m_debugController) {
        m_debugController->onUserInput(userInput);
        m_debugController->setMoveRate(10);
        m_debugController->setPosition(Vector3(0, 0, 4));
        m_debugController->lookAt(Vector3::zero());
        m_debugController->setEnabled(false);
        m_debugCamera->setPosition(m_debugController->translation());
        m_debugCamera->lookAt(Vector3::zero());
        addWidget(m_debugController);
        setCameraManipulator(m_debugController);
    }
 
    showDebugText               = true;
    escapeKeyAction             = ACTION_QUIT;
    showRenderingStats          = true;
    manageUserInput             = true;

    {
        GConsole::Settings settings;
        settings.backgroundColor = Color3::green() * 0.1f;
        console = GConsole::create(debugFont, settings, staticConsoleCallback, this);
        console->setActive(false);
        addWidget(console);
    }

    if (settings.film.enabled) {
        alwaysAssertM(GLCaps::supports_GL_ARB_shading_language_100() && GLCaps::supports_GL_ARB_texture_non_power_of_two() &&
                      (GLCaps::supports_GL_ARB_framebuffer_object() || GLCaps::supports_GL_EXT_framebuffer_object()) &&
                      GLCaps::supportsTexelFetch(),
                      "Unsupported OpenGL version for G3D::Film");

        const ImageFormat* colorFormat = GLCaps::firstSupportedTexture(m_settings.film.preferredColorFormats);
        
        if (colorFormat == NULL) {
            // This GPU can't support the film class
            logPrintf("Warning: Disabled GApp::Settings::film.enabled because none of the provided color formats could be supported on this GPU.");
        } else {
            m_film = Film::create(colorFormat);
            m_frameBuffer = Framebuffer::create("GApp::m_frameBuffer");
            
            // The actual buffer allocation code:
            resize(renderDevice->width(), renderDevice->height());
        }
    }

    m_debugController->setMouseMode(FirstPersonManipulator::MOUSE_DIRECT_RIGHT_BUTTON);
    m_debugController->setEnabled(true);

    shared_ptr<GFont>    arialFont = GFont::fromFile(System::findDataFile("icon.fnt"));
    shared_ptr<GuiTheme> theme     = GuiTheme::fromFile(System::findDataFile("osx-10.7.gtm"), arialFont);

    debugWindow = GuiWindow::create("Control Window", theme, 
        Rect2D::xywh(0.0f, 0.0f, (float)settings.window.width, 150.0f), GuiTheme::PANEL_WINDOW_STYLE, GuiWindow::NO_CLOSE);
    debugPane = debugWindow->pane();
    debugWindow->setVisible(false);
    addWidget(debugWindow);

    debugAssertGLOk();

    m_simTime     = 0;
    m_realTime    = 0;
    m_lastWaitTime  = System::time();

    m_depthOfField = DepthOfField::create();
    m_motionBlur   = MotionBlur::create();

    renderDevice->setColorClearValue(Color3(0.1f, 0.5f, 1.0f));

    m_ambientOcclusion = AmbientOcclusion::create();

    logPrintf("Done GApp::GApp()\n\n");
}


const SceneVisualizationSettings& GApp::sceneVisualizationSettings() const {
    if (notNull(developerWindow) && notNull(developerWindow->sceneEditorWindow)) {
        return developerWindow->sceneEditorWindow->sceneVisualizationSettings();
    } else {
        static SceneVisualizationSettings v;
        return v;
    }
}


void GApp::createDeveloperHUD() {

    const shared_ptr<UprightSplineManipulator>& splineManipulator = UprightSplineManipulator::create(m_debugCamera);
    addWidget(splineManipulator);
        
    shared_ptr<GFont>      arialFont   = GFont::fromFile(System::findDataFile("arial.fnt"));
    shared_ptr<GuiTheme>   theme       = GuiTheme::fromFile(System::findDataFile("osx-10.7.gtm"), arialFont);

    developerWindow = DeveloperWindow::create
        (this,
            m_debugController, 
            splineManipulator,
            Pointer<shared_ptr<Manipulator> >(this, &GApp::cameraManipulator, &GApp::setCameraManipulator), 
            m_debugCamera,
            scene(),
            m_film,
            theme,
            console,
            Pointer<bool>(debugWindow, &GuiWindow::visible, &GuiWindow::setVisible),
            &showRenderingStats,
            &showDebugText,
            m_settings.screenshotDirectory);

    addWidget(developerWindow);
}

    
shared_ptr<GuiWindow> GApp::show(const shared_ptr<PixelTransferBuffer>& t, const std::string& windowCaption) {
    return show(Texture::fromPixelTransferBuffer("", t, NULL, Texture::DIM_2D_NPOT, Texture::Settings::buffer()), windowCaption);
}
    

shared_ptr<GuiWindow> GApp::show(const shared_ptr<Image>& t, const std::string& windowCaption) {
    return show(dynamic_pointer_cast<PixelTransferBuffer>(t->toPixelTransferBuffer()), windowCaption);
}


shared_ptr<GuiWindow> GApp::show(const shared_ptr<Texture>& t, const std::string& windowCaption) {
    static const Vector2 offset(25, 15);
    static Vector2 lastPos(0, 0);
    static float y0 = 0;
    
    lastPos += offset;

    std::string name;
    std::string dayTime;

    {
        // Use the current time as the name
        time_t t1;
        ::time(&t1);
        tm* t = localtime(&t1);
        static const char* day[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        int hour = t->tm_hour;
        const char* ap = "am";
        if (hour == 0) {
            hour = 12;
        } else if (hour >= 12) {
            ap = "pm";
            if (hour > 12) {
                hour -= 12;
            }
        }
        dayTime = format("%s %d:%02d:%02d %s", day[t->tm_wday], hour, t->tm_min, t->tm_sec, ap);
    }

    
    if (! windowCaption.empty()) {
        name = windowCaption + " - ";
    }
    name += dayTime;
    
    shared_ptr<GuiWindow> display = 
        GuiWindow::create(name, shared_ptr<GuiTheme>(), Rect2D::xywh(lastPos,Vector2(0,0)), GuiTheme::NORMAL_WINDOW_STYLE, GuiWindow::REMOVE_ON_CLOSE);

    GuiTextureBox* box = display->pane()->addTextureBox(t);
    box->setSizeFromInterior(t->vector2Bounds().min(Vector2(window()->width() * 0.9f, window()->height() * 0.9f)));
    box->zoomTo1();
    display->pack();

    // Cascade, but don't go off the screen
    if ((display->rect().x1() > window()->width()) || (display->rect().y1() > window()->height())) {
        lastPos = offset;
        lastPos.y += y0;
        y0 += offset.y;
        
        display->moveTo(lastPos);
        
        if (display->rect().y1() > window()->height()) {
            y0 = 0;
            lastPos = offset;
            display->moveTo(lastPos);
        }
    }
    
    addWidget(display);
    return display;
}


void GApp::drawMessage(const std::string& message) {
    drawTitle(message, "", Any(), Color3::black(), Color4(Color3::white(), 0.8f));
}


void GApp::drawTitle(const std::string& title, const std::string& subtitle, const Any& any, const Color3& fontColor, const Color4& backColor) {
    renderDevice->push2D();
    {
        // Background
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        Draw::fastRect2D(renderDevice->viewport(), renderDevice, backColor);

        // Text
        const shared_ptr<GFont> font = debugWindow->theme()->defaultStyle().font;
        const float titleWidth = font->bounds(title, 1).x;
        const float titleSize  = min(30.0f, renderDevice->viewport().width() / titleWidth * 0.80f);
        font->draw2D(renderDevice, title, renderDevice->viewport().center(), titleSize, 
                     fontColor, backColor, GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
        float subtitleSize = 0.0f;
        if (subtitle != "") {
            const float subtitleWidth = font->bounds(subtitle, 1).x;
            subtitleSize = min(22.5f, renderDevice->viewport().width() / subtitleWidth * 0.60f);
            font->draw2D(renderDevice, subtitle, renderDevice->viewport().center() + Vector2(0.0f, font->bounds(title, titleSize).y), subtitleSize, 
                         fontColor, backColor, GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
        }
        if (!any.isNil()) {
            any.verifyType(Any::TABLE);
            const float anyTextSize = 20.0f;
            const float baseHeight = renderDevice->viewport().center().y + font->bounds(title, titleSize).y + font->bounds(subtitle, subtitleSize).y;
            const int maxEntriesPerColumn = (int) ((renderDevice->viewport().height() - baseHeight) / font->bounds("l", anyTextSize).y);
            const int cols                = (int) (1 + any.size() / maxEntriesPerColumn);

            Array<std::string> keys = any.table().getKeys();
            //determine the longest key in order to allign columns well
            Array<float> keyWidth;
            for (int c = 0; c < any.size()/cols; ++c) {
                keyWidth.append(0.0f);
                for (int i = c * maxEntriesPerColumn; i < min((c+1) * maxEntriesPerColumn, any.size()); ++i) {
                    float kwidth = font->bounds(keys[i], anyTextSize).x;
                    keyWidth[c]  = kwidth > keyWidth[c] ? kwidth : keyWidth[c];
                }
            }
            
            const float horizontalBuffer  = font->bounds("==", anyTextSize).x;
            const float heightIncrement   = font->bounds("==", anyTextSize).y;

            //distance from an edge of a screen to the center of a column, and between centers of columns
            const float centerDist = renderDevice->viewport().width() / (2 * cols);

            for (int c = 0; c < any.size()/cols; ++c) {
                float height = baseHeight;
                for (int i = c * maxEntriesPerColumn; i < min((c+1) * maxEntriesPerColumn, any.size()); ++i) {
                    float columnIndex = 2.0f*c + 1.0f;
                    font->draw2D(renderDevice, keys[i], Vector2(centerDist * (columnIndex) - (horizontalBuffer + keyWidth[c]), height), anyTextSize, 
                         fontColor, backColor, GFont::XALIGN_LEFT, GFont::YALIGN_CENTER);
                    font->draw2D(renderDevice, " = ", Vector2(centerDist * (columnIndex), height), anyTextSize, 
                         fontColor, backColor, GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
                    font->draw2D(renderDevice, any[keys[i]].unparse(), Vector2(centerDist * (columnIndex) + horizontalBuffer, height), anyTextSize, 
                         fontColor, backColor, GFont::XALIGN_LEFT, GFont::YALIGN_CENTER);
                    height += heightIncrement;
                }
            }
        }   
    }
    renderDevice->pop2D();
    renderDevice->swapBuffers();
}


void GApp::setExitCode(int code) {
    m_endProgram = true;
    m_exitCode = code;
}


void GApp::loadFont(const std::string& fontName) {
    logPrintf("Entering GApp::loadFont(\"%s\")\n", fontName.c_str());
    const std::string& filename = System::findDataFile(fontName);
    logPrintf("Found \"%s\" at \"%s\"\n", fontName.c_str(), filename.c_str());
    if (FileSystem::exists(filename)) {
        debugFont = GFont::fromFile(filename);
    } else {
        logPrintf(
            "Warning: G3D::GApp could not load font \"%s\".\n"
            "This may be because the G3D::GApp::Settings::dataDir was not\n"
            "properly set in main().\n",
            filename.c_str());

        debugFont.reset();
    }
    logPrintf("Done GApp::loadFont(...)\n");
}


GApp::~GApp() {
    currentGApp = NULL;

    // Drop pointers to all OpenGL resources before shutting down the RenderDevice
    m_cameraManipulator.reset();

    m_film.reset();
    m_posed3D.clear();
    m_posed2D.clear();
    m_frameBuffer.reset();
    m_widgetManager.reset();
    developerWindow.reset();
    debugShapeArray.clear();
    debugLabelArray.clear();

    debugPane = NULL;
    debugWindow.reset();
    m_debugController.reset();
    m_debugCamera.reset();
    m_activeCamera.reset();

    NetworkDevice::cleanup();

    debugFont.reset();
    delete userInput;
    userInput = NULL;

    VertexBuffer::cleanupAllVertexBuffers();
    if (!_hasUserCreatedRenderDevice && _hasUserCreatedWindow) {
        // Destroy the render device explicitly (rather than waiting for the Window to do so)
        renderDevice->cleanup();
        delete renderDevice;
    }
    renderDevice = NULL;

    if (! _hasUserCreatedWindow) {
        delete _window;
        _window = NULL;
    }
}


int GApp::run() {
    int ret = 0;
    if (catchCommonExceptions) {
        try {
            onRun();
            ret = m_exitCode;
        } catch (const char* e) {
            alwaysAssertM(false, e);
            ret = -1;
        } catch (const Image::Error& e) {
            alwaysAssertM(false, e.reason + "\n" + e.filename);
            ret = -1;
        } catch (const std::string& s) {
            alwaysAssertM(false, s);
            ret = -1;
        } catch (const TextInput::WrongTokenType& t) {
            alwaysAssertM(false, t.message);
            ret = -1;
        } catch (const TextInput::WrongSymbol& t) {
            alwaysAssertM(false, t.message);
            ret = -1;
        } catch (const LightweightConduit::PacketSizeException& e) {
            alwaysAssertM(false, e.message);
            ret = -1;
        } catch (const ParseError& e) {
            alwaysAssertM(false, e.formatFileInfo() + e.message);
            ret = -1;
        } catch (const FileNotFound& e) {
            alwaysAssertM(false, e.message);
            ret = -1;
        }
    } else {
        onRun();
        ret = m_exitCode;
    }

    return ret;
}


void GApp::onRun() {
    if (window()->requiresMainLoop()) {
        
        // The window push/pop will take care of 
        // calling beginRun/oneFrame/endRun for us.
        window()->pushLoopBody(this);

    } else {
        beginRun();

        // Main loop
        do {
            oneFrame();
        } while (! m_endProgram);

        endRun();
    }
}


void GApp::renderDebugInfo() {
    if (debugFont && (showRenderingStats || (showDebugText && (debugText.length() > 0)))) {
        // Capture these values before we render debug output
        int majGL  = renderDevice->stats().majorOpenGLStateChanges;
        int majAll = renderDevice->stats().majorStateChanges;
        int minGL  = renderDevice->stats().minorOpenGLStateChanges;
        int minAll = renderDevice->stats().minorStateChanges;
        int pushCalls = renderDevice->stats().pushStates;

        renderDevice->push2D();
            const static float size = 10;
            if (showRenderingStats) {
                renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
                Draw::fastRect2D(Rect2D::xywh(2.0f, 2.0f, renderDevice->width() - 4.0f, size * 5.8f + 2), renderDevice, Color4(0, 0, 0, 0.3f));
            }

            debugFont->begin2DQuads(renderDevice);
            float x = 5;
            Vector2 pos(x, 5);

            if (showRenderingStats) {

                Color3 statColor = Color3::yellow();
                debugFont->begin2DQuads(renderDevice);

                const char* build = 
#               ifdef G3D_DEBUG
                    "";
#               else
                    " (Optimized)";
#               endif

                static const std::string description = renderDevice->getCardDescription() + "   " + System::version() + build;
                debugFont->send2DQuads(renderDevice, description, pos, size, Color3::white());
                pos.y += size * 1.5f;
                
                float fps = renderDevice->stats().smoothFrameRate;
                const std::string& s = format(
                    "% 4d fps (% 3d ms)  % 5.1fM tris  GL Calls: %d/%d Maj;  %d/%d Min;  %d push; %d Surfaces; %d Surface2Ds", 
                    iRound(fps),
                    iRound(1000.0f / fps),
                    iRound(renderDevice->stats().smoothTriangles / 1e5) * 0.1f,
                    /*iRound(renderDevice->stats().smoothTriangleRate / 1e4) * 0.01f,*/
                    majGL, majAll, minGL, minAll, pushCalls,
                    m_posed3D.size(), m_posed2D.size());
                debugFont->send2DQuads(renderDevice, s, pos, size, statColor);

                pos.x = x;
                pos.y += size * 1.5f;

                {
                    int g = iRound(m_graphicsWatch.smoothElapsedTime() / units::milliseconds());
                    int p = iRound(m_poseWatch.smoothElapsedTime() / units::milliseconds());
                    int n = iRound(m_networkWatch.smoothElapsedTime() / units::milliseconds());
                    int s = iRound(m_simulationWatch.smoothElapsedTime() / units::milliseconds());
                    int L = iRound(m_logicWatch.smoothElapsedTime() / units::milliseconds());
                    int u = iRound(m_userInputWatch.smoothElapsedTime() / units::milliseconds());
                    int w = iRound(m_waitWatch.smoothElapsedTime() / units::milliseconds());

                    int swapTime = iRound(renderDevice->swapBufferTimer().smoothElapsedTime() / units::milliseconds());

                    const std::string& str = 
                        format("Time:%4d ms Gfx,%4d ms Swap,%4d ms Sim,%4d ms Pose,%4d ms AI,%4d ms Net,%4d ms UI,%4d ms idle", 
                               g, swapTime, s, p, L, n, u, w);
                    debugFont->send2DQuads(renderDevice, str, pos, size, statColor);
                }

                pos.x = x;
                pos.y += size * 1.5f;

                const char* esc = NULL;
                switch (escapeKeyAction) {
                case ACTION_QUIT:
                    esc = "ESC: QUIT      ";
                    break;
                case ACTION_SHOW_CONSOLE:
                    esc = "ESC: CONSOLE   ";
                    break;
                case ACTION_NONE:
                    esc = "               ";
                    break;
                }

                const char* screenshot = 
                    (developerWindow && 
                     developerWindow->videoRecordDialog &&
                     developerWindow->videoRecordDialog->enabled()) ?
                    "F4: SCREENSHOT  " :
                    "                ";

                const char* reload = "F5: RELOAD SHADERS ";

                const char* video = 
                    (developerWindow && 
                     developerWindow->videoRecordDialog &&
                     developerWindow->videoRecordDialog->enabled()) ?
                    "F6: MOVIE     " :
                    "              ";

                const char* camera =
                    (cameraManipulator() && 
                     m_debugController) ? 
                    "F2: DEBUG CAMERA  " :
                    "                  ";

                const char* cubemap = "F8: RENDER CUBEMAP";

                const char* time = notNull(developerWindow) && notNull(developerWindow->sceneEditorWindow) ? 
                    "F9: START/STOP TIME ":
                    "                    ";
                    
                const char* dev = developerWindow ? 
                    "F11: DEV WINDOW":
                    "               ";

                const std::string& Fstr = format("%s     %s     %s     %s     %s     %s     %s     %s", esc, camera, screenshot, reload, video, cubemap, time, dev);
                debugFont->send2DQuads(renderDevice, Fstr, pos, 8, Color3::white());

                pos.x = x;
                pos.y += size;

            }

            m_debugTextMutex.lock();
            for (int i = 0; i < debugText.length(); ++i) {
                debugFont->send2DQuads(renderDevice, debugText[i], pos, size, m_debugTextColor, m_debugTextOutlineColor);
                pos.y += size * 1.5f;
            }
            m_debugTextMutex.unlock();
            debugFont->end2DQuads(renderDevice);
        renderDevice->pop2D();
    }
}


void GApp::loadScene(const std::string& sceneName) {
    // Use immediate mode rendering to force a simple message onto the screen
    drawMessage("Loading " + sceneName + "...");

    const std::string oldSceneName = scene()->name();

    // Load the scene
    try {
        const Any& any = scene()->load(sceneName);

        // TODO: Parse extra fields that you've added to the .scn.any
        // file here.
        (void)any;

        // If the debug camera was active and the scene is the same as before, retain the old camera.  Otherwise,
        // switch to the default camera specified by the scene.

        if ((oldSceneName != scene()->name()) || (activeCamera()->name() != "(Debug Camera)")) {

            // Because the CameraControlWindow is hard-coded to the
            // m_debugCamera, we have to copy the camera's values here
            // instead of assigning a pointer to it.
            m_debugCamera->copyParametersFrom(scene()->defaultCamera());
            m_debugController->setFrame(m_debugCamera->frame());

            setActiveCamera(scene()->defaultCamera());
        }

    } catch (const ParseError& e) {
        const std::string& msg = e.filename + format(":%d(%d): ", e.line, e.character) + e.message;
        debugPrintf("%s", msg.c_str());
        drawMessage(msg);
        System::sleep(5);
        scene()->clear();
    }
}


void GApp::saveScene() {
    // Called when the "save" button is pressed
    if (scene()) {
        Any a = scene()->toAny();
        const std::string& filename = a.source().filename;
        if (filename != "") {
            a.save(filename);
            debugPrintf("Saved %s\n", filename.c_str());
        } else {
            debugPrintf("Could not save: empty filename");
        }
    }
}


bool GApp::onEvent(const GEvent& event) {
    if (event.type == GEventType::VIDEO_RESIZE) {
        resize(event.resize.w, event.resize.h);
        // Don't consume the resize event--we want subclasses to be able to handle it as well
        return false;
    }

    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::F5)) { 
        Shader::reloadAll();
        return true;
    } else if (event.type == GEventType::KEY_DOWN && event.key.keysym.sym == GKey::F8) {
        Array<shared_ptr<Texture> > output;
        renderCubeMap(renderDevice, output, m_debugCamera, shared_ptr<Texture>(), 2048);
        drawMessage("Saving Cube Map...");
        const Texture::CubeMapInfo& cubeMapInfo = Texture::cubeMapInfo(CubeMapConvention::DIRECTX);
        for (int f = 0; f < 6; ++f) {
            const Texture::CubeMapInfo::Face& faceInfo = cubeMapInfo.face[f];
            //shared_ptr<PixelTransferBuffer> buf;
            shared_ptr<Image> temp = Image::fromPixelTransferBuffer(output[f]->toPixelTransferBuffer(ImageFormat::RGB8()));
            temp->flipVertical();
            temp->rotateCW(toRadians(90.0) * (-faceInfo.rotations));
            if (faceInfo.flipY) { temp->flipVertical();   }
            if (faceInfo.flipX) { temp->flipHorizontal(); }
            temp->save(format("cube-%s.png", faceInfo.suffix.c_str()));
        }
        return true;
    } else if (event.type == GEventType::FILE_DROP) {
        Array<std::string> fileArray;
        window()->getDroppedFilenames(fileArray);

        if (endsWith(toLower(fileArray[0]), ".scn.any") || endsWith(toLower(fileArray[0]), ".scene.any")) {

            // Load a scene
            loadScene(fileArray[0]);
            return true;

        } else if (endsWith(toLower(fileArray[0]), ".am.any") || endsWith(toLower(fileArray[0]), ".articulatedmodel.any")) {

            // Trace a ray from the drop point
            Model::HitInfo hitInfo;
            scene()->intersectEyeRay(activeCamera(), Vector2(event.drop.x + 0.5f, event.drop.y + 0.5f), renderDevice->viewport(), settings().depthGuardBandThickness, false, Array<shared_ptr<Entity> >(), hitInfo);

            if (hitInfo.point.isNaN()) {
                // The drop wasn't on a surface, so choose a point in front of the camera at a reasonable distance
                CFrame cframe = activeCamera()->frame();
                hitInfo.set(shared_ptr<Model>(), shared_ptr<Entity>(), shared_ptr<Material>(), Vector3::unitY(), cframe.lookVector() * 4 + cframe.translation);
            }

            // Insert a Model
            Any modelAny;
            modelAny.load(fileArray[0]);
            int nameModifier = -1;
            Array<std::string> entityNames;
            scene()->getEntityNames(entityNames);
            
            // creates a unique name in order to avoid conflicts from multiple models being dragged in
            std::string name = FilePath::base(fileArray[0]);
            if (entityNames.contains(name)) { 
                do {
                    ++nameModifier;
                } while (entityNames.contains(name + (format("%d", nameModifier))));
                name.append(format("%d", nameModifier));
            }

            const std::string& newModelName  = name;
            const std::string& newEntityName = name;
            
            scene()->createModel(modelAny, newModelName);

            Any entityAny(Any::TABLE, "VisibleEntity");
            // Insert an Entity for that model
            entityAny["frame"] = CFrame(hitInfo.point);
            entityAny["model"] = newModelName;

            scene()->createEntity("VisibleEntity", newEntityName, entityAny);
            return true;
        }
    } else if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == 'g') && (activeCamera() == m_debugCamera)) {
        Model::HitInfo info;
        Vector2 mouse;
        uint8 ignore;
        window()->getRelativeMouseState(mouse, ignore);
        const shared_ptr<Entity>& selection = 
            scene()->intersectEyeRay(activeCamera(), mouse + Vector2(0.5f,0.5f), renderDevice->viewport(), 
                        settings().depthGuardBandThickness, sceneVisualizationSettings().showMarkers, Array<shared_ptr<Entity> >(), info);

        if (notNull(selection)) {   
            m_debugCamera->setFrame(CFrame(m_debugCamera->frame().rotation, info.point + (m_debugCamera->frame().rotation * Vector3(0.0f, 0.0f, 1.5f))));
            m_debugController->setFrame(m_debugCamera->frame());
        }
    }

    return false;
}


void GApp::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces) {
    drawDebugShapes();
}


void GApp::onGraphics2D(RenderDevice* rd, Array<Surface2DRef>& posed2D) {
    Surface2D::sortAndRender(rd, posed2D);
}


void GApp::onGraphics(RenderDevice* rd, Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) {
    rd->pushState(); {
        debugAssert(notNull(activeCamera()));
        rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
        onGraphics3D(rd, posed3D);
    } rd->popState();
    debugAssertGLOk();

    rd->push2D(); {
        onGraphics2D(rd, posed2D);
    } rd->pop2D();
    debugAssertGLOk();
}


void GApp::addWidget(const shared_ptr<Widget>& module, bool setFocus) {
    m_widgetManager->add(module);
    
    if (setFocus) {
        m_widgetManager->setFocusedWidget(module);
    }
}


void GApp::removeWidget(const shared_ptr<Widget>& module) {
    m_widgetManager->remove(module);
}


void GApp::resize(int w, int h) {
    // Add the color guard band

    /**ensure a minimum size before guard band*/

    w = max(8,w);
    h = max(8,h);
    w += m_settings.depthGuardBandThickness.x * 2;
    h += m_settings.depthGuardBandThickness.y * 2;

    // Does the film need to be reallocated?  Do this even if we
    // aren't using it at the moment, but not if we are minimized
    if (m_film && !window()->isIconified() &&
        (isNull(m_colorBuffer0) ||
        (m_colorBuffer0->width() != w) ||
        (m_colorBuffer0->height() != h))) {

        m_frameBuffer->clear();

        const ImageFormat* colorFormat = GLCaps::firstSupportedTexture(m_settings.film.preferredColorFormats);
        const ImageFormat* depthFormat = GLCaps::firstSupportedTextureOrRenderBuffer(m_settings.film.preferredDepthFormats);

        m_colorBuffer0 = Texture::createEmpty("GApp::m_colorBuffer0", w, h, colorFormat, Texture::DIM_2D_NPOT, Texture::Settings::buffer(), 1);

        m_frameBuffer->set(Framebuffer::COLOR0, m_colorBuffer0);

        if (notNull(depthFormat)) {
            // Prefer creating a texture if we can

            const Framebuffer::AttachmentPoint p = (depthFormat->stencilBits > 0) ? Framebuffer::DEPTH_AND_STENCIL : Framebuffer::DEPTH;
            if (GLCaps::supportsTexture(depthFormat)) {
                m_depthBuffer  = Texture::createEmpty("GApp::m_depthBuffer", w, h, depthFormat, Texture::DIM_2D_NPOT, Texture::Settings::buffer(), 1);
                m_frameBuffer->set(p, m_depthBuffer);
            } else {
                m_depthRenderBuffer  = Renderbuffer::createEmpty("GApp::m_depthRenderBuffer", w, h, depthFormat);
                m_frameBuffer->set(p, m_depthRenderBuffer);
            }
        }
    }
}


void GApp::oneFrame() {
    currentGApp = this;

    for (int repeat = 0; repeat < max(1, m_renderPeriod); ++repeat) {
		Profiler::nextFrame();
        m_lastTime = m_now;
        m_now = System::time();
        RealTime timeStep = m_now - m_lastTime;

        // User input
        m_userInputWatch.tick();
        if (manageUserInput) {
            processGEventQueue();
        }
        onAfterEvents();
        onUserInput(userInput);
        m_userInputWatch.tock();

        // Network
        m_networkWatch.tick();
        onNetwork();
        m_networkWatch.tock();

        // Logic
        m_logicWatch.tick();
        {
            onAI();
        }
        m_logicWatch.tock();

        // Simulation
        m_simulationWatch.tick();
        {
            RealTime rdt = timeStep;

            SimTime sdt = m_simTimeStep;
            if (sdt == MATCH_REAL_TIME_TARGET) {
                sdt = m_wallClockTargetDuration;
            } else if (sdt == REAL_TIME) {
                sdt = timeStep;
            }
            sdt *= m_simTimeScale;

            SimTime idt = m_wallClockTargetDuration;

            onBeforeSimulation(rdt, sdt, idt);
            onSimulation(rdt, sdt, idt);
            onAfterSimulation(rdt, sdt, idt);

            if (m_cameraManipulator) {
                m_debugCamera->setFrame(m_cameraManipulator->frame());
            }

            m_previousSimTimeStep = float(sdt);
            m_previousRealTimeStep = float(rdt);
            setRealTime(realTime() + rdt);
            setSimTime(simTime() + sdt);
        }
        m_simulationWatch.tock();
    }


    // Pose
    m_poseWatch.tick(); {
        m_posed3D.fastClear();
        m_posed2D.fastClear();
        onPose(m_posed3D, m_posed2D);
    } m_poseWatch.tock();

    // Wait 
    // Note: we might end up spending all of our time inside of
    // RenderDevice::beginFrame.  Waiting here isn't double waiting,
    // though, because while we're sleeping the CPU the GPU is working
    // to catch up.

    m_waitWatch.tick(); {
        RealTime nowAfterLoop = System::time();

        // Compute accumulated time
        RealTime cumulativeTime = nowAfterLoop - m_lastWaitTime;

        // Perform wait for actual time needed
        RealTime duration = m_wallClockTargetDuration;
        if (! window()->hasFocus() && m_lowerFrameRateInBackground) {
            // Lower frame rate
            duration = 1.0 / BACKGROUND_FRAME_RATE;
        }
        RealTime desiredWaitTime = max(0.0, duration - cumulativeTime);
        onWait(max(0.0, desiredWaitTime - m_lastFrameOverWait) * 0.97);

        // Update wait timers
        m_lastWaitTime = System::time();
        RealTime actualWaitTime = m_lastWaitTime - nowAfterLoop;

        // Learn how much onWait appears to overshoot by and compensate
        double thisOverWait = actualWaitTime - desiredWaitTime;
        if (abs(thisOverWait - m_lastFrameOverWait) / max(abs(m_lastFrameOverWait), abs(thisOverWait)) > 0.4) {
            // Abruptly change our estimate
            m_lastFrameOverWait = thisOverWait;
        } else {
            // Smoothly change our estimate
            m_lastFrameOverWait = lerp(m_lastFrameOverWait, thisOverWait, 0.1);
        }
    }  m_waitWatch.tock();


    // Graphics
    renderDevice->beginFrame();
    m_graphicsWatch.tick(); {
        renderDevice->pushState(); {
            onGraphics(renderDevice, m_posed3D, m_posed2D);
        } renderDevice->popState();
        renderDebugInfo();
    }  m_graphicsWatch.tock();

    renderDevice->endFrame();

    // Remove all expired debug shapes
    for (int i = 0; i < debugShapeArray.size(); ++i) {
        if (debugShapeArray[i].endTime <= m_now) {
            debugShapeArray.fastRemove(i);
            --i;
        }
    }

    for (int i = 0; i < debugLabelArray.size(); ++i) {
        if (debugLabelArray[i].endTime <= m_now) {
            debugLabelArray.fastRemove(i);
            --i;
        }
    }

    debugText.fastClear();

    if (m_endProgram && window()->requiresMainLoop()) {
        window()->popLoopBody();
    }
}


void GApp::swapBuffers() {
    if (m_activeVideoRecordDialog) {
        m_activeVideoRecordDialog->maybeRecord(renderDevice);        
    }
	renderDevice->swapBuffers();
}


void GApp::drawDebugShapes() {
    renderDevice->setObjectToWorldMatrix(CFrame());

    if (debugShapeArray.size() > 0) {

        renderDevice->setPolygonOffset(-1.0f);
        for (int i = 0; i < debugShapeArray.size(); ++i) {
            const DebugShape& s = debugShapeArray[i];
            s.shape->render(renderDevice, s.frame, s.solidColor, s.wireColor); 
        }
        renderDevice->setPolygonOffset(0.0f);
    }

    if (debugLabelArray.size() > 0) {
        for (int i = 0; i < debugLabelArray.size(); ++i) {
            const DebugLabel& label = debugLabelArray[i];
            if (label.text.text() != "") {
                static const shared_ptr<GFont> defaultFont = GFont::fromFile(System::findDataFile("arial.fnt"));
                const shared_ptr<GFont> f = label.text.element(0).font(defaultFont);
                f->draw3DBillboard(renderDevice, label.text, label.wsPos, label.size, label.text.element(0).color(Color3::black()), Color4::clear(), label.xalign, label.yalign);
            }
        }
    }
}


void GApp::removeAllDebugShapes() {
    debugShapeArray.fastClear();
    debugLabelArray.fastClear();
}


void GApp::removeDebugShape(DebugID id) {
    for (int i = 0; i < debugShapeArray.size(); ++i) {
        if (debugShapeArray[i].id == id) {
            debugShapeArray.fastRemove(i);
            return;
        }
    }
}


void GApp::onWait(RealTime t) {
    System::sleep(max(0.0, t));
}


void GApp::setRealTime(RealTime r) {
    m_realTime = r;
}


void GApp::setSimTime(SimTime s) {
    m_simTime = s;
}


void GApp::onInit() {
    setScene(Scene::create());
}


void GApp::onCleanup() {}


void GApp::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    m_debugController->setEnabled(activeCamera() == m_debugCamera);

    m_widgetManager->onSimulation(rdt, sdt, idt);

    if (scene()) { scene()->onSimulation(sdt); }

    // Update the camera's previous frame.  The debug camera
    // is usually controlled by the camera manipulator and is
    // a copy of one from a scene, but is not itself in the scene, so it needs an explicit
    // simulation call here.
    m_debugCamera->onSimulation(0, idt);
}


void GApp::onBeforeSimulation(RealTime& rdt, SimTime& sdt, SimTime& idt) {        
    (void)idt;
    (void)rdt;
    (void)sdt;
}


void GApp::onAfterSimulation(RealTime rdt, SimTime sdt, SimTime idt) {        
    (void)idt;
    (void)rdt;
    (void)sdt;
}


void GApp::onPose(Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) {
    m_widgetManager->onPose(surface, surface2D);

    if (scene()) {
        scene()->onPose(surface);
    }
}


void GApp::onNetwork() {
    m_widgetManager->onNetwork();
}


void GApp::onAI() {
    m_widgetManager->onAI();
}


void GApp::beginRun() {
    currentGApp  = this;
    m_endProgram = false;
    m_exitCode = 0;

    onInit();

    // Move the controller to the camera's location
    if (m_debugController) {
        m_debugController->setFrame(m_debugCamera->frame());
    }

    m_now = System::time() - 0.001;
}


void GApp::endRun() {
    currentGApp = this;

    onCleanup();

    Log::common()->section("Files Used");
    Set<std::string>::Iterator end = FileSystem::usedFiles().end();
    Set<std::string>::Iterator f = FileSystem::usedFiles().begin();
    
    while (f != end) {
        Log::common()->println(*f);
        ++f;
    }
    Log::common()->println("");

    if (window()->requiresMainLoop() && m_endProgram) {
        ::exit(m_exitCode);
    }
}


void GApp::staticConsoleCallback(const std::string& command, void* me) {
    ((GApp*)me)->onConsoleCommand(command);
}


void GApp::onConsoleCommand(const std::string& cmd) {
    if (trimWhitespace(cmd) == "exit") {
        setExitCode(0);
        return;
    }
}


void GApp::onUserInput(UserInput* userInput) {
    m_widgetManager->onUserInput(userInput);
}


void GApp::processGEventQueue() {
    userInput->beginEvents();

    // Event handling
    GEvent event;
    while (window()->pollEvent(event)) {
        bool eventConsumed = false;

        // For event debugging
        //if (event.type != GEventType::MOUSE_MOTION) {
        //    printf("%s\n", event.toString().c_str());
        //}

        eventConsumed = WidgetManager::onEvent(event, m_widgetManager);

        if (! eventConsumed) {

            eventConsumed = onEvent(event);

            if (! eventConsumed) {
                switch(event.type) {
                case GEventType::QUIT:
                    setExitCode(0);
                    break;

                case GEventType::KEY_DOWN:

                    if (isNull(console) || ! console->active()) {
                        switch (event.key.keysym.sym) {
                        case GKey::ESCAPE:
                            switch (escapeKeyAction) {
                            case ACTION_QUIT:
                                setExitCode(0);
                                break;
                    
                            case ACTION_SHOW_CONSOLE:
                                console->setActive(true);
                                eventConsumed = true;
                                break;

                            case ACTION_NONE:
                                break;
                            }
                            break;

                        // Add other key handlers here
                        default:;
                        }
                    }
                    break;

                // Add other event handlers here

                default:;
                }
            } // consumed
        } // consumed


        // userInput sees events if they are not consumed, or if they are release events
        if (! eventConsumed || (event.type == GEventType::MOUSE_BUTTON_UP) || (event.type == GEventType::KEY_UP)) {
            // userInput always gets to process events, so that it
            // maintains the current state correctly.
            userInput->processEvent(event);
        }
    }

    userInput->endEvents();
}


void GApp::onAfterEvents() {
    m_widgetManager->onAfterEvents();
}


void GApp::setActiveCamera(const shared_ptr<Camera>& camera) {
    m_activeCamera = camera;
}


void GApp::renderCubeMap(RenderDevice* rd, Array<shared_ptr<Texture> >& output, const shared_ptr<Camera>& camera, shared_ptr<Texture> depthMap, int resolution) {
    // Obtain the surface argument needed later for onGraphics
    Array<shared_ptr<Surface> > surface;
    {
        Array<shared_ptr<Surface2D> > ignore;
        onPose(surface, ignore);
    }
const ImageFormat* imageFormat = ImageFormat::RGB16F();
    if ((output.size() == 0) || isNull(output[0])) {
        // allocate cube maps
        output.resize(6);
        for (int face = 0; face < 6; ++face) {
            output[face] = Texture::createEmpty(CubeFace(face).toString(), resolution, resolution, imageFormat, Texture::DIM_2D_NPOT, Texture::Settings::buffer());
        }
    }

    
    shared_ptr<Texture> oldColorBuffer = m_colorBuffer0;
    shared_ptr<Framebuffer> oldFrameBuffer = m_frameBuffer;

    m_frameBuffer = Framebuffer::create("temp");
    m_frameBuffer->set(Framebuffer::DEPTH, m_depthBuffer);

    shared_ptr<Camera> oldCamera = activeCamera(); 
    const Projection oldProjection = camera->projection();
    const CFrame     oldCFrame     = camera->frame();
    bool motionBlur = camera->motionBlurSettings().enabled();
    bool depthOfField = camera->depthOfFieldSettings().enabled();

    camera->depthOfFieldSettings().setEnabled(false);
    camera->motionBlurSettings().setEnabled(false);

    Vector2int16 colorGuard = m_settings.colorGuardBandThickness;
    Vector2int16 depthGuard = m_settings.depthGuardBandThickness;

    m_settings.colorGuardBandThickness = Vector2int16(128, 128);
    m_settings.depthGuardBandThickness = Vector2int16(256, 256);

    camera->setFieldOfView(2.0f * ::atan(1.0f + 2.0f*(float(m_settings.colorGuardBandThickness.x) / float(resolution)) ), FOVDirection::HORIZONTAL);
    // Configure the base camera
    CFrame cframe = camera->frame();

    for (int face = 0; face < 6; ++face) {

        m_colorBuffer0 = Texture::createEmpty(CubeFace(face).toString(), resolution + 2*m_settings.colorGuardBandThickness.x, resolution + 2*m_settings.colorGuardBandThickness.y, imageFormat, Texture::DIM_2D_NPOT, Texture::Settings::buffer());
        m_frameBuffer->set(Framebuffer::COLOR0, m_colorBuffer0);
        Texture::getCubeMapRotation(CubeFace(face), cframe.rotation);
        camera->setFrame(cframe);
        setActiveCamera(camera);
        onGraphics3D(rd, surface);
        m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_colorBuffer0, output[face]);
    }
    m_frameBuffer = oldFrameBuffer;
    m_colorBuffer0 = oldColorBuffer;
    camera->setProjection(oldProjection);
    camera->setFrame(oldCFrame);
    camera->depthOfFieldSettings().setEnabled(depthOfField);
    camera->motionBlurSettings().setEnabled(motionBlur);
    setActiveCamera(oldCamera);
    m_settings.colorGuardBandThickness = colorGuard;
    m_settings.depthGuardBandThickness = depthGuard;

}
}
