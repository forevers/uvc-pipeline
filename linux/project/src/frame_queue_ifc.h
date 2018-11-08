#ifndef FRAME_QUEUE_IFC_H
#define FRAME_QUEUE_IFC_H

#include <memory>


class IFrameQueue {

public:

    struct FrameSize {
        size_t width;
        size_t height;
        size_t allocation_units;
    };

    struct Frame {
        uint8_t* buffer;
        FrameSize frame_size;
    };

    virtual ~IFrameQueue() {};

    virtual void Signal() = 0;

    //virtual CameraError GetFrameDescription(CameraFormat* camera_format, int* height, int* width) = 0;

    virtual Frame GetFrame(void) = 0;

    //virtual void ReleaseFrame(CameraFrame* camera_frame) = 0;

    //virtual void UnregisterClient(void) = 0;
};

#endif // FRAME_ACCESS_IFC_H
