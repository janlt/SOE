#ifndef SOE_KVS_SRC_EQUAL_COMP_H_
#define SOE_KVS_SRC_EQUAL_COMP_H_

#include <string>

namespace LzKVStore {

class Dissection;

// A Equalcomp object provides a total order across slices that are
// used as keys in an sstable or a database.  A Equalcomp implementation
// must be thread-safe since LzKVStore may invoke its methods concurrently
// from multiple threads.
class Equalcomp
{
public:
    virtual ~Equalcomp();

    // Three-way comparison.  Returns value:
    //   < 0 iff "a" < "b",
    //   == 0 iff "a" == "b",
    //   > 0 iff "a" > "b"
    virtual int Compare(const Dissection& a, const Dissection& b) const = 0;

    // The name of the comparator.  Used to check for comparator
    // mismatches (i.e., a KVDatabase created with one comparator is
    // accessed using a different comparator.
    //
    // The client of this package should switch to a new name whenever
    // the comparator implementation changes in a way that will cause
    // the relative ordering of any two keys to change.
    //
    // Names starting with "LzKVStore." are reserved and should not be used
    // by any clients of this package.
    virtual const char* Name() const = 0;

    // Advanced functions: these are used to reduce the space requirements
    // for internal data structures like index blocks.

    // If *start < limit, changes *start to a short string in [start,limit).
    // Simple comparator implementations may return with *start unchanged,
    // i.e., an implementation of this method that does nothing is correct.
    virtual void FindShortestSeparator(std::string* start,
            const Dissection& limit) const = 0;

    // Changes *key to a short string >= *key.
    // Simple comparator implementations may return with *key unchanged,
    // i.e., an implementation of this method that does nothing is correct.
    virtual void FindShortSuccessor(std::string* key) const = 0;
};

// Return a builtin comparator that uses lexicographic byte-wise
// ordering.  The result remains the property of this module and
// must not be deleted.
extern const Equalcomp* BytewiseEqualcomp();

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_EQUAL_COMP_H_
