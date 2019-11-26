/**
 * @file    soesubsets.cpp
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
 * soesubsets.cpp
 *
 *  Created on: Jun 8, 2017
 *      Author: Jan Lisowiec
 */

#include <stdint.h>
#include <string.h>
#include <dlfcn.h>
#include <stdio.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <new>

#include "soe_soe.hpp"

using namespace std;

namespace Soe {

// ------------------------------------------------------------------------------------
// Disjoint
// ------------------------------------------------------------------------------------

/**
 * @brief DisjointSubset constructor
 *
 * Create an instance of DisjointSubset, protected constructor
 *
 * @param[in]  _name           Subset name
 * @param[in]  _name_len       Subset name length
 * @param[in]  _sess           Session used for this
 * @param[in]  _create         Create if doesn't exist
 *
 */
DisjointSubset::DisjointSubset(const char *_name, size_t _name_len, shared_ptr<Session> _sess, bool _create) throw(runtime_error)
    :Subsetable(_sess, Subsetable::eSubsetType::eSubsetTypeDisjoint),
     name_len(_name_len),
     pref_len(strlen(DisjointSubset::disjoint_pref)),
     value_buf(0),
     value_buf_size(32000)
{
    ::memset(name, '\0', sizeof(name));
    ::memcpy(name, _name, MIN(sizeof(name), name_len));

    ::memset(key_name, '\0', sizeof(key_name));
    ::memcpy(key_name, DisjointSubset::disjoint_pref, pref_len);
    ::memcpy(&key_name[pref_len], name, MIN(sizeof(key_name) - pref_len, name_len));
    key_name_len = pref_len + name_len;

    sess->InitDesc(args);

    Session::Status sts = GetIdRecord();
    if ( sts != Session::Status::eOk ) {
        if ( sts == Session::Status::eNotFoundKey ) {
            if ( _create == true ) {
                sts = PutIdRecord();
            }
        }
    }
    last_status = sts;
    if ( sts != Session::Status::eOk ) {
        throw runtime_error((string("Can't GetIdRecord()")));
    }
}

DisjointSubset::~DisjointSubset()
{
}

int DisjointSubset::CreateKey(const char *key, size_t key_len, char *key_buf, size_t key_buf_len, size_t *key_ret_len)
{
    return Subsetable::MakeKey(key_name, key_name_len, key, key_len, key_buf, key_buf_len, key_ret_len);
}

Session::Status DisjointSubset::GetIdRecord() throw(runtime_error)
{
    char buf[1024];
    char key_buf[1024];
    ::memcpy(key_buf, disjoint_subset_id, strlen(disjoint_subset_id));

    int ret = CreateKey("", 0, &key_buf[strlen(disjoint_subset_id)], sizeof(key_buf) - strlen(disjoint_subset_id));
    if ( ret ) {
        return Session::Status::eTruncatedValue;
    }
    args.key = key_buf;
    args.key_len = key_name_len + strlen(disjoint_subset_id);
    args.buf = buf;
    args.buf_len = sizeof(buf);

    Session::Status id_sts = sess->Get(args);
    last_status = id_sts;
    if ( id_sts != Session::Status::eOk ) {
        return id_sts;
    }
    if ( ::memcmp(buf, name, name_len) ) {
        return Session::Status::eInternalError;
    }
    return id_sts;
}

Session::Status DisjointSubset::PutIdRecord() throw(runtime_error)
{
    char data[1024];
    char key_buf[1024];
    ::memcpy(key_buf, disjoint_subset_id, strlen(disjoint_subset_id));
    ::memcpy(data, name, name_len);

    int ret = CreateKey("", 0, &key_buf[strlen(disjoint_subset_id)], sizeof(key_buf) - strlen(disjoint_subset_id));
    if ( ret ) {
        last_status = Session::Status::eTruncatedValue;
        return last_status;
    }
    args.key = key_buf;
    args.key_len = key_name_len + strlen(disjoint_subset_id);
    args.data = data;
    args.data_len = name_len;

    last_status = sess->Put(args);
    return last_status;
}

Session::Status DisjointSubset::DeleteIdRecord() throw(runtime_error)
{
    char data[1024];
    char key_buf[1024];
    ::memcpy(key_buf, disjoint_subset_id, strlen(disjoint_subset_id));
    ::memcpy(data, name, name_len);

    int ret = CreateKey("", 0, &key_buf[strlen(disjoint_subset_id)], sizeof(key_buf) - strlen(disjoint_subset_id));
    if ( ret ) {
        last_status = Session::Status::eTruncatedValue;
        return last_status;
    }
    args.key = key_buf;
    args.key_len = key_name_len + strlen(disjoint_subset_id);

    last_status = sess->Delete(args);
    return last_status;
}

/**
 * @brief DisjointSubset API
 *
 * Put an item in DisjointSubset
 *
 * @param[in]  key            Key
 * @param[in]  key_size       Key size
 * @param[in]  data           Data
 * @param[in]  data_size      Data size
 * @param[out] status         Return status (see soesession.hpp)
 *
 */
Session::Status DisjointSubset::Put(const char *key, size_t key_size, const char *data, size_t data_size) throw(runtime_error)
{
    char key_buf[1024];

    int ret = CreateKey(key, key_size, key_buf, sizeof(key_buf));
    if ( ret ) {
        last_status = Session::Status::eTruncatedValue;
        return last_status;
    }
    args.key = key_buf;
    args.key_len = key_name_len + key_size;
    args.data = data;
    args.data_len = data_size;

    last_status = sess->Put(args);
    return last_status;
}

/**
 * @brief DisjointSubset API
 *
 * Get an item from DisjointSubset
 *
 * @param[in]  key            Key
 * @param[in]  key_size       Key size
 * @param[in]  buf            Buffer
 * @param[in]  buf_size       Buffer size
 * @param[out] status         Return status (see soesession.hpp)
 *
 */
Session::Status DisjointSubset::Get(const char *key, size_t key_size, char *buf, size_t *buf_size) throw(runtime_error)
{
    char key_buf[1024];

    int ret = CreateKey(key, key_size, key_buf, sizeof(key_buf));
    if ( ret ) {
        last_status = Session::Status::eTruncatedValue;
        return last_status;
    }
    args.key = key_buf;
    args.key_len = key_name_len + key_size;
    args.buf = buf;
    args.buf_len = *buf_size;

    last_status = sess->Get(args);
    return last_status;
}

/**
 * @brief DisjointSubset API
 *
 * Merge an item in DisjointSubset
 *
 * @param[in]  key            Key
 * @param[in]  key_size       Key size
 * @param[in]  data           Data
 * @param[in]  data_size      Data size
 * @param[out] status         Return status (see soesession.hpp)
 *
 */
Session::Status DisjointSubset::Merge(const char *key, size_t key_size, const char *data, size_t data_size) throw(runtime_error)
{
    char key_buf[1024];

    int ret = CreateKey(key, key_size, key_buf, sizeof(key_buf));
    if ( ret ) {
        last_status = Session::Status::eTruncatedValue;
        return last_status;
    }
    args.key = key_buf;
    args.key_len = key_name_len + key_size;
    args.data = data;
    args.data_len = data_size;

    last_status = sess->Merge(args);
    return last_status;
}

/**
 * @brief DisjointSubset API
 *
 * Delete an item from DisjointSubset
 *
 * @param[in]  key            Key
 * @param[in]  key_size       Key size
 * @param[out] status         Return status (see soesession.hpp)
 *
 */
Session::Status DisjointSubset::Delete(const char *key, size_t key_size) throw(runtime_error)
{
    char key_buf[1024];

    int ret = CreateKey(key, key_size, key_buf, sizeof(key_buf));
    if ( ret ) {
        last_status = Session::Status::eTruncatedValue;
        return last_status;
    }
    args.key = key_buf;
    args.key_len = key_name_len + key_size;

    last_status = sess->Delete(args);
    return last_status;
}

/**
 * @brief DisjointSubset API
 *
 * GetSet - get an entire set of items matching items keys from DisjointSubset
 *
 * @param[in]  items                 Vector of items with keys, on return the matching keys items will
 *                                   have their values filled in
 * @param[in]  fail_on_first_nfound  Specifies whether or not to fail the function on the first
 *                                   not found key
 * @param[in,out]  fail_pos          Position within items with not found key that caused a failure
 * @param[out] status                Return status (see soesession.hpp)
 *                                   If items contains a key(s) that don't exist the function will return an error
 *                                   and the caller has to discard the vector as invalid
 *
 */
Session::Status DisjointSubset::GetSet(vector<pair<Duple, Duple>> &items,
    bool fail_on_first_nfound,
    size_t *fail_pos) throw(runtime_error)
{
    Session::Status sts = Session::Status::eInvalidArgument;

    if ( fail_pos ) {
        *fail_pos = 0;
    }
    for ( vector<pair<Duple, Duple>>::iterator it = items.begin() ; it != items.end() ; ++it ) {
        sts = Get((*it).first.data(), (*it).first.size(), (*it).second.buffer(), (*it).second.buffer_size());
        if ( sts != Session::Status::eOk ) {
            if ( sts == Session::Status::eNotFoundKey ) {
                if ( fail_on_first_nfound == true ) {
                    break;
                } else {
                    if ( fail_pos ) {
                        (*fail_pos)++;
                    }
                    continue;
                }
            }
            break;
        }
        if ( fail_pos ) {
            (*fail_pos)++;
        }
    }

    if ( sts == Session::Status::eNotFoundKey ) {
        if ( fail_on_first_nfound == false ) {
            sts = Session::Status::eOk;
        }
    }
    last_status = sts;
    return sts;
}

/**
 * @brief DisjointSubset API
 *
 * PutSet - put an entire set of items in DisjointSubset
 *
 * @param[in] items                  Vector of items with keys to be inserted in DisjointSubset
 * @param[in] fail_on_first_dup      Specifies whether or not to fail the function on the first
 *                                   duplicate key
 * @param[in,out] fail_pos           Position within items with not found key that caused a failure
 * @param[out] status                Return status (see soesession.hpp)
 *                                   If items contains key(s) that don't exist they'll be created, matching keys
 *                                   will be updated
 *
 */
Session::Status DisjointSubset::PutSet(vector<pair<Duple, Duple>> &items,
    bool fail_on_first_dup,
    size_t *fail_pos) throw(runtime_error)
{
    Session::Status sts = Session::Status::eInvalidArgument;

    if ( fail_pos ) {
        *fail_pos = 0;
    }
    for ( vector<pair<Duple, Duple>>::iterator it = items.begin() ; it != items.end() ; ++it ) {
        sts = Put((*it).first.data(), (*it).first.size(), (*it).second.data(), (*it).second.size());
        if ( sts != Session::Status::eOk ) {
            if ( sts == Session::Status::eDuplicateKey ) {
                if ( fail_on_first_dup == true ) {
                    break;
                } else {
                    if ( fail_pos ) {
                        (*fail_pos)++;
                    }
                    continue;
                }
            }
            break;
        }
        if ( fail_pos ) {
            (*fail_pos)++;
        }
    }

    if ( sts == Session::Status::eDuplicateKey ) {
        if ( fail_on_first_dup == false ) {
            sts = Session::Status::eOk;
        }
    }
    last_status = sts;
    return sts;
}

/**
 * @brief DisjointSubset API
 *
 * DeleteSet - delete an entire set of items from DisjointSubset
 *
 * @param[in]  items                 Vector of items with keys to be deleted from DisjointSubset
 * @param[in]  fail_on_first_nfound  Specifies whether or not to fail the function on the first
 *                                   not found key
 * @param[in,out]  fail_pos          Position within items with not found key that caused a failure
 * @param[out] status                Return status (see soesession.hpp)
 *                                   If items contains key(s) that don't exist, an error will be returned but
 *                                   all the keys up to that point will be deleted
 *
 */
Session::Status DisjointSubset::DeleteSet(vector<Duple> &keys,
    bool fail_on_first_nfound,
    size_t *fail_pos) throw(runtime_error)
{
    Session::Status sts = Session::Status::eInvalidArgument;

    if ( fail_pos ) {
        *fail_pos = 0;
    }
    for ( vector<Duple>::iterator it = keys.begin() ; it != keys.end() ; ++it ) {
        sts = Delete((*it).data(), (*it).size());
        if ( sts != Session::Status::eOk ) {
            if ( sts == Session::Status::eNotFoundKey ) {
                if ( fail_on_first_nfound == true ) {
                    break;
                } else {
                    if ( fail_pos ) {
                        (*fail_pos)++;
                    }
                    continue;
                }
            }
            break;
        }
        if ( fail_pos ) {
            (*fail_pos)++;
        }
    }

    if ( sts == Session::Status::eNotFoundKey ) {
        if ( fail_on_first_nfound == false ) {
            sts = Session::Status::eOk;
        }
    }
    last_status = sts;
    return sts;
}

/**
 * @brief DisjointSubset API
 *
 * MergeSet - merge an entire set of items in DisjointSubset
 *
 * @param[in]  items          Vector of items with keys to be merge in DisjointSubset
 * @param[out] status         Return status (see soesession.hpp)
 *                            If items contains key(s) that don't exist they'll be created, matching keys
 *                            will be updated
 *
 */
Session::Status DisjointSubset::MergeSet(vector<pair<Duple, Duple>> &items) throw(runtime_error)
{
    Session::Status sts = Session::Status::eInvalidArgument;

    for ( vector<pair<Duple, Duple>>::iterator it = items.begin() ; it != items.end() ; ++it ) {
        sts = Merge((*it).first.data(), (*it).first.size(), (*it).second.data(), (*it).second.size());
        if ( sts != Session::Status::eOk ) {
            break;
        }
    }

    last_status = sts;
    return sts;
}

void DisjointSubset::AddKey(const char *key, size_t key_len)
{
    keys.push_back(Duple(key, key_len));
}

void DisjointSubset::ResetKeys()
{
    for ( vector<Duple>::iterator it = keys.begin() ; it != keys.end() ; ++it ) {
        if ( (*it).empty() != true ) {
            delete (*it).data();
        }
    }
    keys.clear();
}

bool DisjointSubset::RangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context)
{
    if ( !context ) {
        return false;
    }

    DisjointSubset *obj = reinterpret_cast<DisjointSubset *>(context);

    if ( key_len >= obj->key_name_len && !memcmp(key, obj->key_name, obj->key_name_len) ) {
        char *new_key = new char[key_len + 1];
        ::memcpy(new_key, key, key_len);
        new_key[key_len] = '\0';
        obj->AddKey(new_key, key_len);
    }
    return true;
}

