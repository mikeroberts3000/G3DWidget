G3DWidget
=========

G3DWidget is a Qt Widget written in C++ that can host 3D rendering code from the G3D Innovation Engine. The G3DWidget class has been carefully designed so that multiple G3DWidget objects can coexist in the same application. Demo application included.

![Alt text](/documentation/images/screenshot.jpg)

### Requirements

* __Mac OSX__. I'm using Mountain Lion 10.8.4, but this code should also work for other recent versions of Mac OSX. If you're not on Mac OSX, there is a minimal amount of very easy boilerplate OpenGL code you'll need to implement to get everything working. Drop me a line if you hit any roadblocks.
* __XCode Commandline Tools__. I'm using XCode 4.5.2, but this code should also work for other recent versions of XCode.
  * https://developer.apple.com/downloads/index.action
* __The G3D 9.00 Source Code__. You'll need to be able to build the G3D 9.00 source code.
  * http://sourceforge.net/projects/g3d/files/g3d-cpp/9.00/G3D-9.00-src.zip/download
* __The Qt SDK__. I'm using Qt 4.8.2 but this code should also work for other recent versions of Qt.
  * http://download.qt-project.org/archive/qt/4.8/4.8.2/qt-mac-opensource-4.8.2.dmg
* __Qt Creator__. I'm using Qt Creator 2.8.1 but this code should work for other recent versions of Qt Creator. Advanced users can work directly from the commandline using qmake and make. See the Qt Creator build output for guidance.
  * http://download.qt-project.org/official_releases/qtcreator/2.8/2.8.1/qt-creator-mac-opensource-2.8.1.dmg

### Build Instructions

1. Replace the files in G3D9-source/GLG3D.lib/... (part of the G3D 9.00 source code) with the files in G3DWidget/Code/GLG3D.lib/... (part of this GitHub repository). There are only 4 files that need to be replaced. This change to the G3D source code is neccesary to add support for multiple rendering windows. I tried to make the changes to the G3D source code is minimally intrusive as possible.
2. Build G3D 9.00 as outlined in G3D documentation.
3. Build G3DWidget/Code/G3DWidgetDemo.pro in Qt Creator. In the Projects > Build and Run > Manage Kits... > Build and Run > Kits menu, set the compiler to Clang (x86 64bit) and set your Qt mkspec to "unsupported/macx-clang". Depending on your exact version of Qt, the exact string you need to use for your Qt mkspec might vary slightly. In the Projects > Build and Run > Build Environment, be sure to add the G3D9DATA environment variable.
4. Now you can run and debug the G3DWidgetDemo application.

If you hit any roadblocks, you can check out my <a href='https://www.evernote.com/shard/s231/sh/8bb33cf0-d293-40b1-97e3-5a83e8a06ccf/ad73af4d68a2f062ad22c4557d5136cc'>development environment setup instructions</a>.
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
    // our GLG3D::RenderDevice, we call cleanup() as usual. As a matter of style, we call
    // these cleanup methods in the opposite order as we called their corresponding
    // initialization methods.
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

[![githalytics.com alpha](https://cruel-carlota.pagodabox.com/249bed55fb0fcff9c1c23df10f8de899 "githalytics.com")](http://githalytics.com/mroberts3000/G3DWidget)
