#pragma once

#include <boost/circular_buffer.hpp>
#include <condition_variable>
#include <mutex>
#include <stddef.h>
#include <vector>

#include "sync-log.h"

// std::function
// #include <functional>


// template <class T, int QUEUE_CAPACITY>
template <class T>
class FrameQueue {

public:

    FrameQueue();
    virtual ~FrameQueue();

    void Init(size_t num_elems, size_t data_bytes);
    void ClearPool();

    int AddFrameToQueue(T *frame, bool is_running);

    void NullFrameOnEmpty(bool enable);

    T* GetQueueFrame();

    T* WaitForFrame(bool is_running);

    T* GetPoolFrame();
    void ReturnPoolFrame(T *frame);

    void FlushQueueFrames();

    T* AllocateFrame(size_t payload_bytes, uint32_t reuse_token);

    void FreeFrame(T* frame);

    typedef boost::circular_buffer<T*> CircularBuffer;

    CircularBuffer circular_buffer_;

    /* pool of preallocated frames available for use */
    std::vector<T*> frame_pool_;

    /* frame queue pool and frame queue access mutexes */
    std::mutex pool_mutex_;
    std::mutex queue_mutex_;

    /* queue frame available conditional signal */
    std::condition_variable queue_frame_avail_cv_;

    bool null_frame_on_empty_;

private:

    bool initialized_;
    size_t num_frames_;
    std::shared_ptr<SyncLog> synclog_;
};


template <class T>
FrameQueue<T>::FrameQueue() :
    null_frame_on_empty_{false},
    initialized_{false},
    num_frames_(0),
    synclog_(SyncLog::GetLog())
{
    synclog_->LogV(FFL,"entry");

    // queue_elem_pool_.reserve(QUEUE_CAPACITY);
    // synclog_->LogV(FFL," queue_elem_pool_.size(): %zu", queue_elem_pool_.size());

    synclog_->LogV(FFL,"exit");
};


template <class T>
FrameQueue<T>::~FrameQueue()
{
    synclog_->LogV(FFL,"entry");

    FlushQueueFrames();
    ClearPool();

    synclog_->LogV(FFL,"exit");
}


template <class T>
void FrameQueue<T>::Init(size_t num_frames, size_t data_bytes)
{
    synclog_->LogV(FFL,"entry");

    if (initialized_) {
        synclog_->LogV(FFL,"already initialized");
        return;
    }

    synclog_->LogV(FFL,"frames: ",num_frames,", frame size: ",data_bytes);

    initialized_ = true;
    num_frames_ = num_frames;
    circular_buffer_.set_capacity(num_frames_);

    std::lock_guard<std::mutex> lk(pool_mutex_);
    for (size_t i = 0; i < num_frames_; i++) {
        frame_pool_.push_back(AllocateFrame(data_bytes, i));
    }
    synclog_->LogV(FFL," queue_elem_pool_.size(): ",frame_pool_.size());

    synclog_->LogV(FFL,"exit");
}


template <class T>
void FrameQueue<T>::ClearPool()
{
    synclog_->LogV(FFL,"entry");

    std::lock_guard<std::mutex> lk(pool_mutex_);
    const int n = frame_pool_.size();
    for (int i = 0; i < n; i++) {
        /* nullptr may be used for signalling */
        if (frame_pool_[i]) FreeFrame(frame_pool_[i]);
    }
    frame_pool_.clear();
    synclog_->LogV(FFL," frame_pool_.size(): ",frame_pool_.size());

    synclog_->LogV(FFL,"exit");
}


template <class T>
T* FrameQueue<T>::GetQueueFrame()
{
    T* frame = nullptr;
    {
        std::lock_guard<std::mutex> lk(queue_mutex_);
        if (circular_buffer_.size() > 0) {
            frame = circular_buffer_.front();
            circular_buffer_.pop_front();
        }
    }

    return frame;
}