/**
 * @brief DisjointSubset API
 *
 * DeleteAll - delete an entire DisjointSubset
 *
 * @param[out] status         Return status (see soesession.hpp)
 *
 */
Session::Status DisjointSubset::DeleteAll() throw(runtime_error)
{
#ifdef __USE_SUBSET_KEY__
    char start_subset_key_buf[2048];
    char start_subset_key[1024];
    size_t start_subset_key_len;
    ::memset(start_subset_key, 0x00, sizeof(start_subset_key));
    int ret = CreateKey(start_subset_key, sizeof(start_subset_key), start_subset_key_buf, sizeof(start_subset_key_buf), &start_subset_key_len);
    if ( ret ) {
        return Session::Status::eTruncatedValue;
    }

    char end_subset_key_buf[2048];
    char end_subset_key[1024];
    ::memset(end_subset_key, 0xff, sizeof(end_subset_key));
    ret = CreateKey(end_subset_key, sizeof(end_subset_key), end_subset_key_buf, sizeof(end_subset_key_buf));
    if ( ret ) {
        return Session::Status::eTruncatedValue;
    }

    args.key = start_subset_key;
    args.key_len = sizeof(start_subset_key);
    args.end_key = end_subset_key;
#endif
    value_buf = new char[value_buf_size];

#ifdef __USE_SUBSET_KEY__
    Session::Status sts = sess->GetRange(start_subset_key_buf, end_subset_key_buf, start_subset_key_len, value_buf, &value_buf_size, &DisjointSubset::RangeCallback, this);
#else
    Session::Status sts = sess->GetRange(key_name, key_name_len, 0, 0, value_buf, &value_buf_size, &DisjointSubset::RangeCallback, this);
#endif
    if ( sts != Session::Status::eOk ) {
        ResetKeys();
        delete [] value_buf;
        last_status = sts;
        return sts;
    }
    if ( keys.size() ) {
        sts = sess->DeleteSet(keys);
    }
    ResetKeys();
    delete [] value_buf;

    DeleteIdRecord();
    last_status = sts;
    return sts;
}

