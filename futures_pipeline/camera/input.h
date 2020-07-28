#pragma once

#include <cstdint>
#include <vector>

#include "sync-log.h"

class Input
{
public:
    Input(uint8_t niceness) :
        niceness_(niceness)
    {
        synclog_ = SyncLog::GetLog();
        synclog_->Log("Input()"); 
        instance_ = ++instances_;
    }
    ~Input() {
        synclog_->Log(std::to_string(instance_)+": ~Input()"); 
    }
    int64_t instance_;
    static int64_t instances_;

    uint8_t niceness_;

    int32_t load_;
    std::vector<uint8_t> data_[1024];

private:
    SyncLog* synclog_;
};
int64_t Input::instances_{0};
