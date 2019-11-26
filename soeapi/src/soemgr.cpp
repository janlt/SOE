/**
 * @file    soemgr.cpp
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
 * soemgr.cpp
 *
 *  Created on: Sep 20, 2018
 *      Author: Jan Lisowiec
 */

#include <sys/syscall.h>
#include <string.h>
#include <dlfcn.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <new>

#include "soeutil.h"
#include "soe_soe.hpp"

#include "soestatistics.hpp"
#include "soemgr.hpp"

using namespace std;

namespace Soe {

Lock Manager::instance_lock;
Manager *Manager::instance = 0;

Manager *Manager::getManager()
{
    WriteLock w_lock(Manager::instance_lock);
    if ( !instance ) {
        instance = new Manager();
    }
    return instance;
}

void Manager::putManager()
{
    WriteLock w_lock(Manager::instance_lock);
    if ( instance ) {
        delete instance;
        instance = 0;
    }
}

void Manager::SetBackupRestoreDir(const string &_bckp_dir)
{
    backup_restore_dir = _bckp_dir;
}

const string &Manager::GetBackupRestoreDir()
{
    return backup_restore_dir;
}

void Manager::SetProvider(const string &_pr)
{
    provider = _pr;
}

const string &Manager::GetProvider()
{
    return provider;
}

void Manager::SetIncremental(bool _incremental)
{
    incremental = _incremental;
}

bool Manager::GetIncremental()
{
    return incremental;
}

Manager::State::eState Manager::State::SetInProgress()
{
    WriteLock w_lock(Manager::State::state_lock);
    if ( state == Manager::State::eState::eIdle) {
        state = Manager::State::eState::eInProgress;
    }
    return state;
}

Manager::State::eState Manager::State::SetIdle()
{
    WriteLock w_lock(Manager::State::state_lock);
    state = Manager::State::eState::eIdle;
    return state;
}


Session *&Manager::getDiscoverySess()
{
    return discovery_sess;
}

/**
 * @brief Manager GetRootPath
 *
 * Get root path for a given provider on a local machine
 *
 * @param[out] root_path      Soe root path for a given provider on a local machine
 * @param[in]  pr             Provider
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Manager::GetRootPath(string &root_path, const string &pr)
{
    return Session::Status::eOk;
}

/**
 * @brief Manager GetStatistics
 *
 * Get statistics for a specified type (derived from Statistic) for a given provider
 *
 * @param[in/out] stats          Statistic object of a derived type from Statistic
 * @param[in]     pr             Provider
 * @param[out]    status         return status (see soesession.hpp)
 */
Session::Status Manager::GetStatistic(Statistic *stats, const string &pr)
{
    return Session::Status::eOk;
}

Session::Status Manager::StartSession(const string &cl,
    uint64_t cl_id,
    const string &sp,
    uint64_t sp_id,
    const string &st,
    uint64_t st_id,
    const string &pr,
    Session *&_sess)
{
    Session::Status sts = Session::Status::eOk;
    try {
        _sess = new Session(cl, sp, st, pr, false, false, true, Session::eSyncDefault);
        sts = _sess->Open(cl, sp, st, pr, false, false, true, Session::eSyncDefault);
        if ( sts != Session::Status::eOk ) {
            sts = _sess->Create();
        }
    } catch (const std::runtime_error &ex) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " runtime_error exception caught when creating backup session: " << ex.what() << endl << flush;
        logErrMsg(str_buf.str().c_str());
        sts = Session::Status::eInternalError;
    } catch ( ... ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " unknown exception caught when creating backup session: " << endl << flush;
        logErrMsg(str_buf.str().c_str());
        sts = Session::Status::eInternalError;
    }

    if ( sts != Session::Status::eOk ) {
        delete _sess;
        return sts;
    }

    return sts;
}

void Manager::EndSession(Session *&_sess)
{
    if ( _sess ) {
        delete _sess;
        _sess = 0;
    }
}

void Manager::DestroySession(Session *&_sess)
{
    if ( _sess ) {
        _sess->DestroyStore();
        delete _sess;
        _sess = 0;
    }
}

/**
 * @brief Manager BackupStore
 *
 * Single Store Backup
 *
 * @param[in]  cl             Cluster
 * @param[in]  cl_id          Cluster id
 * @param[in]  sp             Space
 * @param[in]  sp_id          Space id
 * @param[in]  st             Store
 * @param[in]  st_id          Store id
 * @param[out] status         Return status (see soesession.hpp)
 *
 */