/**
 * @brief DisjointSubset API
 *
 * CreateIterator - create an SubsetIterator on DisjointSubset
 *
 * @param[out] status         Return an iterator pointer, or NULL if no more iterators can be created
 *
 */
Iterator *DisjointSubset::CreateIterator(Iterator::eIteratorDir dir,
    const char *start_key,
    size_t start_key_size,
    const char *end_key,
    size_t end_key_size) throw(runtime_error)
{
    char it_start_key[1024];
    char it_end_key[1024];
    size_t it_start_size = 0;
    size_t it_end_size = 0;

    int ret = CreateKey(start_key, start_key_size, it_start_key, sizeof(it_start_key), &it_start_size);
    if ( ret ) {
        last_status = Session::Status::eInternalError;
        return 0;
    }

    ret = CreateKey(end_key, end_key_size, it_end_key, sizeof(it_end_key), &it_end_size);
    if ( ret ) {
        last_status = Session::Status::eInternalError;
        return 0;
    }

    return sess->CreateSubsetIterator(dir, this, it_start_key, it_start_size, it_end_key, it_end_size);
}

/**
 * @brief DisjointSubset API
 *
 * DestroyIterator - destroy an SubsetIterator on DisjointSubset
 *
 * @param[out] status         Return status (see soesession.hpp)
 *
 */
