#ifndef FRAME_ACCESS_REGISTRATION_IFC_H
#define FRAME_ACCESS_REGISTRATION_IFC_H

#include <memory>

#include "webcam_include.h"
#include "webcam_frame_access_ifc.h"

typedef std::shared_ptr<ICameraFrameAccess> FrameAccessSharedPtr;


class IFrameAccessRegistration {

public:

    virtual ~IFrameAccessRegistration() {};

    virtual FrameAccessSharedPtr RegisterClient(void) = 0;
};

#endif // FRAME_ACCESS_REGISTRATION_IFC_H
