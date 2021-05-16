#include "camera-capabilities.h"

#include <iostream>
#include <sstream>  
#include <stdio.h>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "exec-shell.h"


using namespace rapidjson;
using namespace std;


CameraCapabilities::CameraCapabilities() :
    detected_{false}
{
    camera_doc_.SetObject();

    // TODO linux udev rule, android equivalent to detect usb vbus and register/deregister cameras
    /* v4l2-ctl camera mode detection */
    if (true == (detected_ = DetectCameras())) {
        cout<<"cameras detected"<<endl;
    } else {
        cout<<"no cameras detected"<<endl;
    }
}


CameraCapabilities::~CameraCapabilities()
{
    
}


bool CameraCapabilities::DetectCameras() 
{
    size_t pos = 0;
    size_t pos_end = 0;
    bool detected = false;

    string camera_card_type;

    Value camera_array(kArrayType);
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

        /* /dev/video<x> */
        Value device_node_val;
        device_node_val.SetString(device_node.c_str(), device_node.length(), allocator);

        Value camera_card_type_val;
        pos = 0;
        string delimiter = "Card type     : ";
        if (string::npos != (pos = device_formats.find(delimiter, pos))) {

            pos += delimiter.length();

            if (string::npos != (pos_end = device_formats.find("\n", pos))) {

                size_t len = pos_end - pos;
                camera_card_type = device_formats.substr(pos, len);
                pos = pos_end;
                camera_card_type_val.SetString(camera_card_type.c_str(), camera_card_type.length(), allocator);
            }
        }

        Value capture_type_array(kArrayType);
        delimiter = "ioctl: VIDIOC_ENUM_FMT";
        if (string::npos != (pos = device_formats.find(delimiter, pos))) {
            
            pos += delimiter.length();

            int index; 
            string type, format, name;

            /* ioctl: VIDIOC_ENUM_FMT Index's */
            delimiter = "Index       : ";
            while (string::npos != (pos = device_formats.find(delimiter, pos))) {

                pos += delimiter.length();
                if (string::npos != (pos_end = device_formats.find("\n", pos))) {
                    size_t len = pos_end - pos;
                    index = stoi(device_formats.substr(pos, len));
                    pos = pos_end;
                }

                delimiter = "Type        : ";
                if (string::npos != (pos = device_formats.find(delimiter, pos))) {
                    pos += delimiter.length();
                    if (string::npos != (pos_end = device_formats.find("\n", pos))) {
                        size_t len = pos_end - pos;
                        type = device_formats.substr(pos, len);
                        pos = pos_end;
                    }
                }

                delimiter = "Pixel Format: ";
                if (string::npos != (pos = device_formats.find(delimiter, pos))) {
                    pos += delimiter.length();
                    if (string::npos != (pos_end = device_formats.find('\n', pos))) {
                        size_t len = pos_end - pos;
                        format = device_formats.substr(pos, len);
                        pos = pos_end;
                    }
                }

                delimiter = "Name        :";
                if (string::npos != (pos = device_formats.find(delimiter, pos))) {
                    pos += delimiter.length();
                    if (string::npos != (pos_end = device_formats.find('\n', pos))) {
                        size_t len = pos_end - pos;
                        name = device_formats.substr(pos, len);
                        pos = pos_end;
                    }
                }

                /* add capture types */
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

                    string res_rate = device_formats.substr(pos);
                    stringstream res_rate_ss(res_rate);
                    string line;
                    string resolution;

                    // TODO change above the getline

                    /* eat a blank line */
                    getline(res_rate_ss, line);
                    getline(res_rate_ss, line);

                    while (!res_rate_ss.eof()) {
                        
                        Value res_rate_val;
                        res_rate_val.SetObject();

                        Value resolution_val;
                        resolution_val.SetObject();

                        Value rate_array(kArrayType);

                        delimiter = "Size: Discrete ";
                        if (string::npos != (pos = line.find(delimiter))) {

                            resolution = line.substr(pos + delimiter.length());
                            resolution_val.SetString(resolution.c_str(), resolution.length(), allocator);

                            Value rate_value;
                            rate_value.SetObject();
                            do {

                                getline(res_rate_ss, line);

                                delimiter = "(";
                                if (string::npos != (pos = line.find(delimiter))) {

                                    string rate = line.substr(pos+1, string::npos);

                                    delimiter = ")";
                                    if (string::npos != (pos_end = rate.find(delimiter))) {

                                        Value rate_value;
                                        rate_value.SetObject();
                                        rate_value.SetString(rate.c_str(), rate.length()-1, allocator);
                                        rate_array.PushBack(rate_value, allocator);
                                    }
                                }
                            } while (string::npos != pos);
                        } else {
                            getline(res_rate_ss, line);
                        }

                        if (rate_array.Size()) {
                            res_rate_val.AddMember("resolution", resolution_val, allocator);
                            res_rate_val.AddMember("rates", rate_array, allocator);
                            res_rate_array.PushBack(res_rate_val, allocator);
                        }
                    }

                    if (res_rate_array.Size()) {
                        capture_format_value.AddMember("res rates", res_rate_array, allocator);
                        capture_type_array.PushBack(capture_format_value, allocator);
                    }
                }

                /* next format */
                delimiter = "Index       : ";

            } /* end while ioctl: VIDIOC_ENUM_FMT */

            if (capture_type_array.Size()) {
                camera_device_value.AddMember("device node", device_node_val, allocator);
                camera_device_value.AddMember("card type", camera_card_type_val, allocator);
                camera_device_value.AddMember("formats", capture_type_array, allocator);
            }

            // if (camera_device_value.ObjectEmpty()) {
            //     cout<<"empty"<<endl;
            // }

        } /* if more lines for this device node */

        /* only add dev nodes with VIDIOC_ENUM_FMT */
        if (!camera_device_value.ObjectEmpty()) {
            camera_array.PushBack(camera_device_value, allocator);
        }

    } /* while (device_list_ss >> device_node) */

    camera_doc_.AddMember("cameras", camera_array, allocator);

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