Session::Status DisjointSubset::DestroyIterator(Iterator *it) const throw(runtime_error)
{
    SetCLastStatus(sess->DestroyIterator(it));
    return last_status;
}

// ------------------------------------------------------------------------------------
// Union
// ------------------------------------------------------------------------------------
UnionSubset::UnionSubset(const char *_name, size_t _name_len, shared_ptr<Session> _sess, bool _create) throw(runtime_error)
    :Subsetable(_sess, Subsetable::eSubsetType::eSubsetTypeUnion),
     name_len(_name_len)
{
    ::memset(name, '\0', sizeof(name));
    ::memcpy(name, _name, MIN(sizeof(name), name_len));

    sess->InitDesc(args);
}

UnionSubset::~UnionSubset()
{}

Session::Status UnionSubset::Join(vector<shared_ptr<Subsetable>> &subsets) throw(runtime_error)
{
    return Session::Status::eOpNotSupported;
}

Session::Status UnionSubset::DeleteAll() throw(runtime_error)
{
    return Session::Status::eOpNotSupported;
}

int UnionSubset::CreateKey(const char *key,
    size_t key_len,
    char *key_buf,
    size_t key_buf_len,
    size_t *key_ret_len)
{
    return 0;
}

Iterator *UnionSubset::CreateIterator(Iterator::eIteratorDir dir,
    const char *start_key ,
    size_t start_key_size,
    const char *end_key,
    size_t end_key_size) throw(runtime_error)
{
    return 0;
}

