/**
 * @file    soebackupengine.hpp
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
 * soebackupengine.hpp
 *
 *  Created on: Feb 3, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOE_INCLUDE_SOEBACKUPENGINE_HPP_
#define SOE_SOE_SOE_SOE_INCLUDE_SOEBACKUPENGINE_HPP_

namespace Soe {

/**
 * @brief BackupEngine class
 *
 * This class allows creating backup engines and store backups.
 *
 * Backups created using the same backup name are incremental backups. Each time a new backup
 * is created using the same backup name, only newly created items will be copied to the backup's
 * repository.
 *
 * Backup engines are created by invoking BackupEngine constructor.
 *
 * Backup engines have their scope within the scope of a session used to create them.
 *
 * Once a backup engine has been created, a store backup can be created. Multiple backups
 * can be created using the same backup engine. When backup engine is no longer needed it can
 * be deleted. The backups created using that engine will still reside as part of the store.
 * Subsequently, when more backups are needed, a new backup engine and new backups can be created,
 * using new session. When an entire store is destroyed, all the backups residing within that
 * store will be discarded as well.
 *
 */
class BackupEngine
{
friend class Session;

private:
    uint32_t                       backup_engine_no;
    uint32_t                       backup_engine_id;
    std::string                    backup_engine_name;
    std::string                    backup_root_dir;
    const static uint32_t          invalid_backup_engine = -1;
    std::shared_ptr<Session>       sess;

    BackupEngine(std::shared_ptr<Session> _sess,
        const std::string _nm = "backup_1",
        const std::string _brd = "",
        uint32_t _no = BackupEngine::invalid_backup_engine,
        uint32_t _id = BackupEngine::invalid_backup_engine);
    ~BackupEngine();

public:
    Session::Status CreateBackup() throw(std::runtime_error);
    Session::Status RestoreBackup(const std::string &cl_n = "",
        const std::string &sp_n = "",
        const std::string &st_n = "",
        bool _overwrite = true) throw(std::runtime_error);
    Session::Status DestroyBackup() throw(std::runtime_error);
    Session::Status VerifyBackup(const std::string &cl_n,
        const std::string &sp_n,
        const std::string &st_n) throw(std::runtime_error);

    std::string GetName()
    {
        return backup_engine_name;
    }

    const std::string &GetConstName() const
    {
        return backup_engine_name;
    }

    void SetName(const std::string &_nm)
    {
        backup_engine_name = _nm;
    }

    std::string GetRootDir()
    {
        return backup_root_dir;
    }

    const std::string &GetConstRootDir() const
    {
        return backup_root_dir;
    }

    void SetRootDir(const std::string &_brd)
    {
        backup_root_dir = _brd;
    }

    uint32_t GetNo()
    {
        return backup_engine_no;
    }

    void SetNo(uint32_t _no)
    {
        backup_engine_no = _no;
    }

    uint32_t GetId()
    {
        return backup_engine_id;
    }

    void SetId(uint32_t _id)
    {
        backup_engine_id = _id;
    }
};

}

#endif /* SOE_SOE_SOE_SOE_INCLUDE_SOEBACKUPENGINE_HPP_ */
