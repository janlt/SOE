/**
 * @file    dbsrvmsgiovasynctypes.hpp
 * @author  Jan Lisowiec <jlisowiec@gmail.com>
 * @version 1.0
 * @brief   Soe Msgs API
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
 * dbsrvmsgiovasynctypes.hpp
 *
 *  Created on: Dec 1, 2017
 *      Author: Jan Lisowiec
 */

#ifndef RENTCONTROL_SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGIOVASYNCTYPES_HPP_
#define RENTCONTROL_SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGIOVASYNCTYPES_HPP_

namespace MetaMsgs {

// -----------------------------------------------------------------------------------------

template <class T>
class GetAsyncReq: public IovMsgAsyncReq<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<GetAsyncReq, MemMgmt::ClassAlloc<GetAsyncReq> > poolT;

public:
    GetAsyncReq();
    virtual ~GetAsyncReq();

    static GetAsyncReq *get();
    static void put(GetAsyncReq *obj);
    static void registerPool();
};

template <class T> int GetAsyncReq<T>::class_msg_type = eMsgGetAsyncReq;
template <class T> MemMgmt::GeneralPool<GetAsyncReq<T>, MemMgmt::ClassAlloc<GetAsyncReq<T>> > GetAsyncReq<T>::poolT(10, "GetAsyncReq");

template <class T>
GetAsyncReq<T>::GetAsyncReq()
    :IovMsgAsyncReq<T>(static_cast<eMsgType>(GetAsyncReq<T>::class_msg_type))
{}

template <class T>
GetAsyncReq<T>::~GetAsyncReq()
{}

template <class T>
GetAsyncReq<T> *GetAsyncReq<T>::get()
{
    return GetAsyncReq<T>::poolT.get();
}

template <class T>
void GetAsyncReq<T>::put(GetAsyncReq<T> *obj)
{
    GetAsyncReq<T>::poolT.put(obj);
}

