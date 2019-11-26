/**
 * @file    rcsdbstoreiterator.hpp
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
 * rcsdbstoreiterator.hpp
 *
 *  Created on: Jul 13, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_RCSDBEM_SRC_RCSDBSTOREITERATOR_HPP_
#define SOE_SOE_SOE_RCSDBEM_SRC_RCSDBSTOREITERATOR_HPP_

namespace RcsdbLocalFacade {

//#define SOE_DEBUG_ITER

#ifdef SOE_DEBUG_ITER
#define SOESTORE_DEBUG_ITER(a) ((a))
#else
#define SOESTORE_DEBUG_ITER(a)
#endif

//
// Iterator class
//
class StoreIterator
{
public:
    typedef enum _eType {
        eStoreIteratorTypeContainer = 0,
        eStoreIteratorTypeSubset    = 1
    } eType;

private:
    std::shared_ptr<Iterator> rcsdb_iter;
    Slice                     start_key;
    Slice                     end_key;
    Slice                     key_pref;
    char                      start_key_buf[1024];
    char                      end_key_buf[1024];
    char                      key_pref_buf[1024];
    bool                      dir_forward;
    bool                      start_key_empty;
    bool                      end_key_empty;
    uint32_t                  id;
    uint32_t                  idx;
    bool                      initialized;
    bool                      ended; // when end key has been reached
    StoreIterator::eType      type;

public:
    StoreIterator(StoreIterator::eType _type);
    StoreIterator(Store &_store,
        StoreIterator::eType _type,
        const Slice &_start_key,
        const Slice &_end_key,
        const Slice &_key_pref,
        bool _dir_forward,
        ReadOptions &_read_options,
        uint32_t _i);
    ~StoreIterator();
    Slice &StartKey() { return start_key; }
    Slice &EndKey() { return end_key; }
    Slice &KeyPref() { return key_pref; }
    void SeekForNext(Slice &_key);
    void SeekForPrev(Slice &_key);
    void SeekToFirst();
    void SeekToLast();
    void Seek(Slice &_key);
    void Next();
    void Prev();
    bool Valid();
    Slice Key();
    Slice Value();
    bool Ended();
    void SetEnded();
    bool Initialized();
    void SetInitialized();
    bool DirForward();
    uint32_t Idx();
    StoreIterator::eType Type();
    bool InRange(Slice &key, bool check_validity = false);
    bool StartKeyEmpty();
    bool EndKeyEmpty();
    bool ValidKey(Slice &key); // if it's a subset iterator, it might be subset's "placeholder" key
};

}

#endif /* SOE_SOE_SOE_RCSDBEM_SRC_RCSDBSTOREITERATOR_HPP_ */
