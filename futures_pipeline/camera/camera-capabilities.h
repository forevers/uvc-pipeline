#pragma once

#include <string>

#include "rapidjson/document.h"

#include "schema.h"
#include "rapidjson/schema.h"
// #include "rapidjson/document.h"

class CameraCapabilities
{
public:

    CameraCapabilities();
    ~CameraCapabilities();

    /* detect cameras capabilities */
    bool DetectCameras();

    std::string GetSupportedVideoModes();

private:

    bool detected_;

    // /* rapidJSON DOM- Default template parameter uses UTF8 and MemoryPoolAllocator */
    rapidjson::Document camera_modes_doc_;

    // // TODO udevadm monitor to see usb register and unregistered devices
    /* rapidJSON DOM- Default template parameter uses UTF8 and MemoryPoolAllocator */
    rapidjson::Document camera_doc_;

    // static bool camera_types_validated_{false};
};
