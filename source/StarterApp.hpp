#ifndef STARTER_APP_HPP
#define STARTER_APP_HPP

#include "G3D/G3D.h"
#include "GLG3D/GLG3D.h"

namespace G3D
{

class StarterApp : public GApp {
protected:
    bool                m_showWireframe;

    /** Called from onInit */
    void makeGUI();

    /** Called from onInit */
    void makeGBuffer();

public:

    StarterApp(const GApp::Settings& settings = GApp::Settings(), OSWindow* window=NULL, RenderDevice* rd=NULL);

    virtual void onInit() override;
    virtual void onAI() override;
    virtual void onNetwork() override;
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
    virtual void onPose(Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) override;

    // You can override onGraphics if you want more control over the rendering loop.
    // virtual void onGraphics(RenderDevice* rd, Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) override;

    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) override;
    virtual void onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& surface2D) override;

    virtual bool onEvent(const GEvent& e) override;
    virtual void onUserInput(UserInput* ui) override;
    virtual void onCleanup() override;

    /** Sets m_endProgram to true. */
    virtual void endProgram();
};

}

#endif
