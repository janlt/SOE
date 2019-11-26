/**
 * @file    metadbsrvrangecontext.hpp
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
 * metadbsrvrangecontext.hpp
 *
 *  Created on: Dec 28, 2017
 *      Author: Jan Lisowiec
 */

#ifndef RENTCONTROL_SOE_SOE_SOE_METADBSRV_SRC_METADBSRVRANGECONTEXT_HPP_
#define RENTCONTROL_SOE_SOE_SOE_METADBSRV_SRC_METADBSRVRANGECONTEXT_HPP_

namespace Metadbsrv {

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

//#define SOE_DEBUG_RANGE_CONTEXT

#ifdef SOE_DEBUG_RANGE_CONTEXT
#define SOESTORE_DEBUG_RANGE_CONTEXT(a) ((a))
#else
#define SOESTORE_DEBUG_RANGE_CONTEXT(a)
#endif

//
// RangeContext class to handle range ops
//
class RangeContext
{
public:
    friend class DeleteRangeContext;
    friend class PutRangeContext;
    friend class GetRangeContext;
    friend class MergeRangeContext;

    typedef enum _eType {
        eRangeContextDeleteContainer = 0,
        eRangeContextDeleteSubset    = 1,
        eRangeContextPutContainer    = 2,
        eRangeContextPutSubset       = 3,
        eRangeContextGetContainer    = 4,
        eRangeContextGetSubset       = 5,
        eRangeContextMergeContainer  = 6,
        eRangeContextMergeSubset     = 7
    } eType;

private:
    RangeContext::eType       type;

public:
    RangeContext(RangeContext::eType _type);
    virtual ~RangeContext();

    RangeContext::eType Type();
    virtual uint32_t Idx() = 0;
    virtual void ExecuteRange(Store &_store) = 0;
};

class DeleteRangeContext: public RangeContext
{
private:
    Slice                     start_key;
    Slice                     end_key;
    Slice                     key_pref;
    char                      start_key_buf[10000];
    char                      end_key_buf[10000];
    char                      key_pref_buf[10000];
    bool                      dir_forward;
    bool                      start_key_empty;
    bool                      end_key_empty;
    uint32_t                  idx;
    std::vector<Slice>        keys;

public:
    DeleteRangeContext(RangeContext::eType _type);
    DeleteRangeContext(Store &_store,
        RangeContext::eType _type,
        const Slice &_start_key,
        const Slice &_end_key,
        const Slice &_key_pref,
        bool _dir_forward,
        uint32_t _i);
    ~DeleteRangeContext();

    Slice &StartKey() { return start_key; }
    Slice &EndKey() { return end_key; }
    Slice &KeyPref() { return key_pref; }
    bool Valid();
    void AddKey(const Slice &key);
    void AddValue(const Slice &value);
    bool DirForward();
    virtual uint32_t Idx();
    bool InRange(Slice &key, bool check_validity = false);
    bool StartKeyEmpty();
    bool EndKeyEmpty();
    bool ValidKey(Slice &key); // if it's a subset range context, it might be subset's "placeholder" key
    virtual void ExecuteRange(Store &_store);

    // server side range callbacks
    static bool DeleteStoreRangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context);
    static bool DeleteSubsetRangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context);
};

class GetRangeContext: public RangeContext
{
public:
    GetRangeContext(RangeContext::eType _type);

    virtual uint32_t Idx();
    virtual void ExecuteRange(Store &_store);

    // server side range callbacks
    static bool GetStoreRangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context);
    static bool GetSubsetRangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context);
};

class PutRangeContext: public RangeContext
{
public:
    PutRangeContext(RangeContext::eType _type);

    virtual uint32_t Idx();
    virtual void ExecuteRange(Store &_store);

    // server side range callbacks
    static bool PutStoreRangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context);
    static bool PutSubsetRangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context);
};

class MergeRangeContext: public RangeContext
{
public:
    MergeRangeContext(RangeContext::eType _type);

    virtual uint32_t Idx();
    virtual void ExecuteRange(Store &_store);

    // server side range callbacks
    static bool MergeStoreRangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context);
    static bool MergeSubsetRangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context);
};

}

#endif /* RENTCONTROL_SOE_SOE_SOE_METADBSRV_SRC_METADBSRVRANGECONTEXT_HPP_ */