Session::Status Manager::BackupStore(const string &cl,
    uint64_t cl_id,
    const string &sp,
    uint64_t sp_id,
    const string &st,
    uint64_t st_id)
{
    Session *new_sess = 0;
    Session::Status sts = getManager()->StartSession(cl, cl_id, sp, sp_id, st, st_id, provider, new_sess);
    if ( sts != Session::Status::eOk ) {
        return sts;
    }
    assert(new_sess);

    Soe::BackupEngine *bckp_engine = new_sess->CreateBackupEngine((string("BackupEngine_") + st).c_str(),
        backup_restore_dir.empty() ? "" : backup_restore_dir.c_str(), incremental);
    if ( !bckp_engine ) {
        sts = new_sess->GetLastStatus();
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " Error(" << Session::StatusToText(sts) << ") creating backup engine" << endl << flush;
        logErrMsg(str_buf.str().c_str());
        getManager()->EndSession(new_sess);
        return sts;
    } else {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " created backup engine(" << bckp_engine->GetName() <<
            ") root dir(" << bckp_engine->GetRootDir() << ") for store backup" << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    sts = bckp_engine->CreateBackup();
    if ( sts != Session::Status::eOk ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " Error(" << Session::StatusToText(sts) << ") creating backup(" << st << ")" << endl << flush;
        logErrMsg(str_buf.str().c_str());
    } else {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " created store backup using backup_engine(" << bckp_engine->GetName() << ")" << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    new_sess->DestroyBackupEngine(bckp_engine);
    getManager()->EndSession(new_sess);

    return sts;
}

/**
 * @brief Manager BackupStores
 *
 * Multiple Store Backup
 *
 * @param[in]  list_only              List only
 * @param[in]  discovery_store_name   Name of the discovery store
 * @param[out] status                 Return status (see soesession.hpp)
 *
 */
Session::Status Manager::BackupStores(bool list_only, const string &discov_store_name)
{
    std::vector<std::pair<uint64_t, string> > clusters;
    Session::Status c_sts;

    try {
        c_sts = discovery_sess->ListClusters(clusters);

        for ( auto i : clusters ) {
            std::vector<std::pair<uint64_t, string> > spaces;

            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Discovered cluster(" << i.second << "/" << i.first << ")" << endl << flush;
            logInfoMsg(str_buf.str().c_str());

            c_sts = discovery_sess->ListSpaces(i.second, i.first, spaces);

            for ( auto j : spaces ) {
                std::vector<std::pair<uint64_t, string> > stores;

                ostringstream str_buf;
                str_buf << __FUNCTION__ << ":" << __LINE__ << " Discovered space(" << j.second << "/" << j.first << ")" << endl << flush;
                logInfoMsg(str_buf.str().c_str());

                c_sts = discovery_sess->ListStores(i.second, i.first, j.second, j.first, stores);

                for ( auto k : stores ) {
                    if ( list_only == true ) {
                        ostringstream str_buf;
                        str_buf << __FUNCTION__ << ":" << __LINE__ << " Discovered store(" << k.second << "/" << k.first << ")" << endl << flush;
                        logInfoMsg(str_buf.str().c_str());
                        continue;
                    }

                    if ( k.second == discov_store_name ) {
                        continue;
                    }

                    ostringstream str_buf;
                    str_buf << __FUNCTION__ << ":" << __LINE__ << " Backing up store(" << k.second << "/" << k.first << ")" << endl << flush;
                    logInfoMsg(str_buf.str().c_str());

                    c_sts = BackupStore(i.second, i.first, j.second, j.first, k.second, k.first);
                    if ( c_sts != Session::Status::eOk ) {
                        ostringstream str_buf;
                        str_buf << __FUNCTION__ << ":" << __LINE__ << " Error(" << Session::StatusToText(c_sts) << ") backing up store(" << k.second << "/" << k.first <<
                            ") space(" << j.second << "/" << j.first << ") cluster(" << i.second <<
                            "/" << i.first << ")" << endl << flush;
                        logErrMsg(str_buf.str().c_str());
                    } else {
                        ostringstream str_buf;
                        str_buf << __FUNCTION__ << ":" << __LINE__ << " backed up store(" << k.second << "/" << k.first <<
                            ") space(" << j.second << "/" << j.first << ") cluster(" << i.second <<
                            "/" << i.first << ") on(" <<
                            backup_restore_dir << "/" << i.second << "/" << j.second << "/" << k.second << ")" << endl << flush;
                        logInfoMsg(str_buf.str().c_str());
                    }
                }
            }
        }
    } catch (const std::runtime_error &ex) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " runtime_error exception caught when backing up stores: " << ex.what() << endl << flush;
        logErrMsg(str_buf.str().c_str());
        c_sts = Session::Status::eInternalError;
    } catch ( ... ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " unknown exception caught when backing up stores: " << endl << flush;
        logErrMsg(str_buf.str().c_str());
        c_sts = Session::Status::eInternalError;
    }

    return c_sts;
}

/**
 * @brief Manager RestoreStore
 *
 * Restore Single Store
 *
 * @param[in]  cl             Cluster
 * @param[in]  cl_id          Cluster id
 * @param[in]  sp             Space
 * @param[in]  sp_id          Space id
 * @param[in]  st             Store
 * @param[in]  st_id          Store id
 * @param[out] status         Return status (see soesession.hpp)
 *
 */
Session::Status Manager::RestoreStore(Session *restore_sess,
    const string &cl,
    uint64_t cl_id,
    const string &sp,
    uint64_t sp_id,
    const string &st,
    uint64_t st_id,
    const string &_pr,
    bool _overwrite)
{
    assert(restore_sess);

    // check if the database exists and create an empty one if it doesn't
    Session *check_sess = 0;
    Session::Status c_sts = getManager()->StartSession(cl, Manager::invalid_id,
        sp, Manager::invalid_id,
        st, Manager::invalid_id,
        _pr,
        check_sess);
    if ( c_sts != Session::Status::eOk ) {
        return c_sts;
    }
    assert(check_sess);
    getManager()->EndSession(check_sess);

    Session::Status sts;
    string full_backup_path = backup_restore_dir + "/" + cl + "/" + sp + "/" + st + "/";
    Soe::BackupEngine *bckp_engine = restore_sess->CreateBackupEngine((string("BackupEngine_XXXX_") + cl + "_" + sp + "_" + st).c_str(), full_backup_path.c_str(), true);
    if ( !bckp_engine ) {
        sts = restore_sess->GetLastStatus();
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " Error(" << Session::StatusToText(sts) << ") creating restore engine" << endl << flush;
        logErrMsg(str_buf.str().c_str());
        return sts;
    } else {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " created restore engine(" << bckp_engine->GetName() <<
            ") root dir(" << bckp_engine->GetRootDir() << ") for store restore" << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    sts = bckp_engine->RestoreBackup(cl, sp, st, _overwrite);
    if ( sts != Session::Status::eOk ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " Error(" << Session::StatusToText(sts) << ") restoring store(" << st << ") from backup_dir(" << backup_restore_dir << ")" << endl << flush;
        logErrMsg(str_buf.str().c_str());
    } else {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " restored store using backup_engine(" << bckp_engine->GetName() << ") from backup_dir(" << backup_restore_dir << ")" << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    restore_sess->DestroyBackupEngine(bckp_engine);
    return sts;
}

/**
 * @brief Manager RestoreStores
 *
 * Restore Multiple Stores
 *
 * @param[in]  list_only              List only
 * @param[in]  discovery_store_name   Name of the discovery store
 * @param[out] status                 Return status (see soesession.hpp)
 *
 */
Session::Status Manager::RestoreStores(bool list_only, const string &_pr, bool _overwrite, const string &discov_store_name)
{
    std::vector<std::pair<uint64_t, string> > clusters;
    Session::Status c_sts;

    try {
        c_sts = discovery_sess->ListClusters(clusters, backup_restore_dir);

        for ( auto i : clusters ) {
            if ( i.second == discov_store_name ) {
                continue;
            }

            std::vector<std::pair<uint64_t, string> > spaces;

            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << "Restoring cluster(" << i.second << "/" << i.first << ")" << endl << flush;
            logInfoMsg(str_buf.str().c_str());

            c_sts = discovery_sess->ListSpaces(i.second, i.first, spaces, backup_restore_dir);

            for ( auto j : spaces ) {
                if ( j.second == discov_store_name ) {
                    continue;
                }

                std::vector<std::pair<uint64_t, string> > stores;

                ostringstream str_buf;
                str_buf << __FUNCTION__ << ":" << __LINE__ << "    Restoring space(" << j.second << "/" << j.first << ")" << endl << flush;
                logInfoMsg(str_buf.str().c_str());

                c_sts = discovery_sess->ListStores(i.second, i.first, j.second, j.first, stores, backup_restore_dir);

                for ( auto k : stores ) {
                    if ( k.second == discov_store_name ) {
                        continue;
                    }

                    ostringstream str_buf;
                    str_buf << __FUNCTION__ << ":" << __LINE__ << "        Restoring from backup store(" << k.second << "/" << k.first << ")" << endl << flush;
                    logInfoMsg(str_buf.str().c_str());

                    c_sts = RestoreStore(discovery_sess, i.second, i.first, j.second, j.first, k.second, k.first, _pr, _overwrite);
                    if ( c_sts != Session::Status::eOk ) {
                        ostringstream str_buf;
                        str_buf << __FUNCTION__ << ":" << __LINE__ << " Error(" << Session::StatusToText(c_sts) << ") restoring from backup store(" << k.second << "/" << k.first <<
                            ") space(" << j.second << "/" << j.first << ") cluster(" << i.second <<
                            "/" << i.first << ")" << endl << flush;
                        logErrMsg(str_buf.str().c_str());
                    } else {
                        ostringstream str_buf;
                        str_buf << __FUNCTION__ << ":" << __LINE__ << " restored from backup store(" << k.second << "/" << k.first <<
                            ") space(" << j.second << "/" << j.first << ") cluster(" << i.second <<
                            "/" << i.first << ") on(" <<
                            backup_restore_dir << "/" << i.second << "/" << j.second << "/" << k.second << ")" << endl << flush;
                        logInfoMsg(str_buf.str().c_str());
                    }
                }
            }
        }
    } catch (const std::runtime_error &ex) {
        ostringstream str_buf;
        str_buf << "runtime_error exception caught when restoring from backup: " << ex.what() << endl;
        logErrMsg(str_buf.str().c_str());
        c_sts = Session::Status::eInternalError;
    } catch ( ... ) {
        ostringstream str_buf;
        str_buf << "unknown exception caught when restoring from backup: " << endl;
        logErrMsg(str_buf.str().c_str());
        c_sts = Session::Status::eInternalError;
    }

    return c_sts;
}

/**
 * @brief Manager DropStoreBackup
 *
 * Drop Store Backup
 *
 * @param[in]  cl             Cluster
 * @param[in]  cl_id          Cluster id
 * @param[in]  sp             Space
 * @param[in]  sp_id          Space id
 * @param[in]  st             Store
 * @param[in]  st_id          Store id
 * @param[out] status         Return status (see soesession.hpp)
 *
 */
Session::Status Manager::DropStoreBackup()
{
    Session::Status sts;
    Soe::BackupEngine *bckp_engine = discovery_sess->CreateBackupEngine("BackupEngine_Drop",
        backup_restore_dir.empty() ? "" : backup_restore_dir.c_str(), incremental);
    if ( !bckp_engine ) {
        sts = discovery_sess->GetLastStatus();
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " Error(" << Session::StatusToText(sts) << ") creating drop engine" << endl << flush;
        logErrMsg(str_buf.str().c_str());
        return sts;
    } else {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " created drop engine(" << bckp_engine->GetName() <<
            ") root dir(" << bckp_engine->GetRootDir() << ")" << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    sts = bckp_engine->DestroyBackup();
    if ( sts != Session::Status::eOk ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " Error(" << Session::StatusToText(sts) << ") dropping backup(" << sts << ")" << endl << flush;
        logErrMsg(str_buf.str().c_str());
    } else {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " dropped backup(" << GetBackupRestoreDir() << ")" << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    discovery_sess->DestroyBackupEngine(bckp_engine);
    return sts;
}

/**
 * @brief Manager DropStoreBackups
 *
 * Drop Multiple Store Backups
 *
 * @param[in]  list_only              List only
 * @param[in]  discovery_store_name   Name of the discovery store
 * @param[out] status                 Return status (see soesession.hpp)
 *
 */
Session::Status Manager::DropStoreBackups(bool list_only, const string &discov_store_name)
{
    std::vector<std::pair<uint64_t, string> > clusters;
    Session::Status c_sts;

    try {
        c_sts = discovery_sess->ListClusters(clusters);

        c_sts = DropStoreBackup();
        if ( c_sts != Session::Status::eOk ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Error(" << Session::StatusToText(c_sts) << ") dropping backup(" << GetBackupRestoreDir() << ")" << endl << flush;
            logErrMsg(str_buf.str().c_str());
        } else {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " dropped backup(" << GetBackupRestoreDir() << ")"  << endl << flush;
            logInfoMsg(str_buf.str().c_str());
        }
    } catch (const std::runtime_error &ex) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " runtime_error exception caught when dropping backup: " << ex.what() << endl << flush;
        logErrMsg(str_buf.str().c_str());
        c_sts = Session::Status::eInternalError;
    } catch ( ... ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " unknown exception caught when dropping backup: " << endl << flush;
        logErrMsg(str_buf.str().c_str());
        c_sts = Session::Status::eInternalError;
    }

    return c_sts;
}

/**
 * @brief Manager VerifyBackups
 *
 * Verify Multiple Store Backups
 *
 * @param[in]  list_only              List only
 * @param[in]  discovery_store_name   Name of the discovery store
 * @param[out] status                 Return status (see soesession.hpp)
 *
 */
Session::Status Manager::VerifyBackups(const string &discov_store_name)
{
    std::vector<std::pair<uint64_t, string> > clusters;
    Session::Status c_sts;

    try {
        c_sts = discovery_sess->ListClusters(clusters, backup_restore_dir);

        for ( auto i : clusters ) {
            if ( i.second == discov_store_name ) {
                continue;
            }

            std::vector<std::pair<uint64_t, string> > spaces;

            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << "Verifying cluster(" << i.second << "/" << i.first << ")" << endl << flush;
            logInfoMsg(str_buf.str().c_str());

            c_sts = discovery_sess->ListSpaces(i.second, i.first, spaces, backup_restore_dir);

            for ( auto j : spaces ) {
                if ( j.second == discov_store_name ) {
                    continue;
                }

                std::vector<std::pair<uint64_t, string> > stores;

                ostringstream str_buf;
                str_buf << __FUNCTION__ << ":" << __LINE__ << "    Verifying space(" << j.second << "/" << j.first << ")" << endl << flush;
                logInfoMsg(str_buf.str().c_str());

                c_sts = discovery_sess->ListStores(i.second, i.first, j.second, j.first, stores, backup_restore_dir);

                for ( auto k : stores ) {
                    if ( k.second == discov_store_name ) {
                        continue;
                    }

                    ostringstream str_buf;
                    str_buf << __FUNCTION__ << ":" << __LINE__ << "        Verifying store backup(" << k.second << "/" << k.first << ")" << endl << flush;
                    logInfoMsg(str_buf.str().c_str());

                    c_sts = VerifyBackup(i.second, i.first, j.second, j.first, k.second, k.first);
                    if ( c_sts != Session::Status::eOk ) {
                        ostringstream str_buf;
                        str_buf << __FUNCTION__ << ":" << __LINE__ << " Error(" << Session::StatusToText(c_sts) << ") restoring from backup store(" << k.second << "/" << k.first <<
                            ") space(" << j.second << "/" << j.first << ") cluster(" << i.second <<
                            "/" << i.first << ")" << endl << flush;
                        logErrMsg(str_buf.str().c_str());
                    } else {
                        ostringstream str_buf;
                        str_buf << __FUNCTION__ << ":" << __LINE__ << " verified from backup store(" << k.second << "/" << k.first <<
                            ") space(" << j.second << "/" << j.first << ") cluster(" << i.second <<
                            "/" << i.first << ") on(" <<
                            backup_restore_dir << "/" << i.second << "/" << j.second << "/" << k.second << ")" << endl << flush;
                        logInfoMsg(str_buf.str().c_str());
                    }
                }
            }
        }
    } catch (const std::runtime_error &ex) {
        ostringstream str_buf;
        str_buf << "runtime_error exception caught when restoring from backup: " << ex.what() << endl;
        logErrMsg(str_buf.str().c_str());
        c_sts = Session::Status::eInternalError;
    } catch ( ... ) {
        ostringstream str_buf;
        str_buf << "unknown exception caught when restoring from backup: " << endl;
        logErrMsg(str_buf.str().c_str());
        c_sts = Session::Status::eInternalError;
    }

    return c_sts;
}

/**
 * @brief Manager VerifyBackup
 *
 * Verify Single Store Backup
 *
 * @param[in]  cl             Cluster
 * @param[in]  cl_id          Cluster id
 * @param[in]  sp             Space
 * @param[in]  sp_id          Space id
 * @param[in]  st             Store
 * @param[in]  st_id          Store id
 * @param[out] status         Return status (see soesession.hpp)
 *
 */
Session::Status Manager::VerifyBackup(const string &cl, uint64_t cl_id, const string &sp, uint64_t sp_id, const string &st, uint64_t st_id)
{
    Session::Status sts;
    string full_backup_path = backup_restore_dir + "/" + cl + "/" + sp + "/" + st + "/";
    Soe::BackupEngine *bckp_engine = discovery_sess->CreateBackupEngine("BackupEngine_Verify", full_backup_path.c_str(), true);
    if ( !bckp_engine ) {
        sts = discovery_sess->GetLastStatus();
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " Error(" << Session::StatusToText(sts) << ") creating verify engine" << endl << flush;
        logErrMsg(str_buf.str().c_str());
        return sts;
    } else {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " created verify engine(" << bckp_engine->GetName() <<
            ") root dir(" << bckp_engine->GetRootDir() << ")" << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    sts = bckp_engine->VerifyBackup(cl, sp, st);
    if ( sts != Session::Status::eOk ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " Error(" << Session::StatusToText(sts) << ") verifying backup(" << sts << ")" << endl << flush;
        logErrMsg(str_buf.str().c_str());
    } else {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " verified backup(" << GetBackupRestoreDir() << ")" << "(eOk)" << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    discovery_sess->DestroyBackupEngine(bckp_engine);
    return sts;
}

/**
 * @brief Manager Backup
 *
 * Back up all of the Stores
 *
 * @param[in]  path             Backup path (root dir)
 * @param[in]  _pr              Provider
 * @param[in]  _incremental     Do incremental backup
 * @param[in]  _list_only       List only
 * @param[out] status           Return status (see soesession.hpp)
 *
 */
Session::Status Manager::Backup(const string &path, const string &_pr, bool _incremental, bool _list_only)
{
    Manager::State::eState backup_state = state.SetInProgress();
    if ( backup_state != Manager::State::eState::eInProgress ) {
        return Session::Status::eBackupInProgress;
    }

    string discov_name = "backup_dbs_xxxx";
    Session *new_sess = 0;
    Session::Status sts = getManager()->StartSession(discov_name, Manager::invalid_id,
        discov_name, Manager::invalid_id,
        discov_name, Manager::invalid_id,
        _pr,
        new_sess);
    if ( sts != Session::Status::eOk ) {
        (void )state.SetIdle();
        return sts;
    }
    assert(new_sess);

    discovery_sess = new_sess;
    SetBackupRestoreDir(path);
    SetProvider(_pr);
    SetIncremental(_incremental);

    // start backup, which involves discovering every store and backing it up iteratively
    sts = BackupStores(_list_only, discov_name);

    getManager()->DestroySession(discovery_sess);
    discovery_sess = 0;

    (void )state.SetIdle();
    return sts;
}

/**
 * @brief Manager AsyncBackup
 *
 * Async Store Backup
 *
 * @param[in]  _pr              Provider
 * @param[in]  _incremental     Do incremental backup
 * @param[in]  _list_only       List only * @param[in]  cl             Cluster
 * @param[out] status           Return status (see soesession.hpp)
 *
 */
BackupFuture *Manager::AsyncBackup(const string &path, const string &_pr, bool _incremental, Futurable *_par)
{
    Manager::State::eState backup_state = state.SetInProgress();
    if ( backup_state != Manager::State::eState::eInProgress ) {
        return 0;
    }

    string discov_name = "backup_async_dbs_xxxx";
    Session *new_sess = 0;
    Session::Status sts = getManager()->StartSession(discov_name, Manager::invalid_id,
        discov_name, Manager::invalid_id,
        discov_name, Manager::invalid_id,
        _pr,
        new_sess);
    if ( sts != Session::Status::eOk ) {
        (void )state.SetIdle();
        return 0;
    }
    assert(new_sess);

    discovery_sess = new_sess;
    SetBackupRestoreDir(path);
    SetProvider(_pr);
    SetIncremental(_incremental);

    LocalArgsDescriptor *args = new LocalArgsDescriptor;
    getDiscoverySess()->InitDesc(*args);
    args->key = path.c_str();
    args->key_len = path.length();
    args->group_idx = static_cast<uint32_t>(incremental); // overloaded field
    args->future_signaller = Soe::SignalFuture;

    BackupFuture *f = new BackupFuture(std::shared_ptr<Session>(getDiscoverySess()), _par);
    f->SetPath(path);
    args->seq_number = f->GetSeq();

    getDiscoverySess()->pending_num_futures++;
    Session::Status ret = getDiscoverySess()->Put(*args);

    if ( ret != Session::eOk ) {
        delete f;
        return 0;
    }

    DEBUG_FUTURES("Session::" << __FUNCTION__ << " fut=" << hex << f << dec << " sess=" << hex << getDiscoverySess() << dec <<
        " pending_num_futures=" << getSess()->pending_num_futures << endl);
    f->SetPostRet(ret);
    return f;
}

Session::Status Manager::ListDbs(ostringstream &str_buf, const std::string &_pr)
{
    string discov_name = "list_list_list_dbs_xxxx";
    Session *new_sess = 0;
    Session::Status c_sts = getManager()->StartSession(discov_name, Manager::invalid_id,
        discov_name, Manager::invalid_id,
        discov_name, Manager::invalid_id,
        _pr,
        new_sess);
    if ( c_sts != Session::Status::eOk ) {
        return c_sts;
    }
    assert(new_sess);

    discovery_sess = new_sess;

    std::vector<std::pair<uint64_t, string> > clusters;

    try {
        c_sts = discovery_sess->ListClusters(clusters);

        for ( auto i : clusters ) {
            if ( i.second == discov_name ) {
                continue;
            }

            std::vector<std::pair<uint64_t, string> > spaces;

            str_buf << "Discovered cluster(" << i.second << "/" << i.first << ")" << endl;

            c_sts = discovery_sess->ListSpaces(i.second, i.first, spaces);

            for ( auto j : spaces ) {
                if ( j.second == discov_name ) {
                    continue;
                }

                std::vector<std::pair<uint64_t, string> > stores;

                str_buf << "    Discovered space(" << j.second << "/" << j.first << ")" << endl;

                c_sts = discovery_sess->ListStores(i.second, i.first, j.second, j.first, stores);

                for ( auto k : stores ) {
                    if ( k.second == discov_name ) {
                        continue;
                    }

                    str_buf << "        Discovered store(" << k.second << "/" << k.first << ")" << endl;
                }
            }
        }
    } catch (const std::runtime_error &ex) {
        str_buf << "runtime_error exception caught when listing dbs: " << ex.what() << endl;
        c_sts = Session::Status::eInternalError;
    } catch ( ... ) {
        str_buf << "unknown exception caught when listing dbs: " << endl;
        c_sts = Session::Status::eInternalError;
    }

    getManager()->DestroySession(discovery_sess);
    discovery_sess = 0;

    return c_sts;
}

/**
 * @brief Manager BackupStore
 *
 * Async List Dbs
 *
 * @param[in]  path           Backup path
 * @param[in]  _pr            Provider
 * @param[in]  _par           Parent future
 * @param[out] BackupoFuture  Return future
 *
 */
BackupFuture *Manager::AsyncListDbs(ostringstream &str_buf, const std::string &_pr, Futurable *_par)
{
    return 0;
}

/**
 * @brief Manager ListBackups
 *
 * List Backups
 *
 * @param[in]  path           Backup path
 * @param[in/out] str_buf     Buffer to put backup info in
 * @param[in]  _pr            Provider
 * @param[in]  _par           Parent future
 * @param[out] BackupoFuture  Return future
 *
 */
Session::Status Manager::ListBackupDbs(std::string &root_path, std::ostringstream &str_buf, const std::string &_pr)
{
    string discov_name = "list_list_backup_dbs_xxxx";
    Session *new_sess = 0;
    Session::Status c_sts = getManager()->StartSession(discov_name, Manager::invalid_id,
        discov_name, Manager::invalid_id,
        discov_name, Manager::invalid_id,
        _pr,
        new_sess);
    if ( c_sts != Session::Status::eOk ) {
        return c_sts;
    }
    assert(new_sess);

    discovery_sess = new_sess;

    std::vector<std::pair<uint64_t, string> > clusters;

    try {
        c_sts = discovery_sess->ListClusters(clusters, root_path);

        for ( auto i : clusters ) {
            if ( i.second == discov_name ) {
                continue;
            }

            std::vector<std::pair<uint64_t, string> > spaces;

            str_buf << "Backed up cluster(" << i.second << "/" << i.first << ")" << endl;

            c_sts = discovery_sess->ListSpaces(i.second, i.first, spaces, root_path);

            for ( auto j : spaces ) {
                if ( j.second == discov_name ) {
                    continue;
                }

                std::vector<std::pair<uint64_t, string> > stores;

                str_buf << "    Backed up space(" << j.second << "/" << j.first << ")" << endl;

                c_sts = discovery_sess->ListStores(i.second, i.first, j.second, j.first, stores, root_path);

                for ( auto k : stores ) {
                    if ( k.second == discov_name ) {
                        continue;
                    }

                    str_buf << "        Backed up store(" << k.second << "/" << k.first << ")" << endl;
                }
            }
        }
    } catch (const std::runtime_error &ex) {
        str_buf << "runtime_error exception caught when listing backup dbs: " << ex.what() << endl;
        c_sts = Session::Status::eInternalError;
    } catch ( ... ) {
        str_buf << "unknown exception caught when listing backup dbs: " << endl;
        c_sts = Session::Status::eInternalError;
    }

    getManager()->DestroySession(discovery_sess);
    discovery_sess = 0;

    return c_sts;
}

/**
 * @brief Manager AsyncListBackups
 *
 * Async List Backups
 *
 * @param[in]  path           Backup path
 * @param[in/out] str_buf     Buffer to put backup info in
 * @param[in]  _pr            Provider
 * @param[in]  _par           Parent future
 * @param[out] BackupoFuture  Return future
 *
 */
BackupFuture *Manager::AsyncListBackupDbs(std::string &root_path, std::ostringstream &str_buf, const std::string &pr, Futurable *_par)
{
    return 0;
}

/**
 * @brief Manager BackupStore
 *
 * Restore Backup
 *
 * @param[in]  cl             Cluster
 * @param[in]  cl_id          Cluster id
 * @param[in]  sp             Space
 * @param[in]  sp_id          Space id
 * @param[in]  st             Store
 * @param[in]  st_id          Store id
 * @param[out] status         Return status (see soesession.hpp)
 *
 */
Session::Status Manager::Restore(const string &path, const string &_pr, bool _overwrite, bool _list_only)
{
    Manager::State::eState backup_state = state.SetInProgress();
    if ( backup_state != Manager::State::eState::eInProgress ) {
        return Session::Status::eRestoreInProgress;
    }

    string discov_name = "restore_backup_dbs_xxxx";
    Session *new_sess = 0;
    Session::Status sts = getManager()->StartSession(discov_name, Manager::invalid_id,
        discov_name, Manager::invalid_id,
        discov_name, Manager::invalid_id,
        _pr,
        new_sess);
    if ( sts != Session::Status::eOk ) {
        (void )state.SetIdle();
        return sts;
    }
    assert(new_sess);

    discovery_sess = new_sess;
    SetBackupRestoreDir(path);
    SetProvider(_pr);

    // start restore, which involves discovering every store and restoring it up iteratively
    sts = RestoreStores(_list_only, _pr, _overwrite, discov_name);

    getManager()->DestroySession(discovery_sess);
    discovery_sess = 0;

    (void )state.SetIdle();
    return sts;
}

/**
 * @brief Manager AsyncRestore
 *
 * Async Restore Backup
 *
 * @param[in]  path           Backup path
 * @param[in]  _pr            Provider
 * @param[in]  _par           Parent future
 * @param[out] BackupFuture   Return future
 *
 */
BackupFuture *Manager::AsyncRestore(const string &path, const string &_pr, bool _overwrite, Futurable *_par)
{
    Manager::State::eState backup_state = state.SetInProgress();
    if ( backup_state != Manager::State::eState::eInProgress ) {
        return 0;
    }

    string discov_name = "restore_backup_xxxx";
    Session *new_sess = 0;
    Session::Status sts = getManager()->StartSession(discov_name, Manager::invalid_id,
        discov_name, Manager::invalid_id,
        discov_name, Manager::invalid_id,
        _pr,
        new_sess);
    if ( sts != Session::Status::eOk ) {
        (void )state.SetIdle();
        return 0;
    }
    assert(new_sess);

    discovery_sess = new_sess;
    SetBackupRestoreDir(path);
    SetProvider(_pr);

    LocalArgsDescriptor *args = new LocalArgsDescriptor;
    getDiscoverySess()->InitDesc(*args);
    args->key = path.c_str();
    args->key_len = path.length();
    args->group_idx = static_cast<uint32_t>(incremental); // overloaded field
    args->future_signaller = Soe::SignalFuture;

    BackupFuture *f = new BackupFuture(std::shared_ptr<Session>(getDiscoverySess()), _par);
    f->SetPath(path);
    args->seq_number = f->GetSeq();

    getDiscoverySess()->pending_num_futures++;
    Session::Status ret = getDiscoverySess()->Put(*args);

    if ( ret != Session::eOk ) {
        delete f;
        return 0;
    }

    DEBUG_FUTURES("Session::" << __FUNCTION__ << " fut=" << hex << f << dec << " sess=" << hex << getDiscoverySess() << dec <<
        " pending_num_futures=" << getSess()->pending_num_futures << endl);
    f->SetPostRet(ret);
    return f;
}

/**
 * @brief Manager Drop
 *
 * Drop Backup
 *
 * @param[in]  path           Backup path
 * @param[in]  _pr            Provider
 * @param[in]  _list_only     Lisy only, defaults to false
 * @param[out] Status         Return status
 *
 */
Session::Status Manager::Drop(const string &path, const string &_pr, bool _list_only)
{
    Manager::State::eState backup_state = state.SetInProgress();
    if ( backup_state != Manager::State::eState::eInProgress ) {
        return Session::Status::eStoreBusy;
    }

    string discov_name = "drop_backup_xxxx";
    Session *new_sess = 0;
    Session::Status sts = getManager()->StartSession(discov_name, Manager::invalid_id,
        discov_name, Manager::invalid_id,
        discov_name, Manager::invalid_id,
        _pr,
        new_sess);
    if ( sts != Session::Status::eOk ) {
        (void )state.SetIdle();
        return sts;
    }
    assert(new_sess);

    discovery_sess = new_sess;
    SetBackupRestoreDir(path);
    SetProvider(_pr);

    // start restore, which involves discovering every store and restoring it up iteratively
    sts = DropStoreBackups(_list_only, discov_name);

    getManager()->DestroySession(discovery_sess);
    discovery_sess = 0;

    (void )state.SetIdle();
    return sts;
}

/**
 * @brief Manager Verify
 *
 * Verify Backup
 *
 * @param[in]  path           Backup path
 * @param[in]  _pr            Provider
 * @param[out] Status         Return status
 *
 */
Session::Status Manager::Verify(const string &path, const string &_pr)
{
    Manager::State::eState backup_state = state.SetInProgress();
    if ( backup_state != Manager::State::eState::eInProgress ) {
        return Session::Status::eRestoreInProgress;
    }

    string discov_name = "verify_backup_dbs_xxxx";
    Session *new_sess = 0;
    Session::Status sts = getManager()->StartSession(discov_name, Manager::invalid_id,
        discov_name, Manager::invalid_id,
        discov_name, Manager::invalid_id,
        _pr,
        new_sess);
    if ( sts != Session::Status::eOk ) {
        (void )state.SetIdle();
        return sts;
    }
    assert(new_sess);

    discovery_sess = new_sess;
    SetBackupRestoreDir(path);
    SetProvider(_pr);

    // start restore, which involves discovering every store and restoring it up iteratively
    sts = VerifyBackups(discov_name);

    getManager()->DestroySession(discovery_sess);
    discovery_sess = 0;

    (void )state.SetIdle();
    return sts;
}

/**
 * @brief Manager AsyncVerify
 *
 * Restore Backup
 *
 * @param[in]  path           Backup path
 * @param[in]  _pr            Provider
 * @param[in]  _par           Parent future
 * @param[out] BackupFuture   Return future
 *
 */
BackupFuture *Manager::AsyncVerify(const string &path, const string &_pr, Futurable *_par)
{
    return 0;
}

/**
 * @brief Manager AsyncDrop
 *
 * Async Drop Backup
 *
 * @param[in]  path           Backup path
 * @param[in]  _pr            Provider
 * @param[in]  _par           Parent future
 * @param[out] BackupFuture   Return future
 *
 */
BackupFuture *Manager::AsyncDrop(const string &path, const string &_pr, Futurable *_par)
{
    return 0;
}

void Manager::logDbgMsg(const char *msg) const
{
    soe_log(SOE_DEBUG, "%s\n", msg);
}

void Manager::logInfoMsg(const char *msg) const
{
    soe_log(SOE_INFO, "%s\n", msg);
}

void Manager::logErrMsg(const char *msg) const
{
    soe_log(SOE_ERROR, "%s\n", msg);
}

}
