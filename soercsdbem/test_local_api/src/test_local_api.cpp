/*
 * test_local_api.cpp
 *
 *  Created on: Dec 1, 2016
 *      Author: Jan Lisowiec
 */

#include <string.h>
#include <dlfcn.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <new>

#include "soe_bind_api.hpp"
#include "rcsdbargdefs.hpp"
#include "rcsdbfacadeapi.hpp"

using namespace std;
using namespace SoeApi;
using namespace RcsdbFacade;

#define MMIN(a,b) ((a)<(b)?(a):(b))

std::string MakeN2Key(const char *num_1, const char *num_2)
{
    size_t s_1 = strlen(num_1);
    size_t s_2 = strlen(num_2);

    if ( s_2 >= s_1 ) {
        return std::string(num_2);
    }

    char tmp_buf[1024];
    strncpy(tmp_buf, num_1, MMIN(s_1, sizeof(tmp_buf)));
    memcpy(&tmp_buf[s_1 - s_2], num_2, s_2);
    tmp_buf[s_1] = '\0';

    return std::string(tmp_buf);
}

typedef uint64_t (CreateStaticFun)(LocalArgsDescriptor &);
typedef void (DestroyStaticFun)(LocalArgsDescriptor &);
typedef uint64_t (GetByNameStaticFun)(LocalArgsDescriptor &);
typedef void (OpObjectFun)(LocalArgsDescriptor &);

typedef struct _RCSDBAPI {
    boost::function<CreateStaticFun>      create_fun;
    boost::function<DestroyStaticFun>     destroy_fun;
    boost::function<GetByNameStaticFun>   get_by_name_fun;
    boost::function<OpObjectFun>          open;
    boost::function<OpObjectFun>          close;
    boost::function<OpObjectFun>          put;
    boost::function<OpObjectFun>          get;
    boost::function<OpObjectFun>          del;
    boost::function<OpObjectFun>          get_range;
    boost::function<OpObjectFun>          start_transaction;
    boost::function<OpObjectFun>          commit_transaction;
    boost::function<OpObjectFun>          snapshot;
    boost::function<OpObjectFun>          merge;
} RCSDBAPI;

static void OpenCreateCluster(LocalArgsDescriptor &args,
    const std::string &cluster,
    RCSDBAPI &api)
{
     // get Kvs local static functions to find cluster by name and create one if not found
     api.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName("", "", "", 0, FUNCTION_CREATE));
     api.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName("", "", "", 0, FUNCTION_DESTROY));
     api.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName("", "", "", 0, FUNCTION_GET_BY_NAME));

     args.cluster_name = &cluster;
     args.cluster_id = 100;
     uint64_t clu_id = api.get_by_name_fun(args);
     if ( clu_id == (uint64_t )-1 ) {
         clu_id = api.create_fun(args);
     }
     std::cout << "Cluster avaialble for ops id: " << clu_id << " name: " << cluster << std::endl << std::flush;


     // get Kvs object functions to manipulate a cluster
     api.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, "", "", 0, FUNCTION_OPEN));
     api.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, "", "", 0, FUNCTION_CLOSE));
     api.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, "", "", 0, FUNCTION_PUT));
     api.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, "", "", 0, FUNCTION_GET));
     api.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, "", "", 0, FUNCTION_DELETE));
     api.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, "", "", 0, FUNCTION_GET_RANGE));
}

static void OpenCreateSpace(LocalArgsDescriptor &args,
    const std::string &cluster,
    const std::string &space,
    RCSDBAPI &api)
{
    // get Kvs local static functions to find space by name and create one if not found
    api.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, "", "", 0, FUNCTION_CREATE));
    api.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, "", "", 0, FUNCTION_DESTROY));
    api.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, "", "", 0, FUNCTION_GET_BY_NAME));

    args.space_name = &space;
    args.space_id = 1000;
    uint64_t sp_id = api.get_by_name_fun(args);
    if ( sp_id == (uint64_t )-1 ) {
        sp_id = api.create_fun(args);
    }
    std::cout << "Space avaialble for ops id: " << sp_id << " name: " << space << std::endl << std::flush;


    // get Kvs object functions to manipulate a space
    api.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, "", 0, FUNCTION_OPEN));
    api.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, "", 0, FUNCTION_CLOSE));
    api.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, "", 0, FUNCTION_PUT));
    api.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, "", 0, FUNCTION_GET));
    api.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, "", 0, FUNCTION_DELETE));
    api.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, "", 0, FUNCTION_GET_RANGE));
}

static void OpenCreateStore(LocalArgsDescriptor &args,
    const std::string &cluster,
    const std::string &space,
    const std::string &store,
    RCSDBAPI &api)
{
    // get Kvs local static functions to find space by name and create one if not found
    api.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_CREATE));
    api.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_GET_BY_NAME));

    args.store_name = &store;
    args.store_id = 10000;
    uint64_t st_id = api.get_by_name_fun(args);
    if ( st_id == (uint64_t )-1 ) {
        st_id = api.create_fun(args);
    }
    std::cout << "Store avaialble for ops id: " << st_id << " name: " << store << std::endl << std::flush;


    // get Kvs object functions to manipulate a space
    api.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_OPEN));
    api.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_CLOSE));
    api.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_PUT));
    api.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET));
    api.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_DELETE));
    api.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET_RANGE));

    // open store
    api.open(args);
}

static void ReadStore(LocalArgsDescriptor &args,
    const std::string &cluster,
    const std::string &space,
    const std::string &store,
    RCSDBAPI &api)
{
    // get Kvs local static functions to find space by name and create one if not found
    api.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_CREATE));
    api.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_DESTROY));
    api.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_GET_BY_NAME));

    args.cluster_name = &cluster;
    args.space_name = &space;
    args.store_name = &store;
    uint64_t st_id = api.get_by_name_fun(args);

    std::cout << "Store avaialble for ops id: " << st_id << " name: " << store << std::endl << std::flush;


    // get Kvs object functions to manipulate a space
    api.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_OPEN));
    api.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_CLOSE));
    api.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_PUT));
    api.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET));
    api.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_DELETE));
    api.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET_RANGE));

    // open store
    api.open(args);

    char buf[1024];
    args.buf = buf;
    args.buf_len = sizeof(buf);
    api.get(args);

    std::cout << "buf: " << buf << std::endl << std::flush;
}

