/**
 * @file    soefutures.hpp
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
 * soefutures.hpp
 *
 *  Created on: Dec 4, 2017
 *      Author: Jan Lisowiec
 */

#ifndef RENTCONTROL_SOE_SOE_SOE_SOEAPI_INCLUDE_SOEFUTURES_HPP_
#define RENTCONTROL_SOE_SOE_SOE_SOEAPI_INCLUDE_SOEFUTURES_HPP_

/*
 * @brief Future API description
 *
 * Future API consists of a class hierarchy.
 * Soe futures can be created and destroyed only using session API. Sessions provide context
 * for future's invocation and handling. Future's constructors and destructors are protected
 * thus preventing users from creating futures outside of sessions.
 *
 * There are different future classes, depending on the requested operation.
 * Single i/o requests, i.e. GetAsync/PutAsync/DeleteAsync/MergeAsync have corresponding future types
 * as follows:
 * GetAsync -> GetFuture
 * PutAsync -> PutFuture
 * DeletetAsync -> DeleteFuture
 * MergeAsync -> MergeFuture
 *
 * For vectored requests, i.e. GetSetAsync/PutSetAsync/DeleteSetAsync/MergeSetAsync the return future
 * types are the following:
 * GetSetAsync -> GetSetFuture
 * PutSetAsync -> PutSetFuture
 * DeletetSetAsync -> DeleteSetFuture
 * MergeSetAsync -> MergeSetFuture
 *
 * Once a future is created via session's async API, it can be used later on to obtain
 * the status and the result, i.e. key(s) and value(s). Future's API provides Get() method to do that.
 * Get() method will synchronize future with the return status and result, so potentially it's
 * a blocking call. Get() may block the caller if a future has not yet received its result.
 * On vectored requests, Get() will synchronize its future with the results of all the elements of the
 * vector. For example, if the input vector for PutSetAsync() contains 100 elements, the PutSetFuture
 * will be synchronized with the statuses of all individual Put requests, i.e. Get() will return
 * when all of the elements of the input vector have been written and their statuses communicated back
 * to the future object.
 * Like in synchronous vectored requests, where an element of the input vector may contain a duplicate
 * or non-existing key, a vectored future will become available upon encountering the first eDuplicateKey
 * or eNotFoundKey, provided the boolean flag fail_on_first is set to true.
 */

namespace Soe {

/*
 * @brief Future API description
 *
 * SimpleFuture abstract class
 * parent points to aggregate SetFuture class
 *
 */
class SimpleFuture: public Futurable
{
    friend class Session;

    std::shared_ptr<Session>     sess;           // session owning this future
    Futurable                   *parent;         // parent future for this future

public:
    uint64_t                     futures_idxs[2];
    static const uint64_t        invalid_idx = -1;

private:
    const static uint32_t        initial_timeout_value = 20;
    uint32_t                     timeout_value;  // timeout in secs, will be recalculated based on type/size

protected:
    SimpleFuture(std::shared_ptr<Session> _sess, Session::Status _post_ret, uint64_t _seq, Futurable *_par = 0);
    SimpleFuture(std::shared_ptr<Session> _sess, Futurable *_par = 0);
    virtual ~SimpleFuture();

    virtual void Signal(LocalArgsDescriptor *_args);
    virtual void Resolve(LocalArgsDescriptor *_args) = 0;

public:
    virtual Session::Status Get();

    virtual const char *GetKey() = 0;
    virtual size_t GetKeySize() = 0;
    virtual Duple GetDupleKey() = 0;

    virtual const char *GetValue() = 0;
    virtual size_t GetValueSize() = 0;
    virtual Duple GetDupleValue() = 0;

    virtual const std::string GetPath();

protected:
    virtual void SetParent(Futurable *par);
    virtual void Reset();
    virtual void SetTimeoutValueFactor(uint32_t _fac);

public:
    virtual Futurable *GetParent();
};

/*
 * @brief Future API description
 *
 * PutFuture concrete class
 *
 */
class PutFuture: public SimpleFuture
{
    friend class Session;

    Duple      key;       // key passed in by the caller
    Duple      value;     // value passed in by the caller

protected:
    PutFuture(std::shared_ptr<Session> _sess, Session::Status _post_ret, uint64_t _seq, Futurable *_par = 0);
    PutFuture(std::shared_ptr<Session> _sess, Futurable *_par = 0);
    virtual ~PutFuture();
    virtual void Resolve(LocalArgsDescriptor *_args);

public:
    virtual const char *GetKey();
    virtual size_t GetKeySize();
    virtual Duple GetDupleKey();

    virtual const char *GetValue();
    virtual size_t GetValueSize();
    virtual Duple GetDupleValue();

