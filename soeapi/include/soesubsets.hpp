/**
 * @file    soesubsets.hpp
 * @author  Jan Lisowiec <jlisowiec@gmail.com>
 * @version 1.0
 * @brief   Soe Session API
 *
 *
 * @section Copyright
 *
 *   Copyright Notice:
 *
 *    Copyright 2014-2019 Jan Lisowiec jlisowiec@gmail.com,
 *                   
 *
 *    This product is free software; you can redistribute it and/or,
 *    modify it under the terms of the GNU Library General Public
 *    License as published by the Free Software Foundation; either
 *    version 2 of the License, or (at your option) any later version.
 *    This software is distributed in the hope that it will be useful.
 *
 *
 * @section Licence
 *
 *   GPL 2 or later
 *
 *
 * @section Description
 *
 *   soe_soe
 *
 * @section Source
 *
 *    
 *    
 *
 */

/*
 * soesubsets.hpp
 *
 *  Created on: Jun 8, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOEAPI_INCLUDE_SOESUBSETS_HPP_
#define SOE_SOE_SOE_SOEAPI_INCLUDE_SOESUBSETS_HPP_

namespace Soe {

class Session;
class SubsetIterator;

/**
 * @brief DisjointSubset class
 *
 * This class allows creating, manipulating and destroying sets of items within a store having unique keys.
 * DisjointSubset is guaranteed to have empty intersection with any other DisjopintSubset belonging to the same store.
 *
 * DisjointSession object is created by invoking CreateSubset() method of Session class and specifying
 * subset name and type. A subset has to explicitly created prior adding items to it.
 *
 * Once a subset is created, it can be manipulated by invoking Put(), Get(), Delete() or Merge() methods. These methods
 * have the same syntax as Session methods.
 *
 * When a subset is destroyed, by invoking DestroySubset() on Session, all of its items will be discarded.
 *
 */
class DisjointSubset: public Subsetable
{
    friend class Iterator;
    friend class SubsetIterator;
    friend class Session;

private:
    char                name[256]; // doesn't have to be 0 terminated
    size_t              name_len;
    char                key_name[1024]; // doesn't have to be 0 terminated
    size_t              key_name_len;

    const char         *disjoint_pref = "DisjointSubset0987654321_";
    const char         *disjoint_subset_id = "1234567890_Nonsense_value_";
    size_t              pref_len;
    char               *value_buf;
    size_t              value_buf_size;
    std::vector<Duple>  keys;

    LocalArgsDescriptor args;

protected:
    DisjointSubset(const char *_name, size_t _name_len, std::shared_ptr<Session> _sess, bool _create = true)  throw(std::runtime_error);
    virtual ~DisjointSubset();

public:
    virtual const char *GetName(size_t &len)
    {
        len = name_len;
        return name;
    }

    virtual const char *GetPrefix(size_t &len)
    {
        len = pref_len;
        return disjoint_pref;
    }

    virtual const char *GetKeyName(size_t &len)
    {
        len = key_name_len;
        return key_name;
    }

    virtual size_t GetKeyNameLen()
    {
        return key_name_len;
    }

    virtual Session::Status Put(const char *key, size_t key_size, const char *data, size_t data_size) throw(std::runtime_error);
    virtual Session::Status Get(const char *key, size_t key_size, char *buf, size_t *buf_size) throw(std::runtime_error);
    virtual Session::Status Merge(const char *key, size_t key_size, const char *data, size_t data_size) throw(std::runtime_error);
    virtual Session::Status Delete(const char *key, size_t key_size) throw(std::runtime_error);

    virtual Session::Status GetSet(std::vector<std::pair<Duple, Duple>> &items, bool fail_on_first_nfound = true, size_t *fail_pos = 0) throw(std::runtime_error);
    virtual Session::Status PutSet(std::vector<std::pair<Duple, Duple>> &items, bool fail_on_first_dup = true, size_t *fail_pos = 0) throw(std::runtime_error);
    virtual Session::Status DeleteSet(std::vector<Duple> &keys, bool fail_on_first_nfound = true, size_t *fail_pos = 0) throw(std::runtime_error);
    virtual Session::Status MergeSet(std::vector<std::pair<Duple, Duple>> &items) throw(std::runtime_error);

