/**
 * @file    soefuturable.hpp
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
 * soefuturable.hpp
 *
 *  Created on: Dec 4, 2017
 *      Author: Jan Lisowiec
 */

#ifndef RENTCONTROL_SOE_SOE_SOE_SOEAPI_INCLUDE_SOEFUTURABLE_HPP_
#define RENTCONTROL_SOE_SOE_SOE_SOEAPI_INCLUDE_SOEFUTURABLE_HPP_

namespace Soe {

//#define __DEBUG_FUTURES__
#ifdef __DEBUG_FUTURES__
#define DEBUG_FUTURES(a) \
{ \
    WriteLock io_w_lock(Futurable::debug_io_lock); \
    cout << a; \
}
#else
#define DEBUG_FUTURES(a)
#endif

void SignalFuture(RcsdbFacade::FutureSignal *sig);

class Futurable
{
    friend class Session;
    friend class SimpleFuture;
    friend class SetFuture;
    friend class GetFuture;
    friend class PutFuture;
    friend class DeleteFuture;
    friend class MergeFuture;
    friend class GetSetFuture;
    friend class PutSetFuture;
    friend class DeleteSetFuture;
    friend class MergeSetFuture;

public:
    const static uint32_t        extra_check_value = 0xc5c5c5c5;
    uint32_t                     extra_check;


#ifdef __DEBUG_FUTURES__
    static Lock                  debug_io_lock;
    static Lock                  futurables_lock;
    static std::vector<uint64_t> futurables;
#endif

private:
    Session::Status              post_ret;
    SeqNumber                    seq;
    bool                         received;
    boost::promise<int>          prom;

protected:
    Futurable(Session::Status _post_ret = Session::Status::eOk, SeqNumber _seq = 0)
        :extra_check(Futurable::extra_check_value),
         post_ret(_post_ret),
         seq(_seq),
         received(false)
    {
#ifdef __DEBUG_FUTURES__
        WriteLock io_w_lock(Futurable::futurables_lock);
        Futurable::futurables.push_back(reinterpret_cast<uint64_t>(this));
#endif
    }

    void SetPostRet(Session::Status _post_ret)
    {
        post_ret = _post_ret;
    }

    void ResetExtraCheck()
    {
        extra_check = 0;
    }

public:
    virtual ~Futurable() {}
    virtual Session::Status Get() = 0;
    virtual void Signal(LocalArgsDescriptor *_args);
    Session::Status GetPostRet()
    {
        return post_ret;
    }
    virtual Session::Status Status() = 0;

protected:
    virtual void Reset() = 0;
    void SetSeq(SeqNumber _seq)
    {
        seq = _seq;
    }
    SeqNumber GetSeq()
    {
        return seq;
    }
    boost::promise<int> &GetPromise()
    {
        return prom;
    }
    void SetReceived()
    {
        received = true;
    }
    bool GetReceived()
    {
        return received;
    }

    virtual void SetParent(Futurable *par) = 0;
    virtual void SetTimeoutValueFactor(uint32_t _fac) = 0;

public:
    virtual Futurable *GetParent() = 0;
};

inline void Futurable::Signal(LocalArgsDescriptor *_args)
{
    //std::cout << "Futurable::" << __FUNCTION__ << std::endl;
}

}

#endif /* RENTCONTROL_SOE_SOE_SOE_SOEAPI_INCLUDE_SOEFUTURABLE_HPP_ */
