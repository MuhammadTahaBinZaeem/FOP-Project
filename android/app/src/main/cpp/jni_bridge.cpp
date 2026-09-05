#include <jni.h>
#include "pocket_engineer/engine.hpp"
#include <string>

// Byte arrays carry standard UTF-8 (not JNI's Modified UTF-8), so supplementary
// Unicode in input and explanations survives the Kotlin/C++ round trip.
extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_pocketengineer_app_MainActivity_nativeDispatch(
    JNIEnv* environment, jobject, jint method, jbyteArray input) {
    const auto size = input ? environment->GetArrayLength(input) : 0;
    if (size > 32768) return nullptr;
    std::string request(static_cast<std::size_t>(size), '\0');
    if (size) environment->GetByteArrayRegion(input, 0, size, reinterpret_cast<jbyte*>(request.data()));
    if (environment->ExceptionCheck()) return nullptr;
    const char* result = method == 0 ? pocket_engineer::pe_solve_json(request.c_str())
                       : method == 1 ? pocket_engineer::pe_identify_json(request.c_str())
                       : method == 2 ? pocket_engineer::pe_catalog_json() : nullptr;
    if (!result) return nullptr;
    const std::string response(result);
    pocket_engineer::pe_free_string(result);
    const auto length = static_cast<jsize>(response.size());
    auto output = environment->NewByteArray(length);
    if (output) environment->SetByteArrayRegion(output, 0, length, reinterpret_cast<const jbyte*>(response.data()));
    return output;
}