static void WriteStore(LocalArgsDescriptor &args,
    const std::string &cluster,
    const std::string &space,
    const std::string &store,
    RCSDBAPI &api)
{
    // get Kvs local static functions to find space by name and create one if not found
    api.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_CREATE));
    api.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_DESTROY));
    api.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_GET_BY_NAME));

    args.cluster_name = &cluster;
    args.space_name = &space;
    args.store_name = &store;
    uint64_t st_id = api.get_by_name_fun(args);

    std::cout << "Store avaialble for ops id: " << st_id << " name: " << store << std::endl << std::flush;


    // get Kvs object functions to manipulate a space
    api.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_OPEN));
    api.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_CLOSE));
    api.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_PUT));
    api.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET));
    api.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_DELETE));
    api.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET_RANGE));

    // open store
    api.open(args);

    api.put(args);
}

static void DeleteStore(LocalArgsDescriptor &args,
    const std::string &cluster,
    const std::string &space,
    const std::string &store,
    RCSDBAPI &api)
{
    // get Kvs local static functions to find space by name and create one if not found
    api.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_CREATE));
    api.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_DESTROY));
    api.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_GET_BY_NAME));

    args.cluster_name = &cluster;
    args.space_name = &space;
    args.store_name = &store;
    uint64_t st_id = api.get_by_name_fun(args);

    std::cout << "Store avaialble for ops id: " << st_id << " name: " << store << std::endl << std::flush;


    // get Kvs object functions to manipulate a space
    api.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_OPEN));
    api.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_CLOSE));
    api.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_PUT));
    api.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET));
    api.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_DELETE));
    api.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET_RANGE));

    // open store
    api.open(args);

    api.del(args);
}

static void WriteStoreN(LocalArgsDescriptor &args,
    const std::string &cluster,
    const std::string &space,
    const std::string &store,
    RCSDBAPI &api,
    uint32_t cnt)
{
    // get Kvs local static functions to find space by name and create one if not found
    api.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_CREATE));
    api.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_DESTROY));
    api.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_GET_BY_NAME));

    args.cluster_name = &cluster;
    args.space_name = &space;
    args.store_name = &store;
    uint64_t st_id = api.get_by_name_fun(args);

    std::cout << "Store avaialble for ops id: " << st_id << " name: " << store << std::endl << std::flush;


    // get Kvs object functions to manipulate a space
    api.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_OPEN));
    api.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_CLOSE));
    api.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_PUT));
    api.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET));
    api.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_DELETE));
    api.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET_RANGE));

    // open store
    api.open(args);

    uint64_t num = atol(args.key);
    char key_buf[1024];
    char key_init_buf[1024];
    memset(key_init_buf, '\0', sizeof(key_init_buf));
    memcpy(key_init_buf, args.key, MMIN(args.key_len, sizeof(key_init_buf)));
    uint32_t quant = cnt/16;
    if ( !quant ) {
        quant = 1;
    }
    char *data_ptr = const_cast<char*>(args.data);
    for ( uint64_t i = 0 ; i < cnt ; i++ ) {
        sprintf(key_buf, "%lu", i);
        std::string tmp_key = MakeN2Key(key_init_buf, key_buf);
        args.key = tmp_key.c_str();
        args.key_len = tmp_key.size();
        sprintf(data_ptr, "%lu%lu%lul%lu", num, 2*num, 3*num, 4*num);
        data_ptr[strlen(data_ptr)] = 'B';
        args.data = data_ptr;

        if ( !(i%quant) ) {
            if ( args.data_len > 16 ) {
                std::cout << "args.key: " << args.key << " args.key_len: " << args.key_len <<
                     " args.data_len: " << args.data_len << std::endl << std::flush;
            } else {
                std::cout << "args.key: " << args.key << " args.key_len: " << args.key_len <<
                     " args.data: " << args.data << " args.data_len: " << args.data_len << std::endl << std::flush;
            }
        }

        api.put(args);
    }
}

static void WriteStoreN(LocalArgsDescriptor &args,
    const std::string &cluster,
    const std::string &space,
    const std::string &store,
    uint32_t cnt)
{
    RCSDBAPI api;

    // get Kvs local static functions to find space by name and create one if not found
    api.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_CREATE));
    api.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_DESTROY));
    api.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_GET_BY_NAME));

    args.cluster_name = &cluster;
    args.space_name = &space;
    args.store_name = &store;
    uint64_t st_id = api.get_by_name_fun(args);

    std::cout << "Store avaialble for ops id: " << st_id << " name: " << store << std::endl << std::flush;


    // get Kvs object functions to manipulate a space
    api.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_OPEN));
    api.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_CLOSE));
    api.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_PUT));
    api.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET));
    api.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_DELETE));
    api.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET_RANGE));

    // open store
    api.open(args);

    uint64_t num = atol(args.key);
    char key_buf[1024];
    char key_init_buf[1024];
    memset(key_init_buf, '\0', sizeof(key_init_buf));
    memcpy(key_init_buf, args.key, MMIN(args.key_len, sizeof(key_init_buf)));
    uint32_t quant = cnt/16;
    if ( !quant ) {
        quant = 1;
    }
    char *data_ptr = const_cast<char*>(args.data);
    for ( uint64_t i = 0 ; i < cnt ; i++ ) {
        sprintf(key_buf, "%lu", i);
        std::string tmp_key = MakeN2Key(key_init_buf, key_buf);
        args.key = tmp_key.c_str();
        args.key_len = tmp_key.size();
        sprintf(data_ptr, "%lu%lu%lul%lu", num, 2*num, 3*num, 4*num);
        data_ptr[strlen(data_ptr)] = 'B';
        args.data = data_ptr;

        if ( !(i%quant) ) {
            if ( args.data_len > 16 ) {
                std::cout << "args.key: " << args.key << " args.key_len: " << args.key_len <<
                     " args.data_len: " << args.data_len << std::endl << std::flush;
            } else {
                std::cout << "args.key: " << args.key << " args.key_len: " << args.key_len <<
                     " args.data: " << args.data << " args.data_len: " << args.data_len << std::endl << std::flush;
            }
        }

        api.put(args);
    }
}

