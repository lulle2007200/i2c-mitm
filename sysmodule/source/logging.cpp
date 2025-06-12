#include "logging.hpp"

namespace ams::log {

    namespace {

        constexpr const char LogFilePath[] = "sdmc:/atmosphere/logs/i2c-mitm.log";
        fs::FileHandle LogFile;
        s64 LogOffset;

        constinit os::SdkMutex g_log_lock;
        
    }

    Result Initialize() {
        // Check if log file exists and create it if not
        bool has_file;
        R_TRY(fs::HasFile(&has_file, LogFilePath));
        if (!has_file) {
            R_TRY(fs::CreateFile(LogFilePath, 0));
        }

        // Get file write offset
        R_TRY(fs::OpenFile(&LogFile, LogFilePath, fs::OpenMode_Write | fs::OpenMode_AllowAppend));
        R_TRY(GetFileSize(&LogOffset, LogFile));
        
        char buff[0x100];
        int len = util::TSNPrintf(buff, sizeof(buff), "\n======================== LOG STARTED ========================\n");
        R_ABORT_UNLESS(fs::WriteFile(LogFile, LogOffset, buff, len, fs::WriteOption::Flush));
        LogOffset += len;

        fs::CloseFile(LogFile);

        R_SUCCEED();
    }

    void Finalize() {
        fs::FlushFile(LogFile);
        fs::CloseFile(LogFile);
    }

    void DebugLogImpl(const char *fmt, std::va_list args) {
        R_ABORT_UNLESS(fs::OpenFile(&LogFile, LogFilePath, fs::OpenMode_Write | fs::OpenMode_AllowAppend));

        char buff[0x400];
        int len;

        auto thread = os::GetCurrentThread();
        auto ts = os::GetSystemTick().ToTimeSpan();
        len = util::TSNPrintf(buff, sizeof(buff), "[ts: %6lums t: %-13s p: %d/%d] ",
            ts.GetMilliSeconds(),
            os::GetThreadNamePointer(thread),
            os::GetThreadPriority(thread) + 28,
            os::GetThreadCurrentPriority(thread)+ 28
        );
        R_ABORT_UNLESS(fs::WriteFile(LogFile, LogOffset, buff, len, fs::WriteOption::None));
        LogOffset += len;

        //len = std::vsnprintf(buff, sizeof(buff), fmt, args);
        len = util::TVSNPrintf(buff, sizeof(buff), fmt, args);
        R_ABORT_UNLESS(fs::WriteFile(LogFile, LogOffset, buff, len, fs::WriteOption::Flush));
        LogOffset += len;

        fs::CloseFile(LogFile);
    }

    void DebugLog(const char *fmt, ...) {
        std::scoped_lock lk(g_log_lock);

        std::va_list args;
        va_start(args, fmt);
        DebugLogImpl(fmt, args);
        va_end(args);
    }

    void DebugDataDumpImpl(const void *data, size_t size) {
        size_t buff_size = 4*size + 5;
        std::unique_ptr<char[]> buff(new char[buff_size]);

        R_ABORT_UNLESS(fs::OpenFile(&LogFile, LogFilePath, fs::OpenMode_Write | fs::OpenMode_AllowAppend));

        int len = 0;
        for (size_t i = 0; i < size; ++i) {
            if ((i % 16) == 0) {
                len += util::TSNPrintf(buff.get() + len, buff_size - len, " ");
            }

            len += util::TSNPrintf(buff.get() + len, buff_size - len, "%02x%c",
                reinterpret_cast<const u8 *>(data)[i],
                (i+1) % 16 ? ' ' : '\n'
            );
        }
        len += util::TSNPrintf(buff.get() + len, buff_size - len, "\n");

        R_ABORT_UNLESS(fs::WriteFile(LogFile, LogOffset, buff.get(), len, fs::WriteOption::Flush));
        LogOffset += len;

        fs::CloseFile(LogFile);
    }

    void DebugDataDump(const void *data, size_t size, const char *fmt, ...) {
        std::scoped_lock lk(g_log_lock);

        std::va_list args;
        va_start(args, fmt);
        DebugLogImpl(fmt, args);
        va_end(args);
        DebugDataDumpImpl(data, size);
    }

}
