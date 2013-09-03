/**
   \file GLG3D/GApp.h
 
   \maintainer Morgan McGuire, http://graphics.cs.williams.edu

   \created 2003-11-03
   \edited  2013-07-31

   Copyright 2000-2013, Morgan McGuire.
   All rights reserved.
*/

#ifndef G3D_GApp_h
#define G3D_GApp_h

#include "G3D/Stopwatch.h"
#include "G3D/CoordinateFrame.h"
#include "GLG3D/Camera.h"
#include "G3D/NetworkDevice.h"
#include "G3D/GThread.h"
#include "GLG3D/GFont.h"
#include "GLG3D/FirstPersonManipulator.h"
#include "GLG3D/OSWindow.h"
#include "GLG3D/Widget.h"
#include "GLG3D/GConsole.h"
#include "GLG3D/DeveloperWindow.h"
#include "GLG3D/Shape.h"
#include "GLG3D/Film.h"
#include "GLG3D/Shape.h"
#include "GLG3D/Renderbuffer.h"
#include "GLG3D/Scene.h"

namespace G3D {

// forward declare heavily dependent classes
class RenderDevice;
class UserInput;
class DepthOfField;
class MotionBlur;
class Log;
class Scene;

/** Used with debugDraw. */
typedef int DebugID;

/**
 \brief Schedule a G3D::Shape for later rendering.

 Adds this shape and the specified information to the current G3D::GApp::debugShapeArray, 
 to be rendered at runtime for debugging purposes.

 Sample usage is:
 \code
 debugDraw(new SphereShape(Sphere(center, radius)));
 \endcode

 \beta

 \param displayTime Real-world time in seconds to display the shape
 for.  A shape always displays for at least one frame. 0 = one frame.
 inf() = until explicitly removed by the GApp.

 \return The ID of the shape, which can be used to clear it for shapes that are displayed "infinitely".

 \sa debugPrintf, logPrintf, screenPrintf, GApp::drawDebugShapes, GApp::removeDebugShape, GApp::removeAllDebugShapes
 */
DebugID debugDraw
(const shared_ptr<Shape>&           shape, 
 float                              displayTime = 0.0f,
 const Color4&                      solidColor  = Color3::white(), 
 const Color4&                      wireColor   = Color3::black(), 
 const CoordinateFrame&             cframe      = CoordinateFrame());

/** overloaded forms of debugDraw so that more common parameters can be passed (e.g Boxes, Spheres) */
DebugID debugDraw
(const Box&                         b,
 float                              displayTime = 0.0f,
 const Color4&                      solidColor  = Color3::white(), 
 const Color4&                      wireColor   = Color3::black(), 
 const CoordinateFrame&             cframe      = CoordinateFrame());

DebugID debugDraw
(const Array<Vector3>&              vertices,
 const Array<int>&                  indices,
 float                              displayTime = 0.0f,
 const Color4&                      solidColor  = Color3::white(), 
 const Color4&                      wireColor   = Color3::black(), 
 const CoordinateFrame&             cframe      = CoordinateFrame());

DebugID debugDraw
(const CPUVertexArray&              vertices,
 const Array<Tri>&                  tris,
 float                              displayTime = 0.0f,
 const Color4&                      solidColor  = Color3::white(), 
 const Color4&                      wireColor   = Color3::black(), 
 const CoordinateFrame&             cframe      = CoordinateFrame());

DebugID debugDraw
(const Sphere&                      s,
 float                              displayTime = 0.0f,
 const Color4&                      solidColor  = Color3::white(), 
 const Color4&                      wireColor   = Color3::black(), 
 const CoordinateFrame&             cframe      = CoordinateFrame());

DebugID debugDraw
(const CoordinateFrame&             cf,
 float                              displayTime = 0.0f,
 const Color4&                      solidColor  = Color3::white(), 
 const Color4&                      wireColor   = Color3::black(), 
 const CoordinateFrame&             cframe      = CoordinateFrame());

/** Draws a label onto the screen for debug purposes*/
DebugID debugDrawLabel
(const Point3&                      wsPos,
 const Vector3&                     csOffset,
 const GuiText                      text,
 float                              displayTime = 0.0f,
 float                              size = 0.1f, //size in meters, by default
 bool                               sizeInPixels = false,
 const GFont::XAlign                xalign  = GFont::XALIGN_CENTER,
 const GFont::YAlign                yalign  = GFont::YALIGN_CENTER);

DebugID debugDrawLabel
(const Point3&                      wsPos,
 const Vector3&                     csOffset,
 const std::string&                 text,
 const Color3&                      color,
 float                              displayTime = 0.0f,
 float                              size = 0.1f,
 bool                               sizeInPixels = false,
 const GFont::XAlign                xalign  = GFont::XALIGN_CENTER,
 const GFont::YAlign                yalign  = GFont::YALIGN_CENTER);


/**
 \brief Optional base class for quickly creating 3D applications.

 GApp has several event handlers implemented as virtual methods.  It invokes these in
 a cooperative, round-robin fashion.  This avoids the need for threads in most
 applications.  The methods are, in order of invocation from GApp::oneFrame:
 
 <ul>
 <li> GApp::onEvent - invoked once for each G3D::GEvent
 <li> GApp::onAfterEvents - latch any polled state before onUserInput processing
 <li> GApp::onUserInput - process the current state of the keyboard, mouse, and game pads
 <li> GApp::onNetwork - receive network packets; network <i>send</i> occurs wherever it is needed
 <li> GApp::onAI - game logic and NPC AI
 <li> GApp::onSimulation - physical simulation
 <li> GApp::onPose - create arrays of Surface and Surface2D for rendering
 <li> GApp::onWait - tasks to process while waiting for the next frame to start (when there is a refresh limiter)
 <li> GApp::onGraphics - render the Surface and Surface2D arrays.  By default, this invokes two helper methods:
   <ul>
    <li> GApp::onGraphics3D - render the Surface array and any immediate mode 3D 
    <li> GApp::onGraphics2D - render the Surface2D array and any immediate mode 2D 
   </ul>
 </ul>
 
 The GApp::run method starts the main loop.  It invokes GApp::onInit, runs the main loop
 until completion, and then invokes GApp::onCleanup.

 onWait runs before onGraphics because the beginning of onGraphics causes the CPU to block, waiting for the GPU
 to complete the previous frame.

 When you override a method, invoke the GApp version of that method to ensure that Widget%s still work
 properly.  This allows you to control whether your per-app operations occur before or after the Widget ones.

 \sa GApp::Settings, OSWindow, RenderDevice, G3D_START_AT_MAIN
*/
class GApp {
public:
    friend class OSWindow;
    friend class VideoRecordDialog;
    friend class DeveloperWindow;
    friend class SceneEditorWindow;

