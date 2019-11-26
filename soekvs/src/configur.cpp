#include "sandbox.hpp"
#include "configur.hpp"

namespace LzKVStore {

Configuration::~Configuration()
{
}

Status Configuration::NewAppendableFile(const std::string& fname,
        ModDatabase** result)
{
    return Status::NotSupported("NewAppendableFile", fname);
}

SeqDatabase::~SeqDatabase()
{
}

RADatabase::~RADatabase()
{
}

ModDatabase::~ModDatabase()
{
}

Messanger::~Messanger()
{
}

DatabaseLock::~DatabaseLock()
{
}

void Log(Messanger* info_log, const char* format, ...)
{
    if (info_log != NULL) {
        va_list ap;
        va_start(ap, format);
        info_log->Logv(format, ap);
        va_end(ap);
    }
}

static Status DoWriteStringToFile(Configuration* env, const Dissection& data,
        const std::string& fname, bool should_sync)
{
    ModDatabase* file;
    Status s = env->NewModDatabase(fname, &file);
    if (!s.ok()) {
        return s;
    }
    s = file->Append(data);
    if (s.ok() && should_sync) {
        s = file->Sync();
    }
    if (s.ok()) {
        s = file->Close();
    }
    delete file;  // Will auto-close if we did not close above
    if (!s.ok()) {
        env->DeleteFile(fname);
    }
    return s;
}

Status WriteStringToFile(Configuration* env, const Dissection& data,
        const std::string& fname)
{
    return DoWriteStringToFile(env, data, fname, false);
}

Status WriteStringToFileSync(Configuration* env, const Dissection& data,
        const std::string& fname)
{
    return DoWriteStringToFile(env, data, fname, true);
}

Status ReadFileToString(Configuration* env, const std::string& fname,
        std::string* data)
{
    data->clear();
    SeqDatabase* file;
    Status s = env->NewSeqDatabase(fname, &file);
    if (!s.ok()) {
        return s;
    }
    static const int kBufferSize = 8192;
    char* space = new char[kBufferSize];
    while (true) {
        Dissection fragment;
        s = file->Read(kBufferSize, &fragment, space);
        if (!s.ok()) {
            break;
        }
        data->append(fragment.data(), fragment.size());
        if (fragment.empty()) {
            break;
        }
    }
    delete[] space;
    delete file;
    return s;
}

ConfigurationWrapper::~ConfigurationWrapper()
{
}

}  // namespace LzKVStore
