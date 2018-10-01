#ifndef OPENCV_CONTROL_IFC_H
#define OPENCV_CONTROL_IFC_H

#include "frame_access_registration_ifc.h"



class IOpenCVControl {

public:

    enum class ProcessingMode {
        PROCESSING_MODE_NONE,
        PROCESSING_MODE_BALL_TRACKER,
        PROCESSING_MODE_NUM
    };

    virtual ~IOpenCVControl() {};

    virtual IFrameAccessRegistration* GetFrameAccessIfc(int interface_number) = 0;

    virtual CameraError Prepare() = 0;

    virtual void Version() = 0;

    virtual int Start() = 0;

    virtual void ConfigProcessingMode(IOpenCVControl::ProcessingMode demo_mode) = 0;

    virtual int Stop() = 0;

    virtual int CycleProcessingMode() = 0;

    virtual int ScaleIfGray16(bool downscale) = 0;
};

#endif // OPENCV_CONTROL_IFC_H
