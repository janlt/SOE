#ifndef SOE_KVS_KVS_LIST_H_
#define SOE_KVS_KVS_LIST_H_

#include <assert.h>
#include <stdlib.h>
#include "kvsarch.hpp"
#include "sandbox.hpp"
#include "random.hpp"

namespace LzKVStore {

class Sandbox;

template<typename Key, class Equalcomp>
class MultiLinkList
{
private:
    struct Node;

public:
    // Create a new MultiLinkList object that will use "cmp" for comparing keys,
    // and will allocate memory using "*arena".  Objects allocated in the arena
    // must remain allocated for the lifetime of the skiplist object.
    explicit MultiLinkList(Equalcomp cmp, Sandbox* arena);

    // Insert key into the list.
    // REQUIRES: nothing that compares equal to key is currently in the list.
    void Insert(const Key& key);

    // Returns true iff an entry that compares equal to key is in the list.
    bool Contains(const Key& key) const;

    // Iteration over the contents of a skip list
    class Iterator {
    public:
        // Initialize an iterator over the specified list.
        // The returned iterator is not valid.
        explicit Iterator(const MultiLinkList* list);

        // Returns true iff the iterator is positioned at a valid node.
        bool Valid() const;

        // Returns the key at the current position.
        // REQUIRES: Valid()
        const Key& key() const;

        // Advances to the next position.
        // REQUIRES: Valid()
        void Next();

        // Advances to the previous position.
        // REQUIRES: Valid()
        void Prev();

        // Advance to the first entry with a key >= target
        void Seek(const Key& target);

        // Position at the first entry in list.
        // Final state of iterator is Valid() iff list is not empty.
        void SeekToFirst();

        // Position at the last entry in list.
        // Final state of iterator is Valid() iff list is not empty.
        void SeekToLast();

    private:
        const MultiLinkList* list_;
        Node* node_;
        // Intentionally copyable
    };

private:
    enum {
        kMaxHeight = 12
    };

    // Immutable after construction
    Equalcomp const compare_;
    Sandbox* const arena_;    // Sandbox used for allocations of nodes

    Node* const head_;

    // Modified only by Insert().  Read racily by readers, but stale
    // values are ok.
    KVSArch::AtomicPointer max_height_;   // Height of the entire list

    inline int GetMaxHeight() const
    {
        return static_cast<int>(reinterpret_cast<intptr_t>(max_height_.NoBarrier_Load()));
    }

    // Read/written only by Insert().
    Random rnd_;

    Node* NewNode(const Key& key, int height);
    int RandomHeight();
    bool Equal(const Key& a, const Key& b) const
    {
        return (compare_(a, b) == 0);
    }

    // Return true if key is greater than the data stored in "n"
    bool KeyIsAfterNode(const Key& key, Node* n) const;

    // Return the earliest node that comes at or after key.
    // Return NULL if there is no such node.
    //
    // If prev is non-NULL, fills prev[level] with pointer to previous
    // node at "level" for every level in [0..max_height_-1].
    Node* FindGreaterOrEqual(const Key& key, Node** prev) const;

    // Return the latest node with a key < key.
    // Return head_ if there is no such node.
    Node* FindLessThan(const Key& key) const;

    // Return the last node in the list.
    // Return head_ if list is empty.
    Node* FindLast() const;

    // No copying allowed
    MultiLinkList(const MultiLinkList&);
    void operator=(const MultiLinkList&);
};

// Implementation details follow
template<typename Key, class Equalcomp>
struct MultiLinkList<Key, Equalcomp>::Node {
    explicit Node(const Key& k) :
            key(k) {
    }

    Key const key;

    // Accessors/mutators for links.  Wrapped in methods so we can
    // add the appropriate barriers as necessary.
    Node* Next(int n)
    {
        assert(n >= 0);
        // Use an 'acquire load' so that we observe a fully initialized
        // version of the returned Node.
        return reinterpret_cast<Node*>(next_[n].Acquire_Load());
    }
    void SetNext(int n, Node* x)
    {
        assert(n >= 0);
        // Use a 'release store' so that anybody who reads through this
        // pointer observes a fully initialized version of the inserted node.
        next_[n].Release_Store(x);
    }

