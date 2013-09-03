#include "MainWindow.hpp"

#include <boost/type_traits.hpp>

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtWebKit/QWebView>

#include <GLG3D/RenderDevice.h>
#include <GLG3D/GApp.h>

#include "StarterApp.hpp"
#include "PixelShaderApp.hpp"

#include "QtUtil.hpp"
#include "G3DWidgetOpenGLContext.hpp"
#include "G3DWidget.hpp"

namespace mojo
{

//
// When creating G3DWidgets, we need to pass in a G3DWidgetOpenGLContext and a
// GLG3D::RenderDevice. Decoupling the creation of G3DWidgets from OpenGL resources,
// e.g., G3DWidgetOpenGLContext and GLG3D::RenderDevice, allows these resources
// to be shared across multiple G3DWidgets. This is useful, e.g., for rendering
// the same scene from multiple angles in different G3DWidgets.
//
MainWindow::MainWindow(QWidget* parent) :
    QMainWindow             (parent),
    m_ui                    (new Ui::MainWindow),
    m_g3dWidgetOpenGLContext(new G3DWidgetOpenGLContext(G3D::OSWindow::Settings())),
    m_renderDevice          (new G3D::RenderDevice),
    m_starterAppWidget      (new G3DWidget(m_g3dWidgetOpenGLContext, m_renderDevice, this)),
    m_pixelShaderAppWidget  (new G3DWidget(m_g3dWidgetOpenGLContext, m_renderDevice, this)),
    m_timer                 (new QTimer(this)),
    m_g3dWidgetsInitialized (false) {
    m_ui->setupUi(this);
    m_starterAppWidget->setMinimumSize(800, 800);
    m_pixelShaderAppWidget->setMinimumSize(400, 400);
    m_pixelShaderAppWidget->setMaximumSize(400, 400);

    QDockWidget* dockWidgetTop    = findChild<QDockWidget*>("dockWidgetTop");
    QDockWidget* dockWidgetBottom = findChild<QDockWidget*>("dockWidgetBottom");

    QWebView* webView = new QWebView(dockWidgetBottom);
    webView->setUrl(QUrl("http://g3d.sourceforge.net/"));

    setCentralWidget(m_starterAppWidget);
    dockWidgetTop->setWidget(m_pixelShaderAppWidget);
    dockWidgetBottom->setWidget(webView);

    MOJO_QT_SAFE(connect(m_timer, SIGNAL(timeout()), this, SLOT(onTimerTimeout())));
    m_timer->start(15);
}

MainWindow::~MainWindow() {}

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
        // Now that we have initialized our G3DWidgets, we can initialized our
        // GLG3D::RenderDevice. Note that arbitrarily choose a single G3DWidget to
        // initialize our GLG3D::RenderDevice. Instead of passing in m_starterAppWidget,
        // we could have just as easily passed in m_pixelShaderAppWidget. We need to
        // pass in some G3DWidget to prevent the GLG3D::RenderDevice from creating its
        // own.
        //
        m_starterAppWidget->makeCurrent();
        m_renderDevice->init(m_starterAppWidget);

        //
        // Now that we have have initialized our GLG3D::RenderDevice, we can create our
        // GLG3D::GApps. Note that the G3DWidget we pass in must be current.
        //
        m_starterAppWidget->makeCurrent();
        m_starterApp = std::tr1::shared_ptr<G3D::GApp>(new G3D::StarterApp(
            G3D::GApp::Settings(),
            m_starterAppWidget,
            m_renderDevice.get()));

        m_pixelShaderAppWidget->makeCurrent();
        m_pixelShaderApp = std::tr1::shared_ptr<G3D::GApp>(new G3D::PixelShaderApp(
            G3D::GApp::Settings(),
            m_pixelShaderAppWidget,
            m_renderDevice.get()));

        //
        // We complete the wiring up of our G3DWidgets by binding a specific GLG3D::GApp
        // to each of them.
        //
        m_starterAppWidget->pushLoopBody(m_starterApp.get());
        m_pixelShaderAppWidget->pushLoopBody(m_pixelShaderApp.get());

        m_g3dWidgetsInitialized = true;
    }
}

void MainWindow::closeEvent(QCloseEvent*) {
    m_starterAppWidget->terminate();
    m_pixelShaderAppWidget->terminate();
    m_renderDevice->cleanup();
    m_timer->stop();
}

void MainWindow::onTimerTimeout() {
    m_starterAppWidget->update();
    m_pixelShaderAppWidget->update();
}

}