template <class T>
void GetAsyncReq<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(GetAsyncReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&GetAsyncReq<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(GetAsyncReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&GetAsyncReq<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// ====

template <class T>
class GetAsyncRsp: public IovMsgAsyncRsp<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<GetAsyncRsp, MemMgmt::ClassAlloc<GetAsyncRsp> > poolT;

public:
    GetAsyncRsp();
    virtual ~GetAsyncRsp();

    static GetAsyncRsp *get();
    static void put(GetAsyncRsp *obj);
    static void registerPool();
};

template <class T> int GetAsyncRsp<T>::class_msg_type = eMsgGetAsyncRsp;
template <class T> MemMgmt::GeneralPool<GetAsyncRsp<T>, MemMgmt::ClassAlloc<GetAsyncRsp<T>> > GetAsyncRsp<T>::poolT(10, "GetAsyncRsp");

template <class T>
GetAsyncRsp<T>::GetAsyncRsp()
    :IovMsgAsyncRsp<T>(static_cast<eMsgType>(GetAsyncRsp<T>::class_msg_type))
{}

template <class T>
GetAsyncRsp<T>::~GetAsyncRsp()
{}

template <class T>
GetAsyncRsp<T> *GetAsyncRsp<T>::get()
{
    return GetAsyncRsp<T>::poolT.get();
}

template <class T>
void GetAsyncRsp<T>::put(GetAsyncRsp<T> *obj)
{
    GetAsyncRsp<T>::poolT.put(obj);
}

template <class T>
void GetAsyncRsp<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(GetAsyncRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&GetAsyncRsp<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(GetAsyncRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&GetAsyncRsp<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}
// -----------------------------------------------------------------------------------------

template <class T>
class PutAsyncReq: public IovMsgAsyncReq<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<PutAsyncReq, MemMgmt::ClassAlloc<PutAsyncReq> > poolT;

public:
    PutAsyncReq();
    virtual ~PutAsyncReq();

    static PutAsyncReq *get();
    static void put(PutAsyncReq *obj);
    static void registerPool();
};

template <class T> int PutAsyncReq<T>::class_msg_type = eMsgPutAsyncReq;
template <class T> MemMgmt::GeneralPool<PutAsyncReq<T>, MemMgmt::ClassAlloc<PutAsyncReq<T>> > PutAsyncReq<T>::poolT(10, "PutAsyncReq");

template <class T>
PutAsyncReq<T>::PutAsyncReq()
    :IovMsgAsyncReq<T>(static_cast<eMsgType>(PutAsyncReq<T>::class_msg_type))
{}

template <class T>
PutAsyncReq<T>::~PutAsyncReq()
{}

template <class T>
PutAsyncReq<T> *PutAsyncReq<T>::get()
{
    return PutAsyncReq<T>::poolT.get();
}

template <class T>
void PutAsyncReq<T>::put(PutAsyncReq<T> *obj)
{
    PutAsyncReq<T>::poolT.put(obj);
}

template <class T>
void PutAsyncReq<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(PutAsyncReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&PutAsyncReq<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(PutAsyncReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&PutAsyncReq<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// ====

template <class T>
class PutAsyncRsp: public IovMsgAsyncRsp<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<PutAsyncRsp, MemMgmt::ClassAlloc<PutAsyncRsp> > poolT;

public:
    PutAsyncRsp();
    virtual  ~PutAsyncRsp();

    static PutAsyncRsp *get();
    static void put(PutAsyncRsp *obj);
    static void registerPool();
};

template <class T> int PutAsyncRsp<T>::class_msg_type = eMsgPutAsyncRsp;
template <class T> MemMgmt::GeneralPool<PutAsyncRsp<T>, MemMgmt::ClassAlloc<PutAsyncRsp<T>> > PutAsyncRsp<T>::poolT(10, "PutAsyncRsp");

template <class T>
PutAsyncRsp<T>::PutAsyncRsp()
    :IovMsgAsyncRsp<T>(static_cast<eMsgType>(PutAsyncRsp<T>::class_msg_type))
{}

template <class T>
PutAsyncRsp<T>::~PutAsyncRsp()
{}

template <class T>
PutAsyncRsp<T> *PutAsyncRsp<T>::get()
{
    return PutAsyncRsp<T>::poolT.get();
}

template <class T>
void PutAsyncRsp<T>::put(PutAsyncRsp<T> *obj)
{
    PutAsyncRsp<T>::poolT.put(obj);
}

template <class T>
void PutAsyncRsp<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(PutAsyncRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&PutAsyncRsp<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(PutAsyncRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&PutAsyncRsp<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// -----------------------------------------------------------------------------------------

template <class T>
class DeleteAsyncReq: public IovMsgAsyncReq<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<DeleteAsyncReq, MemMgmt::ClassAlloc<DeleteAsyncReq> > poolT;

public:
    DeleteAsyncReq();
    ~DeleteAsyncReq();

    static DeleteAsyncReq *get();
    static void put(DeleteAsyncReq *obj);
    static void registerPool();
};

template <class T> int DeleteAsyncReq<T>::class_msg_type = eMsgDeleteAsyncReq;
template <class T> MemMgmt::GeneralPool<DeleteAsyncReq<T>, MemMgmt::ClassAlloc<DeleteAsyncReq<T>> > DeleteAsyncReq<T>::poolT(10, "DeleteAsyncReq");

template <class T>
DeleteAsyncReq<T>::DeleteAsyncReq()
    :IovMsgAsyncReq<T>(static_cast<eMsgType>(DeleteAsyncReq<T>::class_msg_type))
{}

template <class T>
DeleteAsyncReq<T>::~DeleteAsyncReq()
{}

template <class T>
DeleteAsyncReq<T> *DeleteAsyncReq<T>::get()
{
    return DeleteAsyncReq<T>::poolT.get();
}

template <class T>
void DeleteAsyncReq<T>::put(DeleteAsyncReq<T> *obj)
{
    DeleteAsyncReq<T>::poolT.put(obj);
}

template <class T>
void DeleteAsyncReq<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(DeleteAsyncReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&DeleteAsyncReq<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(DeleteAsyncReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&DeleteAsyncReq<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// ====

template <class T>
class DeleteAsyncRsp: public IovMsgAsyncRsp<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<DeleteAsyncRsp, MemMgmt::ClassAlloc<DeleteAsyncRsp> > poolT;

public:
    DeleteAsyncRsp();
    virtual ~DeleteAsyncRsp();

    static DeleteAsyncRsp *get();
    static void put(DeleteAsyncRsp *obj);
    static void registerPool();
};

template <class T> int DeleteAsyncRsp<T>::class_msg_type = eMsgDeleteAsyncRsp;
template <class T> MemMgmt::GeneralPool<DeleteAsyncRsp<T>, MemMgmt::ClassAlloc<DeleteAsyncRsp<T>> > DeleteAsyncRsp<T>::poolT(10, "DeleteAsyncRsp");

template <class T>
DeleteAsyncRsp<T>::DeleteAsyncRsp()
    :IovMsgAsyncRsp<T>(static_cast<eMsgType>(DeleteAsyncRsp::class_msg_type))
{}

template <class T>
DeleteAsyncRsp<T>::~DeleteAsyncRsp()
{}

template <class T>
DeleteAsyncRsp<T> *DeleteAsyncRsp<T>::get()
{
    return DeleteAsyncRsp<T>::poolT.get();
}

template <class T>
void DeleteAsyncRsp<T>::put(DeleteAsyncRsp<T> *obj)
{
    DeleteAsyncRsp<T>::poolT.put(obj);
}

template <class T>
void DeleteAsyncRsp<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(DeleteAsyncRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&DeleteAsyncRsp<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(DeleteAsyncRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&DeleteAsyncRsp<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// -----------------------------------------------------------------------------------------

template <class T>
class GetRangeAsyncReq: public IovMsgAsyncReq<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<GetRangeAsyncReq, MemMgmt::ClassAlloc<GetRangeAsyncReq> > poolT;

public:
    GetRangeAsyncReq();
    ~GetRangeAsyncReq();

    static GetRangeAsyncReq *get();
    static void put(GetRangeAsyncReq *obj);
    static void registerPool();
};

template <class T> int GetRangeAsyncReq<T>::class_msg_type = eMsgGetRangeAsyncReq;
template <class T> MemMgmt::GeneralPool<GetRangeAsyncReq<T>, MemMgmt::ClassAlloc<GetRangeAsyncReq<T>> > GetRangeAsyncReq<T>::poolT(10, "GetRangeAsyncReq");

template <class T>
GetRangeAsyncReq<T>::GetRangeAsyncReq()
    :IovMsgAsyncReq<T>(static_cast<eMsgType>(GetRangeAsyncReq<T>::class_msg_type))
{}

template <class T>
GetRangeAsyncReq<T>::~GetRangeAsyncReq()
{}

template <class T>
GetRangeAsyncReq<T> *GetRangeAsyncReq<T>::get()
{
    return GetRangeAsyncReq<T>::poolT.get();
}

template <class T>
void GetRangeAsyncReq<T>::put(GetRangeAsyncReq<T> *obj)
{
    GetRangeAsyncReq<T>::poolT.put(obj);
}

template <class T>
void GetRangeAsyncReq<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(GetRangeAsyncReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&GetRangeAsyncReq<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(GetRangeAsyncReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&GetRangeAsyncReq<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// ====

template <class T>
class GetRangeAsyncRsp: public IovMsgAsyncRsp<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<GetRangeAsyncRsp, MemMgmt::ClassAlloc<GetRangeAsyncRsp> > poolT;

public:
    GetRangeAsyncRsp();
    ~GetRangeAsyncRsp();

    static GetRangeAsyncRsp *get();
    static void put(GetRangeAsyncRsp *obj);
    static void registerPool();
};

template <class T> int GetRangeAsyncRsp<T>::class_msg_type = eMsgGetRangeAsyncRsp;
template <class T> MemMgmt::GeneralPool<GetRangeAsyncRsp<T>, MemMgmt::ClassAlloc<GetRangeAsyncRsp<T>> > GetRangeAsyncRsp<T>::poolT(10, "GetRangeAsyncRsp");

template <class T>
GetRangeAsyncRsp<T>::GetRangeAsyncRsp()
    :IovMsgAsyncRsp<T>(static_cast<eMsgType>(GetRangeAsyncRsp<T>::class_msg_type))
{}

template <class T>
GetRangeAsyncRsp<T>::~GetRangeAsyncRsp()
{}

template <class T>
GetRangeAsyncRsp<T> *GetRangeAsyncRsp<T>::get()
{
    return GetRangeAsyncRsp<T>::poolT.get();
}

template <class T>
void GetRangeAsyncRsp<T>::put(GetRangeAsyncRsp<T> *obj)
{
    GetRangeAsyncRsp<T>::poolT.put(obj);
}

template <class T>
void GetRangeAsyncRsp<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(GetRangeAsyncRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&GetRangeAsyncRsp<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(GetRangeAsyncRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&GetRangeAsyncRsp<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// -----------------------------------------------------------------------------------------

template <class T>
class MergeAsyncReq: public IovMsgAsyncReq<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<MergeAsyncReq, MemMgmt::ClassAlloc<MergeAsyncReq> > poolT;

public:
    MergeAsyncReq();
    virtual ~MergeAsyncReq();

    static MergeAsyncReq *get();
    static void put(MergeAsyncReq *obj);
    static void registerPool();
};

template <class T> int MergeAsyncReq<T>::class_msg_type = eMsgMergeAsyncReq;
template <class T> MemMgmt::GeneralPool<MergeAsyncReq<T>, MemMgmt::ClassAlloc<MergeAsyncReq<T>> > MergeAsyncReq<T>::poolT(10, "MergeAsyncReq");

template <class T>
MergeAsyncReq<T>::MergeAsyncReq()
    :IovMsgAsyncReq<T>(static_cast<eMsgType>(MergeAsyncReq<T>::class_msg_type))
{}

template <class T>
MergeAsyncReq<T>::~MergeAsyncReq()
{}

template <class T>
MergeAsyncReq<T> *MergeAsyncReq<T>::get()
{
    return MergeAsyncReq<T>::poolT.get();
}

template <class T>
void MergeAsyncReq<T>::put(MergeAsyncReq<T> *obj)
{
    MergeAsyncReq<T>::poolT.put(obj);
}

template <class T>
void MergeAsyncReq<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(MergeAsyncReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&MergeAsyncReq<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(MergeAsyncReq<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&MergeAsyncReq<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// ====

template <class T>
class MergeAsyncRsp: public IovMsgAsyncRsp<T>
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<MergeAsyncRsp, MemMgmt::ClassAlloc<MergeAsyncRsp> > poolT;

public:
    MergeAsyncRsp();
    virtual ~MergeAsyncRsp();

    static MergeAsyncRsp *get();
    static void put(MergeAsyncRsp *obj);
    static void registerPool();
};

template <class T> int MergeAsyncRsp<T>::class_msg_type = eMsgMergeAsyncRsp;
template <class T> MemMgmt::GeneralPool<MergeAsyncRsp<T>, MemMgmt::ClassAlloc<MergeAsyncRsp<T>> > MergeAsyncRsp<T>::poolT(10, "MergeAsyncRsp");

template <class T>
MergeAsyncRsp<T>::MergeAsyncRsp()
    :IovMsgAsyncRsp<T>(static_cast<eMsgType>(MergeAsyncRsp<T>::class_msg_type))
{}

template <class T>
MergeAsyncRsp<T>::~MergeAsyncRsp()
{}

template <class T>
MergeAsyncRsp<T> *MergeAsyncRsp<T>::get()
{
    return MergeAsyncRsp<T>::poolT.get();
}

template <class T>
void MergeAsyncRsp<T>::put(MergeAsyncRsp *obj)
{
    MergeAsyncRsp<T>::poolT.put(obj);
}

template <class T>
void MergeAsyncRsp<T>::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(MergeAsyncRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&MergeAsyncRsp<T>::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(MergeAsyncRsp<T>::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&MergeAsyncRsp<T>::put));
    } catch (const std::runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
    }
}

// -----------------------------------------------------------------------------------------

template <class T>
class IovAsyncMsgPoolInitializer
{
public:
    static bool initialized;
    IovAsyncMsgPoolInitializer();
    ~IovAsyncMsgPoolInitializer() {}
};

template <class T>
bool IovAsyncMsgPoolInitializer<T>::initialized = false;

template <class T>
IovAsyncMsgPoolInitializer<T>::IovAsyncMsgPoolInitializer()
{
    if ( initialized != true ) {
        GetAsyncReq<T>::registerPool();
        GetAsyncRsp<T>::registerPool();
        PutAsyncReq<T>::registerPool();
        PutAsyncRsp<T>::registerPool();
        DeleteAsyncReq<T>::registerPool();
        DeleteAsyncRsp<T>::registerPool();
        GetRangeAsyncReq<T>::registerPool();
        GetRangeAsyncRsp<T>::registerPool();
        MergeAsyncReq<T>::registerPool();
        MergeAsyncRsp<T>::registerPool();
    }
    initialized = true;
}

}

#endif /* RENTCONTROL_SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGIOVASYNCTYPES_HPP_ */