    class Settings {
    public:
        OSWindow::Settings       window;

        /**
           If "<AUTO>", will be set to the directory in which the executable resides.
           This is used to invoke System::setDataDir()
        */
        std::string             dataDir;
        
        /**
           Can be relative to the G3D data directory (e.g. "font/dominant.fnt")
           or relative to the current directory.
           Default is "console-small.fnt"
        */
        std::string             debugFontName;
        
        std::string             logFilename;

        /** If true, the G3D::DeveleloperWindow and G3D::CameraControlWindow will be enabled and
            accessible by pushing F12.
            These require osx.gtm, arial.fnt, greek.fnt, and icon.fnt to be in locations where
            System::findDataFile can locate them (the program working directory is one such location).
        */  
        bool                    useDeveloperTools;
        
        /** 
            When true, GAapp ensures that g3d-license.txt exists in the current
            directory.  That file is written from the return value of G3D::license() */
        bool                    writeLicenseFile;

        /** 
            The default call to Film::exposeAndRender in the sample "starter" project crops these off, and the default 
            App::onGraphics3D in that project. 

            The use of a guard band allows screen-space effects to avoid boundary cases at the edge of the screen. For example,
            G3D::AmbientOcclusion, G3D::MotionBlur, and G3D::DepthOfField.

            Guard band pixels count against the field of view (this keeps rendering and culling code simpler), so the effective
            field of view observed.

            Note that a 128-pixel guard band at 1920x1080 allocates 40% more pixels than no guard band, so there may be a substantial
            memory overhead to a guard band even though there is little per-pixel rendering cost due to using
            RenderDevice::clip2D.
               
            Must be non-negative. Default value is (0, 0). 
            
            \image html guardBand.png
            */
        Vector2int16            colorGuardBandThickness;

        /** 
          Must be non-negative and at least as large as colorGuardBandThickness. Default value is (0, 0). 
         \image html guardBand.png
        */
        Vector2int16            depthGuardBandThickness;

        class FilmSettings {
        public:
            /** If true, allocate GApp::m_frameBuffer and use the m_film class when rendering.  
                On older GPUs the Film class may add too much memory or processing overhead.

                Defaults to true.*/
            bool                        enabled;

            /** Size of the film backbuffer. Set to -1, -1 to automatically size to the window.*/
            Vector2int16                dimensions;

