#include "Assert.hpp"

#include "StarterApp.hpp"

namespace G3D
{

StarterApp::StarterApp(const GApp::Settings& settings, OSWindow* window, RenderDevice* rd) :
    GApp(settings, window, rd) {
}


// Called before the application loop begins.  Load data here and
// not in the constructor so that common exceptions will be
// automatically caught.
void StarterApp::onInit() {
    GApp::onInit();

    // This program renders to texture for most 3D rendering, so it can
    // explicitly delay calling swapBuffers until the Film::exposeAndRender call,
    // since that is the first call that actually affects the back buffer.  This
    // reduces frame tearing without forcing vsync on.
    renderDevice->setSwapBuffersAutomatically(false);

    setFrameDuration(1.0f / 30.0f);

    // Call setScene(shared_ptr<Scene>()) or setScene(MyScene::create()) to replace
    // the default scene here.

    showRenderingStats      = false;
    m_showWireframe         = false;

    makeGBuffer();
    makeGUI();

    // For higher-quality screenshots:
    // developerWindow->videoRecordDialog->setScreenShotFormat("PNG");
    // developerWindow->videoRecordDialog->setCaptureGui(false);
    developerWindow->cameraControlWindow->moveTo(Point2(developerWindow->cameraControlWindow->rect().x0(), 0));
    loadScene(developerWindow->sceneEditorWindow->selectedSceneName());
}


void StarterApp::makeGBuffer() {
    // If you do not use motion blur or deferred shading, you can avoid allocating the GBuffer here to save resources
    GBuffer::Specification specification;

    specification.format[GBuffer::Field::SS_POSITION_CHANGE]    = GLCaps::supportsTexture(ImageFormat::RG8()) ? ImageFormat::RG8() : ImageFormat::RGBA8();
    specification.encoding[GBuffer::Field::SS_POSITION_CHANGE]  = Vector2(128.0f, -64.0f);

    specification.format[GBuffer::Field::CS_FACE_NORMAL]        = ImageFormat::RGB8();
    specification.encoding[GBuffer::Field::CS_FACE_NORMAL]      = Vector2(2.0f, -1.0f);

    specification.format[GBuffer::Field::DEPTH_AND_STENCIL]     = ImageFormat::DEPTH32();
    specification.depthEncoding = DepthEncoding::HYPERBOLIC;

    m_gbuffer = GBuffer::create(specification);

    m_gbuffer->resize(renderDevice->width(), renderDevice->height());
    m_gbuffer->texture(GBuffer::Field::SS_POSITION_CHANGE)->visualization = Texture::Visualization::unitVector();

    // Share the depth buffer with the forward-rendering pipeline
    m_depthBuffer = m_gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL);
    m_frameBuffer->set(Framebuffer::DEPTH, m_depthBuffer);
}


void StarterApp::makeGUI() {
    // Initialize the developer HUD (using the existing scene)
    createDeveloperHUD();
    debugWindow->setVisible(true);
    developerWindow->videoRecordDialog->setEnabled(true);

    GuiPane* infoPane = debugPane->addPane("Info", GuiTheme::ORNATE_PANE_STYLE);
    infoPane->addCheckBox("Show wireframe", &m_showWireframe);

    // Example of how to add debugging controls
    infoPane->addLabel("You can add more GUI controls");
    infoPane->addLabel("in App::onInit().");
    infoPane->addButton("Exit", this, &StarterApp::endProgram);
    infoPane->pack();

    // More examples of debugging GUI controls:
    // debugPane->addCheckBox("Use explicit checking", &explicitCheck);
    // debugPane->addTextBox("Name", &myName);
    // debugPane->addNumberBox("height", &height, "m", GuiTheme::LINEAR_SLIDER, 1.0f, 2.5f);
    // button = debugPane->addButton("Run Simulator");

    debugWindow->pack();
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}


