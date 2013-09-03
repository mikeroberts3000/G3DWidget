#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <string>
#include <tr1/memory>

#include <QtCore/QTimer>
#include <QtGui/QMainWindow>

#include "ui_MainWindow.h"

namespace G3D
{
class GApp;
class RenderDevice;
}

namespace mojo
{

class G3DWidgetOpenGLContext;
class G3DWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = 0);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent* e);
    void closeEvent(QCloseEvent* e);

private slots:
    void onTimerTimeout();

private:
    std::tr1::shared_ptr<Ui::MainWindow>         m_ui;
    std::tr1::shared_ptr<G3DWidgetOpenGLContext> m_g3dWidgetOpenGLContext;
    std::tr1::shared_ptr<G3D::RenderDevice>      m_renderDevice;
    std::tr1::shared_ptr<G3D::GApp>              m_starterApp;
    std::tr1::shared_ptr<G3D::GApp>              m_pixelShaderApp;
    G3DWidget*                                   m_starterAppWidget;
    G3DWidget*                                   m_pixelShaderAppWidget;
    QTimer*                                      m_timer;
    bool                                         m_g3dWidgetsInitialized;
};

}

#endif
