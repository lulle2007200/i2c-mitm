#pragma once
#include <stratosphere.hpp>

namespace ams::log {

    Result Initialize();
    void Finalize();

    void DebugLog(const char *fmt, ...);
    void DebugDataDump(const void *data, size_t size, const char *fmt, ...);

    #ifdef DEBUG
    #define DEBUG_LOG(fmt, ...) ::ams::log::DebugLog(fmt "\n", ##__VA_ARGS__)
    #define DEBUG_DATA_DUMP(data, size, fmt, ...) ::ams::log::DebugDataDump(data, size, fmt "\n", ##__VA_ARGS__)
    #else
    #define DEBUG_LOG(fmt, ...)
    #define DEBUG_DATA_DUMP(data, size, fmt, ...)
    #endif

}