static void WriteThreadAsyncStoreN(LocalArgsDescriptor *argsptr)
{
    RCSDBAPI api;
    LocalArgsDescriptor &args = *argsptr;
    uint32_t cnt = args.cluster_idx;
    std::string cluster = *args.cluster_name;
    std::string space = *args.space_name;
    std::string store = *args.store_name;

    try {
        // get Kvs local static functions to find space by name and create one if not found
        api.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_CREATE));
        api.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_DESTROY));
        api.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_GET_BY_NAME));

        uint64_t st_id = api.get_by_name_fun(args);

        std::cout << "Store avaialble for ops id: " << st_id << " name: " << store << std::endl << std::flush;


        // get Kvs object functions to manipulate a space
        api.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_OPEN));
        api.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_CLOSE));
        api.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_PUT));
        api.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET));
        api.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_DELETE));
        api.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET_RANGE));

        // open store
        api.open(args);

        uint64_t num = atol(args.key);
        char key_buf[1024];
        char key_init_buf[1024];
        memset(key_init_buf, '\0', sizeof(key_init_buf));
        memcpy(key_init_buf, args.key, MMIN(args.key_len, sizeof(key_init_buf)));
        uint32_t quant = cnt/16;
        if ( !quant ) {
            quant = 1;
        }

        char *data_ptr = const_cast<char*>(args.data);
        boost::thread::id t_id = boost::this_thread::get_id();

        for ( uint64_t i = 0 ; i < cnt ; i++ ) {
            sprintf(key_buf, "%lu", i);
            std::string tmp_key = MakeN2Key(key_init_buf, key_buf);
            args.key = tmp_key.c_str();
            args.key_len = tmp_key.size();
            sprintf(data_ptr, "%lu%lu%lul%lu", num, 2*num, 3*num, 4*num);
            data_ptr[strlen(data_ptr)] = 'B';
            args.data = data_ptr;

            if ( !(i%quant) ) {
                if ( args.data_len > 16 ) {
                    std::cout << "(" << t_id << ")" << "args.key: " << args.key << " args.key_len: " << args.key_len <<
                         " args.data_len: " << args.data_len << std::endl << std::flush;
                } else {
                    std::cout << "(" << t_id << ")" << "args.key: " << args.key << " args.key_len: " << args.key_len <<
                         " args.data: " << args.data << " args.data_len: " << args.data_len << std::endl << std::flush;
                }
            }

            api.put(args);
        }
    } catch (const RcsdbFacade::Exception &e) {
        std::cout << "RcsdbFacade::Exception: " << e.what() << std::endl << std::flush;
    } catch (const SoeApi::Exception &e) {
        std::cout << "SoeApi::Exception: " << e.what() << std::endl << std::flush;
    } catch (const std::bad_cast &e) {
        std::cout << "Exception std::bad_cast " << e.what() << std::endl << std::flush;
    } catch (const std::exception &e) {
        std::cout << "Exception std::exception " << e.what() << std::endl << std::flush;
    } catch (...) {
        std::cout << "Exception ... " << std::endl << std::flush;
    }
}

static void ReadStoreN(LocalArgsDescriptor &args,
    const std::string &cluster,
    const std::string &space,
    const std::string &store,
    uint32_t cnt)
{
    RCSDBAPI api;

    // get Kvs local static functions to find space by name and create one if not found
    api.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_CREATE));
    api.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_DESTROY));
    api.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_GET_BY_NAME));

    args.cluster_name = &cluster;
    args.space_name = &space;
    args.store_name = &store;
    uint64_t st_id = api.get_by_name_fun(args);

    std::cout << "Store avaialble for ops id: " << st_id << " name: " << store << std::endl << std::flush;


    // get Kvs object functions to manipulate a space
    api.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_OPEN));
    api.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_CLOSE));
    api.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_PUT));
    api.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET));
    api.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_DELETE));
    api.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET_RANGE));

    // open store
    api.open(args);

    //long num = atol(args.key);
    char key_buf[1024];
    char key_init_buf[1024];
    memset(key_init_buf, '\0', sizeof(key_init_buf));
    memcpy(key_init_buf, args.key, MMIN(args.key_len, sizeof(key_init_buf)));
    uint32_t quant = cnt/16;
    quant = 1;
    for ( uint64_t i = 0 ; i < cnt ; i++ ) {
        sprintf(key_buf, "%lu", i);
        std::string tmp_key = MakeN2Key(key_init_buf, key_buf);
        args.key = tmp_key.c_str();
        args.key_len = tmp_key.size();

        if ( !(i%quant) ) {
            if ( args.buf_len < 16 ) {
                std::cout << "args.key: " << args.key << " args.key_len: " << args.key_len <<
                     " args.buf: " << args.buf << " args.buf_len: " << args.data_len << std::endl << std::flush;
            } else {
                std::cout << "args.key: " << args.key << " args.key_len: " << args.key_len <<
                     " args.buf_len: " << args.buf_len << std::endl << std::flush;
            }
        }

        try {
            api.get(args);
        } catch (const RcsdbFacade::Exception &e) {
            std::cout << "RcsdbFacade::Exception: " << e.what() << std::endl << std::flush;
        }
    }
}

static void ReadThreadAsyncStoreN(LocalArgsDescriptor *argsptr)
{
    RCSDBAPI api;
    LocalArgsDescriptor &args = *argsptr;
    uint32_t cnt = args.cluster_idx;
    std::string cluster = *args.cluster_name;
    std::string space = *args.space_name;
    std::string store = *args.store_name;

    try {
        // get Kvs local static functions to find space by name and create one if not found
        api.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_CREATE));
        api.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_DESTROY));
        api.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_GET_BY_NAME));

        uint64_t st_id = api.get_by_name_fun(args);

        std::cout << "Store avaialble for ops id: " << st_id << " name: " << store << std::endl << std::flush;


        // get Kvs object functions to manipulate a space
        api.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_OPEN));
        api.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_CLOSE));
        api.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_PUT));
        api.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET));
        api.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_DELETE));
        api.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET_RANGE));

        // open store
        api.open(args);

        //long num = atol(args.key);
        char key_buf[1024];
        char key_init_buf[1024];
        memset(key_init_buf, '\0', sizeof(key_init_buf));
        memcpy(key_init_buf, args.key, MMIN(args.key_len, sizeof(key_init_buf)));
        uint32_t quant = cnt/16;
        quant = 1;
        boost::thread::id t_id = boost::this_thread::get_id();

        for ( uint64_t i = 0 ; i < cnt ; i++ ) {
            sprintf(key_buf, "%lu", i);
            std::string tmp_key = MakeN2Key(key_init_buf, key_buf);
            args.key = tmp_key.c_str();
            args.key_len = tmp_key.size();

            if ( !(i%quant) ) {
                if ( args.buf_len < 16 ) {
                    std::cout << "(" << t_id << ")" << "args.key: " << args.key << " args.key_len: " << args.key_len <<
                         " args.buf: " << args.buf << " args.buf_len: " << args.data_len << std::endl << std::flush;
                } else {
                    std::cout << "(" << t_id << ")" << "args.key: " << args.key << " args.key_len: " << args.key_len <<
                         " args.buf_len: " << args.buf_len << std::endl << std::flush;
                }
            }

            try {
                api.get(args);
            } catch (const RcsdbFacade::Exception &e) {
                std::cout << "RcsdbFacade::Exception: " << e.what() << std::endl << std::flush;
            }
        }
    } catch (const RcsdbFacade::Exception &e) {
        std::cout << "RcsdbFacade::Exception: " << e.what() << std::endl << std::flush;
    } catch (const SoeApi::Exception &e) {
        std::cout << "SoeApi::Exception: " << e.what() << std::endl << std::flush;
    } catch (const std::bad_cast &e) {
        std::cout << "Exception std::bad_cast " << e.what() << std::endl << std::flush;
    } catch (const std::exception &e) {
        std::cout << "Exception std::exception " << e.what() << std::endl << std::flush;
    } catch (...) {
        std::cout << "Exception ... " << std::endl << std::flush;
    }
}