    virtual Session::Status Join(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Intersect(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Complement(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status NotComplement(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }

    virtual Iterator *CreateIterator(Iterator::eIteratorDir dir = Iterator::eIteratorDir::eIteratorDirForward,
        const char *start_key = 0,
        size_t start_key_size = 0,
        const char *end_key = 0,
        size_t end_key_size = 0) throw(std::runtime_error);
    virtual Session::Status DestroyIterator(Iterator *it) const throw(std::runtime_error);

    Session *GetSession() { return sess.get(); }

protected:
    virtual int CreateKey(const char *key,
        size_t key_len,
        char *key_buf,
        size_t key_buf_len,
        size_t *key_ret_len = 0);

    virtual Session::Status DeleteAll() throw(std::runtime_error);
    Session::Status GetIdRecord() throw(std::runtime_error);
    Session::Status PutIdRecord() throw(std::runtime_error);
    Session::Status DeleteIdRecord() throw(std::runtime_error);
    static bool RangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context);
    void AddKey(const char *key, size_t key_len);
    void ResetKeys();
};

/**
 * @brief UnionSubset class
 *
 * NOT IMPLEMENTED
 *
 */
class UnionSubset: public Subsetable
{
    friend class Session;

private:
    char                name[256]; // doesn't have to be 0 terminated
    size_t              name_len;

    LocalArgsDescriptor args;

protected:
    UnionSubset(const char *_name, size_t _name_len, std::shared_ptr<Session> _sess, bool _create = true)  throw(std::runtime_error);
    virtual ~UnionSubset();

public:
    virtual Session::Status Put(const char *key, size_t key_size, const char *data, size_t data_size) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Get(const char *key, size_t key_size, char *buf, size_t *buf_size) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Merge(const char *key, size_t key_size, const char *data, size_t data_size) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Delete(const char *key, size_t key_size) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }

    virtual Session::Status GetSet(std::vector<std::pair<Duple, Duple>> &items, bool fail_on_first_nfound = true, size_t *fail_pos = 0) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status PutSet(std::vector<std::pair<Duple, Duple>> &items, bool fail_on_first_dup = true, size_t *fail_pos = 0) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status DeleteSet(std::vector<Duple> &keys, bool fail_on_first_nfound = true, size_t *fail_pos = 0) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status MergeSet(std::vector<std::pair<Duple, Duple>> &items) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }

    virtual Session::Status Join(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error);
    virtual Session::Status Intersect(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Complement(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status NotComplement(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }

    virtual Iterator *CreateIterator(Iterator::eIteratorDir dir = Iterator::eIteratorDir::eIteratorDirForward,
        const char *start_key = 0,
        size_t start_key_size = 0,
        const char *end_key = 0,
        size_t end_key_size = 0) throw(std::runtime_error);
    virtual Session::Status DestroyIterator(Iterator *it) const throw(std::runtime_error);

    virtual const char *GetName(size_t &len) { return 0; }
    virtual const char *GetPrefix(size_t &len) { return 0; }
    virtual const char *GetKeyName(size_t &len) { return 0; }
    virtual size_t GetKeyNameLen() { return 0; }

protected:
    virtual Session::Status DeleteAll() throw(std::runtime_error);
    virtual int CreateKey(const char *key,
        size_t key_len,
        char *key_buf,
        size_t key_buf_len,
        size_t *key_ret_len = 0);
};

/**
 * @brief ComplementSubset class
 *
 * NOT IMPLEMENTED
 *
 */
class ComplementSubset: public Subsetable
{
    friend class Session;

private:
    char                name[256]; // doesn't have to be 0 terminated
    size_t              name_len;

    LocalArgsDescriptor args;

protected:
    ComplementSubset(const char *_name, size_t _name_len, std::shared_ptr<Session> _sess, bool _create = true) throw(std::runtime_error);
    virtual ~ComplementSubset();

public:
    virtual Session::Status Put(const char *key, size_t key_size, const char *data, size_t data_size) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Get(const char *key, size_t key_size, char *buf, size_t *buf_size) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Merge(const char *key, size_t key_size, const char *data, size_t data_size) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Delete(const char *key, size_t key_size) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }

    virtual Session::Status GetSet(std::vector<std::pair<Duple, Duple>> &items, bool fail_on_first_nfound = true, size_t *fail_pos = 0) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status PutSet(std::vector<std::pair<Duple, Duple>> &items, bool fail_on_first_dup = true, size_t *fail_pos = 0) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status DeleteSet(std::vector<Duple> &keys, bool fail_on_first_nfound = true, size_t *fail_pos = 0) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status MergeSet(std::vector<std::pair<Duple, Duple>> &items) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }

