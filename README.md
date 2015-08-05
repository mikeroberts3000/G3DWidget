![Alt text](/documentation/images/screenshot.jpg)

This repository contains the source code for G3DWidget. G3DWidget is a Qt Widget written in C++ that can host 3D rendering code from the G3D Innovation Engine. The G3DWidget class has been carefully designed so that multiple G3DWidget objects can coexist in the same application. Demo application included.

### Requirements

* __Mac OSX__. I'm using Yosemite 10.10.4, but this code should also work for other recent versions of Mac OSX. If you're not on Mac OSX, there is a minimal amount of very easy boilerplate OpenGL code you'll need to implement to get everything working. Drop me a line if you hit any roadblocks.
* __XCode Commandline Tools__. I'm using XCode 4.5.2, but this code should also work for other recent versions of XCode.
  * https://developer.apple.com/downloads/index.action
* __G3D10__. You'll need to be able to compile and link against the G3D10 headers and libraries.
  * https://g3d.codeplex.com/
* __Qt__. You'll need to be able to compile and link against the Qt headers and libraries. I'm using Qt 5.5 but this code should also work for other recent versions of Qt.
  * https://www.qt.io/download-open-source/
* __Qt Creator__. I'm using Qt Creator 3.4.2 but this code should work for other recent versions of Qt Creator. Advanced users can work directly from the commandline using qmake and make. See the Qt Creator build output for guidance.
  * https://www.qt.io/download-open-source/
* __Boost__. I'm using Boost 1.58, which I obtained from MacPorts, but this code should also work for other recent versions of Boost.

### Build Instructions

1. Build G3DWidget/Code/G3DWidgetDemo.pro in Qt Creator. In the Projects > Build and Run > Build Environment, be sure to add the G3D10DATA environment variable. This step assumes that you have access to the G3D10 headers and libraries, the Qt headers and libraries, and the Boost headers, as outlined above.
2. Now you can run and debug the G3DWidgetDemo application.

### Usage Example

```cpp

//
// When creating G3DWidgets, we need to pass in a G3DWidgetOpenGLContext and a
// GLG3D::RenderDevice. Decoupling the creation of G3DWidgets from OpenGL resources,
// e.g., G3DWidgetOpenGLContext and GLG3D::RenderDevice, allows these resources
// to be shared across multiple G3DWidgets. This is useful, e.g., for rendering
// the same scene from multiple angles in different G3DWidgets.
//
MainWindow::MainWindow(QWidget* parent) :
    QMainWindow             (parent),
    m_g3dWidgetOpenGLContext(new G3DWidgetOpenGLContext(G3D::OSWindow::Settings())),
    m_renderDevice          (new G3D::RenderDevice),
    m_starterAppWidget      (new G3DWidget(m_g3dWidgetOpenGLContext, m_renderDevice, this)),
    m_pixelShaderAppWidget  (new G3DWidget(m_g3dWidgetOpenGLContext, m_renderDevice, this)),
    m_timer                 (new QTimer(this)),
    m_g3dWidgetsInitialized (false) {
    
    connect(m_timer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));
    m_timer->start(15);
}

void MainWindow::paintEvent(QPaintEvent* e) {

    //
    // Note that we need to defer the wiring up of our G3DWidgets until the first
    // paint event because otherwise they are not guaranteed to have valid window handles.
    //
    if (!m_g3dWidgetsInitialized) {

        //
        // Our first step is to initialize the G3DWidgets.
        //
        m_starterAppWidget->initialize();
        m_pixelShaderAppWidget->initialize();

        //
        // Now that we have initialized our G3DWidgets, we can initialize our
        // GLG3D::RenderDevice. Note that we arbitrarily choose a single G3DWidget to
        // initialize our GLG3D::RenderDevice (i.e., instead of passing in m_starterAppWidget,
        // we could have just as easily passed in m_pixelShaderAppWidget). We need to
        // pass in some G3DWidget to prevent the GLG3D::RenderDevice from creating its
        // own. Note also that the G3DWidget we pass in must be current.
        //
        m_starterAppWidget->makeCurrent();
        m_renderDevice->init(m_starterAppWidget);

        //
        // Now that we have have initialized our GLG3D::RenderDevice, we can create our
        // GLG3D::GApps. Note that the G3DWidget we pass in to each GLG3D::GApp constructor
        // must be current. Note also that the G3D::StarterApp and G3D::PixelShaderApp classes
        // created here are identical to those in the starter and pixelShader sample applications
        // from the G3D 9.00 source code. 
        //
        m_starterAppWidget->makeCurrent();
        m_starterApp = new G3D::StarterApp(G3D::GApp::Settings(), m_starterAppWidget, m_renderDevice);

        m_pixelShaderAppWidget->makeCurrent();
        m_pixelShaderApp = new G3D::PixelShaderApp(G3D::GApp::Settings(), m_pixelShaderAppWidget, m_renderDevice);

        //
        // We complete the wiring up of our G3DWidgets by calling pushLoopBody(...) and
        // passing in a specific GLG3D::GApp
        //
        m_starterAppWidget->pushLoopBody(m_starterApp);
        m_pixelShaderAppWidget->pushLoopBody(m_pixelShaderApp);

        m_g3dWidgetsInitialized = true;
    }
}

void MainWindow::closeEvent(QCloseEvent*) {

    m_timer->stop();

    //
    // To clean up our G3DWidgets, we call popLoopBody() and then terminate(). To clean up
    // our GLG3D::RenderDevice, we call cleanup() as usual. We call these cleanup methods in
    // the opposite order as we called their corresponding initialization methods.
    //
    m_starterAppWidget->popLoopBody();
    m_pixelShaderAppWidget->popLoopBody();
    m_renderDevice->cleanup();
    m_starterAppWidget->terminate();
    m_pixelShaderAppWidget->terminate();
}

void MainWindow::onTimerTimeout() {

    //
    // To invoke the loop body of each GLG3D::GApp, we call update() on its
    // corresponding G3DWidget.
    //
    m_starterAppWidget->update();
    m_pixelShaderAppWidget->update();
}
```