static void ReadContStoreN(LocalArgsDescriptor &args,
    const std::string &cluster,
    const std::string &space,
    const std::string &store,
    uint32_t cnt)
{
    RCSDBAPI api;

    // get Kvs local static functions to find space by name and create one if not found
    api.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_CREATE));
    api.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_DESTROY));
    api.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_GET_BY_NAME));

    args.cluster_name = &cluster;
    args.space_name = &space;
    args.store_name = &store;
    uint64_t st_id = api.get_by_name_fun(args);

    std::cout << "Store avaialble for ops id: " << st_id << " name: " << store << std::endl << std::flush;


    // get Kvs object functions to manipulate a space
    api.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_OPEN));
    api.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_CLOSE));
    api.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_PUT));
    api.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET));
    api.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_DELETE));
    api.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET_RANGE));

    // open store
    api.open(args);

    //long num = atol(args.key);
    char key_buf[1024];
    char key_init_buf[1024];
    memset(key_init_buf, '\0', sizeof(key_init_buf));
    memcpy(key_init_buf, args.key, MMIN(args.key_len, sizeof(key_init_buf)));
    uint32_t quant = cnt/16;
    quant = 1;
    for ( ;; ) {
        for ( uint64_t i = 0 ; i < cnt ; i++ ) {
            sprintf(key_buf, "%lu", i);
            std::string tmp_key = MakeN2Key(key_init_buf, key_buf);
            args.key = tmp_key.c_str();
            args.key_len = tmp_key.size();

            if ( !(i%quant) ) {
                if ( args.buf_len < 16 ) {
                    std::cout << "args.key: " << args.key << " args.key_len: " << args.key_len <<
                         " args.buf: " << args.buf << " args.buf_len: " << args.data_len << std::endl << std::flush;
                } else {
                    std::cout << "args.key: " << args.key << " args.key_len: " << args.key_len <<
                         " args.buf_len: " << args.buf_len << std::endl << std::flush;
                }
            }

            try {
                api.get(args);
            } catch (const RcsdbFacade::Exception &e) {
                std::cout << "RcsdbFacade::Exception: " << e.what() << std::endl << std::flush;
            }
        }
    }
}

static void ReadContThreadAsyncStoreN(LocalArgsDescriptor *argsptr)
{
    RCSDBAPI api;
    LocalArgsDescriptor &args = *argsptr;
    uint32_t cnt = args.cluster_idx;
    std::string cluster = *args.cluster_name;
    std::string space = *args.space_name;
    std::string store = *args.store_name;

    try {
        // get Kvs local static functions to find space by name and create one if not found
        api.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_CREATE));
        api.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_DESTROY));
        api.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_GET_BY_NAME));

        uint64_t st_id = api.get_by_name_fun(args);

        std::cout << "Store avaialble for ops id: " << st_id << " name: " << store << std::endl << std::flush;


        // get Kvs object functions to manipulate a space
        api.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_OPEN));
        api.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_CLOSE));
        api.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_PUT));
        api.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET));
        api.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_DELETE));
        api.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET_RANGE));

        // open store
        api.open(args);

        //long num = atol(args.key);
        char key_buf[1024];
        char key_init_buf[1024];
        memset(key_init_buf, '\0', sizeof(key_init_buf));
        memcpy(key_init_buf, args.key, MMIN(args.key_len, sizeof(key_init_buf)));
        uint32_t quant = cnt/16;
        quant = 1;
        boost::thread::id t_id = boost::this_thread::get_id();

        for ( ;; ) {
            for ( uint64_t i = 0 ; i < cnt ; i++ ) {
                sprintf(key_buf, "%lu", i);
                std::string tmp_key = MakeN2Key(key_init_buf, key_buf);
                args.key = tmp_key.c_str();
                args.key_len = tmp_key.size();

                if ( !(i%quant) ) {
                    if ( args.buf_len < 16 ) {
                        std::cout << "(" << t_id << ")" << "args.key: " << args.key << " args.key_len: " << args.key_len <<
                             " args.buf: " << args.buf << " args.buf_len: " << args.data_len << std::endl << std::flush;
                    } else {
                        std::cout << "(" << t_id << ")" << "args.key: " << args.key << " args.key_len: " << args.key_len <<
                             " args.buf_len: " << args.buf_len << std::endl << std::flush;
                    }
                }

                try {
                    api.get(args);
                } catch (const RcsdbFacade::Exception &e) {
                    std::cout << "RcsdbFacade::Exception: " << e.what() << std::endl << std::flush;
                }
            }
        }
    } catch (const RcsdbFacade::Exception &e) {
        std::cout << "RcsdbFacade::Exception: " << e.what() << std::endl << std::flush;
    } catch (const SoeApi::Exception &e) {
        std::cout << "SoeApi::Exception: " << e.what() << std::endl << std::flush;
    } catch (const std::bad_cast &e) {
        std::cout << "Exception std::bad_cast " << e.what() << std::endl << std::flush;
    } catch (const std::exception &e) {
        std::cout << "Exception std::exception " << e.what() << std::endl << std::flush;
    } catch (...) {
        std::cout << "Exception ... " << std::endl << std::flush;
    }
}

static void ReadRangeStore(LocalArgsDescriptor &args,
    const std::string &cluster,
    const std::string &space,
    const std::string &store)
{
    RCSDBAPI api;

    // get Kvs local static functions to find space by name and create one if not found
    api.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_CREATE));
    api.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_DESTROY));
    api.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_GET_BY_NAME));

    args.cluster_name = &cluster;
    args.space_name = &space;
    args.store_name = &store;
    uint64_t st_id = api.get_by_name_fun(args);

    std::cout << "Store avaialble for ops id: " << st_id << " name: " << store << std::endl << std::flush;

    // get Kvs object functions to manipulate a space
    api.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_OPEN));
    api.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_CLOSE));
    api.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_PUT));
    api.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET));
    api.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_DELETE));
    api.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET_RANGE));

    // open store
    api.open(args);

    api.get_range(args);
}

