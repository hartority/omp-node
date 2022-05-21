#pragma once
#include "sdk.hpp"
#include "v8.h"
#include "../utils.hpp"
#include "../logger.hpp"

#define ENTER_FUNCTION_CALLBACK(info) \
    auto isolate = (info).GetIsolate(); \
    v8::Locker locker(isolate); \
    v8::Isolate::Scope isolateScope(isolate); \
    v8::HandleScope scope(isolate); \
    v8::Local<v8::Context> context = isolate->GetCurrentContext(); \
    v8::Context::Scope contextScope(context);

#define WRAP_BASIC_CALL_RETURN(ExternalType, getFunction, ConversionToJSFunction, ...) \
    void getFunction(const v8::FunctionCallbackInfo<v8::Value> &info) { \
        ENTER_FUNCTION_CALLBACK(info); \
        auto external = GetContextExternalPointer<ExternalType>(info); \
        auto value = external->getFunction(__VA_ARGS__); \
        auto valueHandle = ConversionToJSFunction(value, isolate); \
        info.GetReturnValue().Set(valueHandle); \
    }

#define WRAP_HANDLE_STORAGE_GET(ExternalType, getFunction) \
    void getFunction(const v8::FunctionCallbackInfo<v8::Value> &info) { \
        ENTER_FUNCTION_CALLBACK(info) \
        auto external = GetContextExternalPointer<ExternalType>(info); \
        auto value = &external->getFunction(); \
        auto valueHandle = GetHandleStorageExtension(value)->get(); \
        info.GetReturnValue().Set(valueHandle); \
    }

#define WRAP_BASIC_CALL(ExternalType, setFunction, ...) \
    void setFunction(const v8::FunctionCallbackInfo<v8::Value> &info) { \
        ENTER_FUNCTION_CALLBACK(info); \
        auto external = GetContextExternalPointer<ExternalType>(info); \
        external->setFunction(__VA_ARGS__); \
    }

static const UID HandleStorage_UID = UID(0x9f8d31e2f071cfff);
struct IHandleStorage : public IExtension {
    PROVIDE_EXT_UID(HandleStorage_UID);

    v8::Isolate *isolate;
    v8::UniquePersistent<v8::Value> storedValue;

    IHandleStorage(v8::Isolate *isolate, v8::Local<v8::Value> value);

    v8::Local<v8::Value> get();

    void freeExtension() override;

    void reset() override;
};

IHandleStorage *GetHandleStorageExtension(IExtensible *extensible);

typedef std::vector<std::pair<std::string, v8::FunctionCallback>> ObjectMethods;
typedef std::vector<std::tuple<std::string, v8::FunctionCallback, v8::FunctionCallback>> ObjectAccessors;

template<class Interface>
v8::Local<v8::Object> InterfaceToObject(Interface *pointer,
                                        v8::Local<v8::Context> context,
                                        const ObjectMethods &methods) {
    auto isolate = context->GetIsolate();

    auto objectTemplate = v8::ObjectTemplate::New(isolate);

    objectTemplate->SetInternalFieldCount(1);

    for (auto &entry: methods) {
        objectTemplate->Set(v8::String::NewFromUtf8(isolate,
                                                    entry.first.c_str(),
                                                    v8::NewStringType::kNormal).ToLocalChecked(),
                            v8::FunctionTemplate::New(isolate, entry.second));
    }

    auto object = objectTemplate->NewInstance(context).ToLocalChecked();

    object->SetInternalField(0, v8::External::New(isolate, pointer));

    return object;
}

template<class Interface>
v8::Local<v8::Object> MutableToObject(Interface *pointer,
                                      v8::Local<v8::Context> context,
                                      const ObjectAccessors &accessors) {
    auto isolate = context->GetIsolate();

    auto objectTemplate = v8::ObjectTemplate::New(isolate);

    objectTemplate->SetInternalFieldCount(1);

    for (auto &entry: accessors) {
        objectTemplate->SetAccessorProperty(v8::String::NewFromUtf8(isolate,
                                                                    std::get<0>(entry).c_str()).ToLocalChecked(),
                                            v8::FunctionTemplate::New(isolate, std::get<1>(entry)),
                                            v8::FunctionTemplate::New(isolate, std::get<2>(entry)));
    }

    auto object = objectTemplate->NewInstance(context).ToLocalChecked();

    object->SetInternalField(0, v8::External::New(isolate, pointer));

    return object;
}

template<class Interface>
Interface *GetContextExternalPointer(const v8::FunctionCallbackInfo<v8::Value> &info) {
    v8::Handle<v8::External> pointer = v8::Handle<v8::External>::Cast(info.This()->GetInternalField(0));

    return static_cast<Interface *>( pointer->Value());
}

float JSToFloat(v8::Local<v8::Value> value, v8::Local<v8::Context> context);
int JSToInt(v8::Local<v8::Value> value, v8::Local<v8::Context> context);
Impl::String JSToString(v8::Local<v8::Value> value, v8::Local<v8::Context> context);
WorldTimePoint JSToWorldTimePoint(v8::Local<v8::Value> value);
bool JSToBool(v8::Local<v8::Value> value, v8::Local<v8::Context> context);
Vector3 JSToVector3(v8::Local<v8::Value> value, v8::Local<v8::Context> context);

BanEntry JSToBanEntry(v8::Local<v8::Value> value, v8::Local<v8::Context> context);

v8::Local<v8::Number> FloatToJS(float value, v8::Isolate *isolate);
v8::Local<v8::Integer> IntToJS(int value, v8::Isolate *isolate);
v8::Local<v8::Integer> UIntToJS(unsigned int value, v8::Isolate *isolate);
v8::Local<v8::String> StringViewToJS(StringView str, v8::Isolate *isolate);
v8::Local<v8::Date> WorldTimePointToJS(WorldTimePoint point, v8::Local<v8::Context> context);
v8::Local<v8::Boolean> BoolToJS(bool value, v8::Isolate *isolate);

v8::Local<v8::Object> PlayerKeyDataToJS(PlayerKeyData &data, v8::Local<v8::Context>);
v8::Local<v8::Object> BanEntryToJS(const BanEntry &entry, v8::Local<v8::Context>);

template<typename EnumType>
v8::Local<v8::Object> EnumToObject(std::vector<std::pair<std::string, EnumType>> values,
                                   v8::Local<v8::Context> context) {
    auto isolate = context->GetIsolate();

    auto objectTemplate = v8::ObjectTemplate::New(isolate);

    for (auto entry: values) {
        auto name = v8::String::NewFromUtf8(isolate, entry.first.c_str()).ToLocalChecked();
        auto val = IntToJS((int)entry.second, isolate);
        auto valString = val->ToString(context).ToLocalChecked();

        objectTemplate->Set(name, val);
        objectTemplate->Set(valString, name);
    }

    auto object = objectTemplate->NewInstance(context).ToLocalChecked();

    return object;
}