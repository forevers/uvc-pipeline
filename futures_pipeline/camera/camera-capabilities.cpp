#include "camera-capabilities.h"

#include <iostream>
#include <sstream>
#include <stdio.h>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "exec-shell.h"


using namespace rapidjson;
using namespace std;


const std::set<std::string> CameraCapabilities::supported_video_formats_ = {
    "YUYV 4:2:2",
    "UYVY 4:2:2"
    };


CameraCapabilities::CameraCapabilities() :
    detected_{false}
{
    camera_doc_.SetObject();

    // TODO linux udev rule, android equivalent to detect usb vbus and register/deregister cameras
    /* v4l2-ctl camera mode detection */
    if (true == (detected_ = DetectCameras())) {
        //cout<<"cameras detected"<<endl;
    } else {
        cout<<"no cameras detected"<<endl;
    }
}


CameraCapabilities::~CameraCapabilities()
{

}


bool CameraCapabilities::DetectCameras()
{
    size_t pos{0};
    bool detected = false;

    /* 'Card type     : ' */
    string camera_card_type;
    /* one per camera dev node */
    Value camera_array(kArrayType);

    auto supported_video_formats = SupportedVideoFormats_();

    Document::AllocatorType& allocator = camera_doc_.GetAllocator();

    /* active video device nodes */
    string device_list = exec("ls -1 /dev/video*");
    stringstream device_list_ss(device_list);

    /* extract supported rate and resolutions data from camera devices */
    string device_node;
    while (device_list_ss >> device_node) {

        Value camera_device_value;
        camera_device_value.SetObject();

        string list_formats = "v4l2-ctl --device=" + device_node + " -D --list-formats-ext";
        string device_formats = exec(list_formats.c_str());
        stringstream device_formats_ss(device_formats);

        /* /dev/video<x> */
        Value device_node_val;
        device_node_val.SetString(device_node.c_str(), device_node.length(), allocator);

        Value camera_card_type_val;

        string line;
        string delimiter;

        /* 'Card type     : ' */
        bool parsed = false;
        delimiter = "Card type     : ";
        while (!device_formats_ss.eof()) {
            getline(device_formats_ss, line);
            if (string::npos != (pos = line.find(delimiter))) {
                pos += delimiter.length();
                camera_card_type = line.substr(pos);
                parsed = true;
                camera_card_type_val.SetString(camera_card_type.c_str(), camera_card_type.length(), allocator);
                break;
            }
        }
        if (parsed == false) {
            cout<<"'Card type     :' parse failure"<<endl;
            return false;
        }

        /* 'ioctl: VIDIOC_ENUM_FMT' */
        Value capture_type_array(kArrayType);
        parsed = false;
        delimiter = "ioctl: VIDIOC_ENUM_FMT";
        while (!device_formats_ss.eof()) {
            getline(device_formats_ss, line);
            if (string::npos != (pos = line.find(delimiter))) {
                parsed = true;
                break;
            }
        }
        if (parsed == false) {
            cout<<"'ioctl: VIDIOC_ENUM_FMT' parse failure"<<endl;
            return false;
        }

        int index;
        string type, format, name;

        /* parse camera device node information */
        while (!device_formats_ss.eof()) {

            getline(device_formats_ss, line);

            /* end of device node information */
            if (device_formats_ss.eof()) break;

            /* 'Index       : ' */
            parsed = false;
            delimiter = "Index       : ";
            if (string::npos != (pos = line.find(delimiter))) {
                index = stoi(line.substr(pos + delimiter.length()));
                parsed = true;
            }
            if (parsed == false) {
                cout<<"*****Index       : parse failure"<<endl;
// cout<<"*****line.length(): "<<line.length()<<endl;
// cout<<"*****line: "<<line<<endl;
                return false;
            }

            /* 'Type        : ' */
            parsed = false;
            delimiter = "Type        : ";
            getline(device_formats_ss, line);
            if (string::npos != (pos = line.find(delimiter))) {
                pos += delimiter.length();
                type = line.substr(pos);
                parsed = true;
            }
            if (parsed == false) {
                cout<<"Type        : parse failure"<<endl;
                return false;
            }

            /* 'Pixel Format: ' */
            parsed = false;
            delimiter = "Pixel Format: ";
            while (!device_formats_ss.eof()) {
                getline(device_formats_ss, line);
                if (string::npos != (pos = line.find(delimiter))) {
                    format = line.substr(pos + delimiter.length());
                    parsed = true;
                    break;
                }
            }
            if (parsed == false) {
                cout<<"Pixel Format: parse failure"<<endl;
                return false;
            }

            /* 'Name        :' */
            parsed = false;
            delimiter = "Name        : ";
            while (!device_formats_ss.eof()) {
                getline(device_formats_ss, line);
                if (string::npos != (pos = line.find(delimiter))) {
                    name = line.substr(pos + delimiter.length());
                    parsed = true;
                    break;
                }
            }
            if (parsed == false) {
                cout<<"Name: parse failure"<<endl;
                return false;
            }

            /* capture type and supported format */
            if (!type.compare("Video Capture")) {

                Value capture_format_value;
                capture_format_value.SetObject();

                Value index_val;
                index_val.SetInt(index);
                capture_format_value.AddMember("index", index_val, allocator);

                Value type_val;
                type_val.SetString(type.c_str(), type.length(), allocator);
                capture_format_value.AddMember("type", type_val, allocator);

                Value format_val;
                format_val.SetString(format.c_str(), format.length(), allocator);
                capture_format_value.AddMember("format", format_val, allocator);

                Value name_val;
                name_val.SetString(name.c_str(), name.length(), allocator);
                capture_format_value.AddMember("name", name_val, allocator);

                /* each resolution supports multiple rates */
                Value res_rate_array(kArrayType);

                /* first resolution line */
                getline(device_formats_ss, line);

                while (line.length() != 0) {

                    Value resolution_val;
                    resolution_val.SetObject();

                    /* rates for a specific resolution */
                    Value rate_array(kArrayType);

                    /* 'Size: Discrete ' */
                    parsed = false;
                    delimiter = "Size: Discrete ";
                    if (string::npos != (pos = line.find(delimiter))) {

                        string resolution = line.substr(pos + delimiter.length());
                        resolution_val.SetString(resolution.c_str(), resolution.length(), allocator);
                        parsed = true;

                        Value res_rate_val;
                        res_rate_val.SetObject();

                        Value rate_value;
                        rate_value.SetObject();
                        bool rate_found;
                        do {
                            rate_found = false;

                            getline(device_formats_ss, line);

                            delimiter = "(";
                            if (string::npos != (pos = line.find(delimiter))) {
                                string rate = line.substr(pos+1, string::npos);
                                delimiter = ")";
                                if (string::npos != (pos = rate.find(delimiter))) {
                                    Value rate_value;
                                    rate_value.SetObject();
                                    rate_value.SetString(rate.c_str(), rate.length()-1, allocator);
                                    rate_array.PushBack(rate_value, allocator);
                                    rate_found = true;
                                } else {
                                    rate_found = false;
                                }
                            }

                        } while (rate_found == true);

                        if (rate_array.Size()) {
// cout<<"*****rates added"<<endl;
                            res_rate_val.AddMember("resolution", resolution_val, allocator);
                            res_rate_val.AddMember("rates", rate_array, allocator);
                            res_rate_array.PushBack(res_rate_val, allocator);
                        }

                    } else {
                        cout<<"Size: Discrete : parse failure"<<endl;
                        return false; // TODO resources returned ???
                    }

                    /* continue loop with next read line */

                } /* end of while res_rate_ss.len() != 0 */

                // TODO handle supported modes earlier
                if (res_rate_array.Size() && (supported_video_formats.find(name) != supported_video_formats.end())) {
                    capture_format_value.AddMember("res rates", res_rate_array, allocator);
                    capture_type_array.PushBack(capture_format_value, allocator);
                }

                /* may be empty line for next Index */

            } // if (!type.compare("Video Capture"))
            if (parsed == false) {
                cout<<"'Index       : ' parse failure"<<endl;
                return false;
            }

            /* next camera device node */

        } /* while (!device_formats_ss.eof()) */

        if (capture_type_array.Size()) {
            camera_device_value.AddMember("device node", device_node_val, allocator);
            camera_device_value.AddMember("card type", camera_card_type_val, allocator);
            camera_device_value.AddMember("formats", capture_type_array, allocator);
            camera_array.PushBack(camera_device_value, allocator);
        }

    } /* while (device_list_ss >> device_node) */

    if (camera_array.Size()) {
        camera_doc_.AddMember("cameras", camera_array, allocator);
    }

    if (camera_doc_.MemberCount()) {
        detected = true;
    }

    // {
        // online schema generator
        // https://www.liquid-technologies.com/online-json-to-schema-converter

        // schema online validator
        // https://www.jsonschemavalidator.net/


        /* test schema */

        // /* Compile a Document to SchemaDocument */
        // SchemaDocument schema(camera_doc_);

        // Document d;

        // /* construct a SchemaValidator */
        // SchemaValidator validator(schema);
        // // if (!d.Accept(validator)) {
        // if (!camera_doc_.Accept(validator)) {
        //     // Input JSON is invalid according to the schema
        //     // Output diagnostic information
        //     StringBuffer sb;
        //     validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
        //     printf("Invalid schema: %s\n", sb.GetString());
        //     printf("Invalid keyword: %s\n", validator.GetInvalidSchemaKeyword());
        //     sb.Clear();
        //     validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
        //     printf("Invalid document: %s\n", sb.GetString());
        // } else {
        //     cout<<"schema passes *************"<<endl;

        //     StringBuffer sb;
        //     // sb.Clear();
        //     // PrettyWriter<StringBuffer> writer(sb);
        //     Writer<StringBuffer> writer(sb);
        //     camera_doc_.Accept(writer);
        //     puts(sb.GetString());
        // }
    // }

    // // valid schema
    // {
    //     rapidjson::Document document;
    //     document.Parse(g_schema);

    //     rapidjson::SchemaDocument schemaDocument(document);
    //     rapidjson::SchemaValidator validator(schemaDocument);

    //     /* parse JSON string */
    //     rapidjson::Document modelDoc;
    //     modelDoc.Parse(sb.GetString());

    //     if (!modelDoc.Accept(validator)) {
    //         cout<<"schema invalidated"<<endl;
    //     } else {
    //         cout<<"schema validated"<<endl;
    //     }
    // }

    // // invalid schema
    // {
    //     rapidjson::Document document;
    //     document.Parse(g_schema_invalid);

    //     rapidjson::SchemaDocument schemaDocument(document);
    //     rapidjson::SchemaValidator validator(schemaDocument);

    //     rapidjson::Document modelDoc;
    //     modelDoc.Parse(sb.GetString());

    //     if (!modelDoc.Accept(validator)) {
    //         cout<<"schema invalidated"<<endl;
    //     } else {
    //         cout<<"schema validated"<<endl;
    //     }
    // }

    // {
    //     /* Compile a Document to SchemaDocument */
    //     SchemaDocument schema(document);

    //     rapidjson::Document sd;

    //     schema.GetRoot
    //     sd.Parse(schemaJson.c_str());
    // }

    return detected;
}


string CameraCapabilities::GetSupportedVideoModes()
{
    StringBuffer sb;
    PrettyWriter<StringBuffer> writer(sb);
    camera_doc_.Accept(writer);

    return string(sb.GetString(), sb.GetSize());
}

//     rapidjson::Document camera_modes_doc_;
//     static bool camera_types_validated_{false};
//     static size_t camera_dev_nodes_size_{0};
//     static std::string camera_dev_node_{"none"};
//     static size_t camera_dev_node_index_{0};

//     static size_t camera_formats_size_{0};
//     static std::string camera_format_{"none"};
//     static size_t camera_format_index_{0};

//     static size_t camera_format_resolutions_size_{0};
//     static std::string camera_format_resolution_{"none"};
//     static size_t camera_format_resolution_index_{0};

//     static size_t camera_format_resolution_rates_size_{0};
//     static std::string camera_format_resolution_rate_{"none"};
//     static size_t camera_format_resolution_rate_index_{0};

//     static std::string camera_rate_{"none"};
// };