            /** Formats to attempt to use for the Film, in order of decreasing preference */
            Array<const ImageFormat*>   preferredColorFormats;

            /** Formats to attempt to use for the Film, in order of decreasing preference. 
               NULL (or an empty list) indicates that no depth buffer should be allocated. 
               
               If you want separate depth and stencil attachments, you must explicitly allocate
               the stencil buffer yourself and attach it to the depth buffer.
              */
            Array<const ImageFormat*>   preferredDepthFormats;

            FilmSettings() : enabled(true), dimensions(-1, -1) {     

                preferredColorFormats.append(ImageFormat::R11G11B10F(), ImageFormat::RGB16F(), ImageFormat::RGBA8());
                preferredDepthFormats.append(ImageFormat::DEPTH24(), ImageFormat::DEPTH16(), ImageFormat::DEPTH32());
            }
        };

        FilmSettings            film;

        /** Arguments to the program, from argv.  The first is the name of the program. */
        Array<std::string>      argArray;

        /** Directory in which F4 and F6 save screenshots and videos; the current directory by default. */
        std::string             screenshotDirectory;

        /** Also invokes intiGLG3D() */
        Settings();

        /** Also invokes intiGLG3D() */
        Settings(int argc, const char* argv[]);
    };

    class DebugShape {
    public:
        ShapeRef        shape;
        Color4          solidColor;
        Color4          wireColor;
        CoordinateFrame frame;
        DebugID         id;
        /** Clear after this time (always draw before clearing) */
        RealTime        endTime;
    };

    class DebugLabel {
    public:
        Point3 wsPos;
        GuiText text;
        DebugID id;
        GFont::XAlign xalign;
        GFont::YAlign yalign;
        float size;
        RealTime endTime;
    };


    /** Last DebugShape::id issued */
    DebugID              m_lastDebugID;


    /** \brief Shapes to be rendered each frame.  

        Added to by G3D::debugDraw.
        Rendered by drawDebugShapes();
        Automatically cleared once per frame.
      */
    Array<DebugShape>    debugShapeArray;
    
    /** Labels to be rendered each frame, updated at the same times as debugShapeArray */
    Array<DebugLabel>    debugLabelArray;

    /** \brief Draw everything in debugShapeArray.

        Subclasses should call from onGraphics3D() or onGraphics().
        This will sort the debugShapeArray from back to front
        according to the current camera.

        \sa debugDraw, Shape, DebugID, removeAllDebugShapes, removeDebugShape
     */
    void drawDebugShapes();

    /** 
        \brief Clears all debug shapes, regardless of their pending display time.

        \sa debugDraw, Shape, DebugID, removeDebugShape, drawDebugShapes
    */
    void removeAllDebugShapes();

    /** 
        \brief Clears just this debug shape (if it exists), regardless of its pending display time.

        \sa debugDraw, Shape, DebugID, removeAllDebugShapes, drawDebugShapes
    */
    void removeDebugShape(DebugID id);

private:

    /** Called from init. */
    void loadFont(const std::string& fontName);

    /** When recording, this dialog registers here */
    VideoRecordDialog*       m_activeVideoRecordDialog;

    OSWindow*                _window;
    bool                    _hasUserCreatedWindow;

    shared_ptr<Scene>       m_scene;

protected:

    Stopwatch               m_graphicsWatch;
    Stopwatch               m_poseWatch;
    Stopwatch               m_logicWatch;
    Stopwatch               m_networkWatch;
    Stopwatch               m_userInputWatch;
    Stopwatch               m_simulationWatch;
    Stopwatch               m_waitWatch;

    /** The original settings */
    Settings                m_settings;

    /** onPose(), onGraphics(), and onWait() execute once every m_renderPeriod 
        simulation frames. This allows UI/network/simulation to be clocked much faster
        than rendering to increase responsiveness. */
    int                     m_renderPeriod;

    shared_ptr<WidgetManager>  m_widgetManager;

    bool                    m_endProgram;
    int                     m_exitCode;
    
    /**
       Used to find the frame for defaultCamera.
    */
    shared_ptr<Manipulator> m_cameraManipulator;

    GMutex                  m_debugTextMutex;

    /**
       Strings that have been printed with screenPrintf.
       Protected by m_debugTextMutex.
    */
    Array<std::string>      debugText;

    Color4                  m_debugTextColor;
    Color4                  m_debugTextOutlineColor;

