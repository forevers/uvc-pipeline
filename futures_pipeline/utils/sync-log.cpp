#include "sync-log.h"

std::mutex SyncLog::log_mtx_;
std::shared_ptr<SyncLog> SyncLog::log_ = nullptr;


std::shared_ptr<SyncLog> SyncLog::GetLog()
{
    if (log_ == nullptr) log_ = std::make_shared<SyncLog>();
    return log_;
}


void SyncLog::Log(std::string msg)
{
    std::lock_guard<std::mutex> lck (SyncLog::log_mtx_);
    std::cout << msg << std::endl;
}


SyncLog::SyncLog() 
{
}

