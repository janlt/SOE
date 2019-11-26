/**
 * @file    dbsrviovtypesnt.hpp
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
 * dbsrviovtypesnt.hpp
 *
 *  Created on: Feb 27, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGS_INCLUDE_DBSRVIOVTYPESNT_HPP_
#define SOE_SOE_SOE_MSGS_INCLUDE_DBSRVIOVTYPESNT_HPP_

namespace MetaMsgs {

class IovMsgNTPoolInitializer
{
public:
    static bool initialized;
    IovMsgNTPoolInitializer();
    ~IovMsgNTPoolInitializer() {}
};

// -----------------------------------------------------------------------------------------

class GetReqNT: public IovMsgReqNT
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<GetReqNT, MemMgmt::ClassAlloc<GetReqNT> > poolT;

public:
    GetReqNT();
    virtual ~GetReqNT();

    static GetReqNT *get();
    static void put(GetReqNT *obj);
    static void registerPool();
};

// ====

class GetRspNT: public IovMsgRspNT
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<GetRspNT, MemMgmt::ClassAlloc<GetRspNT> > poolT;

public:
    GetRspNT();
    virtual ~GetRspNT();

    static GetRspNT *get();
    static void put(GetRspNT *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class PutReqNT: public IovMsgReqNT
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<PutReqNT, MemMgmt::ClassAlloc<PutReqNT> > poolT;

public:
    PutReqNT();
    virtual ~PutReqNT();

    static PutReqNT *get();
    static void put(PutReqNT *obj);
    static void registerPool();
};

// ====

class PutRspNT: public IovMsgRspNT
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<PutRspNT, MemMgmt::ClassAlloc<PutRspNT> > poolT;

public:
    PutRspNT();
    virtual  ~PutRspNT();

    static PutRspNT *get();
    static void put(PutRspNT *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class DeleteReqNT: public IovMsgReqNT
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<DeleteReqNT, MemMgmt::ClassAlloc<DeleteReqNT> > poolT;

public:
    DeleteReqNT();
    ~DeleteReqNT();

    static DeleteReqNT *get();
    static void put(DeleteReqNT *obj);
    static void registerPool();
};

// ====

class DeleteRspNT: public IovMsgRspNT
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<DeleteRspNT, MemMgmt::ClassAlloc<DeleteRspNT> > poolT;

public:
    DeleteRspNT();
    virtual ~DeleteRspNT();

    static DeleteRspNT *get();
    static void put(DeleteRspNT *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class GetRangeReqNT: public IovMsgReqNT
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<GetRangeReqNT, MemMgmt::ClassAlloc<GetRangeReqNT> > poolT;

public:
    GetRangeReqNT();
    ~GetRangeReqNT();

    static GetRangeReqNT *get();
    static void put(GetRangeReqNT *obj);
    static void registerPool();
};

// ====

class GetRangeRspNT: public IovMsgRspNT
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<GetRangeRspNT, MemMgmt::ClassAlloc<GetRangeRspNT> > poolT;

public:
    GetRangeRspNT();
    ~GetRangeRspNT();

    static GetRangeRspNT *get();
    static void put(GetRangeRspNT *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class MergeReqNT: public IovMsgReqNT
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<MergeReqNT, MemMgmt::ClassAlloc<MergeReqNT> > poolT;

public:
    MergeReqNT();
    virtual ~MergeReqNT();

    static MergeReqNT *get();
    static void put(MergeReqNT *obj);
    static void registerPool();
};

// ====

class MergeRspNT: public IovMsgRspNT
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<MergeRspNT, MemMgmt::ClassAlloc<MergeRspNT> > poolT;

public:
    MergeRspNT();
    virtual ~MergeRspNT();

    static MergeRspNT *get();
    static void put(MergeRspNT *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

}



#endif /* SOE_SOE_SOE_MSGS_INCLUDE_DBSRVIOVTYPESNT_HPP_ */
