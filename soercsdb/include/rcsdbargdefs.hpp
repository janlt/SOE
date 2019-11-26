/*
 * rcsdbargdefs.hpp
 *
 *  Created on: Dec 16, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBARGDEFS_HPP_
#define SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBARGDEFS_HPP_

typedef bool (*RangeCallback)(const char *key, size_t key_len, const char *data, size_t data_len, void *context);

//
// Overloaded field defs
//
const uint32_t LD_Iterator_First        = 0;
const uint32_t LD_Iterator_Next         = 1;
const uint32_t LD_Iterator_Last         = 2;
const uint32_t LD_Iterator_IsValid      = 3;

const bool     LD_Iterator_Forward      = true;
const bool     LD_Iterator_Backward     = false;

const uint32_t LD_GetByName             = 11111111;

const uint32_t invalid_resource_id      = -1;

const uint32_t LD_group_type_idx        = 33333333;
const uint32_t LD_transaction_type_idx  = 44444444;

const uint32_t LD_Unbounded_key         = 666666666;

// range constants
const uint32_t Store_range_delete       = 11111111;
const uint32_t Subset_range_delete      = 22222222;
const uint32_t Store_range_get          = 33333333;
const uint32_t Subset_range_get         = 44444444;
const uint32_t Store_range_put          = 55555555;
const uint32_t Subset_range_put         = 66666666;
const uint32_t Store_range_merge        = 77777777;
const uint32_t Subset_range_merge       = 88888888;

typedef uint64_t SeqNumber;

namespace RcsdbFacade {
struct FutureSignal;
}

typedef void (*FutureSignalCallback)(RcsdbFacade::FutureSignal *sig);

//
// Common i/o function argument descriptor
// NOTE: clients send request IDs (such as group_idx, iterator_idx, valid, etc) via buf
//       server sends request IDs via data
//
//       Obviously clients send data via data and server sends responses via buf, so data
//       and buf change roles in communication between clients and server
//
typedef struct _LocalArgsDescriptor {
    uint32_t              cluster_idx;          // overloaded: cluster index, Iterator type
    uint64_t              cluster_id;           // cluster id
    const std::string    *cluster_name;         // cluster name

    uint32_t              space_idx;            // space index
    uint64_t              space_id;             // space id
    const std::string    *space_name;           // space name

    uint32_t              store_idx;            // store index
    uint64_t              store_id;             // store id
    const std::string    *store_name;           // store name
    uint32_t              bloom_bits;           // how many bits to be used by Bloom filter
    bool                  overwrite_dups;       // if overwriting dups is allowed, Iterator/GetRange Direction
    bool                  transaction_db;       // if TransactionDB is to be created, Iterator Valid(true), force stop store
    uint32_t              sync_mode;            // overloaded: how i/os will be synced, if at all, LD_Iterator_*, backup incremental

    const char           *key;                  // overloaded: key, backup path
    const char           *end_key;              // overloaded: end key if get range is called, backup path len
    size_t                key_len;              // key length
    size_t                end_key_len;          // end key length

    const char           *data;                 // data, overloaded - key prefix for Subset Iterator
    size_t                data_len;             // data length or end_key_size

    char                 *buf;                  // buffer to receive data from Get
    size_t                buf_len;              // buffer length

    uint32_t              status;               // op status when run by server, overloaded field: 1 = debug on, 2 = debug off,
                                                // op_type
    RangeCallback         range_callback;       // called for every iteration in get range
    char                 *range_callback_arg;   // Context for callback, overloaded field, 1111 - GetByName, 0 - create

    uint32_t              group_idx;            // index into group object array, overloaded field t_pid of the calling thread
    uint32_t              transaction_idx;      // index into transaction object array
    uint32_t              iterator_idx;         // index into iterator object array, overloaded field
                                                // range type, store = 3, subset = 4

    SeqNumber             seq_number;           // async message sequence number, must be set to > 0 if async message is requested
    FutureSignalCallback  future_signaller;     // callback to signal a future associated with a async message

    std::vector<std::pair<uint64_t, std::string> > *list;  // list of clusters, spaces, stores, ... things
} LocalArgsDescriptor;

//
// Common admin function argument descriptor
//
typedef struct _LocalAdmArgsDescriptor {
    uint32_t              cluster_idx;          // cluster index
    uint64_t              cluster_id;           // cluster id
    const std::string    *cluster_name;         // cluster name

    uint32_t              space_idx;            // space index
    uint64_t              space_id;             // space id
    const std::string    *space_name;           // space name

    uint32_t              store_idx;            // store index
    uint64_t              store_id;             // store id
    const std::string    *store_name;           // store name

    const std::string    *snapshot_name;        // overloaded: snapshot name, backup path
    uint32_t              snapshot_idx;         // snapshot index to refer to
    uint32_t              snapshot_id;           // snaphost id

    std::vector<std::pair<std::string, uint32_t> > *snapshots; // Existing snapshots for a given store

    const std::string    *backup_name;          // backup name
    uint32_t              backup_idx;           // overloaded: backup index to refer to, incremental backup
    uint32_t              backup_id;            // backup id

    uint32_t              sync_mode;            // how i/os will be synced, if at all

    std::vector<std::pair<std::string, uint32_t> > *backups; // Existing backups for a given store

    uint32_t              transaction_idx;      // index into group object array
} LocalAdmArgsDescriptor;

//
// API name space defines
//
// The API name space for RCSDB is defined as a hierarchy of names starting
// from the common root RCSDB. Then it forks into "local" and "remote" trees.
// From those it splits further into a hierarchy of clusters, spaces and stores,
// like in the example below:
//     RCSDB.local.europe.denmark.copenhagen
//     RCSDB.local.europe.switzerland.zurich
//     RCSDB.local.asia.thailand.kolipe
//
//     RCSDB.remote.europe.denmark.copenhagen
//     RCSDB.remote.europe.switzerland.zurich
//     RCSDB.remote.asia.thailand.kolipe
//
// Functions pointed by the path fall within two categories, per description below:
//
// Global functions, i.e. in C+ terms static function members or functions common
// for all clusters or spaces, for example:
//     RCSDB.local.Create         -> create a new cluster
//     RCSDB.local.GetByName      -> query for a cluster by name
//     RCSDB.local.europe.Create  -> create a new space
//
// Member functions, i.e. in C++ terms object methods, pertain to a given instance on
// the path, for example:
//     RCSDB.local.europe.Open      -> open "europe" cluster for operations
//     RCSDB.local.europe.Get       -> get a kv item from "europe"
//     RCSDB.local.europe.Put       -> put a kv item in "europe"
//     RCSDB.local.europe.Delete    -> delete a kv item from "europe"
//     RCSDB.local.europe.GetRange  -> get a range of kv items from "europe", could be specified as
//                                   start -> end with arbitrary criteria
//

#define CLUSTER_API_DELIMIT             "."
#define CLUSTER_API_LOCAL               "local"
#define CLUSTER_API_REMOTE              "remote"

// Functional names
#define STATIC_CLUSTER                  "Static_space_functions"
#define STATIC_SPACE                    "Static_store_functions"
#define FUNCTION_CREATE                 "Create"
#define FUNCTION_DESTROY                "Destroy"
#define FUNCTION_GET_BY_NAME            "GetByName"
#define AS_JSON                         "AsJson"
#define FUNCTION_LIST_SUBSETS           "ListSubsets"
#define FUNCTION_LIST_STORES            "ListStores"
#define FUNCTION_LIST_SPACES            "ListSpaces"
#define FUNCTION_LIST_CLUSTERS          "ListClusters"

#define FUNCTION_OPEN                   "Open"
#define FUNCTION_CLOSE                  "Close"
#define FUNCTION_REPAIR                 "Repair"

#define FUNCTION_GET                    "Get"
#define FUNCTION_PUT                    "Put"
#define FUNCTION_DELETE                 "Delete"
#define FUNCTION_GET_RANGE              "GetRange"
#define FUNCTION_MERGE                  "Merge"
#define FUNCTION_ITERATE                "Iterate"

#define FUNCTION_CREATE_GROUP           "CreateGroup"
#define FUNCTION_DESTROY_GROUP          "DestroyGroup"
#define FUNCTION_WRITE_GROUP            "WriteGroup"

#define FUNCTION_START_TRANSACTION      "StartTransaction"
#define FUNCTION_COMMIT_TRANSACTION     "CommitTransaction"
#define FUNCTION_ROLLBACK_TRANSACTION   "RollbackTransaction"

#define FUNCTION_CREATE_SNAPSHOT_ENGINE "CreateSnapshotEngine"
#define FUNCTION_CREATE_SNAPSHOT        "CreateSnapshot"
#define FUNCTION_DESTROY_SNAPSHOT       "DestroySnapshot"
#define FUNCTION_LIST_SNAPSHOTS         "ListSnapshots"

#define FUNCTION_CREATE_BACKUP_ENGINE   "CreateBackupEngine"
#define FUNCTION_CREATE_BACKUP          "CreateBackup"
#define FUNCTION_RESTORE_BACKUP         "RestoreBackup"
#define FUNCTION_DESTROY_BACKUP         "DestroyBackup"
#define FUNCTION_VERIFY_BACKUP          "VerifyBackup"
#define FUNCTION_DESTROY_BACKUP_ENGINE  "DestroyBackupEngine"
#define FUNCTION_LIST_BACKUPS           "ListBackups"

#define FUNCTION_CREATE_ITERATOR        "CreateIterator"
#define FUNCTION_DESTROY_ITERATOR       "DestroyIterator"

namespace RcsdbFacade {
const std::string CreateName(std::string &ret,
    const std::string &cluster,
    const std::string &space,
    const std::string &store,
    uint32_t token,
    const std::string &fun);

const std::string CreateLocalName(const std::string &cluster,
    const std::string &space,
    const std::string &store,
    uint32_t token,
    const std::string &fun);

const std::string CreateRemoteName(const std::string &cluster,
    const std::string &space,
    const std::string &store,
    uint32_t token,
    const std::string &fun);

std::string GetProvider();

}

#endif /* SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBARGDEFS_HPP_ */
