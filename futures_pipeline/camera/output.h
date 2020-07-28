#pragma once

#include <cstdint>
#include <vector>

#include "sync-log.h"


class Output
{
public:
    Output(int64_t instance) {
        synclog_ = SyncLog::GetLog();
        synclog_->Log("Output()"); 
        instance_ = instance;
    }
    ~Output() {
        synclog_->Log(std::to_string(instance_)+": ~Output()"); 
    }

    int64_t instance_;
    std::vector<int8_t> data_[2048];

private:
    SyncLog* synclog_;
};