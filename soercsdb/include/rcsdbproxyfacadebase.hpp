/*
 * rcsdbproxyfacadebase.hpp
 *
 *  Created on: Jun 29, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBPROXYFACADEBASE_HPP_
#define SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBPROXYFACADEBASE_HPP_

namespace RcsdbFacade {

class ProxyNameBase
{
public:
    virtual ~ProxyNameBase() {}

protected:
    ProxyNameBase() {}

    virtual void BindApiFunctions() throw(RcsdbFacade::Exception) = 0;

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
    virtual void DestroyBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void ListBackups(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;

    virtual void CreateIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
    virtual void DestroyIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) = 0;
};

}

#endif /* SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBPROXYFACADEBASE_HPP_ */