    /**
       Processes all pending events on the OSWindow queue into the userInput.
       This is automatically called once per frame.  You can manually call it
       more frequently to get higher resolution mouse tracking or to prevent
       the OS from locking up (and potentially crashing) while in a lengthy
       onGraphics call.
    */
    virtual void processGEventQueue();

    static void staticConsoleCallback(const std::string& command, void* me);

    /** Allocated if GApp::Settings::FilmSettings::enabled was true
        when the constructor executed.  Automatically resized by
        resize() when the screen size changes. 
    */
    shared_ptr<Film>                m_film;

    /** NULL by default.  The DeveloperWindow assumes that this is the GBuffer to be visualized if it is non-NULL. */
    shared_ptr<GBuffer>             m_gbuffer;

    shared_ptr<DepthOfField>        m_depthOfField;

    shared_ptr<MotionBlur>          m_motionBlur;

    /** Framebuffer used for rendering the 3D portion of the scene. */
    shared_ptr<Framebuffer>         m_frameBuffer;

    /** Always bound to m_frameBuffer FrameBuffer::COLOR0. */
    shared_ptr<Texture>             m_colorBuffer0;

    /** Always bound to m_frameBuffer FrameBuffer::DEPTH. If NULL, the
        preferred depth format may not be supported by a Texture.  In
        that case, check m_depthRenderBuffer.
              
        When using a GBuffer, it is usually a good idea to set
        <code>m_depthBuffer = m_gbuffer->texture(GBuffer::DEPTH);</code>
      */
    shared_ptr<Texture>             m_depthBuffer;

    /** Always bound to m_frameBuffer FrameBuffer::DEPTH. This is used if
     the preferred depth format is not supported for Texture%s.*/
    shared_ptr<Renderbuffer>        m_depthRenderBuffer;

    /** Used to track how much onWait overshot its desired target during the previous frame. */
    RealTime                        m_lastFrameOverWait;

    /** Default AO object for the primary view, allocated in GApp::GApp */
    shared_ptr<class AmbientOcclusion> m_ambientOcclusion;

    /**
       A camera that is driven by the debugController.

       This is a copy of the default camera from the scene, but is not itself in the scene.

       Do not reassign this--the CameraControlWindow is hardcoded to the original one.
    */
    shared_ptr<Camera>              m_debugCamera;

    /**
       Allows first person (Quake game-style) control
       using the arrow keys or W,A,S,D and the mouse.

       To disable, use:
       <pre>
       setCameraManipulator(shared_ptr<Manipulator>());
       m_widgetManager->remove(m_debugController);
       m_debugController.reset();
       </pre>

    */
    shared_ptr<FirstPersonManipulator>  m_debugController;

    /** The currently selected camera */
    shared_ptr<Camera>              m_activeCamera;
    
    /** 
      Helper for generating cube maps.  Invokes GApp::onGraphics3D six times, once for each face of a cube map.  This is convenient both for microrendering
      and for generating cube maps to later use offline.

      certain post processing effects are applied to the final image. Motion blur and depth of field are not but AO is if enabled. However AO will causes artifacts on the final image when enabled.

      \param output If empty or the first element is NULL, this is set to a series of new reslolution x resolution ImageFormat::RGB16F() textures.  Otherwise, the provided elements are used. 
      Textures are assumed to be square.  The images are generated in G3D::CubeFace order.

      \param camera the camera will have all of its parameters reset before the end of the call.

      \param depthMap Optional pre-allocated depth texture to use as the depth map when rendering each face.  Will be allocated to match the texture resolution if not provided.
      The default depth format is ImageFormat::DEPTH24().

      Example:
      \code
        Array<shared_ptr<Texture> > output;
        renderCubeMap(renderDevice, output, defaultCamera);

        const Texture::CubeMapInfo& cubeMapInfo = Texture::cubeMapInfo(CubeMapConvention::DIRECTX);
        for (int f = 0; f < 6; ++f) {
            const Texture::CubeMapInfo::Face& faceInfo = cubeMapInfo.face[f];
            shared_ptr<Image> temp = output[f]->toImage(ImageFormat::RGB8());
            temp->flipVertical();
            temp->rotate90CW(-faceInfo.rotations);
            if (faceInfo.flipY) { temp.flipVertical();   }
            if (faceInfo.flipX) { temp.flipHorizontal(); }
            temp->save(format("cube-%s.png", faceInfo.suffix.c_str()));     
        }
        \endcode
    */
    virtual void renderCubeMap(RenderDevice* rd, Array<shared_ptr<Texture> >& output, const shared_ptr<Camera>& camera, shared_ptr<Texture> depthMap = shared_ptr<Texture>(), int resolution = 1024);

