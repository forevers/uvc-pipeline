#ifndef RENDERER_CONTROL_IFC_H
#define RENDERER_CONTROL_IFC_H

#include <android/native_window.h>

class IRendererControl {

public:

    virtual ~IRendererControl() {};

    virtual int SetSurface(ANativeWindow *preview_window) = 0;

    virtual int Start() = 0;

    virtual int Stop() = 0;
};

#endif // RENDERER_CONTROL_IFC_H