    virtual Session::Status Status();

protected:
    void SetKey(Duple &_key);
    void SetValue(Duple &_value);
    virtual void Reset();
};

/*
 * @brief Future API description
 *
 * GetFuture concrete class
 *
 */
class GetFuture: public SimpleFuture
{
    friend class Session;

    Duple      key;       // key passed in by the caller
    Duple      value;     // holds a buffer passed in by the caller

protected:
    GetFuture(std::shared_ptr<Session> _sess, Session::Status _post_ret, uint64_t _seq, Futurable *_par = 0, char *_buf = 0, size_t _buf_size = 0);
    GetFuture(std::shared_ptr<Session> _sess, Futurable *_par = 0, char *_buf = 0, size_t _buf_size = 0);
    virtual ~GetFuture();
    virtual void Resolve(LocalArgsDescriptor *_args);

public:
    virtual const char *GetKey();
    virtual size_t GetKeySize();
    virtual Duple GetDupleKey();

    virtual const char *GetValue();
    virtual size_t GetValueSize();
    virtual Duple GetDupleValue();

    virtual Session::Status Status();

protected:
    void SetKey(Duple &_key);
    void SetValue(Duple &_value);
    virtual void Reset();
};

/*
 * @brief Future API description
 *
 * DeleteFuture concrete class
 *
 */
class DeleteFuture: public SimpleFuture
{
    friend class Session;

    Duple      key;       // key passed in by the caller

protected:
    DeleteFuture(std::shared_ptr<Session> _sess, Session::Status _post_ret, uint64_t _seq, Futurable *_par = 0);
    DeleteFuture(std::shared_ptr<Session> _sess, Futurable *_par = 0);
    virtual ~DeleteFuture();
    virtual void Resolve(LocalArgsDescriptor *_args);

public:
    virtual const char *GetKey();
    virtual size_t GetKeySize();
    virtual Duple GetDupleKey();

    virtual const char *GetValue();
    virtual size_t GetValueSize();
    virtual Duple GetDupleValue();

    virtual Session::Status Status();

protected:
    void SetKey(Duple &_key);
    virtual void Reset();
};

/*
 * @brief Future API description
 *
 * MergeFuture concrete class
 *
 */
class MergeFuture: public SimpleFuture
{
    friend class Session;

    Duple      key;       // key passed in by the caller
    Duple      value;     // holds a buffer passed in by the caller

protected:
    MergeFuture(std::shared_ptr<Session> _sess, Session::Status _post_ret, uint64_t _seq, Futurable *_par = 0);
    MergeFuture(std::shared_ptr<Session> _sess, Futurable *_par = 0);
    virtual ~MergeFuture();
    virtual void Resolve(LocalArgsDescriptor *_args);

public:
    virtual const char *GetKey();
    virtual size_t GetKeySize();
    virtual Duple GetDupleKey();

    virtual const char *GetValue();
    virtual size_t GetValueSize();
    virtual Duple GetDupleValue();

    virtual Session::Status Status();

protected:
    void SetKey(Duple &_key);
    void SetValue(Duple &_value);
    virtual void Reset();
};




/*
 * @brief Future API description
 *
 * SetFuture abstract class, base for any Set future
 *
 */
class SetFuture: public Futurable
{
    friend class Session;
    friend class Futurable;
    friend class SimpleFuture;

    std::shared_ptr<Session>         sess;          // session owning this future

protected:
    std::vector<Futurable *>         futures;       // vector of children futures each of which will be signaled and validated

private:
    size_t                           num_requested; // number of futures requested (should be equal to size of futures)
    size_t                           num_received;  // number of received futures
    size_t                           num_processed; // number of received futures regardless of status
    bool                             fail_detect;   // if errors should detected and failing the SetFuture
    size_t                           fail_pos;      // failed position

    const static size_t              invalid_fail_pos = 2000000000;

    const static uint32_t            initial_timeout_value = 60;
    uint32_t                         timeout_value; // timeout in secs, will be recalculated based on type/size

protected:
    SetFuture(std::shared_ptr<Session> _sess, Session::Status _post_ret, bool _fail_detect = false, size_t *_fail_pos = 0);
    virtual ~SetFuture();

protected:
    virtual void Signal(LocalArgsDescriptor *_args);

public:
    virtual Session::Status Get();
    virtual Session::Status Status();

protected:
    virtual void SetParent(Futurable *par);
    void IncNumReceived(LocalArgsDescriptor *_args);
    void SetFuturesSize(size_t _size);
    virtual bool Validate(SeqNumber seq, const uint64_t *_idxs, LocalArgsDescriptor *_args = 0) = 0;
    virtual void Resolve(LocalArgsDescriptor *_args);

public:
    virtual Futurable *GetParent();
    bool GetFailDetect();
    uint32_t GetFailPos();
    void SetNumRequested(size_t _num_requested);
    std::shared_ptr<Session>  GetSession()
    {
        return sess;
    }

protected:
    size_t GetNumRequested();
    size_t GetNumReceived();
    size_t GetNumProcessed();
    virtual void Reset();
    virtual void SetTimeoutValueFactor(uint32_t _fac);

