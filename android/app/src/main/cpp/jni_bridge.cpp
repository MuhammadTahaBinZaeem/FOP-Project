#include <jni.h>

#include "pocket_engineer/engine.hpp"

#include <cstdlib>
#include <string>

namespace {
std::string native_string(JNIEnv* environment,jstring value) {
    if(value==nullptr) return {};
    const char* utf=environment->GetStringUTFChars(value,nullptr);
    if(utf==nullptr) return {};
    std::string result(utf);
    environment->ReleaseStringUTFChars(value,utf);
    return result;
}

jstring java_string(JNIEnv* environment,const std::string& value) {
    return environment->NewStringUTF(value.c_str());
}
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_pocketengineer_app_MainActivity_00024PocketEngineerBridge_identify(JNIEnv* environment,jobject,jstring input) {
    return java_string(environment,pocket_engineer::Engine{}.identify(native_string(environment,input)).to_json());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_pocketengineer_app_MainActivity_00024PocketEngineerBridge_solve(JNIEnv* environment,jobject,jstring request) {
    const auto input=native_string(environment,request);
    const char* response=pe_solve_json(input.c_str());
    if(response==nullptr) return java_string(environment,R"({"schema_version":"1.0","status":"error","answer":{"text":"Native solver request failed"}})");
    const std::string result(response);
    pe_free_string(response);
    return java_string(environment,result);
}
