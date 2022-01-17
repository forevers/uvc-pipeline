#pragma once

#include <set>
#include <string>

#include "rapidjson/document.h"
#include "rapidjson/schema.h"

#include "schema.h"
#include "sync-log.h"


class CameraCapabilities
{
public:

    CameraCapabilities();
    ~CameraCapabilities();

    /* detect cameras capabilities */
    bool DetectCameras();
    bool DetectCameras_v2();

    std::string GetSupportedVideoModes();

private:

    bool detected_;

    /* supported video formats */
    static const std::set<std::string> supported_video_formats_;
    static const std::set<std::string>& SupportedVideoFormats_() {
        return supported_video_formats_;
    }

    /* rapidJSON DOM- Default template parameter uses UTF8 and MemoryPoolAllocator */
    rapidjson::Document camera_modes_doc_;

    // // TODO udevadm monitor to see usb register and unregistered devices
    /* rapidJSON DOM- Default template parameter uses UTF8 and MemoryPoolAllocator */
    rapidjson::Document camera_doc_;

    // static bool camera_types_validated_{false};

    /* console logger */
    std::shared_ptr<SyncLog> synclog_;
};
