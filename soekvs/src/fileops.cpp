#include <ctype.h>
#include <stdio.h>
#include "fileops.hpp"
#include "kvconfig.hpp"
#include "configur.hpp"
#include "logger.hpp"

namespace LzKVStore {

// A utility routine: write "data" to the named file and Sync() it.
extern Status WriteStringToFileSync(Configuration* env, const Dissection& data,
        const std::string& fname);

static std::string MakeFileName(const std::string& name, uint64_t number,
        const char* suffix)
{
    char buf[100];
    snprintf(buf, sizeof(buf), "/%06llu.%s",
            static_cast<unsigned long long>(number), suffix);
    return name + buf;
}

std::string LogFileName(const std::string& name, uint64_t number)
{
    assert(number > 0);
    return MakeFileName(name, number, "log");
}

std::string KVStoreFileName(const std::string& name, uint64_t number)
{
    assert(number > 0);
    return MakeFileName(name, number, "kvd");
}

std::string MITKVStoreFileName(const std::string& name, uint64_t number)
{
    assert(number > 0);
    return MakeFileName(name, number, "mit");
}

std::string DescriptorFileName(const std::string& dbname, uint64_t number)
{
    assert(number > 0);
    char buf[100];
    snprintf(buf, sizeof(buf), "/MANIFEST-%06llu",
            static_cast<unsigned long long>(number));
    return dbname + buf;
}

std::string CurrentFileName(const std::string& dbname)
{
    return dbname + "/CURRENT";
}

std::string LockFileName(const std::string& dbname)
{
    return dbname + "/LOCK";
}

std::string TempFileName(const std::string& dbname, uint64_t number)
{
    assert(number > 0);
    return MakeFileName(dbname, number, "dbtmp");
}

std::string InfoLogFileName(const std::string& dbname)
{
    return dbname + "/LOG";
}

// Return the name of the old info log file for "dbname".
std::string OldInfoLogFileName(const std::string& dbname)
{
    return dbname + "/LOG.old";
}

// Owned filenames have the form:
//    dbname/CURRENT
//    dbname/LOCK
//    dbname/LOG
//    dbname/LOG.old
//    dbname/MANIFEST-[0-9]+
//    dbname/[0-9]+.(log|mit|kvsd)
bool ParseFileName(const std::string& fname, uint64_t* number, FileType* type)
{
    Dissection rest(fname);
    if (rest == "CURRENT") {
        *number = 0;
        *type = kCurrentFile;
    } else if (rest == "LOCK") {
        *number = 0;
        *type = kKVDatabaseLockFile;
    } else if (rest == "LOG" || rest == "LOG.old") {
        *number = 0;
        *type = kInfoLogFile;
    } else if (rest.starts_with("MANIFEST-")) {
        rest.remove_prefix(strlen("MANIFEST-"));
        uint64_t num;
        if (!ConsumeDecimalNumber(&rest, &num)) {
            return false;
        }
        if (!rest.empty()) {
            return false;
        }
        *type = kDescriptorFile;
        *number = num;
    } else {
        // Avoid strtoull() to keep filename format independent of the
        // current locale
        uint64_t num;
        if (!ConsumeDecimalNumber(&rest, &num)) {
            return false;
        }
        Dissection suffix = rest;
        if (suffix == Dissection(".log")) {
            *type = kLogFile;
        } else if (suffix == Dissection(".mit")
                || suffix == Dissection(".kvd")) {
            *type = kKVStoreFile;
        } else if (suffix == Dissection(".dbtmp")) {
            *type = kTempFile;
        } else {
            return false;
        }
        *number = num;
    }
    return true;
}

Status SetCurrentFile(Configuration* env, const std::string& dbname,
        uint64_t descriptor_number)
{
    // Remove leading "dbname/" and add newline to manifest file name
    std::string manifest = DescriptorFileName(dbname, descriptor_number);
    Dissection contents = manifest;
    assert(contents.starts_with(dbname + "/"));
    contents.remove_prefix(dbname.size() + 1);
    std::string tmp = TempFileName(dbname, descriptor_number);
    Status s = WriteStringToFileSync(env, contents.ToString() + "\n", tmp);
    if (s.ok()) {
        s = env->RenameFile(tmp, CurrentFileName(dbname));
    }
    if (!s.ok()) {
        env->DeleteFile(tmp);
    }
    return s;
}

}  // namespace LzKVStore
