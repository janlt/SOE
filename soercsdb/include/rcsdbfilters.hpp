/*
 * rcsdbfilters.hpp
 *
 *  Created on: Dec 20, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_RCSDB_SRC_RCSDBFILTERS_HPP_
#define SOE_SOE_SOE_RCSDB_SRC_RCSDBFILTERS_HPP_

namespace RcsdbFacade {


class UpdateMerge: public rocksdb::MergeOperator
{
public:
    virtual bool FullMergeV2(const MergeOperationInput& merge_in,
            MergeOperationOutput* merge_out) const override
    {
        merge_out->new_value.clear();
        if (merge_in.existing_value != nullptr) {
            merge_out->new_value.assign(merge_in.existing_value->data(),
                    merge_in.existing_value->size());
        }
        for (const rocksdb::Slice& m : merge_in.operand_list) {
            //fprintf(stderr, "Merge(%s)\n", m.ToString().c_str());
            // the compaction filter filters out bad values
            //assert(m.ToString() != "bad");
            merge_out->new_value.assign(m.data(), m.size());
        }
        return true;
    }

    const char* Name() const override
    {
        return "UpdateMerge";
    }
};

class UpdateFilter: public rocksdb::CompactionFilter
{
public:
    bool Filter(int level, const rocksdb::Slice& key,
            const rocksdb::Slice& existing_value, std::string* new_value,
            bool* value_changed) const override
    {
        fprintf(stderr, "Filter(%s)\n", key.ToString().c_str());
        ++count_;
        assert(*value_changed == false);
        return false;
    }

    bool FilterMergeOperand(int level, const rocksdb::Slice& key,
            const rocksdb::Slice& existing_value) const override
    {
        fprintf(stderr, "FilterMerge(%s)\n", key.ToString().c_str());
        ++merge_count_;
        return true;
    }

    const char* Name() const override
    {
        return "MyFilter";
    }

    mutable int count_ = 0;
    mutable int merge_count_ = 0;
};

}


#endif /* SOE_SOE_SOE_RCSDB_SRC_RCSDBFILTERS_HPP_ */