void StarterApp::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces) {
    if (! scene()) {
        return;
    }

    // Bind the main frameBuffer
    rd->pushState(m_frameBuffer); {
        rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());

        m_gbuffer->resize(rd->width(), rd->height());
        m_gbuffer->prepare(rd, activeCamera(),  0, -(float)previousSimTimeStep(), m_settings.depthGuardBandThickness, m_settings.colorGuardBandThickness);
        rd->clear();

        // Cull and sort
        Array<shared_ptr<Surface> > sortedVisibleSurfaces;
        Surface::cull(activeCamera()->frame(), activeCamera()->projection(), rd->viewport(), allSurfaces, sortedVisibleSurfaces);
        Surface::sortBackToFront(sortedVisibleSurfaces, activeCamera()->frame().lookVector());

        const bool renderTransmissiveSurfaces = false;

        // Intentionally copy the lighting environment for mutation
        LocalLightingEnvironment environment = scene()->localLightingEnvironment();
        environment.ambientOcclusion = m_ambientOcclusion;

        // Render z-prepass and G-buffer.  In this default implementation, it is needed if motion blur is enabled (for velocity) or
        // if face normals have been allocated and ambient occlusion is enabled.
        Surface::renderIntoGBuffer(rd, sortedVisibleSurfaces, m_gbuffer, activeCamera()->previousFrame(), renderTransmissiveSurfaces);

        if (! m_settings.colorGuardBandThickness.isZero()) {
            rd->setGuardBandClip2D(m_settings.colorGuardBandThickness);
        }

        // Compute AO
        m_ambientOcclusion->update(rd, environment.ambientOcclusionSettings, activeCamera(), m_frameBuffer->texture(Framebuffer::DEPTH), shared_ptr<Texture>(), m_gbuffer->texture(GBuffer::Field::CS_FACE_NORMAL), m_gbuffer->specification().encoding[GBuffer::Field::CS_FACE_NORMAL], m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);

        // No need to write depth, since it was covered by the gbuffer pass
        //rd->setDepthWrite(false);
        // Compute shadow maps and forward-render visible surfaces
        Surface::render(rd, activeCamera()->frame(), activeCamera()->projection(), sortedVisibleSurfaces, allSurfaces, environment, Surface::ALPHA_BINARY, true, m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);

        if (m_showWireframe) {
            Surface::renderWireframe(rd, sortedVisibleSurfaces);
        }

        // Call to make the App show the output of debugDraw(...)
        drawDebugShapes();
        scene()->visualize(rd, sceneVisualizationSettings());

        // Post-process special effects
        m_depthOfField->apply(rd, m_colorBuffer0, m_depthBuffer, activeCamera(), m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);

        m_motionBlur->apply(rd, m_colorBuffer0, m_gbuffer->texture(GBuffer::Field::SS_POSITION_CHANGE),
                            m_gbuffer->specification().encoding[GBuffer::Field::SS_POSITION_CHANGE], m_depthBuffer, activeCamera(),
                            m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness);

    } rd->popState();

    //
    // Note that explicitly calling swapBuffers in a GApp is not a supported
    // usage scenario when using multiple G3DWidgets.
    //
    // swapBuffers();

    // Clear the entire screen (needed even though we'll render over it, since
    // AFR uses clear() to detect that the buffer is not re-used.)
    rd->clear();

    // Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
    m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_colorBuffer0);
}


void StarterApp::onAI() {
    GApp::onAI();
    // Add non-simulation game logic and AI code here
}


void StarterApp::onNetwork() {
    GApp::onNetwork();
    // Poll net messages here
}


void StarterApp::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    GApp::onSimulation(rdt, sdt, idt);

    // Example GUI dynamic layout code.  Resize the debugWindow to fill
    // the screen horizontally.
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}


bool StarterApp::onEvent(const GEvent& event) {
    // Handle super-class events
    if (GApp::onEvent(event)) { return true; }

    // If you need to track individual UI events, manage them here.
    // Return true if you want to prevent other parts of the system
    // from observing this specific event.
    //
    // For example,
    // if ((event.type == GEventType::GUI_ACTION) && (event.gui.control == m_button)) { ... return true; }
    // if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::TAB)) { ... return true; }

    return false;
}


void StarterApp::onUserInput(UserInput* ui) {
    GApp::onUserInput(ui);
    (void)ui;
    // Add key handling here based on the keys currently held or
    // ones that changed in the last frame.
}


void StarterApp::onPose(Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) {
    GApp::onPose(surface, surface2D);

    // Append any models to the arrays that you want to later be rendered by onGraphics()
}


void StarterApp::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& posed2D) {
    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction.
    Surface2D::sortAndRender(rd, posed2D);
}


void StarterApp::onCleanup() {
    // Called after the application loop ends.  Place a majority of cleanup code
    // here instead of in the constructor so that exceptions can be caught.
}


void StarterApp::endProgram() {
    m_endProgram = true;
}

}