    uint64_t *InsertFuture(Futurable &_fut, size_t pos);
};


/*
 * @brief Future API description
 *
 * GetSetFuture concrete class
 *
 */
class GetSetFuture: public SetFuture
{
    friend class Session;
    friend class Futurable;
    friend class SimpleFuture;
    friend class SetFuture;

    std::vector<std::pair<Duple, Duple>> &items;

protected:
    GetSetFuture(std::shared_ptr<Session> _sess, std::vector<std::pair<Duple, Duple>> &_items, Session::Status _post_ret, bool _fail_detect = false, size_t *_fail_pos = 0);
    virtual ~GetSetFuture();
    virtual bool Validate(SeqNumber seq, const uint64_t *_idxs, LocalArgsDescriptor *_args = 0);
    virtual void Resolve(LocalArgsDescriptor *_args);
    virtual void Reset();

public:
    std::vector<std::pair<Duple, Duple>> &GetItems();
};


/*
 * @brief Future API description
 *
 * PutSetFuture concrete class
 *
 */
class PutSetFuture: public SetFuture
{
    friend class Session;
    friend class Futurable;
    friend class SimpleFuture;
    friend class SetFuture;

    std::vector<std::pair<Duple, Duple>> &items;

protected:
    PutSetFuture(std::shared_ptr<Session> _sess, std::vector<std::pair<Duple, Duple>> &_items, Session::Status _post_ret, bool _fail_detect = false, size_t *_fail_pos = 0);
    virtual ~PutSetFuture();
    virtual bool Validate(SeqNumber seq, const uint64_t *_idxs, LocalArgsDescriptor *_args = 0);
    virtual void Resolve(LocalArgsDescriptor *_args);
    virtual void Reset();

public:
    std::vector<std::pair<Duple, Duple>> &GetItems();
};


/*
 * @brief Future API description
 *
 * DeleteSetFuture concrete class
 *
 */
class DeleteSetFuture: public SetFuture
{
    friend class Session;
    friend class Futurable;
    friend class SimpleFuture;
    friend class SetFuture;

    std::vector<Duple> &keys;

protected:
    DeleteSetFuture(std::shared_ptr<Session> _sess, std::vector<Duple> &_keys, Session::Status _post_ret, bool _fail_detect = false, size_t *_fail_pos = 0);
    virtual ~DeleteSetFuture();
    virtual bool Validate(SeqNumber seq, const uint64_t *_idxs, LocalArgsDescriptor *_args = 0);
    virtual void Resolve(LocalArgsDescriptor *_args);
    virtual void Reset();

public:
    std::vector<Duple> &GetKeys();
};


/*
 * @brief Future API description
 *
 * MergeSetFuture concrete class
 *
 */
class MergeSetFuture: public SetFuture
{
    friend class Session;
    friend class Futurable;
    friend class SimpleFuture;
    friend class SetFuture;

    std::vector<std::pair<Duple, Duple>> &items;

protected:
    MergeSetFuture(std::shared_ptr<Session> _sess, std::vector<std::pair<Duple, Duple>> &_items, Session::Status _post_ret, bool _fail_detect = false, size_t *_fail_pos = 0);
    virtual ~MergeSetFuture();
    virtual bool Validate(SeqNumber seq, const uint64_t *_idxs, LocalArgsDescriptor *_args = 0);
    virtual void Resolve(LocalArgsDescriptor *_args);
    virtual void Reset();

public:
    std::vector<std::pair<Duple, Duple>> &GetItems();
};

/*
 * @brief Future API description
 *
 * BackupFuture concrete class
 *
 */
class BackupFuture: public SimpleFuture
{
    friend class Session;
    friend class Manager;

    std::string     backup_path;       // where the backup will be cretaed

protected:
    BackupFuture(std::shared_ptr<Session> _sess, Session::Status _post_ret, uint64_t _seq, const std::string &_path, Futurable *_par = 0);
    BackupFuture(std::shared_ptr<Session> _sess, Futurable *_par = 0);
    virtual ~BackupFuture();
    virtual void Resolve(LocalArgsDescriptor *_args);

public:
    virtual const char *GetKey();
    virtual size_t GetKeySize();
    virtual Duple GetDupleKey();

    virtual const char *GetValue();
    virtual size_t GetValueSize();
    virtual Duple GetDupleValue();

    virtual const std::string GetPath();
    virtual Session::Status Status();

protected:
    void SetPath(const std::string &_path);
    virtual void Reset();
};

}

#endif /* RENTCONTROL_SOE_SOE_SOE_SOEAPI_INCLUDE_SOEFUTURES_HPP_ */