static void ReadOneStore(LocalArgsDescriptor &args,
    const std::string &cluster,
    const std::string &space,
    const std::string &store)
{
    RCSDBAPI api;

    // get Kvs local static functions to find space by name and create one if not found
    api.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_CREATE));
    api.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_DESTROY));
    api.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_GET_BY_NAME));

    args.cluster_name = &cluster;
    args.space_name = &space;
    args.store_name = &store;
    uint64_t st_id = api.get_by_name_fun(args);

    std::cout << "Store avaialble for ops id: " << st_id << " name: " << store << std::endl << std::flush;

    // get Kvs object functions to manipulate a space
    api.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_OPEN));
    api.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_CLOSE));
    api.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_PUT));
    api.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET));
    api.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_DELETE));
    api.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET_RANGE));

    // open store
    api.open(args);

    api.get(args);
}




//
// Option handlers
//
static void ConcurrentWriteTest(const LocalArgsDescriptor &a, int cnt, uint32_t write_cnt)
{
    std::string cluster = *a.cluster_name;
    std::string space = *a.space_name;
    std::string store = *a.store_name;

    boost::thread *threads[200];
    if ( cnt > 200 ) {
        std::cout << "cnt to big: " << cnt << std::endl << std::flush;
        return;
    }

    int th_cnt = 0;
    for ( int i = 0 ; i < cnt ; i++ ) {
        cluster = *a.cluster_name;
        cluster += ("_" + to_string(i));
        for ( int j = 0 ; j < cnt ; j++ ) {
            space = *a.space_name;
            space += ("_" + to_string(j));
            for ( int k = 0 ; k < cnt ; k++ ) {
                LocalArgsDescriptor *args = new LocalArgsDescriptor();
                memset(args, '\0', sizeof(*args));
                args->key = a.key;
                args->key_len = a.key_len;
                args->data = a.data;
                args->data_len = a.data_len;

                store = *a.store_name;
                store += ("_" + to_string(k));

                args->cluster_name = new std::string(cluster);
                args->space_name = new std::string(space);
                args->store_name = new std::string(store);
                args->cluster_idx = write_cnt;

                std::cout << "cluster: " << *args->cluster_name <<
                    " space: " << *args->space_name <<
                    " store: " << *args->store_name << std::endl << std::flush;

                threads[th_cnt++] = new boost::thread(&WriteThreadAsyncStoreN, args);
            }
        }
    }

    for ( int i = 0 ; i < th_cnt ; i++ ) {
        std::cout << "Joining ..." << std::endl << std::flush;
        threads[i]->join();
    }
}

static void SerialWriteTest(const LocalArgsDescriptor &a, int cnt, uint32_t write_cnt)
{
    std::string cluster = *a.cluster_name;
    std::string space = *a.space_name;
    std::string store = *a.store_name;

    for ( int i = 0 ; i < cnt ; i++ ) {
        cluster = *a.cluster_name;
        cluster += ("_" + to_string(i));
        for ( int j = 0 ; j < cnt ; j++ ) {
            space = *a.space_name;
            space += ("_" + to_string(j));
            for ( int k = 0 ; k < cnt ; k++ ) {
                LocalArgsDescriptor args;
                memset(&args, '\0', sizeof(args));
                args.key = a.key;
                args.key_len = a.key_len;
                args.data = a.data;
                args.data_len = a.data_len;

                store = *a.store_name;
                store += ("_" + to_string(k));

                std::cout << "cluster: " << cluster <<
                    " space: " << space <<
                    " store: " << store << std::endl << std::flush;
                WriteStoreN(args, cluster, space, store, write_cnt);
            }
        }
    }
}

static void ConcurrentReadTest(const LocalArgsDescriptor &a, int cnt, uint32_t write_cnt)
{
    std::string cluster = *a.cluster_name;
    std::string space = *a.space_name;
    std::string store = *a.store_name;

    boost::thread *threads[200];
    if ( cnt > 200 ) {
        std::cout << "cnt to big: " << cnt << std::endl << std::flush;
        return;
    }

    int th_cnt = 0;
    for ( int i = 0 ; i < cnt ; i++ ) {
        cluster = *a.cluster_name;
        cluster += ("_" + to_string(i));
        for ( int j = 0 ; j < cnt ; j++ ) {
            space = *a.space_name;
            space += ("_" + to_string(j));
            for ( int k = 0 ; k < cnt ; k++ ) {
                LocalArgsDescriptor *args = new LocalArgsDescriptor();
                memset(args, '\0', sizeof(*args));
                args->key = a.key;
                args->key_len = a.key_len;
                args->data = a.data;
                args->data_len = a.data_len;
                args->buf = a.buf;
                args->buf_len = a.buf_len;

                store = *a.store_name;
                store += ("_" + to_string(k));

                args->cluster_name = new std::string(cluster);
                args->space_name = new std::string(space);
                args->store_name = new std::string(store);
                args->cluster_idx = write_cnt;

                std::cout << "cluster: " << *args->cluster_name <<
                    " space: " << *args->space_name <<
                    " store: " << *args->store_name << std::endl << std::flush;

                threads[th_cnt++] = new boost::thread(&ReadThreadAsyncStoreN, args);
            }
        }
    }

    for ( int i = 0 ; i < th_cnt ; i++ ) {
        std::cout << "Joining ..." << std::endl << std::flush;
        threads[i]->join();
    }
}

static void SerialReadTest(const LocalArgsDescriptor &a, int cnt, uint32_t write_cnt)
{
    std::string cluster = *a.cluster_name;
    std::string space = *a.space_name;
    std::string store = *a.store_name;

    for ( int i = 0 ; i < cnt ; i++ ) {
        cluster = *a.cluster_name;
        cluster += ("_" + to_string(i));
        for ( int j = 0 ; j < cnt ; j++ ) {
            space = *a.space_name;
            space += ("_" + to_string(j));
            for ( int k = 0 ; k < cnt ; k++ ) {
                LocalArgsDescriptor args;
                memset(&args, '\0', sizeof(args));
                args.key = a.key;
                args.key_len = a.key_len;
                args.data = a.data;
                args.data_len = a.data_len;
                args.buf = a.buf;
                args.buf_len = a.buf_len;

                store = *a.store_name;
                store += ("_" + to_string(k));

                std::cout << "cluster: " << cluster <<
                    " space: " << space <<
                    " store: " << store << std::endl << std::flush;
                ReadStoreN(args, cluster, space, store, write_cnt);
            }
        }
    }
}