    virtual Session::Status Join(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Intersect(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Complement(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error);
    virtual Session::Status NotComplement(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }

    virtual Iterator *CreateIterator(Iterator::eIteratorDir dir = Iterator::eIteratorDir::eIteratorDirForward,
        const char *start_key = 0,
        size_t start_key_size = 0,
        const char *end_key = 0,
        size_t end_key_size = 0) throw(std::runtime_error);
    virtual Session::Status DestroyIterator(Iterator *it) const throw(std::runtime_error);

    virtual const char *GetName(size_t &len) { return 0; }
    virtual const char *GetPrefix(size_t &len) { return 0; }
    virtual const char *GetKeyName(size_t &len) { return 0; }
    virtual size_t GetKeyNameLen() { return 0; }

protected:
    virtual Session::Status DeleteAll() throw(std::runtime_error);
    virtual int CreateKey(const char *key,
        size_t key_len,
        char *key_buf,
        size_t key_buf_len,
        size_t *key_ret_len = 0);
};

/**
 * @brief NotComplementSubset class
 *
 * NOT IMPLEMENTED
 *
 */
class NotComplementSubset: public Subsetable
{
    friend class Session;

private:
    char                name[256]; // doesn't have to be 0 terminated
    size_t              name_len;

    LocalArgsDescriptor args;

protected:
    NotComplementSubset(const char *_name, size_t _name_len, std::shared_ptr<Session> _sess, bool _create = true)  throw(std::runtime_error);
    virtual ~NotComplementSubset();

public:
    virtual Session::Status Put(const char *key, size_t key_size, const char *data, size_t data_size) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Get(const char *key, size_t key_size, char *buf, size_t *buf_size) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Merge(const char *key, size_t key_size, const char *data, size_t data_size) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Delete(const char *key, size_t key_size) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }

    virtual Session::Status GetSet(std::vector<std::pair<Duple, Duple>> &items, bool fail_on_first_nfound = true, size_t *fail_pos = 0) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status PutSet(std::vector<std::pair<Duple, Duple>> &items, bool fail_on_first_dup = true, size_t *fail_pos = 0) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status DeleteSet(std::vector<Duple> &keys, bool fail_on_first_nfound = true, size_t *fail_pos = 0) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status MergeSet(std::vector<std::pair<Duple, Duple>> &items) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }

    virtual Session::Status Join(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Intersect(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Complement(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status NotComplement(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error);

    virtual Iterator *CreateIterator(Iterator::eIteratorDir dir = Iterator::eIteratorDir::eIteratorDirForward,
        const char *start_key = 0,
        size_t start_key_size = 0,
        const char *end_key = 0,
        size_t end_key_size = 0) throw(std::runtime_error);
    virtual Session::Status DestroyIterator(Iterator *it) const throw(std::runtime_error);

    virtual const char *GetName(size_t &len) { return 0; }
    virtual const char *GetPrefix(size_t &len) { return 0; }
    virtual const char *GetKeyName(size_t &len) { return 0; }
    virtual size_t GetKeyNameLen() { return 0; }

protected:
    virtual Session::Status DeleteAll() throw(std::runtime_error);
    virtual int CreateKey(const char *key,
        size_t key_len,
        char *key_buf,
        size_t key_buf_len,
        size_t *key_ret_len = 0);
};

/**
 * @brief CartesianSubset class
 *
 * NOT IMPLEMENTED
 *
 */
class CartesianSubset: public Subsetable
{
    friend class Session;

private:
    char                name[256]; // doesn't have to be 0 terminated
    size_t              name_len;

    LocalArgsDescriptor args;

protected:
    CartesianSubset(const char *_name, size_t _name_len, std::shared_ptr<Session> _sess, bool _create = true)  throw(std::runtime_error);
    virtual ~CartesianSubset();

public:
    virtual Session::Status Put(const char *key, size_t key_size, const char *data, size_t data_size) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Get(const char *key, size_t key_size, char *buf, size_t *buf_size) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Merge(const char *key, size_t key_size, const char *data, size_t data_size) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Delete(const char *key, size_t key_size) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }

    virtual Session::Status GetSet(std::vector<std::pair<Duple, Duple>> &items, bool fail_on_first_nfound = true, size_t *fail_pos = 0) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status PutSet(std::vector<std::pair<Duple, Duple>> &items, bool fail_on_first_dup = true, size_t *fail_pos = 0) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status DeleteSet(std::vector<Duple> &keys, bool fail_on_first_nfound = true, size_t *fail_pos = 0) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status MergeSet(std::vector<std::pair<Duple, Duple>> &items) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }

    virtual Session::Status Join(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status Intersect(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error);
    virtual Session::Status Complement(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }
    virtual Session::Status NotComplement(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) { return Session::Status::eOpNotSupported; }

    virtual Iterator *CreateIterator(Iterator::eIteratorDir dir = Iterator::eIteratorDir::eIteratorDirForward,
        const char *start_key = 0,
        size_t start_key_size = 0,
        const char *end_key = 0,
        size_t end_key_size = 0) throw(std::runtime_error);
    virtual Session::Status DestroyIterator(Iterator *it) const throw(std::runtime_error);

    virtual const char *GetName(size_t &len) { return 0; }
    virtual const char *GetPrefix(size_t &len) { return 0; }
    virtual const char *GetKeyName(size_t &len) { return 0; }
    virtual size_t GetKeyNameLen() { return 0; }

protected:
    virtual Session::Status DeleteAll() throw(std::runtime_error);
    virtual int CreateKey(const char *key,
        size_t key_len,
        char *key_buf,
        size_t key_buf_len,
        size_t *key_ret_len = 0);
};

}

#endif /* SOE_SOE_SOE_SOEAPI_INCLUDE_SOESUBSETS_HPP_ */
