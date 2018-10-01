#ifndef JSONSTATUS_H
#define JSONSTATUS_H

#include <string>

#include "rapidjson/prettywriter.h" // for stringify JSON
using namespace rapidjson;

class JsonStatus {
public:
    JsonStatus(const std::string& string_ref, int integer_val, double double_val, bool bool_val) :
            string_(string_ref), integer_(integer_val), double_(double_val), bool_(bool_val) {}
    JsonStatus(const JsonStatus& rhs) :
            string_(rhs.string_), integer_(rhs.integer_), double_(rhs.double_), bool_(rhs.bool_) {}
    virtual ~JsonStatus() {};

    JsonStatus& operator=(const JsonStatus& rhs) {
        string_ = rhs.string_;
        integer_ = rhs.integer_;
        double_ = rhs.double_;
        bool_ = rhs.bool_;
        return *this;
    }

    template <typename Writer>
    void Serialize(Writer& writer) const {
        writer.StartObject();

        writer.String("string");
#if RAPIDJSON_HAS_STDSTRING
        writer.String(name_);
#else
        writer.String(string_.c_str(), static_cast<SizeType>(string_.length())); // Supplying length of string is faster.
#endif
        writer.String("int");
        writer.Int(integer_);

        writer.String("double");
        writer.Double(double_);

        writer.String("bool");
        writer.Bool(bool_);

        writer.EndObject();
    }

private:

    std::string string_;
    int integer_;
    double double_;
    bool bool_;
};


#endif // JSONSTATUS_H