template <class T>
T* FrameQueue<T>::GetPoolFrame()
{
    T* frame = nullptr;
    {
        std::lock_guard<std::mutex> lk(pool_mutex_);
        if (!frame_pool_.empty()) {
            frame = frame_pool_.back();
            frame_pool_.pop_back();
        }
    }

    if (!frame) {
        synclog_->LogV(FFL," frame_pool_ empty");
    }
    return frame;
}


template <class T>
void FrameQueue<T>::ReturnPoolFrame(T* frame)
{
    {
        std::lock_guard<std::mutex> lk(pool_mutex_);
        if ((frame_pool_.size() < num_frames_)) {
            frame_pool_.push_back(frame);
            frame = nullptr;
            // synclog_->LogV(FFL,"*****pb");
        }
    }

    if (frame) {
        synclog_->LogV(FFL," frame_pool_ full");
        FreeFrame(frame);
    }
}


template <class T>
int FrameQueue<T>::AddFrameToQueue(T *frame, bool is_running)
{
    {
        std::lock_guard<std::mutex> lk(queue_mutex_);
        if (is_running && circular_buffer_.size() < num_frames_) {
            circular_buffer_.push_back(frame);
            frame = nullptr;
            synclog_->LogV(FFL," notify_one()");
            queue_frame_avail_cv_.notify_one();
        }
    }

    if (frame) {
        ReturnPoolFrame(frame);
        return 0;
    } else {
        return 0;
    }
}


template <class T>
void FrameQueue<T>::NullFrameOnEmpty(bool enable)
{
    std::lock_guard<std::mutex> lk(queue_mutex_);
    null_frame_on_empty_ = enable;
}


template <class T>
T* FrameQueue<T>::WaitForFrame(bool is_running)
{
    T* frame = nullptr;

    synclog_->LogV(FFL, " entry");

    std::unique_lock<std::mutex> lk(queue_mutex_);

    if (!circular_buffer_.size()) {
        if (!null_frame_on_empty_) {
            /* block until elem available */
            synclog_->LogV(FFL," pre wait");
            queue_frame_avail_cv_.wait(lk);
            synclog_->LogV(FFL," post wait");
        }
    }

    synclog_->LogV(FFL, " size: ", circular_buffer_.size());
    if (is_running && circular_buffer_.size() > 0) {
        synclog_->LogV(FFL, "pop the frame");
        frame = circular_buffer_.front();
        circular_buffer_.pop_front();
    }
    else {
        synclog_->LogV(FFL," null frame passed");
    }

    return frame;
}


template <class T>
void FrameQueue<T>::FlushQueueFrames()
{
    synclog_->LogV(FFL,"entry");

    std::lock_guard<std::mutex> lk(queue_mutex_);

    for (size_t i = 0; i < circular_buffer_.size(); i++)
        ReturnPoolFrame(circular_buffer_[i]);

    circular_buffer_.clear();

    synclog_->LogV(FFL,"exit");
}


template <class T>
T* FrameQueue<T>::AllocateFrame(size_t payload_bytes, uint32_t reuse_token)
{
    T* camera_frame = new T;
    memset(camera_frame, 0, sizeof(T));

    if (payload_bytes > 0) {
        camera_frame->reuse_token = reuse_token;
        camera_frame->actual_bytes = camera_frame->data_bytes = payload_bytes;
        synclog_->LogV(FFL,"allocate :",payload_bytes," bytes");
        camera_frame->data = new uint8_t[payload_bytes];

        if (!camera_frame->data) {
            synclog_->LogV(FFL,"allocation failure");
            return NULL ;
        } else{
            synclog_->LogV(FFL,"reuse ",reuse_token," allocated @: ", static_cast<void*>(camera_frame->data));
        }
    }

    return camera_frame;
}


template <class T>
void FrameQueue<T>::FreeFrame(T* frame)
{
    if (frame->data) delete [] frame->data;
    delete frame;
}
