/**
 * @file    soesubsetable.hpp
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
 * soesubsetbase.hpp
 *
 *  Created on: Jun 8, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOEAPI_INCLUDE_SOESUBSETABLE_HPP_
#define SOE_SOE_SOE_SOEAPI_INCLUDE_SOESUBSETABLE_HPP_

namespace Soe {

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

class Session;

class Subsetable
{
    friend class Session;
    friend class Iterator;
    friend class SubsetIterator;

public:
    typedef enum _eSubsetType {
        SOE_SUBSETABLE_SUBSET_TYPE
    } eSubsetType;

    class Deleter
    {
    public:
        void operator()(Subsetable *p) {}
    };

protected:
    eSubsetType                  type;
    std::shared_ptr<Session>     sess;
    std::shared_ptr<Subsetable>  this_subs;
    Session::Status              last_status;

    Subsetable(std::shared_ptr<Session> _sess, Subsetable::eSubsetType _type) throw(std::runtime_error);
    virtual ~Subsetable();

public:
    virtual Session::Status Put(const char *key, size_t key_size, const char *data, size_t data_size) throw(std::runtime_error) = 0;
    virtual Session::Status Get(const char *key, size_t key_size, char *buf, size_t *buf_size) throw(std::runtime_error) = 0;
    virtual Session::Status Merge(const char *key, size_t key_size, const char *data, size_t data_size) throw(std::runtime_error) = 0;
    virtual Session::Status Delete(const char *key, size_t key_size) throw(std::runtime_error) = 0;

    virtual Session::Status GetSet(std::vector<std::pair<Duple, Duple>> &items, bool fail_on_first_nfound = true, size_t *fail_pos = 0) throw(std::runtime_error) = 0;
    virtual Session::Status PutSet(std::vector<std::pair<Duple, Duple>> &items, bool fail_on_first_dup = true, size_t *fail_pos = 0) throw(std::runtime_error) = 0;
    virtual Session::Status DeleteSet(std::vector<Duple> &keys, bool fail_on_first_nfound = true, size_t *fail_pos = 0) throw(std::runtime_error) = 0;
    virtual Session::Status MergeSet(std::vector<std::pair<Duple, Duple>> &items) throw(std::runtime_error) = 0;

    virtual Session::Status Join(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) = 0;
    virtual Session::Status Intersect(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) = 0;
    virtual Session::Status Complement(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) = 0;
    virtual Session::Status NotComplement(std::vector<std::shared_ptr<Subsetable>> &subsets) throw(std::runtime_error) = 0;

    virtual Iterator *CreateIterator(Iterator::eIteratorDir dir = Iterator::eIteratorDir::eIteratorDirForward,
        const char *start_key = 0,
        size_t start_key_size = 0,
        const char *end_key = 0,
        size_t end_key_size = 0) throw(std::runtime_error) = 0;
    virtual Session::Status DestroyIterator(Iterator *it) const throw(std::runtime_error) = 0;

    virtual const char *GetName(size_t &len) = 0;
    virtual const char *GetPrefix(size_t &len) = 0;
    virtual const char *GetKeyName(size_t &len) = 0;
    virtual size_t GetKeyNameLen() = 0;

    Session::Status GetLastStatus() { return last_status; }
    void SetLastStatus(Session::Status _sts) { last_status = _sts; }
    void SetCLastStatus(Session::Status _sts) const { const_cast<Subsetable *>(this)->last_status = _sts; }

protected:
    virtual int CreateKey(const char *key,
        size_t key_len,
        char *key_buf,
        size_t key_buf_len,
        size_t *key_ret_len = 0) = 0;

    virtual int MakeKey(const char *pref,
        size_t pref_len,
        const char *key,
        size_t key_len,
        char *key_buf,
        size_t key_buf_len,
        size_t *key_ret_len = 0);

    virtual Session::Status DeleteAll() throw(std::runtime_error) = 0;
};

}

#endif /* SOE_SOE_SOE_SOEAPI_INCLUDE_SOESUBSETABLE_HPP_ */