static void ConcurrentReadContTest(const LocalArgsDescriptor &a, int cnt, uint32_t write_cnt)
{
    std::string cluster = *a.cluster_name;
    std::string space = *a.space_name;
    std::string store = *a.store_name;

    boost::thread *threads[200];
    if ( cnt > 200 ) {
        std::cout << "cnt to big: " << cnt << std::endl << std::flush;
        return;
    }

    int th_cnt = 0;
    for ( int i = 0 ; i < cnt ; i++ ) {
        cluster = *a.cluster_name;
        cluster += ("_" + to_string(i));
        for ( int j = 0 ; j < cnt ; j++ ) {
            space = *a.space_name;
            space += ("_" + to_string(j));
            for ( int k = 0 ; k < cnt ; k++ ) {
                LocalArgsDescriptor *args = new LocalArgsDescriptor();
                memset(args, '\0', sizeof(*args));
                args->key = a.key;
                args->key_len = a.key_len;
                args->data = a.data;
                args->data_len = a.data_len;
                args->buf = a.buf;
                args->buf_len = a.buf_len;

                store = *a.store_name;
                store += ("_" + to_string(k));

                args->cluster_name = new std::string(cluster);
                args->space_name = new std::string(space);
                args->store_name = new std::string(store);
                args->cluster_idx = write_cnt;

                std::cout << "cluster: " << *args->cluster_name <<
                    " space: " << *args->space_name <<
                    " store: " << *args->store_name << std::endl << std::flush;

                threads[th_cnt++] = new boost::thread(&ReadContThreadAsyncStoreN, args);
            }
        }
    }

    for ( int i = 0 ; i < th_cnt ; i++ ) {
        std::cout << "Joining ..." << std::endl << std::flush;
        threads[i]->join();
    }
}

static void SerialReadContTest(const LocalArgsDescriptor &a, int cnt, uint32_t write_cnt)
{
    std::string cluster = *a.cluster_name;
    std::string space = *a.space_name;
    std::string store = *a.store_name;

    for ( int i = 0 ; i < cnt ; i++ ) {
        cluster = *a.cluster_name;
        cluster += ("_" + to_string(i));
        for ( int j = 0 ; j < cnt ; j++ ) {
            space = *a.space_name;
            space += ("_" + to_string(j));
            for ( int k = 0 ; k < cnt ; k++ ) {
                LocalArgsDescriptor args;
                memset(&args, '\0', sizeof(args));
                args.key = a.key;
                args.key_len = a.key_len;
                args.data = a.data;
                args.data_len = a.data_len;
                args.buf = a.buf;
                args.buf_len = a.buf_len;

                store = *a.store_name;
                store += ("_" + to_string(k));

                std::cout << "cluster: " << cluster <<
                    " space: " << space <<
                    " store: " << store << std::endl << std::flush;
                ReadContStoreN(args, cluster, space, store, write_cnt);
            }
        }
    }
}

static void SerialReadRangeTest(const LocalArgsDescriptor &a, int cnt, uint32_t write_cnt)
{
    std::cout << "cluster: " << *a.cluster_name <<
        " space: " << *a.space_name <<
        " store: " << *a.store_name << std::endl << std::flush;
    ReadRangeStore(const_cast<LocalArgsDescriptor &>(a), *a.cluster_name, *a.space_name, *a.store_name);
}

static void SerialReadOneTest(const LocalArgsDescriptor &a, int cnt, uint32_t write_cnt)
{
    std::cout << "cluster: " << *a.cluster_name <<
        " space: " << *a.space_name <<
        " store: " << *a.store_name << std::endl << std::flush;
    ReadOneStore(const_cast<LocalArgsDescriptor &>(a), *a.cluster_name, *a.space_name, *a.store_name);
    std::cout << "key: " << a.key <<
        " buf_len: " << a.buf_len <<
        " buf: " << a.buf << std::endl << std::flush;
}

static void MergeStore(LocalArgsDescriptor &args,
    RCSDBAPI &api)
{
    std::string cluster = *args.cluster_name;
    std::string space = *args.space_name;
    std::string store = *args.store_name;

    // get Kvs local static functions to find space by name and create one if not found
    api.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_CREATE));
    api.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_DESTROY));
    api.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_GET_BY_NAME));

    uint64_t st_id = api.get_by_name_fun(args);

    std::cout << "Store avaialble for ops id: " << st_id << " name: " << store << std::endl << std::flush;

    // get Kvs object functions to manipulate a space
    api.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_OPEN));
    api.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_CLOSE));
    api.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_PUT));
    api.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET));
    api.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_DELETE));
    api.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_GET_RANGE));
    api.merge = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(CreateLocalName(cluster, space, store, 0, FUNCTION_MERGE));

    // open store
    api.open(args);

    api.merge(args);
}

//
// Main section
//
typedef enum _eOpt {
    eOptC  = 0,
    eOptR  = 1,
    eOptW  = 2,
    eOptD  = 3,
    eOptN  = 4,
    eOptT  = 5,
    eOptS  = 6,
    eOptE  = 7,
    eOptF  = 8,
    eOptK  = 9,
    eOptL  = 10,
    eOptU  = 11,
    eOptZ  = 12,
    eOptX  = 13,
    eOptY  = 14,
    eOptG  = 15
} eOpt;

static void usage(const char *prog)
{
    std::cout << prog << " cluster space store store_cnt -{c|w|d|n|t|s|e|f|k|l|z|u|x|y|u|g} {key|overwr_dup} {data|end_key} cluster_id space_id store_id {read_cnt|write_cnt} data_size" << std::endl << std::flush;
    std::cout << prog << "                               -c (create multiple stores)" << std::endl << std::flush;
    std::cout << prog << "                               -w (write to a specified store)" << std::endl << std::flush;
    std::cout << prog << "                               -d (delete from a specified store)" << std::endl << std::flush;
    std::cout << prog << "                               -n (write write_cnt stores)" << std::endl << std::flush;
    std::cout << prog << "                               -t (write write_cnt concurrently to all stores)" << std::endl << std::flush;
    std::cout << prog << "                               -s (write write_cnt serially to all stores)" << std::endl << std::flush;
    std::cout << prog << "                               -e (read read_cnt concurrently from all stores)" << std::endl << std::flush;
    std::cout << prog << "                               -f (read read_cnt serially from all stores)" << std::endl << std::flush;
    std::cout << prog << "                               -k (read continuously read_cnt concurrently from all stores)" << std::endl << std::flush;
    std::cout << prog << "                               -l (read continuously read_cnt serially from all stores)" << std::endl << std::flush;
    std::cout << prog << "                               -z (search range key-end_end serially from a specified store)" << std::endl << std::flush;
    std::cout << prog << "                               -x (search by a key from a specified store)" << std::endl << std::flush;
    std::cout << prog << "                               -y (merge (update) new value by a key to a specified store)" << std::endl << std::flush;
    std::cout << prog << "                               -u (write/read write_cnt random concurrently to stores)" << std::endl << std::flush;
    std::cout << prog << "                               -g (write/read write_cnt to stores)" << std::endl << std::flush;
}

