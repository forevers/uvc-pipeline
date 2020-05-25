#include "SyncLog.h"

SyncLog *SyncLog::log_ = 0;

SyncLog* SyncLog::GetLog()
{
    if (!log_) log_ = new SyncLog;
    return log_;
}

void SyncLog::Log(std::string msg)
{
    std::lock_guard<std::mutex> lck (log_mtx_);
    std::cout << msg << std::endl;
}


SyncLog::SyncLog() 
{
}

