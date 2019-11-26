/*
 * rcsdbfacadebase.hpp
 *
 *  Created on: Dec 20, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_RCSDB_SRC_RCSDBFACADEBASE_HPP_
#define SOE_SOE_SOE_RCSDB_SRC_RCSDBFACADEBASE_HPP_


namespace RcsdbFacade {

class NameBase
{
public:
    virtual ~NameBase() {}

protected:
    NameBase() {}
    virtual void Create() = 0;
    virtual void SetParent(NameBase *par) = 0;
    virtual void SetPath() = 0;
    virtual const std::string &GetPath() = 0;
    virtual void ParseConfig(const std::string &cfg, bool no_exc = false) = 0;
    virtual std::string CreateConfig() = 0;
    virtual std::string ConfigAddSnapshot(const std::string &name, uint32_t id) = 0;
    virtual std::string ConfigRemoveSnapshot(const std::string &name, uint32_t id) = 0;
    virtual std::string ConfigAddBackup(const std::string &name, uint32_t id) = 0;
    virtual std::string ConfigRemoveBackup(const std::string &name, uint32_t id) = 0;
    virtual bool StopAndArchiveName(LocalArgsDescriptor &args) = 0;

    virtual void RegisterApiFunctions() throw(SoeApi::Exception) = 0;
    virtual void DeregisterApiFunctions() throw(SoeApi::Exception) = 0;

    virtual void Open(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void Close(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void Repair(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;

    virtual void Put(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void Get(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void Delete(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void Merge(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void GetRange(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void Iterate(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;

    virtual void CreateGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void DestroyGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void WriteGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) {}

    virtual void StartTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void CommitTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void RollbackTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;

    virtual void CreateSnapshotEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void CreateSnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void DestroySnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void ListSnapshots(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;

    virtual void CreateBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void CreateBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void RestoreBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void DestroyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void VerifyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void DestroyBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void ListBackups(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;

    virtual void CreateIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void DestroyIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;

    virtual void SetNextAvailable() = 0;
};

}


#endif /* SOE_SOE_SOE_RCSDB_SRC_RCSDBFACADEBASE_HPP_ */
