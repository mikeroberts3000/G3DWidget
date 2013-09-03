#include "PixelShaderApp.hpp"

namespace G3D
{

PixelShaderApp::PixelShaderApp(const Settings& options, OSWindow* window, RenderDevice* rd) :
    GApp(options, window, rd),
    diffuseScalar(0.6f),
    specularScalar(0.5f),
    reflect(0.1f),
    shine(20.0f) {
}

void PixelShaderApp::onInit() {
    createDeveloperHUD();
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
    diffuseColorIndex = 1;
    // Last color is white
    specularColorIndex = colorList.size() - 1;

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

    rd->clear();

    rd->pushState(m_frameBuffer); {
        rd->clear();

        rd->setProjectionAndCameraMatrix(m_debugCamera->projection(), m_debugCamera->frame());

        Draw::skyBox(rd, environment.environmentMapArray[0].texture, environment.environmentMapArray[0].constant);

        rd->pushState(); {
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

        // Render other objects, e.g., the 3D widgets
        Surface::render(rd, m_debugCamera->frame(), m_debugCamera->projection(), surface3D, surface3D, environment);
    } rd->popState();

    // Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
    m_film->exposeAndRender(rd, m_debugCamera->filmSettings(), m_colorBuffer0, 1);
}


void PixelShaderApp::configureShaderArgs(Args& args) {
    const shared_ptr<Light>&  light   = environment.lightArray[0];
    const Color3&       diffuseColor  = colorList[diffuseColorIndex].element(0).color(Color3::white()).rgb();
    const Color3&       specularColor = colorList[specularColorIndex].element(0).color(Color3::white()).rgb();


    // Viewer
    args.setUniform("wsEyePosition",        m_debugCamera->frame().translation);

    // Lighting
    args.setUniform("wsLight",              light->position().xyz().direction());
    args.setUniform("lightColor",           light->color);
    args.setUniform("ambient",              Color3(0.3f));
    args.setUniform("environmentMap",      environment.environmentMapArray[0].texture);
    args.setUniform("environmentConstant", environment.environmentMapArray[0].constant);

    // UniversalMaterial
    args.setUniform("diffuseColor",         diffuseColor);
    args.setUniform("diffuseScalar",        diffuseScalar);

    args.setUniform("specularColor",        specularColor);
    args.setUniform("specularScalar",       specularScalar);

    args.setUniform("shine",                shine);
    args.setUniform("reflect",              reflect);
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
    shared_ptr<GuiWindow> gui = GuiWindow::create("UniversalMaterial Parameters");
    GuiPane* pane = gui->pane();

    pane->beginRow();
    pane->addSlider("Lambertian", &diffuseScalar, 0.0f, 1.0f);
    pane->addDropDownList("", colorList, &diffuseColorIndex)->setWidth(80);
    pane->endRow();

    pane->beginRow();
    pane->addSlider("Glossy",    &specularScalar, 0.0f, 1.0f);
    pane->addDropDownList("", colorList, &specularColorIndex)->setWidth(80);
    pane->endRow();

    pane->addSlider("Mirror",     &reflect, 0.0f, 1.0f);
    pane->addSlider("Smoothness", &shine, 1.0f, 100.0f);

    gui->pack();
    addWidget(gui);
    gui->moveTo(Point2(10, 10));
}


void PixelShaderApp::makeLighting() {
    environment.lightArray.append(Light::directional("Light", Vector3(1.0f, 1.0f, 1.0f), Color3(1.0f), false));

    // The environmentMap is a cube of six images that represents the incoming light to the scene from
    // the surrounding environment.  G3D specifies all six faces at once using a wildcard and loads
    // them into an OpenGL cube map.

    Texture::Specification environmentMapTexture;
    environmentMapTexture.filename   = FilePath::concat(System::findDataFile("noonclouds"), "noonclouds_*.png");

    environmentMapTexture.dimension  = Texture::DIM_CUBE_MAP;
    environmentMapTexture.settings   = Texture::Settings::cubeMap();
    environmentMapTexture.preprocess = Texture::Preprocess::gamma(2.1f);
    // Reduce memory size required to work on older GPUs
    environmentMapTexture.preprocess.scaleFactor = 0.25f;
    environmentMapTexture.settings.interpolateMode = Texture::BILINEAR_NO_MIPMAP;

    environment.environmentMapArray.append(Texture::create(environmentMapTexture), 1.0f);
}

void PixelShaderApp::onWait(RealTime)
{
}

}
