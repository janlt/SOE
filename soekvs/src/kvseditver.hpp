#ifndef SOE_KVS_SRC_KVSEDITVER_H_
#define SOE_KVS_SRC_KVSEDITVER_H_

#include <set>
#include <utility>
#include <vector>
#include "kvconfig.hpp"

namespace LzKVStore {

class VersionSet;

struct FileMetaData {
    int refs;
    int allowed_seeks;          // Seeks allowed until compaction
    uint64_t number;
    uint64_t file_size;         // File size in bytes
    PrimaryKey smallest;       // Smallest internal key served by table
    PrimaryKey largest;        // Largest internal key served by table

    FileMetaData() :
            refs(0), allowed_seeks(1 << 30), file_size(0) {
    }
};

class EditingVersion
{
public:
    EditingVersion()
    {
        Clear();
    }
    ~EditingVersion()
    {
    }

    void Clear();

    void SetEqualcompName(const Dissection& name)
    {
        has_comparator_ = true;
        comparator_ = name.ToString();
    }
    void SetLogNumber(uint64_t num)
    {
        has_log_number_ = true;
        log_number_ = num;
    }
    void SetPrevLogNumber(uint64_t num)
    {
        has_prev_log_number_ = true;
        prev_log_number_ = num;
    }
    void SetNextFile(uint64_t num)
    {
        has_next_file_number_ = true;
        next_file_number_ = num;
    }
    void SetLastSequence(SequenceNumber seq)
    {
        has_last_sequence_ = true;
        last_sequence_ = seq;
    }
    void SetCompactPointer(int level, const PrimaryKey& key)
    {
        compact_pointers_.push_back(std::make_pair(level, key));
    }

    // Add the specified file at the specified number.
    // REQUIRES: This version has not been saved (see VersionSet::SaveTo)
    // REQUIRES: "smallest" and "largest" are smallest and largest keys in file
    void AddFile(int level, uint64_t file, uint64_t file_size,
            const PrimaryKey& smallest, const PrimaryKey& largest)
    {
        FileMetaData f;
        f.number = file;
        f.file_size = file_size;
        f.smallest = smallest;
        f.largest = largest;
        new_files_.push_back(std::make_pair(level, f));
    }

    // Delete the specified "file" from the specified "level".
    void DeleteFile(int level, uint64_t file)
    {
        deleted_files_.insert(std::make_pair(level, file));
    }

    void EncodeTo(std::string* dst) const;
    Status DecodeFrom(const Dissection& src);

    std::string DebugString() const;

private:
    friend class VersionSet;

    typedef std::set<std::pair<int, uint64_t> > DeletedFileSet;

    std::string comparator_;
    uint64_t log_number_;
    uint64_t prev_log_number_;
    uint64_t next_file_number_;
    SequenceNumber last_sequence_;
    bool has_comparator_;
    bool has_log_number_;
    bool has_prev_log_number_;
    bool has_next_file_number_;
    bool has_last_sequence_;

    std::vector<std::pair<int, PrimaryKey> > compact_pointers_;
    DeletedFileSet deleted_files_;
    std::vector<std::pair<int, FileMetaData> > new_files_;
};

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_KVSEDITVER_H_