//
// GetRange callback
//
static bool GetRangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *arg)
{
    char loc_key[1024];
    char loc_data[1024];
    ::memcpy(loc_key, key, key_len);
    ::memcpy(loc_data, data, data_len);
    loc_key[key_len] = '\0';
    loc_data[data_len] = '\0';
    std::cout << "key: " << loc_key << " key_len: " << key_len <<
        " data: " << loc_data << " data_len: " << data_len <<
        " " << std::string(__FUNCTION__) << std::endl << std::flush;
    return true;
}

int main(int argc, char **argv)
{
    std::string cluster = "my_cluster";
    std::string space = "my_space";
    std::string store = "my_store";
    std::string key = "";
    std::string end_key = "";
    std::string data = "DATADATADATADATADATADATADATADATA";
    const char *data_ptr = data.c_str();
    uint64_t cluster_id = 100;
    uint64_t space_id = 1000;
    uint64_t store_id = 10000;
    uint32_t write_cnt = 1;
    size_t data_size = strlen(data_ptr);
    char big_data[1000000];
    bool overwr_dup = false;

    int cnt = 8;
    eOpt opt = eOptR;

    if ( argc > 1 ) {
        if ( std::string(argv[1]) == "-h" ) {
            usage(argv[0]);
            return 0;
        }
    }
    if ( argc > 3 ) {
        cluster = argv[1];
        space = argv[2];
        store = argv[3];
    }
    if ( argc > 4 ) {
        cnt = atoi(argv[4]);
    }
    if ( argc > 5 ) {
        if ( std::string(argv[5]) == "-c" ) {
            opt = eOptC;
        } else if ( std::string(argv[5]) == "-r" ) {
            opt = eOptR;
        } else if ( std::string(argv[5]) == "-w" ) {
            opt = eOptW;
        } else if ( std::string(argv[5]) == "-d" ) {
            opt = eOptD;
        } else if ( std::string(argv[5]) == "-n" ) {
            opt = eOptN;
        } else if ( std::string(argv[5]) == "-t" ) {
            opt = eOptT;
        } else if ( std::string(argv[5]) == "-s" ) {
            opt = eOptS;
        } else if ( std::string(argv[5]) == "-e" ) {
            opt = eOptE;
        } else if ( std::string(argv[5]) == "-e" ) {
            opt = eOptE;
        } else if ( std::string(argv[5]) == "-k" ) {
            opt = eOptK;
        } else if ( std::string(argv[5]) == "-l" ) {
            opt = eOptL;
        } else if ( std::string(argv[5]) == "-u" ) {
            opt = eOptU;
        } else if ( std::string(argv[5]) == "-z" ) {
            opt = eOptZ;
        } else if ( std::string(argv[5]) == "-x" ) {
            opt = eOptX;
        } else if ( std::string(argv[5]) == "-y" ) {
            opt = eOptY;
        } else if ( std::string(argv[5]) == "-g" ) {
            opt = eOptG;
        } else {
            usage(argv[0]);
            return 1;
        }
    }
    if ( argc > 6 ) {
        key = argv[6];
        if ( opt == eOptC ) {
            overwr_dup = static_cast<bool>(atoi(key.c_str()));
        }
    }
    if ( argc > 7 ) {
        data = argv[7];
        data_size = data.size();
        if ( opt == eOptZ ) {
            end_key = data;
        }
    }
    if ( argc > 10 ) {
        cluster_id = static_cast<uint64_t>(atol(argv[8]));
        space_id = static_cast<uint64_t>(atol(argv[9]));
        store_id = static_cast<uint64_t>(atol(argv[10]));
    }
    if ( argc > 11 ) {
        write_cnt = static_cast<uint32_t>(atoi(argv[11]));
    }
    if ( argc > 12 ) {
        data_size = static_cast<size_t>(atoi(argv[12]));
    }

    if ( data_size > sizeof(big_data) ) {
        usage(argv[0]);
        return 2;
    }

    if ( data_size > data.size() ) {
        data_ptr = big_data;
        memset(const_cast<char*>(data_ptr), 'A', data_size);
    }

    std::cout << "cluster: " << cluster <<
        " space: " << space <<
        " store: " << store <<
        " cnt: " << cnt <<
        " key: " << key <<
        " key_size: " << key.size() <<
        " end_key: " << end_key <<
        " cluster_id: " << cluster_id <<
        " space_id: " << space_id <<
        " store_id: " << store_id <<
        " overwr_dup: " << overwr_dup <<
        " write_cnt: " << write_cnt <<
        " data_size: " << data_size <<
        std::endl << std::flush;

    RCSDBAPI my_cluster_api;
    RCSDBAPI my_space_api;
    RCSDBAPI my_store_api;
    try {
        switch ( opt ) {
        case eOptC:
            {
                std::string cluster_new = cluster;
                std::string space_new = space;
                std::string store_new = store;
                for ( int i = 0 ; i < cnt ; i++ ) {
                    LocalArgsDescriptor args;
                    memset(&args, '\0', sizeof(args));
                    args.bloom_bits = 4;

                    cluster_new = cluster;
                    cluster_new += ("_" + to_string(i));

                    OpenCreateCluster(args, cluster_new, my_cluster_api);
                    args.bloom_bits = 4;

                    for ( int j = 0 ; j < cnt ; j++ ) {
                        space_new = space;
                        space_new += ("_" + to_string(j));

                        OpenCreateSpace(args, cluster_new, space_new, my_space_api);
                        args.bloom_bits = 4;

                        for ( int k = 0 ; k < cnt ; k++ ) {
                            store_new = store;
                            store_new += ("_" + to_string(k));

                            args.bloom_bits = 4;
                            args.overwrite_dups = overwr_dup;
                            OpenCreateStore(args, cluster_new, space_new, store_new, my_store_api);
                        }
                    }
                }
            }
            break;
        case eOptR:
            {
                LocalArgsDescriptor args;
                memset(&args, '\0', sizeof(args));
                args.key = key.c_str();
                args.key_len = key.size();
                args.cluster_id = cluster_id;
                args.space_id = space_id;
                args.store_id = store_id;

                ReadStore(args, cluster, space, store, my_store_api);
            }
            break;
        case eOptW:
            {
                LocalArgsDescriptor args;
                memset(&args, '\0', sizeof(args));
                args.key = key.c_str();
                args.key_len = key.size();
                args.data = data_ptr;
                args.data_len = data_size;
                args.cluster_id = cluster_id;
                args.space_id = space_id;
                args.store_id = store_id;

                WriteStore(args, cluster, space, store, my_store_api);
            }
            break;
        case eOptN:
            {
                LocalArgsDescriptor args;
                memset(&args, '\0', sizeof(args));
                args.key = key.c_str();
                args.key_len = key.size();
                args.data = data_ptr;
                args.data_len = data_size;
                args.cluster_id = cluster_id;
                args.space_id = space_id;
                args.store_id = store_id;

                WriteStoreN(args, cluster, space, store, my_store_api, write_cnt);
            }
            break;
        case eOptD:
            {
                LocalArgsDescriptor args;
                memset(&args, '\0', sizeof(args));
                args.key = key.c_str();
                args.key_len = key.size();
                args.cluster_id = cluster_id;
                args.space_id = space_id;
                args.store_id = store_id;

                DeleteStore(args, cluster, space, store, my_store_api);
            }
            break;
        case eOptT:
            {
                LocalArgsDescriptor args;
                memset(&args, '\0', sizeof(args));
                args.key = key.c_str();
                args.key_len = key.size();
                args.data = data_ptr;
                args.data_len = data_size;
                args.cluster_id = cluster_id;
                args.space_id = space_id;
                args.store_id = store_id;
                args.cluster_name = &cluster;
                args.space_name = &space;
                args.store_name = &store;

                ConcurrentWriteTest(args, cnt, write_cnt);
            }
            break;
        case eOptS:
            {
                LocalArgsDescriptor args;
                memset(&args, '\0', sizeof(args));
                args.key = key.c_str();
                args.key_len = key.size();
                args.data = data_ptr;
                args.data_len = data_size;
                args.cluster_id = cluster_id;
                args.space_id = space_id;
                args.store_id = store_id;
                args.cluster_name = &cluster;
                args.space_name = &space;
                args.store_name = &store;

                SerialWriteTest(args, cnt, write_cnt);
            }
            break;
        case eOptE:
            {
                LocalArgsDescriptor args;
                memset(&args, '\0', sizeof(args));
                args.key = key.c_str();
                args.key_len = key.size();
                args.buf = big_data;
                args.buf_len = sizeof(big_data);
                args.cluster_id = cluster_id;
                args.space_id = space_id;
                args.store_id = store_id;
                args.cluster_name = &cluster;
                args.space_name = &space;
                args.store_name = &store;

                SerialReadTest(args, cnt, write_cnt);
            }
            break;
        case eOptF:
            {
                LocalArgsDescriptor args;
                memset(&args, '\0', sizeof(args));
                args.key = key.c_str();
                args.key_len = key.size();
                args.buf = big_data;
                args.buf_len = sizeof(big_data);
                args.cluster_id = cluster_id;
                args.space_id = space_id;
                args.store_id = store_id;
                args.cluster_name = &cluster;
                args.space_name = &space;
                args.store_name = &store;

                ConcurrentReadTest(args, cnt, write_cnt);
            }
            break;
        case eOptK:
            {
                LocalArgsDescriptor args;
                memset(&args, '\0', sizeof(args));
                args.key = key.c_str();
                args.key_len = key.size();
                args.buf = big_data;
                args.buf_len = sizeof(big_data);
                args.cluster_id = cluster_id;
                args.space_id = space_id;
                args.store_id = store_id;
                args.cluster_name = &cluster;
                args.space_name = &space;
                args.store_name = &store;

                ConcurrentReadContTest(args, cnt, write_cnt);
            }
            break;
        case eOptL:
            {
                LocalArgsDescriptor args;
                memset(&args, '\0', sizeof(args));
                args.key = key.c_str();
                args.key_len = key.size();
                args.buf = big_data;
                args.buf_len = sizeof(big_data);
                args.cluster_id = cluster_id;
                args.space_id = space_id;
                args.store_id = store_id;
                args.cluster_name = &cluster;
                args.space_name = &space;
                args.store_name = &store;

                SerialReadContTest(args, cnt, write_cnt);
            }
            break;
        case eOptZ:
            {
                LocalArgsDescriptor args;
                memset(&args, '\0', sizeof(args));
                if ( key != "" ) {
                    args.key = key.c_str();
                    args.key_len = key.size();
                } else {
                    args.key = 0;
                    args.key_len = 0;
                }
                if ( end_key != "" ) {
                    args.end_key = end_key.c_str();
                    args.key_len = end_key.size();
                } else {
                    args.end_key = 0;
                }
                args.buf = big_data;
                args.buf_len = sizeof(big_data);
                args.cluster_id = cluster_id;
                args.space_id = space_id;
                args.store_id = store_id;
                args.cluster_name = &cluster;
                args.space_name = &space;
                args.store_name = &store;
                args.range_callback = GetRangeCallback;
                args.range_callback_arg = 0;

                SerialReadRangeTest(args, cnt, write_cnt);
            }
            break;
        case eOptX:
            {
                LocalArgsDescriptor args;
                memset(&args, '\0', sizeof(args));
                if ( key != "" ) {
                    args.key = key.c_str();
                    args.key_len = key.size();
                } else {
                    args.key = 0;
                    args.key_len = 0;
                }
                args.buf = big_data;
                args.buf_len = sizeof(big_data);
                args.cluster_id = cluster_id;
                args.space_id = space_id;
                args.store_id = store_id;
                args.cluster_name = &cluster;
                args.space_name = &space;
                args.store_name = &store;
                args.range_callback = GetRangeCallback;
                args.range_callback_arg = 0;

                SerialReadOneTest(args, cnt, write_cnt);
            }
            break;
        case eOptY:
            {
                LocalArgsDescriptor args;
                memset(&args, '\0', sizeof(args));
                args.key = key.c_str();
                args.key_len = key.size();
                args.data = data_ptr;
                args.data_len = data_size;
                args.cluster_id = cluster_id;
                args.space_id = space_id;
                args.store_id = store_id;
                args.cluster_name = &cluster;
                args.space_name = &space;
                args.store_name = &store;

                MergeStore(args, my_store_api);
            }
            break;
        case eOptU:
        case eOptG:
        default:
            break;
        }
    } catch (const RcsdbFacade::Exception &e) {
        std::cout << "RcsdbFacade::Exception: " << e.what() << std::endl << std::flush;
        return 1;
    } catch (const SoeApi::Exception &e) {
        std::cout << "SoeApi::Exception: " << e.what() << std::endl << std::flush;
        return 1;
    } catch (const std::bad_cast &e) {
        std::cout << "Exception std::bad_cast " << e.what() << std::endl << std::flush;
        return 1;
    } catch (const std::exception &e) {
        std::cout << "Exception std::exception " << e.what() << std::endl << std::flush;
        return 1;
    } catch (...) {
        std::cout << "Exception ... " << std::endl << std::flush;
        return 1;
    }

    return 0;
}