Session::Status UnionSubset::DestroyIterator(Iterator *it) const throw(runtime_error)
{
    return Session::Status::eOpNotSupported;
}

// ------------------------------------------------------------------------------------
// Complement
// ------------------------------------------------------------------------------------
ComplementSubset::ComplementSubset(const char *_name, size_t _name_len, shared_ptr<Session> _sess, bool _create) throw(runtime_error)
    :Subsetable(_sess, Subsetable::eSubsetType::eSubsetTypeComplement),
     name_len(_name_len)
{
    ::memset(name, '\0', sizeof(name));
    ::memcpy(name, _name, MIN(sizeof(name), name_len));

    sess->InitDesc(args);
}

ComplementSubset::~ComplementSubset()
{}

Session::Status ComplementSubset::Complement(vector<shared_ptr<Subsetable>> &subsets) throw(runtime_error)
{
    return Session::Status::eOpNotSupported;
}

Session::Status ComplementSubset::DeleteAll() throw(runtime_error)
{
    return Session::Status::eOpNotSupported;
}

int ComplementSubset::CreateKey(const char *key,
    size_t key_len,
    char *key_buf,
    size_t key_buf_len,
    size_t *key_ret_len)
{
    return 0;
}

Iterator *ComplementSubset::CreateIterator(Iterator::eIteratorDir dir,
    const char *start_key,
    size_t start_key_size,
    const char *end_key,
    size_t end_key_size) throw(runtime_error)
{
    return 0;
}

