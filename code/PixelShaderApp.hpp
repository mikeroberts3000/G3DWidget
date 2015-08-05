#ifndef PIXEL_SHADER_APP_HPP
#define PIXEL_SHADER_APP_HPP

#include "G3D/G3D.h"
#include "GLG3D/GLG3D.h"

namespace G3D
{

class PixelShaderApp : public GApp {
private:
    shared_ptr<ArticulatedModel>         model;

    float                                lambertianScalar;
    int                                  lambertianColorIndex;

    float                                glossyScalar;
    int                                  glossyColorIndex;

    float                                reflect;
    float                                smoothness;

    ////////////////////////////////////
    // GUI

    /** For dragging the model */
    shared_ptr<ThirdPersonManipulator>   manipulator;
    Array<GuiText>                       colorList;

    void makeGui();
    void makeColorList();
    void makeLighting();
    void configureShaderArgs(Args& args);

public:

    PixelShaderApp(const Settings& options=Settings(), OSWindow* window=NULL, RenderDevice* rd=NULL);

    virtual void onInit();
    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D);
};

}

#endif
