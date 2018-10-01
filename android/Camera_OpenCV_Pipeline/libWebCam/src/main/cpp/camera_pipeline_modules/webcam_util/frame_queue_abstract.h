#ifndef FRAME_QUEUE_ABSTRACT_H
#define FRAME_QUEUE_ABSTRACT_H

#include <boost/circular_buffer.hpp>
#include <stddef.h>
#include <vector>

#include "util.h"

//#define FRAME_QUEUE_ABSTRACT_TAG "FRAME_QUEUE_ABSTRACT"
#define FRAME_QUEUE_ABSTRACT_TAG "test"

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
class FrameQueueAbstract {

public:

    FrameQueueAbstract();
    virtual ~FrameQueueAbstract();

    void InitPool(size_t data_bytes);
    void ClearPool();

    int AddFrameToQueue(T *frame, bool is_running);

    void NullFrameOnEmpty(bool enable);
    T* GetFrameFromQueue(bool is_running);
    T* WaitForQueueFrame(bool is_running);

    T* GetFrameFromPool(size_t data_bytes);
    void RecycleFrameIntoPool(T *frame);

    void ClearQueueFrames();

    virtual T* AllocateFrame(size_t data_bytes) = 0;
    virtual void FreeFrame(T* frame) = 0;

    int MutexLock();
    int MutexUnlock();
    int CondSignal();

    // circular buffer of active frames
    typedef boost::circular_buffer<T*> CircularBuffer;

    CircularBuffer preview_frames_{FRAME_Q_CAPACITY};

    // cache of preallocated frames available for use
    std::vector<T*> frame_pool_;

    // frame pool and frame queue access mutexes
    pthread_mutex_t pool_mutex_, queue_mutex_;

    // frame available conditional signal
    pthread_cond_t queue_sync_;

    bool null_frame_on_empty_;
};

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
FrameQueueAbstract<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::FrameQueueAbstract() {
    ENTER_(FRAMEQUEUE_TAG);

    frame_pool_.reserve(FRAME_POOL_CAPACITY);

    LOGI_(FRAME_QUEUE_ABSTRACT_TAG, " frame_pool_.size(): %zu", frame_pool_.size());

    pthread_cond_init(&queue_sync_, nullptr);
    pthread_mutex_init(&queue_mutex_, nullptr);
    pthread_mutex_init(&pool_mutex_, nullptr);

    EXIT_(FRAMEQUEUE_TAG);
};

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
FrameQueueAbstract<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::~FrameQueueAbstract() {
    ENTER_(FRAMEQUEUE_TAG);

    ClearQueueFrames();

    pthread_mutex_destroy(&queue_mutex_);
    pthread_cond_destroy(&queue_sync_);
    pthread_mutex_destroy(&pool_mutex_);

    EXIT_(FRAMEQUEUE_TAG);
};

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
void FrameQueueAbstract<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::InitPool(size_t data_bytes) {
    ENTER_(FRAMEQUEUE_TAG);

    ClearPool();
    pthread_mutex_lock(&pool_mutex_);
    {
        for (int i = 0; i < FRAME_POOL_CAPACITY; i++) {
            frame_pool_.push_back(AllocateFrame(data_bytes));
        }
        LOGI_(FRAME_QUEUE_ABSTRACT_TAG, " frame_pool_.size(): %zu", frame_pool_.size());
    }
    pthread_mutex_unlock(&pool_mutex_);

    EXIT_(FRAMEQUEUE_TAG);
}

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
void FrameQueueAbstract<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::ClearPool() {
    ENTER_(FRAMEQUEUE_TAG);

    pthread_mutex_lock(&pool_mutex_);
    {
        const int n = frame_pool_.size();
        for (int i = 0; i < n; i++) {
            FreeFrame(frame_pool_[i]);
        }
        frame_pool_.clear();
        LOGI_(FRAME_QUEUE_ABSTRACT_TAG, " frame_pool_.size(): %zu", frame_pool_.size());
    }
    pthread_mutex_unlock(&pool_mutex_);

    EXIT_(FRAMEQUEUE_TAG);
}

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
T* FrameQueueAbstract<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::GetFrameFromPool(size_t data_bytes) {

    T* frame = nullptr;
    pthread_mutex_lock(&pool_mutex_);
    {
        if (!frame_pool_.empty()) {
            frame = frame_pool_.back();
            frame_pool_.pop_back();
        }
    }
    pthread_mutex_unlock(&pool_mutex_);

    if (!frame) {
        LOGE_(FRAME_QUEUE_ABSTRACT_TAG, "GETFrame() frame_pool_ empty");
    }
    return frame;
}

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
void FrameQueueAbstract<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::RecycleFrameIntoPool(T *frame) {

    pthread_mutex_lock(&pool_mutex_);
    if ((frame_pool_.size() < FRAME_POOL_CAPACITY)) {
        frame_pool_.push_back(frame);
        frame = nullptr;
    }
    pthread_mutex_unlock(&pool_mutex_);

    if (frame) {
        LOGE_(FRAME_QUEUE_ABSTRACT_TAG, "RecycleFrameIntoPool() frame_pool_ full");
        FreeFrame(frame);
    }
}

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
int FrameQueueAbstract<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::AddFrameToQueue(T *frame, bool is_running) {

    MutexLock();
    if (is_running && preview_frames_.size() < FRAME_Q_CAPACITY) {
        preview_frames_.push_back(frame);
        frame = nullptr;
        pthread_cond_signal(&queue_sync_);
    }
    MutexUnlock();

    if (frame) {
        RecycleFrameIntoPool(frame);
        return -1;
    } else {
        return 0;
    }
}


