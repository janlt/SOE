#ifndef SOE_KVS_TEST_TESTUTIL_H_
#define SOE_KVS_TEST_TESTUTIL_H_

#include "configur.hpp"
#include "dissect.hpp"
#include "random.hpp"

namespace LzKVStore {
namespace KVSTest {

// Store in *dst a random string of length "len" and return a Dissection that
// references the generated data.
extern Dissection RandomString(Random* rnd, int len, std::string* dst);

// Return a random key with the specified length that may contain interesting
// characters (e.g. \x00, \xff, etc.).
extern std::string RandomKey(Random* rnd, int len);

// Store in *dst a string of length "len" that will compress to
// "N*compressed_fraction" bytes and return a Dissection that references
// the generated data.
extern Dissection CompressibleString(Random* rnd, double compressed_fraction,
        size_t len, std::string* dst);

// A wrapper that allows injection of errors.
class ErrorConfiguration: public ConfigurationWrapper {
public:
    bool writable_file_error_;
    int num_writable_file_errors_;

    ErrorConfiguration() :
            ConfigurationWrapper(Configuration::Default()), writable_file_error_(
                    false), num_writable_file_errors_(0) {
    }

    virtual Status NewModDatabase(const std::string& fname,
            ModDatabase** result) {
        if (writable_file_error_) {
            ++num_writable_file_errors_;
            *result = NULL;
            return Status::IOError(fname, "fake error");
        }
        return target()->NewModDatabase(fname, result);
    }

    virtual Status NewAppendableFile(const std::string& fname,
            ModDatabase** result) {
        if (writable_file_error_) {
            ++num_writable_file_errors_;
            *result = NULL;
            return Status::IOError(fname, "fake error");
        }
        return target()->NewAppendableFile(fname, result);
    }
};

}  // namespace KVSTest
}  // namespace LzKVStore

#endif // #ifndef SOE_KVS_TEST_TESTUTIL_H_
