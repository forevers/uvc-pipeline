#pragma once

#include <string>

#include "rapidjson/prettywriter.h" // for stringify JSON

using namespace rapidjson;


class CameraDevice {

public:

    CameraDevice();
    ~CameraDevice();

    CameraDevice(const std::string& device_node, std::string& camera_card_type) : device_node_(device_node), camera_card_type_(camera_card_type) {}
    ~CameraDevice();

    template <typename Writer>
    void Serialize(Writer& writer) const {
        writer.StartObject();

        Person::Serialize(writer);

        writer.String("device_node");
        writer.String(device_node_);

        writer.String(("camera_card_type"));
        writer.String(camera_card_type.c_str());

        // writer.StartArray();
        // for (std::vector<Dependent>::const_iterator dependentItr = dependents_.begin(); dependentItr != dependents_.end(); ++dependentItr)
        //     dependentItr->Serialize(writer);
        // writer.EndArray();

        writer.EndObject();
    }

private:

    std::string device_node_;
    std::string camera_card_type_;
};