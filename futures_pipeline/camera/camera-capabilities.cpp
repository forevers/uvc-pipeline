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
    detected_{false},
    synclog_(SyncLog::GetLog())
{
    camera_doc_.SetObject();

    // TODO linux udev rule, android equivalent to detect usb vbus and register/deregister cameras
    /* v4l2-ctl camera mode detection */
    // if (true == (detected_ = DetectCameras())) {
    if (true == (detected_ = DetectCameras_v2())) {

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
    synclog_->LogV(FFL, "entry");

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
cout<<"device_formats: "<<device_formats<<endl;
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
cout<<"*****index: "<<index<<endl;
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

    synclog_->LogV(FFL, "exit");

    return detected;
}


bool CameraCapabilities::DetectCameras_v2()
{
    // TODO if v4l2 shell ouput is not consistent ... perform direct ioctl queries instead
    synclog_->LogV(FFL, "entry");

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
// TODO only v4l2 dev nodes please, consider 'v4l2-ctl --list-devices'
cout<<device_list<<endl;
    stringstream device_list_ss(device_list);

    /* extract supported rate and resolutions data from camera devices */
    string device_node;
    while (device_list_ss >> device_node) {

cout<<"device node: "<<device_node<<endl;
        Value camera_device_value;
        camera_device_value.SetObject();

        // TODO another uvc util "uvcdynctrl --device=/dev/video1 -f"

        string list_formats = "v4l2-ctl --device=" + device_node + " -D --list-formats-ext";
        string device_formats = exec(list_formats.c_str());
        stringstream device_formats_ss(device_formats);

cout<<"device_formats: "<<device_formats<<endl;

        /* /dev/video<x> */
        Value device_node_val;
        device_node_val.SetString(device_node.c_str(), device_node.length(), allocator);

        Value camera_card_type_val;

        string line;
        string delimiter;

        /* 'Card type     : ' */
        bool parsed_card_type{false};
        // bool parse_completed{false};
        delimiter = "Card type        : ";
        while (!device_formats_ss.eof()) {
            getline(device_formats_ss, line);
            if (string::npos != (pos = line.find(delimiter))) {
                pos += delimiter.length();
                camera_card_type = line.substr(pos);
cout<<"*****camera_card_type: "<<camera_card_type<<endl;
                parsed_card_type = true;
                camera_card_type_val.SetString(camera_card_type.c_str(), camera_card_type.length(), allocator);
                break;
            }
        }
        if (parsed_card_type == false) {
            cout<<"'Card type     :' parse failure"<<endl;
            return false;
        }

        /* 'ioctl: VIDIOC_ENUM_FMT' */
        Value capture_type_array(kArrayType);
        bool parsed_ioctl{false};
        delimiter = "ioctl: VIDIOC_ENUM_FMT";
        while (!device_formats_ss.eof()) {
            getline(device_formats_ss, line);
            if (string::npos != (pos = line.find(delimiter))) {
                parsed_ioctl = true;
                break;
            }
        }
        if (parsed_ioctl == false) {
            cout<<"'ioctl: VIDIOC_ENUM_FMT' parse failure"<<endl;
            return false;
        }

        int index;
        string type, format, name;

        /* parse camera device node information */
        while (!device_formats_ss.eof()) {

            // getline(device_formats_ss, line);

            /* end of device node information */
            if (device_formats_ss.eof()) break;

            /* 'Type        : ' */
            delimiter = "Type: ";
            getline(device_formats_ss, line);
            if (string::npos != (pos = line.find(delimiter))) {

                pos += delimiter.length();
                type = line.substr(pos);
cout<<"*****type: "<<type<<endl;

                /* capture type */
                bool parsed_video_capture{false};
                if (!type.compare("Video Capture")) {

                    parsed_video_capture = true;

                    // Value capture_format_value;
                    // capture_format_value.SetObject();

                    // /* each resolution supports multiple rates */
                    // Value res_rate_array(kArrayType);

                    /* eat a line */
                    getline(device_formats_ss, line);

                    /* index iter */
                    getline(device_formats_ss, line);
                    bool parsed_index{false};
                    while (!device_formats_ss.eof()) {

cout<<"*****index iter"<<endl;
cout<<"index line: "<<line<<endl;

                        /* Index '[' */
                        delimiter = "[";
                        if (string::npos != (pos = line.find(delimiter))) {

// cout<<"*****found [ at: "<<pos<<endl;

                            auto delimiter_2 = "]";
                            size_t pos_2;
                            if (string::npos != (pos_2 = line.find(delimiter_2))) {

                                /* each resolution supports multiple rates */
                                Value res_rate_array(kArrayType);

                                Value capture_format_value;
                                capture_format_value.SetObject();

// cout<<"*****found ] at: "<<pos_2<<endl;
                                index = stoi(line.substr(pos + 1, pos_2 - pos - 1));
                                parsed_index = true;

                                /* format starts with ' */
                                format = line.substr(pos_2 + 3, 6);

                                /* name starts with ( */
                                // TODO actively parse this (index > 1 digit) */
                                name = line.substr(14, line.length() - 14 - 1);

cout<<"*****index: "<<index<<endl;
                                /* add format index */
                                Value index_val;
                                index_val.SetInt(index);
                                capture_format_value.AddMember("index", index_val, allocator);

cout<<"***** type: "<<type<<endl;
                                /* add 'Video Capture' type */
                                Value type_val;
                                type_val.SetString(type.c_str(), type.length(), allocator);
                                capture_format_value.AddMember("type", type_val, allocator);

cout<<"***** format: "<<format<<endl;
                                /* add format */
                                Value format_val;
                                format_val.SetString(format.c_str(), format.length(), allocator);
                                capture_format_value.AddMember("format", format_val, allocator);

cout<<"***** name: "<<name<<endl;
                                /* add index name */
                                Value name_val;
                                name_val.SetString(name.c_str(), name.length(), allocator);
                                capture_format_value.AddMember("name", name_val, allocator);

                                // /* rate for a specific resolution */
                                // Value resolution_val;
                                // resolution_val.SetObject();

                                if (getline(device_formats_ss, line)) {

                                    /* resolution iter */
                                    bool parsed_resolution;
                                    do {
cout<<"size line: "<<line<<endl;
                                        parsed_resolution = false;
                                        delimiter = "Size: Discrete ";
                                        if (string::npos != (pos = line.find(delimiter))) {

                                            /* rate for a specific resolution */
                                            Value resolution_val;
                                            resolution_val.SetObject();

                                            string resolution = line.substr(pos + delimiter.length());
                                            resolution_val.SetString(resolution.c_str(), resolution.length(), allocator);
                                            parsed_resolution = true;
cout<<"*****resolution: "<<resolution<<endl;
                                            Value res_rate_val;
                                            res_rate_val.SetObject();

                                            /* rates for a specific resolution */
                                            Value rate_array(kArrayType);

                                            Value rate_value;
                                            rate_value.SetObject();

                                            bool parsed_rate;
                                            do {
                                                parsed_rate = false;

                                                if (getline(device_formats_ss, line)) {
cout<<"rate line: "<<line<<endl;
                                                    /* differentiate ( in index line with ( in rate line */
                                                    delimiter = "(";
                                                    if ((string::npos != line.find(" fps)")) && (string::npos != (pos = line.find(delimiter)))) {
// cout<<"found ("<<endl;
                                                        string rate = line.substr(pos+1, string::npos);
                                                        delimiter = ")";
                                                        if (string::npos != (pos = rate.find(delimiter))) {
// cout<<"found )"<<endl;
                                                            Value rate_value;
                                                            rate_value.SetObject();
                                                            rate_value.SetString(rate.c_str(), rate.length()-1, allocator);
cout<<"rate: "<<rate_value.GetString()<<endl;
                                                            rate_array.PushBack(rate_value, allocator);
                                                            parsed_rate = true;
                                                        } else {
                                                            parsed_rate = false;
                                                        }
                                                    }
                                                }

                                            } while (parsed_rate == true);
cout<<"***** end of rates"<<endl;
                                            if (rate_array.Size()) {
cout<<"***** add resolution"<<endl;
                                                res_rate_val.AddMember("resolution", resolution_val, allocator);
cout<<"***** add rate_array"<<endl;
                                                res_rate_val.AddMember("rates", rate_array, allocator);
cout<<"***** push res_rate_val"<<endl;
                                                res_rate_array.PushBack(res_rate_val, allocator);
cout<<"***** pushed res_rate_val"<<endl;
                                            }

                                        }

                                    } while (parsed_resolution == true); /* do resolution iter */
                                }
cout<<"*****"<<endl;
cout<<"*****end of sizes"<<endl;
cout<<"res_rate_array.Size(): "<<res_rate_array.Size()<<endl;
cout<<"name: "<<name<<endl;
cout<<"*****"<<endl;
                                /* push capture_type_array if 4cc supported */
                                if (res_rate_array.Size() && (supported_video_formats.find(name) != supported_video_formats.end())) {
cout<<"***** pushed to capture_type_array"<<endl;
                                    capture_format_value.AddMember("res rates", res_rate_array, allocator);
                                    capture_type_array.PushBack(capture_format_value, allocator);
                                }

                            } /* found size */
else {
cout<<"*****missed ]"<<endl;
}

                        } else {
                            getline(device_formats_ss, line);
cout<<"*****search for Size"<<endl;
                        }

cout<<"*****get next line"<<endl;
                    };

                    if (parsed_index == false) {
                        cout<<"*****Index       : parse failure"<<endl;
                    }

                } /* if capture type */
            }


            // /* 'Type        : ' */
            // parsed = false;
            // delimiter = "Type        : ";
            // getline(device_formats_ss, line);
            // if (string::npos != (pos = line.find(delimiter))) {
            //     pos += delimiter.length();
            //     type = line.substr(pos);
            //     parsed = true;
            // }
            // if (parsed == false) {
            //     cout<<"Type        : parse failure"<<endl;
            //     return false;
            // }

            // /* 'Pixel Format: ' */
            // parsed = false;
            // delimiter = "Pixel Format: ";
            // while (!device_formats_ss.eof()) {
            //     getline(device_formats_ss, line);
            //     if (string::npos != (pos = line.find(delimiter))) {
            //         format = line.substr(pos + delimiter.length());
            //         parsed = true;
            //         break;
            //     }
            // }
            // if (parsed == false) {
            //     cout<<"Pixel Format: parse failure"<<endl;
            //     return false;
            // }

            // /* 'Name        :' */
            // parsed = false;
            // delimiter = "Name        : ";
            // while (!device_formats_ss.eof()) {
            //     getline(device_formats_ss, line);
            //     if (string::npos != (pos = line.find(delimiter))) {
            //         name = line.substr(pos + delimiter.length());
            //         parsed = true;
            //         break;
            //     }
            // }
            // if (parsed == false) {
            //     cout<<"Name: parse failure"<<endl;
            //     return false;
            // }

//             /* capture type and supported format */
//             if (!type.compare("Video Capture")) {

//                 Value capture_format_value;
//                 capture_format_value.SetObject();

//                 Value index_val;
//                 index_val.SetInt(index);
//                 capture_format_value.AddMember("index", index_val, allocator);

//                 Value type_val;
//                 type_val.SetString(type.c_str(), type.length(), allocator);
//                 capture_format_value.AddMember("type", type_val, allocator);

//                 Value format_val;
//                 format_val.SetString(format.c_str(), format.length(), allocator);
//                 capture_format_value.AddMember("format", format_val, allocator);

//                 Value name_val;
//                 name_val.SetString(name.c_str(), name.length(), allocator);
//                 capture_format_value.AddMember("name", name_val, allocator);

//                 /* each resolution supports multiple rates */
//                 Value res_rate_array(kArrayType);

//                 /* first resolution line */
//                 getline(device_formats_ss, line);

//                 while (line.length() != 0) {

//                     Value resolution_val;
//                     resolution_val.SetObject();

//                     /* rates for a specific resolution */
//                     Value rate_array(kArrayType);

//                     /* 'Size: Discrete ' */
//                     parsed = false;
//                     delimiter = "Size: Discrete ";
//                     if (string::npos != (pos = line.find(delimiter))) {

//                         string resolution = line.substr(pos + delimiter.length());
//                         resolution_val.SetString(resolution.c_str(), resolution.length(), allocator);
//                         parsed = true;

//                         Value res_rate_val;
//                         res_rate_val.SetObject();

//                         Value rate_value;
//                         rate_value.SetObject();
//                         bool rate_found;
//                         do {
//                             rate_found = false;

//                             getline(device_formats_ss, line);

//                             delimiter = "(";
//                             if (string::npos != (pos = line.find(delimiter))) {
//                                 string rate = line.substr(pos+1, string::npos);
//                                 delimiter = ")";
//                                 if (string::npos != (pos = rate.find(delimiter))) {
//                                     Value rate_value;
//                                     rate_value.SetObject();
//                                     rate_value.SetString(rate.c_str(), rate.length()-1, allocator);
//                                     rate_array.PushBack(rate_value, allocator);
//                                     rate_found = true;
//                                 } else {
//                                     rate_found = false;
//                                 }
//                             }

//                         } while (rate_found == true);

//                         if (rate_array.Size()) {
// // cout<<"*****rates added"<<endl;
//                             res_rate_val.AddMember("resolution", resolution_val, allocator);
//                             res_rate_val.AddMember("rates", rate_array, allocator);
//                             res_rate_array.PushBack(res_rate_val, allocator);
//                         }

//                     } else {
//                         cout<<"Size: Discrete : parse failure"<<endl;
//                         return false; // TODO resources returned ???
//                     }

//                     /* continue loop with next read line */

//                 } /* end of while res_rate_ss.len() != 0 */

//                 // TODO handle supported modes earlier
//                 if (res_rate_array.Size() && (supported_video_formats.find(name) != supported_video_formats.end())) {
//                     capture_format_value.AddMember("res rates", res_rate_array, allocator);
//                     capture_type_array.PushBack(capture_format_value, allocator);
//                 }

//                 /* may be empty line for next Index */

//             } // if (!type.compare("Video Capture"))
//             if (parsed == false) {
//                 cout<<"'Index       : ' parse failure"<<endl;
//                 return false;
//             }

            /* next camera device node */

        } /* while (!device_formats_ss.eof()) */

        /* push to camera_array */
        if (capture_type_array.Size()) {
cout<<"*****"<<endl;
cout<<"*****capture_type_array.Size(): "<<capture_type_array.Size()<<endl;
cout<<"*****"<<endl;
            camera_device_value.AddMember("device node", device_node_val, allocator);
            camera_device_value.AddMember("card type", camera_card_type_val, allocator);
            camera_device_value.AddMember("formats", capture_type_array, allocator);
            camera_array.PushBack(camera_device_value, allocator);
        }

cout<<"next device node"<<endl;
    } /* while (device_list_ss >> device_node) */

    if (camera_array.Size()) {
        camera_doc_.AddMember("cameras", camera_array, allocator);
    }

    if (camera_doc_.MemberCount()) {
        detected = true;
    }

    synclog_->LogV(FFL, "exit");

cout<<"detected: "<<detected<<endl;
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
