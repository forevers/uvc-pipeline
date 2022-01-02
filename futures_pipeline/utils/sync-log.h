#pragma once

#include <iostream>
#include <memory>
#include <mutex>
#include <string>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define FFL "[",__FILENAME__,":",__func__,":",__LINE__,"]: "

class SyncLog
{
public:

    static std::shared_ptr<SyncLog> GetLog();

    void Log(std::string msg);

    template<class... Args>
    void LogV(Args... args)
    {
        std::lock_guard<std::mutex> lck (SyncLog::log_mtx_);
        (std::cout << ... << args) << "\n" << std::flush;
    };

    // TODO __FILE_NAME__ avail in clang version 9

private:

    static std::mutex log_mtx_;
    static std::shared_ptr<SyncLog> log_;

// TODO make private
public:
    SyncLog();
};