    /** Call from onInit to create the developer HUD. */
    void createDeveloperHUD();

    void setScene(const shared_ptr<Scene>& s) {
        m_scene = s;
    }

    virtual shared_ptr<Scene> scene() const {
        return m_scene;
    }

public:

    virtual void swapBuffers();

    /** Load a new scene.  A GApp may invoke this on itself, and the 
        SceneEditorWindow will invoke this automatically when
        the user presses Reload or chooses a new scene from the GUI. */
    virtual void loadScene(const std::string& sceneName);

    /** Save the current scene over the one on disk. */
    virtual void saveScene();

    /** The currently active camera for the primary view. */
    virtual shared_ptr<Camera> activeCamera() {
        return m_activeCamera;
    }

    virtual const shared_ptr<Camera> activeCamera() const {
        return m_activeCamera;
    }

    /** Exposes the debugging camera */
    virtual shared_ptr<Camera> debugCamera() {
        return m_debugCamera;
    }

    virtual void setActiveCamera(const shared_ptr<Camera>& camera);

    /** Add your own debugging controls to this window.*/
    shared_ptr<GuiWindow>   debugWindow;

    /** debugWindow->pane() */
    GuiPane*                debugPane;

    virtual const SceneVisualizationSettings& sceneVisualizationSettings() const;

    const Settings& settings() const {
                return m_settings;
    }

    void vscreenPrintf
    (const char*                 fmt,
     va_list                     argPtr) G3D_CHECK_VPRINTF_METHOD_ARGS;

    const Stopwatch& graphicsWatch() const {
        return m_graphicsWatch;
    }

    const Stopwatch& waitWatch() const {
        return m_waitWatch;
    }

    const Stopwatch& logicWatch() const {
        return m_logicWatch;
    }

    const Stopwatch& networkWatch() const {
        return m_networkWatch;
    }

    const Stopwatch& userInputWatch() const {
        return m_userInputWatch;
    }

    const Stopwatch& simulationWatch() const {
        return m_simulationWatch;
    }

    /** Initialized to GApp::Settings::dataDir, or if that is "<AUTO>", 
        to  FilePath::parent(System::currentProgramFilename()). To make your program
        distributable, override the default 
        and copy all data files you need to a local directory.
        Recommended setting is "data/" or "./", depending on where
        you put your data relative to the executable.

        Your data directory must contain the default debugging font, 
        "console-small.fnt", unless you change it.
    */
    std::string                     dataDir;

    RenderDevice*                   renderDevice;

    /** Command console. 
    \deprecated */
    GConsoleRef                     console;

    /** The window that displays buttons for debugging.  If GApp::Settings::useDeveloperTools is true
        this will be created and added as a Widget on the GApp.  Otherwise this will be NULL.
    */
    shared_ptr<DeveloperWindow>     developerWindow;

    /**
       NULL if not loaded
    */
    shared_ptr<GFont>               debugFont;
    UserInput*                      userInput;

    /** Invoke to true to end the program at the end of the next event loop. */
    virtual void setExitCode(int code = 0);

    /**
       The manipulator that positions the debugCamera every frame.
       By default, this is set to the debugController.  This may be
       set to NULL to disable explicit camera positioning.
    */
    inline void setCameraManipulator(const Manipulator::Ref& man) {
        m_cameraManipulator = man;
    }

    shared_ptr<Manipulator> cameraManipulator() const {
        return m_cameraManipulator;
    }
    
    OSWindow* window() const {
        return _window;
    }

    /**
       When true, debugPrintf prints to the screen.
       (default is true)
    */
    bool                    showDebugText;

    enum Action {
        ACTION_NONE,
        ACTION_QUIT,
        ACTION_SHOW_CONSOLE
    };

    /**
       When true an GKey::ESCAPE keydown event
       quits the program.
       (default is true)
    */
    Action                  escapeKeyAction;
    
    /**
       When true,   renderDebugInfo prints the frame rate and
       other data to the screen.
    */
    bool                    showRenderingStats;

    /**
       When true, the G3D::UserInput->beginEvents/endEvents processing is handled 
       for you by calling processGEventQueue() before G3D::GApp::onUserInput is called.  If you turn
       this off, you must call processGEventQueue() or provide your own event to userInput processing in onUserInput.
       (default is true)
    */
    bool                    manageUserInput;

    /**
       When true, there is an assertion failure if an exception is 
       thrown.

       Default is true.
    */
    bool                    catchCommonExceptions;

