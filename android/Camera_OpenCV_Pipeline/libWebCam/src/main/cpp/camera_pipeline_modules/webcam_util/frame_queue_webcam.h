#ifndef FRAME_QUEUE_WEBCAM_H
#define FRAME_QUEUE_WEBCAM_H

#include "frame_queue_abstract.h"
#include "webcam_util.h"

//#define FRAME_QUEUE_WEBCAM_TAG "FRAME_QUEUE_WEBCAM"
#define FRAME_QUEUE_WEBCAM_TAG "test"

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
class frame_queue_webcam : public FrameQueueAbstract<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY> {

public:

    frame_queue_webcam();
    ~frame_queue_webcam();

    virtual T* AllocateFrame(size_t data_bytes);
    virtual void FreeFrame(T *frame);
};

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
frame_queue_webcam<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::frame_queue_webcam()
        : FrameQueueAbstract<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>() {
    ENTER_(FRAME_QUEUE_WEBCAM_TAG);
    frame_queue_webcam::NullFrameOnEmpty(false);
    EXIT_(FRAME_QUEUE_WEBCAM_TAG);
}

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
frame_queue_webcam<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::~frame_queue_webcam() {
    ENTER_(FRAME_QUEUE_WEBCAM_TAG);
    frame_queue_webcam::ClearPool();
    EXIT_(FRAME_QUEUE_WEBCAM_TAG);
}

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
T* frame_queue_webcam<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::AllocateFrame(size_t data_bytes) {
    return allocate_camera_frame(data_bytes);

}

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
void frame_queue_webcam<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::FreeFrame(T* frame) {
    free_camera_frame(frame);
};

#endif // FRAME_QUEUE_WEBCAM_H
