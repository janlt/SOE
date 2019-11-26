/**
 * @file    dbsrvmsgiovtypes.hpp
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
 * dbsrvmsgiovtypes.hpp
 *
 *  Created on: Jan 12, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGIOVTYPES_HPP_
#define SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGIOVTYPES_HPP_

namespace MetaMsgs {

// -----------------------------------------------------------------------------------------

template <class T>
class GetReq: public IovMsgReq<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<GetReq, MemMgmt::ClassAlloc<GetReq> > poolT;

public:
    GetReq();
    virtual ~GetReq();

    static GetReq *get();
    static void put(GetReq *obj);
    static void registerPool();
};

template <class T> int GetReq<T>::class_msg_type = eMsgGetReq;
template <class T> MemMgmt::GeneralPool<GetReq<T>, MemMgmt::ClassAlloc<GetReq<T>> > GetReq<T>::poolT(10, "GetReq");

template <class T>
GetReq<T>::GetReq()
    :IovMsgReq<T>(static_cast<eMsgType>(GetReq<T>::class_msg_type))
{}

template <class T>
GetReq<T>::~GetReq()
{}

template <class T>
GetReq<T> *GetReq<T>::get()
{
    return GetReq<T>::poolT.get();
}

template <class T>
void GetReq<T>::put(GetReq<T> *obj)
{
    GetReq<T>::poolT.put(obj);
}

template <class T>
void GetReq<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(GetReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&GetReq<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(GetReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&GetReq<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// ====

template <class T>
class GetRsp: public IovMsgRsp<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<GetRsp, MemMgmt::ClassAlloc<GetRsp> > poolT;

public:
    GetRsp();
    virtual ~GetRsp();

    static GetRsp *get();
    static void put(GetRsp *obj);
    static void registerPool();
};

template <class T> int GetRsp<T>::class_msg_type = eMsgGetRsp;
template <class T> MemMgmt::GeneralPool<GetRsp<T>, MemMgmt::ClassAlloc<GetRsp<T>> > GetRsp<T>::poolT(10, "GetRsp");

template <class T>
GetRsp<T>::GetRsp()
    :IovMsgRsp<T>(static_cast<eMsgType>(GetRsp<T>::class_msg_type))
{}

template <class T>
GetRsp<T>::~GetRsp()
{}

template <class T>
GetRsp<T> *GetRsp<T>::get()
{
    return GetRsp<T>::poolT.get();
}

template <class T>
void GetRsp<T>::put(GetRsp<T> *obj)
{
    GetRsp<T>::poolT.put(obj);
}

template <class T>
void GetRsp<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(GetRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&GetRsp<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(GetRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&GetRsp<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}
// -----------------------------------------------------------------------------------------

template <class T>
class PutReq: public IovMsgReq<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<PutReq, MemMgmt::ClassAlloc<PutReq> > poolT;

public:
    PutReq();
    virtual ~PutReq();

    static PutReq *get();
    static void put(PutReq *obj);
    static void registerPool();
};

template <class T> int PutReq<T>::class_msg_type = eMsgPutReq;
template <class T> MemMgmt::GeneralPool<PutReq<T>, MemMgmt::ClassAlloc<PutReq<T>> > PutReq<T>::poolT(10, "PutReq");

template <class T>
PutReq<T>::PutReq()
    :IovMsgReq<T>(static_cast<eMsgType>(PutReq<T>::class_msg_type))
{}

template <class T>
PutReq<T>::~PutReq()
{}

template <class T>
PutReq<T> *PutReq<T>::get()
{
    return PutReq<T>::poolT.get();
}

template <class T>
void PutReq<T>::put(PutReq<T> *obj)
{
    PutReq<T>::poolT.put(obj);
}

template <class T>
void PutReq<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(PutReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&PutReq<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(PutReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&PutReq<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// ====

template <class T>
class PutRsp: public IovMsgRsp<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<PutRsp, MemMgmt::ClassAlloc<PutRsp> > poolT;

public:
    PutRsp();
    virtual  ~PutRsp();

    static PutRsp *get();
    static void put(PutRsp *obj);
    static void registerPool();
};

template <class T> int PutRsp<T>::class_msg_type = eMsgPutRsp;
template <class T> MemMgmt::GeneralPool<PutRsp<T>, MemMgmt::ClassAlloc<PutRsp<T>> > PutRsp<T>::poolT(10, "PutRsp");

template <class T>
PutRsp<T>::PutRsp()
    :IovMsgRsp<T>(static_cast<eMsgType>(PutRsp<T>::class_msg_type))
{}

template <class T>
PutRsp<T>::~PutRsp()
{}

template <class T>
PutRsp<T> *PutRsp<T>::get()
{
    return PutRsp<T>::poolT.get();
}

template <class T>
void PutRsp<T>::put(PutRsp<T> *obj)
{
    PutRsp<T>::poolT.put(obj);
}

template <class T>
void PutRsp<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(PutRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&PutRsp<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(PutRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&PutRsp<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// -----------------------------------------------------------------------------------------

template <class T>
class DeleteReq: public IovMsgReq<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<DeleteReq, MemMgmt::ClassAlloc<DeleteReq> > poolT;

public:
    DeleteReq();
    ~DeleteReq();

    static DeleteReq *get();
    static void put(DeleteReq *obj);
    static void registerPool();
};

template <class T>
int DeleteReq<T>::class_msg_type = eMsgDeleteReq;
template <class T>
MemMgmt::GeneralPool<DeleteReq<T>, MemMgmt::ClassAlloc<DeleteReq<T>> > DeleteReq<T>::poolT(10, "DeleteReq");

template <class T>
DeleteReq<T>::DeleteReq()
    :IovMsgReq<T>(static_cast<eMsgType>(DeleteReq<T>::class_msg_type))
{}

template <class T>
DeleteReq<T>::~DeleteReq()
{}

template <class T>
DeleteReq<T> *DeleteReq<T>::get()
{
    return DeleteReq<T>::poolT.get();
}

template <class T>
void DeleteReq<T>::put(DeleteReq<T> *obj)
{
    DeleteReq<T>::poolT.put(obj);
}

template <class T>
void DeleteReq<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(DeleteReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&DeleteReq<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(DeleteReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&DeleteReq<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// ====

template <class T>
class DeleteRsp: public IovMsgRsp<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<DeleteRsp, MemMgmt::ClassAlloc<DeleteRsp> > poolT;

public:
    DeleteRsp();
    virtual ~DeleteRsp();

    static DeleteRsp *get();
    static void put(DeleteRsp *obj);
    static void registerPool();
};

template <class T>
int DeleteRsp<T>::class_msg_type = eMsgDeleteRsp;
template <class T>
MemMgmt::GeneralPool<DeleteRsp<T>, MemMgmt::ClassAlloc<DeleteRsp<T>> > DeleteRsp<T>::poolT(10, "DeleteRsp");

template <class T>
DeleteRsp<T>::DeleteRsp()
    :IovMsgRsp<T>(static_cast<eMsgType>(DeleteRsp::class_msg_type))
{}

template <class T>
DeleteRsp<T>::~DeleteRsp()
{}

template <class T>
DeleteRsp<T> *DeleteRsp<T>::get()
{
    return DeleteRsp<T>::poolT.get();
}

template <class T>
void DeleteRsp<T>::put(DeleteRsp<T> *obj)
{
    DeleteRsp<T>::poolT.put(obj);
}

template <class T>
void DeleteRsp<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(DeleteRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&DeleteRsp<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(DeleteRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&DeleteRsp<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// -----------------------------------------------------------------------------------------

template <class T>
class GetRangeReq: public IovMsgReq<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<GetRangeReq, MemMgmt::ClassAlloc<GetRangeReq> > poolT;

public:
    GetRangeReq();
    ~GetRangeReq();

    static GetRangeReq *get();
    static void put(GetRangeReq *obj);
    static void registerPool();
};

template <class T> int GetRangeReq<T>::class_msg_type = eMsgGetRangeReq;
template <class T> MemMgmt::GeneralPool<GetRangeReq<T>, MemMgmt::ClassAlloc<GetRangeReq<T>> > GetRangeReq<T>::poolT(10, "GetRangeReq");

template <class T>
GetRangeReq<T>::GetRangeReq()
    :IovMsgReq<T>(static_cast<eMsgType>(GetRangeReq<T>::class_msg_type))
{}

template <class T>
GetRangeReq<T>::~GetRangeReq()
{}

template <class T>
GetRangeReq<T> *GetRangeReq<T>::get()
{
    return GetRangeReq<T>::poolT.get();
}

template <class T>
void GetRangeReq<T>::put(GetRangeReq<T> *obj)
{
    GetRangeReq<T>::poolT.put(obj);
}

template <class T>
void GetRangeReq<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(GetRangeReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&GetRangeReq<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(GetRangeReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&GetRangeReq<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// ====

template <class T>
class GetRangeRsp: public IovMsgRsp<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<GetRangeRsp, MemMgmt::ClassAlloc<GetRangeRsp> > poolT;

public:
    GetRangeRsp();
    ~GetRangeRsp();

    static GetRangeRsp *get();
    static void put(GetRangeRsp *obj);
    static void registerPool();
};

template <class T>
int GetRangeRsp<T>::class_msg_type = eMsgGetRangeRsp;
template <class T>
MemMgmt::GeneralPool<GetRangeRsp<T>, MemMgmt::ClassAlloc<GetRangeRsp<T>> > GetRangeRsp<T>::poolT(10, "GetRangeRsp");

template <class T>
GetRangeRsp<T>::GetRangeRsp()
    :IovMsgRsp<T>(static_cast<eMsgType>(GetRangeRsp<T>::class_msg_type))
{}

template <class T>
GetRangeRsp<T>::~GetRangeRsp()
{}

template <class T>
GetRangeRsp<T> *GetRangeRsp<T>::get()
{
    return GetRangeRsp<T>::poolT.get();
}

template <class T>
void GetRangeRsp<T>::put(GetRangeRsp<T> *obj)
{
    GetRangeRsp<T>::poolT.put(obj);
}

template <class T>
void GetRangeRsp<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(GetRangeRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&GetRangeRsp<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(GetRangeRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&GetRangeRsp<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// -----------------------------------------------------------------------------------------

template <class T>
class MergeReq: public IovMsgReq<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<MergeReq, MemMgmt::ClassAlloc<MergeReq> > poolT;

public:
    MergeReq();
    virtual ~MergeReq();

    static MergeReq *get();
    static void put(MergeReq *obj);
    static void registerPool();
};

template <class T>
int MergeReq<T>::class_msg_type = eMsgMergeReq;
template <class T>
MemMgmt::GeneralPool<MergeReq<T>, MemMgmt::ClassAlloc<MergeReq<T>> > MergeReq<T>::poolT(10, "MergeReq");

template <class T>
MergeReq<T>::MergeReq()
    :IovMsgReq<T>(static_cast<eMsgType>(MergeReq<T>::class_msg_type))
{}

template <class T>
MergeReq<T>::~MergeReq()
{}

template <class T>
MergeReq<T> *MergeReq<T>::get()
{
    return MergeReq<T>::poolT.get();
}

template <class T>
void MergeReq<T>::put(MergeReq<T> *obj)
{
    MergeReq<T>::poolT.put(obj);
}

template <class T>
void MergeReq<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(MergeReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&MergeReq<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(MergeReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&MergeReq<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// ====

template <class T>
class MergeRsp: public IovMsgRsp<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<MergeRsp, MemMgmt::ClassAlloc<MergeRsp> > poolT;

public:
    MergeRsp();
    virtual ~MergeRsp();

    static MergeRsp *get();
    static void put(MergeRsp *obj);
    static void registerPool();
};

template <class T> int MergeRsp<T>::class_msg_type = eMsgMergeRsp;
template <class T> MemMgmt::GeneralPool<MergeRsp<T>, MemMgmt::ClassAlloc<MergeRsp<T>> > MergeRsp<T>::poolT(10, "MergeRsp");

template <class T>
MergeRsp<T>::MergeRsp()
    :IovMsgRsp<T>(static_cast<eMsgType>(MergeRsp<T>::class_msg_type))
{}

template <class T>
MergeRsp<T>::~MergeRsp()
{}

template <class T>
MergeRsp<T> *MergeRsp<T>::get()
{
    return MergeRsp<T>::poolT.get();
}

template <class T>
void MergeRsp<T>::put(MergeRsp *obj)
{
    MergeRsp<T>::poolT.put(obj);
}

template <class T>
void MergeRsp<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(MergeRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&MergeRsp<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(MergeRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&MergeRsp<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// -----------------------------------------------------------------------------------------

template <class T>
class IovMsgPoolInitializer
{
public:
    static bool initialized;
    IovMsgPoolInitializer();
    ~IovMsgPoolInitializer() {}
};

template <class T>
bool IovMsgPoolInitializer<T>::initialized = false;

template <class T>
IovMsgPoolInitializer<T>::IovMsgPoolInitializer()
{
    if ( initialized != true ) {
        GetReq<T>::registerPool();
        GetRsp<T>::registerPool();
        PutReq<T>::registerPool();
        PutRsp<T>::registerPool();
        DeleteReq<T>::registerPool();
        DeleteRsp<T>::registerPool();
        GetRangeReq<T>::registerPool();
        GetRangeRsp<T>::registerPool();
        MergeReq<T>::registerPool();
        MergeRsp<T>::registerPool();
    }
    initialized = true;
}

}

#endif /* SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGIOVTYPES_HPP_ */