    /**
       Called from GApplet::run immediately after doGraphics to render
       the debugging text.  Does nothing if debugMode is false.  It
       is not usually necessary to override this method.
    */
    virtual void renderDebugInfo();

    /**
       @param window If null, a SDLWindow will be created for you. This
       argument is useful for substituting a different window
       system (e.g. GlutWindow)
    */
    GApp(const Settings& options = Settings(), OSWindow* window = NULL);

    virtual ~GApp();

    /**
       Call this to run the app.
    */
    int run();

    /** Draw a simple, short message in the center of the screen and swap the buffers. 
      Useful for loading screens and other slow operations.*/
    void drawMessage(const std::string& message);

    /** Draws a title card */
    void drawTitle(const std::string& title, const std::string& subtitle, const Any& any, const Color3& fontColor, const Color4& backColor);

    /** Displays the texture in a new GuiWindow */
    shared_ptr<GuiWindow> show(const shared_ptr<Texture>& t, const std::string& windowCaption = "");
    
    shared_ptr<GuiWindow> show(const shared_ptr<PixelTransferBuffer>& t, const std::string& windowCaption = "");
 
    shared_ptr<GuiWindow> show(const shared_ptr<Image>& t, const std::string& windowCaption = "");

private:

    /** Used by onSimulation for elapsed time. */
    RealTime               m_now, m_lastTime;

    /** Used by onWait for elapsed time. */
    RealTime               m_lastWaitTime;

    /** Seconds per frame target for the entire system \sa setFrameDuration*/
    float                  m_wallClockTargetDuration;

    /** \copydoc setLowerFrameRateInBackground */
    bool                   m_lowerFrameRateInBackground;

    /** SimTime seconds per frame, \see setFrameDuration, m_simTimeScale */
    float                  m_simTimeStep;
    float                  m_simTimeScale;
    float                  m_previousSimTimeStep;
    float                  m_previousRealTimeStep;

    RealTime               m_realTime;
    SimTime                m_simTime;
    
protected:

    Array<shared_ptr<Surface> >   m_posed3D;
    Array<Surface2DRef>           m_posed2D;

protected:

    /** Helper for run() that actually starts the program loop. Called from run(). */
    void onRun();

    /**
       Initializes state at the beginning of onRun, including calling onCleanup.
    */
    void beginRun();

    /**
       Cleans up at the end of onRun, including calling onCleanup.
    */
    void endRun();

    /** 
        A single frame of rendering, simulation, AI, events, networking,
        etc.  Invokes the onXXX methods. 
    */
    void oneFrame();

public:

    /**
       Installs a module.  Actual insertion may be delayed until the next frame.
    */
    virtual void addWidget(const Widget::Ref& module, bool setFocus = true);

    /**
       The actual removal of the module may be delayed until the next frame.
    */
    virtual void removeWidget(const Widget::Ref& module);

    /** Accumulated wall-clock time since init was called on this applet. 
        Since this time is accumulated, it may drift from the true
        wall-clock obtained by System::time().*/
    RealTime realTime() const {
        return m_realTime;
    }

    virtual void setRealTime(RealTime r);

    /** In-simulation time since init was called on this applet.  
        Takes into account simTimeSpeed.  Automatically incremented
        after ooSimulation.
    */
    SimTime simTime() const {
        return m_simTime;
    }

    virtual void setSimTime(SimTime s);

    /** For use as the \a simulationStepDuration argument of setFrameDuration */
    enum {
        /** Good for smooth animation in a high but variable-framerate system.        
            Advance simulation using wall-clock time.  
        
            Note that this will keep simulation running in "real-time" even when the system
            is running slowly (e.g., when recording video), which can lead to missed animation frames.
          */
        REAL_TIME = -100,

        /**
          Good for low frame rates when debugging, recording video, or working with a naive physics system.
          Advance simulation by the same amount as the \a wallClockTargetDuration argument
          to setFrameDuration every frame, regardless of the actual time elapsed.
          
          Note that when the system is running slowly, this ensures that rendering and 
          simulation stay in lockstep with each other and no frames are dropped.
         */
        MATCH_REAL_TIME_TARGET = -200
    };

    /** 
      \param realTimeTargetDuration Target duration between successive frames. If simulating and rendering (and all other onX methods) 
      consume less time than this, then GApp will invoke onWait() to throttle.  If the frame takes more time than 
      wallClockTargetDuration, then the system will proceed to the next frame as quickly as it can.

      \param simulationStepDuration Amount to increment simulation time by each frame under normal circumstances (this is modified by setSimulationTimeScale) 
    */
    virtual void setFrameDuration(RealTime realTimeTargetDuration, SimTime simulationStepDuration = MATCH_REAL_TIME_TARGET) {
        m_wallClockTargetDuration = (float)realTimeTargetDuration;
        m_simTimeStep = (float)simulationStepDuration;
    }

