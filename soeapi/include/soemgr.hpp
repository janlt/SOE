/**
 * @file    soemgr.hpp
 * @author  Jan Lisowiec <jlisowiec@gmail.com>
 * @version 1.0
 * @brief   Soe Manager API
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
 * soemgr.hpp
 *
 *  Created on: Sep 20, 2018
 *      Author: Jan Lisowiec
 */

#ifndef RENTCONTROL_SOE_SOE_SOE_SOEAPI_INCLUDE_SOEMGR_HPP_
#define RENTCONTROL_SOE_SOE_SOE_SOEAPI_INCLUDE_SOEMGR_HPP_

namespace Soe {

class Session;
class Provider;
class Statistic;

class Manager
{
private:
    Manager() = default;
    ~Manager() = default;

    static Lock        instance_lock;
    static Manager    *instance;

protected:
    struct State
    {
        typedef enum _eState {
            eIdle        = 0,
            eInProgress  = 1
        } eState;

        eState state = { State::eState::eIdle };
        Lock   state_lock;

        State() = default;
        ~State() = default;

        eState SetInProgress();
        eState SetIdle();
    };

    State            state;
    Session         *discovery_sess = { 0 };
    std::string      backup_restore_dir;
    std::string      provider = { Provider::default_provider };
    bool             incremental = { false };
    const uint64_t   invalid_id = -1;

    Session *&getDiscoverySess();

protected:
    Session::Status StartSession(const std::string &cl,
        uint64_t cl_id,
        const std::string &sp,
        uint64_t sp_id,
        const std::string &st,
        uint64_t st_id,
        const std::string &pr,
        Session *&_sess);
    void EndSession(Session *&_sess);
    void DestroySession(Session *&_sess);
    Session::Status BackupStores(bool list_only, const std::string &discov_store_name);
    Session::Status BackupStore(const std::string &cl, uint64_t cl_id, const std::string &sp, uint64_t sp_id, const std::string &st, uint64_t st_id);
    Session::Status RestoreStores(bool list_only, const std::string &_pr, bool _overwrite, const std::string &discov_store_name);
    Session::Status RestoreStore(Session *restore_sess, const std::string &cl, uint64_t cl_id, const std::string &sp, uint64_t sp_id, const std::string &st, uint64_t st_id, const std::string &_pr, bool _overwrite = true);
    Session::Status DropStoreBackups(bool list_only, const std::string &discov_store_name);
    Session::Status DropStoreBackup();
    Session::Status VerifyBackups(const std::string &discov_store_name);
    Session::Status VerifyBackup(const std::string &cl, uint64_t cl_id, const std::string &sp, uint64_t sp_id, const std::string &st, uint64_t st_id);

    void SetBackupRestoreDir(const std::string &_bckp_dir);
    const std::string &GetBackupRestoreDir();

    void SetProvider(const std::string &_pr);
    const std::string &GetProvider();

    void SetIncremental(bool _incremental);
    bool GetIncremental();

public:
    static Manager *getManager();
    static void putManager();

    Session::Status GetRootPath(std::string &root_path, const std::string &pr = Provider::default_provider);
    Session::Status GetStatistic(Statistic *stats, const std::string &pr = Provider::default_provider);

    Session::Status Backup(const std::string &path, const std::string &pr = Provider::default_provider, bool _incremental = true, bool _list_only = false);
    BackupFuture *AsyncBackup(const std::string &path, const std::string &pr = Provider::default_provider, bool incremental = true, Futurable *_par = 0);
    Session::Status Restore(const std::string &path, const std::string &pr = Provider::default_provider, bool _overwrite = true, bool _list_only = false);
    BackupFuture *AsyncRestore(const std::string &path, const std::string &pr = Provider::default_provider, bool _overwrite = true, Futurable *_par = 0);
    Session::Status ListDbs(std::ostringstream &str_buf, const std::string &pr = Provider::default_provider);
    BackupFuture *AsyncListDbs(std::ostringstream &str_buf, const std::string &pr = Provider::default_provider, Futurable *_par = 0);
    Session::Status ListBackupDbs(std::string &root_path, std::ostringstream &str_buf, const std::string &pr = Provider::default_provider);
    BackupFuture *AsyncListBackupDbs(std::string &root_path, std::ostringstream &str_buf, const std::string &pr = Provider::default_provider, Futurable *_par = 0);
    Session::Status Verify(const std::string &path, const std::string &pr = Provider::default_provider);
    BackupFuture *AsyncVerify(const std::string &path, const std::string &pr = Provider::default_provider, Futurable *_par = 0);
    Session::Status Drop(const std::string &path, const std::string &pr = Provider::default_provider, bool _list_only = false);
    BackupFuture *AsyncDrop(const std::string &path, const std::string &pr = Provider::default_provider, Futurable *_par = 0);

    void logDbgMsg(const char *msg) const;
    void logInfoMsg(const char *msg) const;
    void logErrMsg(const char *msg) const;
};

}

#endif /* RENTCONTROL_SOE_SOE_SOE_SOEAPI_INCLUDE_SOEMGR_HPP_ */
