#pragma once

#include <iostream>
#include <mutex>
#include <string>


class SyncLog
{

public:

    static SyncLog* GetLog();

    void Log(std::string msg);

private:

    std::mutex log_mtx_;
    static SyncLog* log_;

    SyncLog();
};