template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
void FrameQueueAbstract<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::NullFrameOnEmpty(bool enable) {
    MutexLock();
    {
        null_frame_on_empty_ = enable;
    }
    MutexUnlock();
}


template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
T* FrameQueueAbstract<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::WaitForQueueFrame(bool is_running) {
    T* frame = nullptr;
    MutexLock();
    {
        if (!preview_frames_.size()) {
            if (!null_frame_on_empty_) {
                // block until frame available
                pthread_cond_wait(&queue_sync_, &queue_mutex_);
            }
        }

        if (is_running && preview_frames_.size() > 0) {
            frame = preview_frames_.front();
            preview_frames_.pop_front();
        }
        else {
            LOGI_(FRAME_QUEUE_ABSTRACT_TAG, " null frame passed");
        }
    }
    MutexUnlock();

    return frame;
}

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
T* FrameQueueAbstract<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::GetFrameFromQueue(bool is_running) {

    T* frame = nullptr;
    MutexLock();
    {
        if (is_running && preview_frames_.size() > 0) {
            frame = preview_frames_.front();
            preview_frames_.pop_front();
        }
    }
    MutexUnlock();

    return frame;
}

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
void FrameQueueAbstract<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::ClearQueueFrames() {
    ENTER_(FRAME_QUEUE_ABSTRACT_TAG);

    MutexLock();
    {
        for (int i = 0; i < preview_frames_.size(); i++)
            RecycleFrameIntoPool(preview_frames_[i]);
        preview_frames_.clear();
    }
    MutexUnlock();

    EXIT_(FRAME_QUEUE_ABSTRACT_TAG);
}

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
int FrameQueueAbstract<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::MutexLock() {
    return pthread_mutex_lock(&queue_mutex_);
};

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
int FrameQueueAbstract<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::MutexUnlock() {
    return pthread_mutex_unlock(&queue_mutex_);
};

template <class T, int FRAME_Q_CAPACITY, int FRAME_POOL_CAPACITY>
int FrameQueueAbstract<T, FRAME_Q_CAPACITY, FRAME_POOL_CAPACITY>::CondSignal() {
    return pthread_cond_signal(&queue_sync_);
}

#endif // FRAME_QUEUE_ABSTRACT_H