    /** 1.0 / desired frame rate */
    RealTime realTimeTargetDuration() const {
        return m_wallClockTargetDuration;
    }

    /** May also be REAL_TIME or MATCH_REAL_TIME_TARGET. \sa lastSimTimeStep */
    SimTime simStepDuration() const {
        return m_simTimeStep;
    }

    /** A non-negative number that is the amount that time is was advanced by in the previous frame.  Never an enum value. 
        For the first frame, this is the amount that time will be advanced by if rendering runs at speed. */
    SimTime previousSimTimeStep() const {
        return m_previousSimTimeStep;
    }

    /** Actual wall-clock time elapsed between the previous two frames. \sa realTimeTargetDuration */
    RealTime previousRealTimeStep() const {
        return m_previousRealTimeStep;
    }

    /**
     Set the rate at which simulation time actually advances compared to 
     the rate specified by setFrameDuration.  Set to 0 to pause simulation,
     1 for normal behavior, and use other values when fast-forwarding (greater than 1) or 
     showing slow-motion (less than 1).
     */
    virtual void setSimulationTimeScale(float s) {
        m_simTimeScale = s;
    }

    float simulationTimeScale() const {
        return m_simTimeScale;
    }

    /** If true, the \a wallClockTargetDuration from setFrameDuration() is ignored
        when the OSWindow does not have focus 
        and the program switches to running 4fps to avoid slowing down the foreground application.
    */
    virtual void setLowerFrameRateInBackground(bool s) {
        m_lowerFrameRateInBackground = s;
    }

    bool lowerFrameRateInBackground() const {
        return m_lowerFrameRateInBackground;
    }
    
protected:

    /** Change the size of the underlying Film. Called by GApp::GApp() and GApp::onEvent(). This is not an event handler.  If you want 
      to be notified when your app is resized, override GApp::onEvent to handle the resize event (just don't forget to call GApp::onEvent as well).

      The guard band sizes are added to the specified width and height
      */
    virtual void resize(int w, int h);

    /**
       Load your data here.  Unlike the constructor, this catches common exceptions.
       It is called before the first frame is processed.
    */
    virtual void onInit();

    virtual void onAfterEvents();

    /**
       Unload/deallocate your data here.  Unlike the constructor, this catches common exceptions.
       It is called after the last frame is processed.
    */
    virtual void onCleanup();


    /**
       Override this with your simulation code.
       Called from GApp::run.
        
       Default implementation does nothing.

       simTime(), idealSimTime() and realTime() are incremented after
       onSimulation is called, so at the beginning of call the current
       time is the end of the previous frame.

       @param rdt Elapsed real-world time since the last call to onSimulation.

       @param sdt Elapsed sim-world time since the last call to
       onSimulation, computed by multiplying the wall-clock time by the
       simulation time rate.

       @param idt Elapsed ideal sim-world time.  Use this for perfectly
       reproducible timing results.  Ideal time always advances by the
       desiredFrameDuration * simTimeRate, no matter how much wall-clock
       time has elapsed.
    */
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

    /** Invoked before onSimulation is run on the installed GModules and GApp.
        This is not used by most programs; it is primarily a hook for those performing
        extensive physical simulation on the GModules that need a setup and cleanup step.

        If you mutate the timestep arguments then those mutated time steps are passed
        to the onSimulation method.  However, the accumulated time will not be affected by
        the changed timestep.
    */
    virtual void onBeforeSimulation(RealTime& rdt, SimTime& sdt, SimTime& idt);

    /**
       Invoked after onSimulation is run on the installed GModules and GApp.
       Not used by most programs.
    */
    virtual void onAfterSimulation(RealTime rdt, SimTime sdt, SimTime idt);

    /**
       Rendering callback used to paint the screen.  Called automatically.
       RenderDevice::beginFrame and endFrame are called for you before this 
       is invoked.

       The default implementation calls onGraphics2D and onGraphics3D.
     */
    virtual void onGraphics(RenderDevice* rd, Array<shared_ptr<Surface> >& surface, 
                            Array<shared_ptr<Surface2D> >& surface2D);

    /**
       Called from the default onGraphics.
       
       Override and implement. 
   */
    virtual void onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& surface2D);

