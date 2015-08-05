#include "PixelShaderApp.hpp"

namespace G3D
{

PixelShaderApp::PixelShaderApp(const Settings& options, OSWindow* window, RenderDevice* rd) :
    GApp(options, window, rd),
    lambertianScalar(0.6f),
    glossyScalar(0.5f),
    reflect(0.1f),
    smoothness(0.2f) {
}

void PixelShaderApp::onInit() {
    GApp::onInit();
    createDeveloperHUD();
    renderDevice->setSwapBuffersAutomatically(true);

    window()->setCaption("Pixel Shader Demo");

    ArticulatedModel::Specification spec;
    spec.filename = System::findDataFile("teapot/teapot.obj");
    spec.scale = 0.015f;
    spec.stripMaterials = true;
    spec.preprocess.append(ArticulatedModel::Instruction(Any::parse("setCFrame(root(), Point3(0, -0.5, 0));")));
    model = ArticulatedModel::create(spec);

    makeLighting();
    makeColorList();
    makeGui();

    // Color 1 is red
    lambertianColorIndex = 1;
    // The last color is white
    glossyColorIndex = colorList.size() - 1;

    m_debugCamera->setPosition(Vector3(1.0f, 1.0f, 2.5f));
    m_debugCamera->setFieldOfView(45 * units::degrees(), FOVDirection::VERTICAL);
    m_debugCamera->lookAt(Point3::zero());

    // Add axes for dragging and turning the model
    manipulator = ThirdPersonManipulator::create();
    addWidget(manipulator);

    // Turn off the default first-person camera controller and developer UI
    m_debugController->setEnabled(false);
    developerWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
    showRenderingStats = false;
}


void PixelShaderApp::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) {

    m_gbuffer->setSpecification(m_gbufferSpecification);
    m_gbuffer->resize(m_framebuffer->width(), m_framebuffer->height());
    m_gbuffer->prepare(rd, activeCamera(), 0, -(float)previousSimTimeStep(), m_settings.depthGuardBandThickness, m_settings.colorGuardBandThickness);

    m_renderer->render(rd, m_framebuffer, m_depthPeelFramebuffer, scene()->lightingEnvironment(), m_gbuffer, surface3D);

    rd->pushState(m_framebuffer); {

        rd->setProjectionAndCameraMatrix(m_debugCamera->projection(), m_debugCamera->frame());
        Array< shared_ptr<Surface> > mySurfaces;
        // Pose our model based on the manipulator axes
        model->pose(mySurfaces, manipulator->frame());

        // Set up shared arguments
        Args args;
        configureShaderArgs(args);

        // Send model geometry to the graphics card
        CFrame cframe;
        for (int i = 0; i < mySurfaces.size(); ++i) {

            // Downcast to UniversalSurface to access its fields
            shared_ptr<UniversalSurface> surface = dynamic_pointer_cast<UniversalSurface>(mySurfaces[i]);
            if (notNull(surface)) {
                surface->getCoordinateFrame(cframe);
                rd->setObjectToWorldMatrix(cframe);
                surface->gpuGeom()->setShaderArgs(args);

                // (If you want to manually set the material properties and vertex attributes
                // for shader args, they can be accessed from the fields of the gpuGeom.)
                LAUNCH_SHADER("phong.*", args);
            }
        }
    } rd->popState();

    //
    // Note that explicitly calling swapBuffers in a GApp is not a supported
    // usage scenario when using multiple G3DWidgets.
    //
    // swapBuffers();

    rd->clear();
    m_film->exposeAndRender(rd, m_debugCamera->filmSettings(), m_framebuffer->texture(0), 1);
}


void PixelShaderApp::configureShaderArgs(Args& args) {
    const shared_ptr<Light>&  light  = scene()->lightingEnvironment().lightArray[0];
    const Color3&    lambertianColor = colorList[lambertianColorIndex].element(0).color(Color3::white()).rgb();
    const Color3&    glossyColor     = colorList[glossyColorIndex].element(0).color(Color3::white()).rgb();


    // Viewer
    args.setUniform("wsEyePosition",        m_debugCamera->frame().translation);

    // Lighting
    args.setUniform("wsLight",              light->position().xyz().direction());
    args.setUniform("lightColor",           light->color);
    args.setUniform("ambient",              Color3(0.3f));
    args.setUniform("environmentMap",       scene()->lightingEnvironment().environmentMapArray[0], Sampler::cubeMap());

    // Material
    args.setUniform("lambertianColor",      lambertianColor);
    args.setUniform("lambertianScalar",     lambertianScalar);

    args.setUniform("glossyColor",          glossyColor);
    args.setUniform("glossyScalar",         glossyScalar);

    args.setUniform("smoothness",           smoothness);
    args.setUniform("reflectScalar",        reflect);
}


void PixelShaderApp::makeColorList() {
    shared_ptr<GFont> iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));

    // Characters in icon font that make a solid block of color
    static const char* block = "gggggg";

    float size = 18;
    int N = 10;
    colorList.append(GuiText(block, iconFont, size, Color3::black(), Color4::clear()));
    for (int i = 0; i < N; ++i) {
        colorList.append(GuiText(block, iconFont, size, Color3::rainbowColorMap((float)i / N), Color4::clear()));
    }
    colorList.append(GuiText(block, iconFont, size, Color3::white(), Color4::clear()));
}


void PixelShaderApp::makeGui() {
    const shared_ptr<GuiWindow>& gui = GuiWindow::create("Material Parameters");
    GuiPane* pane = gui->pane();

    pane->beginRow();
    pane->addSlider("Lambertian", &lambertianScalar, 0.0f, 1.0f);
    pane->addDropDownList("", colorList, &lambertianColorIndex)->setWidth(80);
    pane->endRow();

    pane->beginRow();
    pane->addSlider("Glossy",    &glossyScalar, 0.0f, 1.0f);
    pane->addDropDownList("", colorList, &glossyColorIndex)->setWidth(80);
    pane->endRow();

    pane->addSlider("Mirror",     &reflect, 0.0f, 1.0f);
    pane->addSlider("Smoothness", &smoothness, 0.0f, 1.0f);

    gui->pack();
    addWidget(gui);
    gui->moveTo(Point2(10, 10));}


void PixelShaderApp::makeLighting() {
    scene()->insert(Light::directional("Light", Vector3(1.0f, 1.0f, 1.0f), Color3(1.0f), false));

    // The environmentMap is a cube of six images that represents the incoming light to the scene from
    // the surrounding environment.  G3D specifies all six faces at once using a wildcard and loads
    // them into an OpenGL cube map.

    Texture::Specification environmentMapTexture;
    environmentMapTexture.filename   = FilePath::concat(System::findDataFile("noonclouds"), "noonclouds_*.png");

    environmentMapTexture.dimension  = Texture::DIM_CUBE_MAP;

    environmentMapTexture.preprocess = Texture::Preprocess::gamma(2.1f);
    environmentMapTexture.generateMipMaps = true;
    scene()->lightingEnvironment().environmentMapArray.append(Texture::create(environmentMapTexture));
    scene()->insert(Skybox::create("Skybox", &*scene(), scene()->lightingEnvironment().environmentMapArray, Array<SimTime>(0), 0.0f, SplineExtrapolationMode::CLAMP, false, false));
}

}