Session::Status ComplementSubset::DestroyIterator(Iterator *it) const throw(runtime_error)
{
    return Session::Status::eOpNotSupported;
}

// ------------------------------------------------------------------------------------
// NotComplement
// ------------------------------------------------------------------------------------
NotComplementSubset::NotComplementSubset(const char *_name, size_t _name_len, shared_ptr<Session> _sess, bool _create) throw(runtime_error)
    :Subsetable(_sess, Subsetable::eSubsetType::eSubsetTypeNotComplement),
     name_len(_name_len)
{
    ::memset(name, '\0', sizeof(name));
    ::memcpy(name, _name, MIN(sizeof(name), name_len));

    sess->InitDesc(args);
}

NotComplementSubset::~NotComplementSubset()
{}

Session::Status NotComplementSubset::NotComplement(vector<shared_ptr<Subsetable>> &subsets) throw(runtime_error)
{
    return Session::Status::eOpNotSupported;
}

Session::Status NotComplementSubset::DeleteAll() throw(runtime_error)
{
    return Session::Status::eOpNotSupported;
}

int NotComplementSubset::CreateKey(const char *key,
    size_t key_len,
    char *key_buf,
    size_t key_buf_len,
    size_t *key_ret_len)
{
    return 0;
}

Iterator *NotComplementSubset::CreateIterator(Iterator::eIteratorDir dir,
    const char *start_key,
    size_t start_key_size,
    const char *end_key,
    size_t end_key_size) throw(runtime_error)
{
    return 0;
}

Session::Status NotComplementSubset::DestroyIterator(Iterator *it) const throw(runtime_error)
{
    return Session::Status::eOpNotSupported;
}

// ------------------------------------------------------------------------------------
// Cartesian
// ------------------------------------------------------------------------------------
CartesianSubset::CartesianSubset(const char *_name, size_t _name_len, shared_ptr<Session> _sess, bool _create) throw(runtime_error)
    :Subsetable(_sess, Subsetable::eSubsetType::eSubsetTypeCartesian),
     name_len(_name_len)
{
    ::memset(name, '\0', sizeof(name));
    ::memcpy(name, _name, MIN(sizeof(name), name_len));

    sess->InitDesc(args);
}

CartesianSubset::~CartesianSubset()
{}

Session::Status CartesianSubset::Intersect(vector<shared_ptr<Subsetable>> &subsets) throw(runtime_error)
{
    return Session::Status::eOpNotSupported;
}

Session::Status CartesianSubset::DeleteAll() throw(runtime_error)
{
    return Session::Status::eOpNotSupported;
}

int CartesianSubset::CreateKey(const char *key,
    size_t key_len,
    char *key_buf,
    size_t key_buf_len,
    size_t *key_ret_len)
{
    return 0;
}

Iterator *CartesianSubset::CreateIterator(Iterator::eIteratorDir dir,
    const char *start_key,
    size_t start_key_size,
    const char *end_key,
    size_t end_key_size) throw(runtime_error)
{
    return 0;
}

Session::Status CartesianSubset::DestroyIterator(Iterator *it) const throw(runtime_error)
{
    return Session::Status::eOpNotSupported;
}

}