    /**
       \brief Called from the default onGraphics.
       Override and implement. 

    The RenderDevice will already be cleared and the default camera set when this method is invoked.
    By default, the Film's Framebuffer is bound and the output will be gamma corrected
    and bloomed.

    A common task is rendering UniversalSurface%s with your own shader, which can be done by:
\code

    rd->pushState();
    rd->setShader(myShader)
    for (int s = 0; s < surface3D.size(); ++s) {
        const shared_ptr<UniversalSurface>& surface = dynamic_pointer_cast<UniversalSurface>(surface3D[s]);
        if (surface) {
            const UniversalSurface::GPUGeom::Ref& geom = surface->gpuGeom();
            const UniversalBSDF::Ref& bsdf = geom->material->bsdf();
            
            rd->setObjectToWorldMatrix(surface->coordinateFrame());
            
            myShader->args.set("lambertianConstant", bsdf->lambertian().constant());
            myShader->args.set("lambertianMap", Texture::whiteIfNull(bsdf->lambertian().texture()));
            ...
                
            rd->beginIndexedPrimitives();
            {
                rd->setNormalArray(geom->normal);
                rd->setVertexArray(geom->vertex);
                rd->setTexCoordArray(0, geom->texCoord0);
                rd->sendIndices(geom->primitive, geom->index);
            }
            rd->endIndexedPrimitives();
        }
    }
    rd->popState();
\endcode
   */
    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface);

    /** Called before onGraphics.  Append any models that you want
        rendered (you can also explicitly pose and render in your
        onGraphics method).  The provided arrays will already contain
        posed models from any installed Widgets. */
    virtual void onPose(Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D);

    /**
       For a networked app, override this to implement your network
       message polling.
    */
    virtual void onNetwork();

    /**
       Task to be used for frame rate limiting.  

       Overriding onWait is not recommended unless you have significant
       computation tasks that cannot be executed conveniently on a separate thread.

       Frame rate limiting is useful
       to avoid overloading a maching that is running background tasks and
       for situations where fixed time steps are needed for simulation and there
       is no reason to render faster.

       Default implementation System::sleep()s on waitTime (which is always non-negative)
    */
    virtual void onWait(RealTime waitTime);


    /**
       Update any state you need to here.  This is a good place for
       AI code, for example.  Called after onNetwork and onUserInput,
       before onSimulation.
    */
    virtual void onAI();

    
    /**
       It is recommended to override onUserInput() instead of this method.

       Override if you need to explicitly handle events raw in the order
       they appear rather than once per frame by checking the current
       system state.
     
       Note that the userInput contains a record of all
       keys pressed/held, mouse, and joystick state, so 
       you do not have to override this method to handle
       basic input events.

       Return true if the event has been consumed (i.e., no-one else 
       including GApp should process it further).

       The default implementation does nothing.

       This runs after the m_widgetManager's onEvent, so a widget may consume
       events before the App sees them.
    */
    virtual bool onEvent(const GEvent& event);

    /**
       Routine for processing user input from the previous frame.  Default implementation does nothing.
    */
    virtual void onUserInput(class UserInput* userInput);

    /**
       Invoked when a user presses enter in the in-game console.  The default implementation
       ends the program if the command is "exit".


       Sample implementation:
       \htmlonly
       <pre>
        void App::onConsoleCommand(const std::string& str) {
            // Add console processing here

            TextInput t(TextInput::FROM_STRING, str);
            if (t.isValid() && (t.peek().type() == Token::SYMBOL)) {
                std::string cmd = toLower(t.readSymbol());
                if (cmd == "exit") {
                    setExitCode(0);
                    return;
                } else if (cmd == "help") {
                    printConsoleHelp();
                    return;
                }

                // Add commands here
            }

            console->printf("Unknown command\n");
            printConsoleHelp();
        }

        void App::printConsoleHelp() {
            console->printf("exit          - Quit the program\n");
            console->printf("help          - Display this text\n\n");
            console->printf("~/ESC         - Open/Close console\n");
            console->printf("F2            - Enable first-person camera control\n");
            console->printf("F4            - Record video\n");
        }
        </pre>
        \endhtmlonly
    */
    virtual void onConsoleCommand(const std::string& cmd);
};


/**
   Displays output on the last G3D::GApp instantiated.  If there was no GApp instantiated,
   does nothing.  Threadsafe.
   
   This is primarily useful for code that prints (almost) the same
   values every frame (e.g., "current position = ...") because those
   values will then appear in the same position on screen.

   For one-off print statements (e.g., "network message received")
   see G3D::consolePrintf.
 */
void screenPrintf(const char* fmt ...) G3D_CHECK_PRINTF_ARGS;

}

#endif