    // No-barrier variants that can be safely used in a few locations.
    Node* NoBarrier_Next(int n)
    {
        assert(n >= 0);
        return reinterpret_cast<Node*>(next_[n].NoBarrier_Load());
    }
    void NoBarrier_SetNext(int n, Node* x)
    {
        assert(n >= 0);
        next_[n].NoBarrier_Store(x);
    }

private:
    // Array of length equal to the node height.  next_[0] is lowest level link.
    KVSArch::AtomicPointer next_[1];
};

template<typename Key, class Equalcomp>
typename MultiLinkList<Key, Equalcomp>::Node*
MultiLinkList<Key, Equalcomp>::NewNode(const Key& key, int height)
{
    char* mem = arena_->AllocateAligned(
            sizeof(Node) + sizeof(KVSArch::AtomicPointer) * (height - 1));
    return new (mem) Node(key);
}

template<typename Key, class Equalcomp>
inline MultiLinkList<Key, Equalcomp>::Iterator::Iterator(
        const MultiLinkList* list)
{
    list_ = list;
    node_ = NULL;
}

template<typename Key, class Equalcomp>
inline bool MultiLinkList<Key, Equalcomp>::Iterator::Valid() const
{
    return node_ != NULL;
}

template<typename Key, class Equalcomp>
inline const Key& MultiLinkList<Key, Equalcomp>::Iterator::key() const
{
    assert(Valid());
    return node_->key;
}

template<typename Key, class Equalcomp>
inline void MultiLinkList<Key, Equalcomp>::Iterator::Next()
{
    assert(Valid());
    node_ = node_->Next(0);
}

template<typename Key, class Equalcomp>
inline void MultiLinkList<Key, Equalcomp>::Iterator::Prev()
{
    // Instead of using explicit "prev" links, we just search for the
    // last node that falls before key.
    assert(Valid());
    node_ = list_->FindLessThan(node_->key);
    if (node_ == list_->head_) {
        node_ = NULL;
    }
}

template<typename Key, class Equalcomp>
inline void MultiLinkList<Key, Equalcomp>::Iterator::Seek(const Key& target)
{
    node_ = list_->FindGreaterOrEqual(target, NULL);
}

template<typename Key, class Equalcomp>
inline void MultiLinkList<Key, Equalcomp>::Iterator::SeekToFirst()
{
    node_ = list_->head_->Next(0);
}

template<typename Key, class Equalcomp>
inline void MultiLinkList<Key, Equalcomp>::Iterator::SeekToLast()
{
    node_ = list_->FindLast();
    if (node_ == list_->head_) {
        node_ = NULL;
    }
}

template<typename Key, class Equalcomp>
int MultiLinkList<Key, Equalcomp>::RandomHeight()
{
    // Increase height with probability 1 in kBranching
    static const unsigned int kBranching = 4;
    int height = 1;
    while (height < kMaxHeight && ((rnd_.Next() % kBranching) == 0)) {
        height++;
    }
    assert(height > 0);
    assert(height <= kMaxHeight);
    return height;
}

template<typename Key, class Equalcomp>
bool MultiLinkList<Key, Equalcomp>::KeyIsAfterNode(const Key& key,
        Node* n) const
{
    // NULL n is considered infinite
    return (n != NULL) && (compare_(n->key, key) < 0);
}

template<typename Key, class Equalcomp>
typename MultiLinkList<Key, Equalcomp>::Node* MultiLinkList<Key, Equalcomp>::FindGreaterOrEqual(
        const Key& key, Node** prev) const
{
    Node* x = head_;
    int level = GetMaxHeight() - 1;
    while (true) {
        Node* next = x->Next(level);
        if (KeyIsAfterNode(key, next)) {
            // Keep searching in this list
            x = next;
        } else {
            if (prev != NULL)
                prev[level] = x;
            if (level == 0) {
                return next;
            } else {
                // Switch to next list
                level--;
            }
        }
    }
}

template<typename Key, class Equalcomp>
typename MultiLinkList<Key, Equalcomp>::Node*
MultiLinkList<Key, Equalcomp>::FindLessThan(const Key& key) const
{
    Node* x = head_;
    int level = GetMaxHeight() - 1;
    while (true) {
        assert(x == head_ || compare_(x->key, key) < 0);
        Node* next = x->Next(level);
        if (next == NULL || compare_(next->key, key) >= 0) {
            if (level == 0) {
                return x;
            } else {
                // Switch to next list
                level--;
            }
        } else {
            x = next;
        }
    }
}

template<typename Key, class Equalcomp>
typename MultiLinkList<Key, Equalcomp>::Node* MultiLinkList<Key, Equalcomp>::FindLast() const
{
    Node* x = head_;
    int level = GetMaxHeight() - 1;
    while (true) {
        Node* next = x->Next(level);
        if (next == NULL) {
            if (level == 0) {
                return x;
            } else {
                // Switch to next list
                level--;
            }
        } else {
            x = next;
        }
    }
}

template<typename Key, class Equalcomp>
MultiLinkList<Key, Equalcomp>::MultiLinkList(Equalcomp cmp, Sandbox* arena) :
        compare_(cmp), arena_(arena), head_(
                NewNode(0 /* any key will do */, kMaxHeight)), max_height_(
                reinterpret_cast<void*>(1)), rnd_(0xdeadbeef)
{
    for (int i = 0; i < kMaxHeight; i++) {
        head_->SetNext(i, NULL);
    }
}

template<typename Key, class Equalcomp>
void MultiLinkList<Key, Equalcomp>::Insert(const Key& key)
{
    // TODO(opt): We can use a barrier-free variant of FindGreaterOrEqual()
    // here since Insert() is externally synchronized.
    Node* prev[kMaxHeight];
    Node* x = FindGreaterOrEqual(key, prev);

    // Our data structure does not allow duplicate insertion
    assert(x == NULL || !Equal(key, x->key));

    int height = RandomHeight();
    if (height > GetMaxHeight()) {
        for (int i = GetMaxHeight(); i < height; i++) {
            prev[i] = head_;
        }
        //fprintf(stderr, "Change height from %d to %d\n", max_height_, height);

        // It is ok to mutate max_height_ without any synchronization
        // with concurrent readers.  A concurrent reader that observes
        // the new value of max_height_ will see either the old value of
        // new level pointers from head_ (NULL), or a new value set in
        // the loop below.  In the former case the reader will
        // immediately drop to the next level since NULL sorts after all
        // keys.  In the latter case the reader will use the new node.
        max_height_.NoBarrier_Store(reinterpret_cast<void*>(height));
    }

    x = NewNode(key, height);
    for (int i = 0; i < height; i++) {
        // NoBarrier_SetNext() suffices since we will add a barrier when
        // we publish a pointer to "x" in prev[i].
        x->NoBarrier_SetNext(i, prev[i]->NoBarrier_Next(i));
        prev[i]->SetNext(i, x);
    }
}

template<typename Key, class Equalcomp>
bool MultiLinkList<Key, Equalcomp>::Contains(const Key& key) const
{
    Node* x = FindGreaterOrEqual(key, NULL);
    if (x != NULL && Equal(key, x->key)) {
        return true;
    } else {
        return false;
    }
}

}  // namespace LzKVStore

#endif  // SOE_KVS_KVS_LIST_H_
