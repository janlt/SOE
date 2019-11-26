/**
 * @file    main.cpp
 * @author  Jan Lisowiec <jlisowiec@gmail.com>
 * @version 1.0
 * @brief   Soe integrity test driver
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
 * main.cpp
 *
 *  Created on: Nov 20, 2017
 *      Author: Jan Lisowiec
 */


#include <pwd.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <string>
#include <vector>

#include <vector>
#include <map>
#include <iostream>
#include <string>
#include <typeinfo>
#include <new>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread.hpp>

#include "soe_soe.hpp"

using namespace std;
using namespace Soe;

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#define SOE_BIG_BUFFER   100000000
#define MAX_THREAD_COUNT 64
#define MAX_NUM_OPS      20000000

#define gettid() syscall(SYS_gettid)

const string rcsdb_p = RcsdbFacade::GetProvider();
const string default_p = Provider::default_provider;
const string metadbcli_p = MetadbcliFacade::GetProvider();
const string kvs_p = KvsFacade::GetProvider();
const string not_p_p = string("NOTPROVIDED");

static const string CurrentDateTimeStr(char *buf)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    long int ms = tp.tv_sec * 1000; // + tp.tv_usec / 1000; //get current timestamp in milliseconds
    sprintf(buf, "%ld", ms);

    return buf;
}

static string GetProviderName(int prov)
{
    switch ( prov ) {
    case 0:
        return rcsdb_p;
        break;
    case 1:
        cerr << "provider currently not supported" << endl;
        return not_p_p;
        break;
    case 2:
        return metadbcli_p;
        break;
    case 3:
        cerr << "provider currently not supported" << endl;
        return not_p_p;
        break;
    case 4:
        return default_p;
        break;
    default:
        cerr << "provider currently not supported" << endl;
        return not_p_p;
        break;
    }

    return not_p_p;
}

Session::eSyncMode GetSyncMode(int sync_m)
{
    Session::eSyncMode sync_mode = Session::eSyncDefault;
    switch ( sync_m ) {
    case 0:
        sync_mode = Session::eSyncDefault;
        break;
    case 1:
        sync_mode = Session::eSyncFsync;
        break;
    case 2:
        sync_mode = Session::eSyncAsync;
        break;
    default:
        sync_mode = Session::eSyncDefault;
        break;
    }

    return sync_mode;
}

typedef enum _TestType {
    eIntegrityRegression1          = 1,
    eIntegrityRegression2          = 2,
    eIntegrityRegression3          = 3,
    eAsyncPut                      = 4,
    eAsyncGet                      = 5,
    eAsyncDelete                   = 6,
    eAsyncMerge                    = 7,
    eAsyncPutSet                   = 8,
    eAsyncGetSet                   = 9,
    eAsyncDeleteSet                = 10,
    eAsyncMergeSet                 = 11,
    eAsyncConcurrentPutSet         = 12,
    eAsyncConcurrentGetSet         = 13,
    eAsyncConcurrentDeleteSet      = 14,
    eAsyncConcurrentMergeSet       = 15,
    eAsyncSyncMixedOpenCloseSet    = 16,
    eAsyncSyncMixedSet             = 17,
    eAsyncPrematureCloseSet        = 18,
    eAsyncMixedPutGet              = 19
} TestType;

// ==============================================================

struct RegressionArgs {
    Session          *sess;
    Subsetable       *sub;
    eSessionSyncMode  sess_sync_mode;
    const char       *cluster_name;
    const char       *space_name;
    const char       *store_name ;
    int               provider;
    const char       *provider_name;
    Iterator         *sess_it;
    Iterator         *sub_it;
    const char       *key;
    size_t            key_size;
    const char       *end_key;
    size_t            end_key_size;
    int               it_dir;
    int               default_first;
    char             *value;
    size_t            value_buffer_size;
    size_t            value_size;
    uint32_t          num_ops;
    uint32_t          num_loops;
    uint32_t          num_regressions;
    const char       *snap_back_subset_name;
    uint32_t          snap_back_id;
    int               transactional;
    int               sync_type;
    int               debug;
    int               wait_for_key;
    int               hex_dump;
    useconds_t        sleep_usecs;
    int               no_print;
    uint64_t          io_counter;
    int               group_write;
    char            **key_ptrs;
    size_t           *key_sizes;
    char            **value_ptrs;
    size_t           *value_sizes;
    uint32_t          thread_count;
    int               random_keys;
    int               destroy_store_before;
    int               use_binary_keys;
};

class IntegrityTest
{
    RegressionArgs   args;

public:
    IntegrityTest(RegressionArgs &_args):
        args(_args)
    {}

    virtual ~IntegrityTest() {}

    RegressionArgs &GetArgs()
    {
        return args;
    }

    int OpenCreateSession();
    int OpenCreateSubset();
    int DeleteSession();
    int DeleteSubset();
    int CloseSession();

    int CreateSessionIterator();
    int CreateSubsetIterator();
    int DeleteSessionIterator();
    int DeleteSubsetIterator();

    int DestroyStore();

    Session *GetSession();
    Subsetable *GetSubset();
    Iterator *GetSessionIterator();
    Iterator *GetSubsetIterator();
};

int IntegrityTest::OpenCreateSession()
{
    int i_ret = -1;

    Session::eSyncMode sess_sync_mode = GetSyncMode(args.sync_type);
    string provider_name = GetProviderName(args.provider);

    cout << "creating session ... " << args.cluster_name << "/" << args.space_name << "/" << args.store_name <<
        " sync: " << sess_sync_mode <<
        " provider: " << provider_name << endl;

    if ( args.sess ) {
        cerr << "args.sess not null" << endl;
        return i_ret;
    }

    try {
        args.sess = new Session(args.cluster_name, args.space_name, args.store_name, provider_name, false, false, true, sess_sync_mode);
        Session::Status sts = args.sess->Open(args.cluster_name, args.space_name, args.store_name, provider_name, false, false, true, sess_sync_mode);
        if ( sts != Session::Status::eOk ) {
            sts = args.sess->Create();
            if ( sts != Session::Status::eOk ) {
                cerr << "failed creating session sts: " << sts << endl;
                delete args.sess;
                args.sess = 0;
            }
        }
        if ( sts == Session::Status::eOk ) {
            i_ret = 0;
        }
    } catch (const std::runtime_error &ex) {
        cerr << "Exception caught when opening/creating session " << ex.what() << endl;
        args.sess = 0;
        i_ret = -1;
    } catch ( ... ) {
        cerr << "Exception caught when opening/creating session " << endl;
        args.sess = 0;
        i_ret = -1;
    }

    return i_ret;
}

int IntegrityTest::DestroyStore()
{
    int i_ret = -1;

    if ( !args.sess ) {
        cerr << __FUNCTION__ << " args.sess null" << endl;
        return i_ret;
    }

    try {
        Session::Status sts = args.sess->DestroyStore();
        if ( sts != Session::Status::eOk ) {
            cerr << __FUNCTION__ << " Destroy store failed last sts: " << args.sess->GetLastStatus() << endl;
        } else {
            i_ret = 0;
            args.sess = 0;
        }
    } catch (const std::runtime_error &ex) {
        cerr << __FUNCTION__ << " Exception caught when destroying store " << ex.what() << endl;
        i_ret = -1;
    } catch ( ... ) {
        cerr << __FUNCTION__ << " Exception caught when destroying store " << endl;
        i_ret = -1;
    }

    return i_ret;
}

int IntegrityTest::OpenCreateSubset()
{
    int i_ret = -1;

    cout << "creating subset ... " << "/" << args.snap_back_subset_name << endl;

    if ( !args.sess ) {
        cerr << __FUNCTION__ << " args.sess null" << endl;
        return i_ret;
    }

    if ( args.sub ) {
        cerr << __FUNCTION__ << " args.sub not null" << endl;
        return i_ret;
    }

    if ( !args.snap_back_subset_name ) {
        cerr << __FUNCTION__ << " no valid subset name specified" << endl;
        return i_ret;
    }

    try {
        args.sub = args.sess->OpenSubset(args.snap_back_subset_name, string(args.snap_back_subset_name).size());
        if ( !args.sub ) {
            args.sub = args.sess->CreateSubset(args.snap_back_subset_name, string(args.snap_back_subset_name).size());
        }

        if ( !args.sub ) {
            cerr << __FUNCTION__ << " create subset failed last sts: " << args.sess->GetLastStatus() << endl;
        } else {
            i_ret = 0;
        }
    } catch (const std::runtime_error &ex) {
        cerr << __FUNCTION__ << " Exception caught when opening/creating subset " << ex.what() << endl;
        args.sub = 0;
        i_ret = -1;
    } catch ( ... ) {
        cerr << __FUNCTION__ << " Exception caught when opening/creating subset " << endl;
        args.sub = 0;
        i_ret = -1;
    }

    return i_ret;
}

int IntegrityTest::DeleteSession()
{
    if ( !args.sess ) {
        cerr << __FUNCTION__ << " args.sess null" << endl;
        return -1;
    }

    if ( args.sess_it ) {
        cerr << __FUNCTION__ << " can't delete session, delete session iterator first ..." << endl;
        return -1;
    }

    if ( args.sub ) {
        cerr << __FUNCTION__ << " can't delete session, delete subset first ..." << endl;
        return -1;
    }

    if ( args.sub_it ) {
        cerr << __FUNCTION__ << " can't delete session, delete subset iterator first ..." << endl;
        return -1;
    }

    delete args.sess;
    args.sess = 0;

    return 0;
}

int IntegrityTest::CloseSession()
{
    if ( !args.sess ) {
        cerr << __FUNCTION__ << " args.sess null" << endl;
        return -1;
    }

    args.sess->Close();

    return 0;
}

int IntegrityTest::DeleteSubset()
{
    int i_ret = -1;

    if ( !args.sub ) {
        cerr << __FUNCTION__ << " args.sub null" << endl;
        return i_ret;
    }

    if ( args.sub_it ) {
        cerr << __FUNCTION__ << " can't delete subset, delete subset iterator first ..." << endl;
        return i_ret;
    }

    try {
        Session::Status sts = args.sess->DeleteSubset(args.sub);
        if ( sts != Session::Status::eOk ) {
            cerr << __FUNCTION__ << " delete subset failed last sts: " << args.sess->GetLastStatus() << endl;
        } else {
            i_ret = 0;
            args.sub = 0;
        }
    } catch (const std::runtime_error &ex) {
        cerr << __FUNCTION__ << " Exception caught when deleting subset " << ex.what() << endl;
        i_ret = -1;
    } catch ( ... ) {
        cerr << __FUNCTION__ << " Exception caught when deleting subset " << endl;
        i_ret = -1;
    }

    return i_ret;
}

int IntegrityTest::CreateSessionIterator()
{
    int i_ret = -1;

    if ( !args.sess ) {
        cerr << __FUNCTION__ << " args.sess null" << endl;
        return i_ret;
    }

    if ( args.sess_it ) {
        cerr << __FUNCTION__ << " args.sess_it not null" << endl;
        return i_ret;
    }

    Iterator::eIteratorDir dir = args.it_dir == 1 ? Iterator::eIteratorDir::eIteratorDirForward : Iterator::eIteratorDir::eIteratorDirBackward;

    try {
        args.sess_it = args.sess->CreateIterator(dir, args.key, args.key_size, args.end_key, args.end_key_size);
        if ( !args.sess_it ) {
            cerr << __FUNCTION__ << " create session iterator failed last sts: " << args.sess->GetLastStatus() << endl;
        } else {
            i_ret = 0;
        }
    } catch (const std::runtime_error &ex) {
        cerr << __FUNCTION__ << " Exception caught when creating session iterator " << ex.what() << endl;
        i_ret = -1;
    } catch ( ... ) {
        cerr << __FUNCTION__ << " Exception caught when creating session iterator " << endl;
        i_ret = -1;
    }

    return i_ret;
}

int IntegrityTest::CreateSubsetIterator()
{
    int i_ret = -1;

    if ( !args.sess ) {
        cerr << __FUNCTION__ << " args.sess null" << endl;
        return i_ret;
    }

    if ( !args.sub ) {
        cerr << __FUNCTION__ << " args.sub null" << endl;
        return i_ret;
    }

    if ( args.sub_it ) {
        cerr << __FUNCTION__ << " args.sub_it not null" << endl;
        return i_ret;
    }

    Iterator::eIteratorDir dir = args.it_dir == 1 ? Iterator::eIteratorDir::eIteratorDirForward : Iterator::eIteratorDir::eIteratorDirBackward;

    try {
        args.sub_it = args.sub->CreateIterator(dir, args.key, args.key_size, args.end_key, args.end_key_size);
        if ( !args.sub_it ) {
            cerr << __FUNCTION__ << " create subset subset failed last sts: " << args.sess->GetLastStatus() << endl;
        } else {
            i_ret = 0;
        }
    } catch (const std::runtime_error &ex) {
        cerr << __FUNCTION__ << " Exception caught when creating subset iterator " << ex.what() << endl;
        i_ret = -1;
    } catch ( ... ) {
        cerr << __FUNCTION__ << " Exception caught when creating subset iterator " << endl;
        i_ret = -1;
    }

    return i_ret;
}

int IntegrityTest::DeleteSessionIterator()
{
    int i_ret = -1;

    if ( !args.sess_it ) {
        cerr << __FUNCTION__ << " args.sess_it null" << endl;
        return i_ret;
    }

    try {
        Session::Status sts = args.sess->DestroyIterator(args.sess_it);
        if ( sts != Session::Status::eOk ) {
            cerr << __FUNCTION__ << " delete session iterator failed last sts: " << sts << endl;
        } else {
            i_ret = 0;
            args.sess_it = 0;
        }
    } catch (const std::runtime_error &ex) {
        cerr << __FUNCTION__ << " Exception caught when deleting session iterator " << ex.what() << endl;
        i_ret = -1;
    } catch ( ... ) {
        cerr << __FUNCTION__ << " Exception caught when deleting session iterator " << endl;
        i_ret = -1;
    }

    return i_ret;
}

int IntegrityTest::DeleteSubsetIterator()
{
    int i_ret = -1;

    if ( !args.sub_it ) {
        cerr << __FUNCTION__ << " args.sess_it null" << endl;
        return i_ret;
    }

    try {
        Session::Status sts = args.sub->DestroyIterator(args.sub_it);
        if ( sts != Session::Status::eOk ) {
            cerr << __FUNCTION__ << " delete subset iterator failed last sts: " << sts << endl;
        } else {
            i_ret = 0;
            args.sub_it = 0;
        }
    } catch (const std::runtime_error &ex) {
        cerr << __FUNCTION__ << " Exception caught when deleting subset iterator " << ex.what() << endl;
        i_ret = -1;
    } catch ( ... ) {
        cerr << __FUNCTION__ << " Exception caught when deleting subset iterator " << endl;
        i_ret = -1;
    }

    return i_ret;
}

Session *IntegrityTest::GetSession()
{
    return args.sess;
}

Subsetable *IntegrityTest::GetSubset()
{
    return args.sub;
}

Iterator *IntegrityTest::GetSessionIterator()
{
    return args.sess_it;
}

Iterator *IntegrityTest::GetSubsetIterator()
{
    return args.sub_it;
}

//
// ======================= IntegrityRegression1 ==============================
//
static int IntegrityRegression1(IntegrityTest &test)
{
    int ret = 0;

    ret = test.OpenCreateSession();
    if ( ret ) {
        return ret;
    }

    ret = test.OpenCreateSubset();
    if ( ret ) {
        return ret;
    }

    ret = test.CreateSessionIterator();
    if ( ret ) {
        return ret;
    }

    ret = test.CreateSubsetIterator();
    if ( ret ) {
        return ret;
    }

    Session *sess = test.GetSession();
    if ( !sess ) {
        return ret;
    }

    Subsetable *sub = test.GetSubset();
    if ( !sub ) {
        ret = -1;
        (void )test.DeleteSession();
        return ret;
    }

    Iterator *sess_it = test.GetSessionIterator();
    if ( !sess_it ) {
        ret = -1;
        (void )test.DeleteSession();
        (void )test.DeleteSubset();
        return ret;
    }

    Iterator *sub_it = test.GetSubsetIterator();
    if ( !sub_it ) {
        ret = -1;
        (void )test.DeleteSession();
        (void )test.DeleteSubset();
        (void )test.DeleteSessionIterator();
        return ret;
    }

    ret = test.DeleteSubsetIterator();
    if ( ret ) {
        return ret;
    }

    ret = test.DeleteSessionIterator();
    if ( ret ) {
        return ret;
    }

    ret = test.DeleteSubset();
    if ( ret ) {
        return ret;
    }

    ret = test.DeleteSession();
    if ( ret ) {
        return ret;
    }

    return ret;
}

// ==============================================================

class RecordCRC32
{
    uint32_t crc;

public:
    RecordCRC32():
        crc(0)
    {}
    virtual ~RecordCRC32() {}
    void Calculate(unsigned char *block, uint32_t block_size);
    bool operator == (const RecordCRC32 &_r)
    {
        return crc == _r.crc;
    }
    bool operator != (const RecordCRC32 &_r)
    {
        return crc != _r.crc;
    }
    uint32_t CRC() { return crc; }
};

void RecordCRC32::Calculate(unsigned char *block, uint32_t block_size)
{
   uint32_t i;
   int j;
   uint32_t byte, mask;

   i = 0;
   uint32_t r_crc = 0xFFFFFFFF;

   uint32_t b_block_size = block_size*sizeof(uint32_t);
   for ( i = 0 ; i < b_block_size ; i++ ) {
      byte = block[i];
      r_crc = r_crc ^ byte;
      for ( j = 7 ; j >= 0 ; j-- ) {
         mask = -(r_crc & 1);
         r_crc = (r_crc >> 1) ^ (0xEDB88320 & mask);
      }
   }

   crc = ~r_crc;
}



class TestRecord
{
protected:
    string         key;
    uint32_t       data_size;
    unsigned char *data;

    static uint32_t counter;

public:
    TestRecord(string &_key, uint32_t _data_size);
    TestRecord(string &_key, unsigned char *_data, uint32_t _data_size);
    virtual ~TestRecord() {}
};

uint32_t TestRecord::counter = 100;

TestRecord::TestRecord(string &_key, uint32_t _data_size):
    key(_key),
    data_size(_data_size),
    data(0)
{
    data = new unsigned char[data_size*sizeof(uint32_t)];

    ::srand(time(0) + TestRecord::counter++);

    for ( uint32_t i = 0 ; i < data_size ; i++ ) {
        *reinterpret_cast<uint32_t *>(&data[i*sizeof(uint32_t)]) = static_cast<uint32_t>(::rand());
    }
}

TestRecord::TestRecord(string &_key, unsigned char *_data, uint32_t _data_size):
    key(_key),
    data_size(_data_size),
    data(_data)
{}



class TestCRC32Record: protected TestRecord
{
    RecordCRC32 crc;

public:
    TestCRC32Record(string &_key, uint32_t _data_size);
    TestCRC32Record(string &_key, char *_data, uint32_t _data_size);
    virtual ~TestCRC32Record() {}
    int CheckCRC32(unsigned char *_record, uint32_t _record_size);
    unsigned char *Data() { return data; }
    uint32_t DataSize() { return data_size; }
    void DumpHexData();
    string Key() { return key; }
    uint32_t KeySize() { return key.size(); }
    uint32_t GetCRC() { return crc.CRC(); }
};

void TestCRC32Record::DumpHexData()
{
    cout << hex;
    for ( uint32_t i = 0 ; i < data_size ; i++ ) {
        cout << "0x" << setw(8) << setfill('0') << *reinterpret_cast<uint32_t *>(&data[i*sizeof(uint32_t)]) << " ";
        if ( !(i%8) ) {
            cout << endl;
        }
    }
    cout << dec;
}

TestCRC32Record::TestCRC32Record(string &_key, uint32_t _data_size):
    TestRecord(_key, _data_size)
{
    crc.Calculate(data, data_size);
}

TestCRC32Record::TestCRC32Record(string &_key, char *_data, uint32_t _data_size):
    TestRecord(_key, reinterpret_cast<unsigned char *>(_data), _data_size)
{
    crc.Calculate(data, data_size);
}

int TestCRC32Record::CheckCRC32(unsigned char *_record, uint32_t _record_size)
{
    RecordCRC32 r_crc;
    r_crc.Calculate(_record, _record_size);

    if ( r_crc != crc ) {
        return -1;
    }

    return 0;
}



//
// ======================= IntegrityRegression2 ==============================
//
static int IntegrityRegression2(IntegrityTest &test)
{
    int ret = 0;

    ret = test.OpenCreateSession();
    if ( ret ) {
        return ret;
    }

    ::srand(time(0));

    // write records and save CRCs
    // i = # different sizes
    // j = # records for each size
    list<TestCRC32Record *> r_list;
    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions ; i++ ) {
        uint32_t size = 8*pow(2, i + 1);
        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            string key = "KEY_" + to_string(static_cast<uint32_t>(::rand())) + "_" + to_string(i) + "_" + to_string(j);
            TestCRC32Record *rec = new TestCRC32Record(key, size);
            r_list.push_back(rec);
        }
    }

    if ( test.GetArgs().hex_dump ) {
        for ( list<TestCRC32Record *>::iterator it = r_list.begin() ; it != r_list.end() ; it++ ) {
            cout << "Key: " << (*it)->Key() << " crc: " << (*it)->GetCRC() << " data_size: " << (*it)->DataSize();
            //(*it)->DumpHexData();
            cout << endl;
        }
    }

    // write records
    Session *sess = test.GetSession();
    if ( !sess ) {
        return ret;
    }

    uint32_t r_size = r_list.size();
    vector<pair<Soe::Duple, Soe::Duple>> elements;
    elements.reserve(r_size);

    for ( list<TestCRC32Record *>::iterator it = r_list.begin() ; it != r_list.end() ; it++ ) {
        elements.emplace_back(pair<Soe::Duple, Soe::Duple>(Soe::Duple((*it)->Key().c_str(), (*it)->KeySize()),
            Soe::Duple(reinterpret_cast<char *>((*it)->Data()), (*it)->DataSize()*sizeof(uint32_t))));
    }

    // write elements
    size_t fail_pos;
    Session::Status sts = sess->PutSet(elements, true, &fail_pos);
    if ( sts != Session::Status::eOk ) {
        ret = -1;
        cerr << "PutSet failed sts: " << sts << " fail_pos: " << fail_pos << endl;
        (void )test.DeleteSessionIterator();
        (void )test.DeleteSession();
        return ret;
    }

    ret = test.CreateSessionIterator();
    if ( ret ) {
        return ret;
    }

    // read one by one and verify CRC
    Iterator *sess_it = test.GetSessionIterator();
    if ( !sess_it ) {
        ret = -1;
        (void )test.DeleteSessionIterator();
        (void )test.DeleteSession();
        return ret;
    }

    uint32_t checked_count = 0;
    Iterator::eStatus it_sts;
    for ( it_sts = sess_it->First() ; it_sts == Iterator::eStatus::eOk && sess_it->Valid() == true ; it_sts = sess_it->Next() ) {
        string key(sess_it->Key().data());
        TestCRC32Record b_rec(key, const_cast<char *>(sess_it->Value().data()), static_cast<uint32_t>(sess_it->Value().size()/sizeof(uint32_t)));

        // find corresponding record in r_list
        bool found = false;
        cout << "Checking record Key: " << sess_it->Key().data() << endl;
        //b_rec.DumpHexData();
        //cout << endl;
        for ( list<TestCRC32Record *>::iterator it = r_list.begin() ; it != r_list.end() ; it++ ) {
            if ( (*it)->Key() == string(b_rec.Key().data()) ) {
                found = true;
                cout << "Found Key: " << (*it)->Key() << " crc: " << (*it)->GetCRC() << " data_size: " << (*it)->DataSize() << b_rec.GetCRC() << endl;

                RecordCRC32 c_crc;
                c_crc.Calculate(reinterpret_cast<unsigned char *>(const_cast<char *>(sess_it->Value().data())), static_cast<uint32_t>(sess_it->Value().size())/sizeof(uint32_t));
                if ( c_crc.CRC() != (*it)->GetCRC() ) {
                    cerr << "CRC32 not matching record crc: " << c_crc.CRC() << " saved crc: " << (*it)->GetCRC() << endl;
                } else {
                    cerr << "CRC32 matching OK ..." << endl;
                }
                break;
            }
        }
        if ( found == false ) {
            cerr << "no record found for key: " << b_rec.Key() << endl;
        } else {
            checked_count++;
        }
    }

    if ( checked_count != r_size ) {
        cerr << "Not all records found checked_count: " << checked_count << " r_size: " << r_size << endl;
    }

    ret = test.DeleteSessionIterator();
    if ( ret ) {
        return ret;
    }

    ret = test.DeleteSession();
    if ( ret ) {
        return ret;
    }

    return ret;
}

//
// ======================= IntegrityRegression3 ==============================
//
static int IntegrityRegression3(IntegrityTest &test)
{
    return test.OpenCreateSession();
}

// ==============================================================

class SetTest: public IntegrityTest
{
public:
    SetTest(RegressionArgs &_args):
        IntegrityTest(_args)
    {}

    virtual ~SetTest() {}
};

class TestSimpleRecord: protected TestRecord
{
public:
    TestSimpleRecord(string &_key, uint32_t _data_size);
    TestSimpleRecord(string &_key, unsigned char *_data, uint32_t _data_size);
    virtual ~TestSimpleRecord() {}
    unsigned char *Data() { return data; }
    uint32_t DataSize() { return data_size; }
    void DumpHexData();
    string Key() { return key; }
    uint32_t KeySize() { return key.size(); }
};

void TestSimpleRecord::DumpHexData()
{
    cout << hex;
    for ( uint32_t i = 0 ; i < data_size ; i++ ) {
        cout << "0x" << setw(8) << setfill('0') << *reinterpret_cast<uint32_t *>(&data[i*sizeof(uint32_t)]) << " ";
        if ( !(i%8) ) {
            cout << endl;
        }
    }
    cout << dec;
}

TestSimpleRecord::TestSimpleRecord(string &_key, uint32_t _data_size):
    TestRecord(_key, _data_size)
{}

TestSimpleRecord::TestSimpleRecord(string &_key, unsigned char *_data, uint32_t _data_size):
    TestRecord(_key, _data, _data_size)
{}



//
// ======================= AsyncPutTest ==============================
//
int AsyncPutTest(SetTest &test)
{
    int ret = 0;

    ret = test.OpenCreateSession();
    if ( ret ) {
        return ret;
    }

    // write records
    Session *sess = test.GetSession();
    if ( !sess ) {
        return ret;
    }

    PutFuture *put_futures[100000];
    char *keys[100000];
    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        // put element
        keys[i] = new char[test.GetArgs().key_size + 32];
        const char *tmp_key;
        size_t tmp_size;
        if ( !i ) {
            tmp_key = test.GetArgs().key;
            tmp_size = test.GetArgs().key_size;
        } else {
            ::memset(keys[i], '\0', test.GetArgs().key_size + 32);
            ::memcpy(keys[i], test.GetArgs().key, test.GetArgs().key_size);
            ::strcat(keys[i], "_");
            ::strcat(keys[i], to_string(i).c_str());

            tmp_size = ::strlen(keys[i]);
            tmp_key = keys[i];
        }
        //cout << "key(" << tmp_key << "/" << tmp_size << ")" << endl;
        PutFuture *p_fut = sess->PutAsync(tmp_key, tmp_size, test.GetArgs().value, test.GetArgs().value_size);
        if ( p_fut ) {
            if ( p_fut->GetPostRet() != Session::Status::eOk ) {
                cerr << "PutAsync failed sts: " << p_fut->GetPostRet() << endl;
                ret = -1;
                sess->DestroyFuture(p_fut);
                (void )test.DeleteSession();
                return ret;
            }
        } else {
            cerr << "PuttAsync returned null future" << endl;
            ret = -1;
            (void )test.DeleteSession();
            return ret;
        }

        put_futures[i] = p_fut;
    }

    for ( uint32_t i = 0 ; i <  test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        Session::Status sts = put_futures[i]->Get();
        if ( sts != Session::Status::eOk || put_futures[i]->GetPostRet() != Session::Status::eOk ) {
            cerr << "PutAsync failed sts: " << sts << " post_ret: " << put_futures[i]->GetPostRet() << endl;
            ret = -1;
        } else {
            cout << "PutAsync ok" << endl;
        }

        sess->DestroyFuture(put_futures[i]);
        delete [] keys[i];
    }

    (void )test.DeleteSession();
    return ret;
}


//
// ======================= AsyncGetTest ==============================
//
int AsyncGetTest(SetTest &test)
{
    int ret = 0;

    ret = test.OpenCreateSession();
    if ( ret ) {
        return ret;
    }

    // write records
    Session *sess = test.GetSession();
    if ( !sess ) {
        return ret;
    }

    // get element
    GetFuture *get_futures[100000];
    char *keys[100000];
    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        // put element
        keys[i] = new char[test.GetArgs().key_size + 32];
        const char *tmp_key;
        size_t tmp_size;
        if ( !i ) {
            tmp_key = test.GetArgs().key;
            tmp_size = test.GetArgs().key_size;
        } else {
            ::memset(keys[i], '\0', test.GetArgs().key_size + 32);
            ::memcpy(keys[i], test.GetArgs().key, test.GetArgs().key_size);
            ::strcat(keys[i], "_");
            ::strcat(keys[i], to_string(i).c_str());

            tmp_size = ::strlen(keys[i]);
            tmp_key = keys[i];
        }
        size_t value_size = test.GetArgs().value_buffer_size;
        GetFuture *g_fut = sess->GetAsync(tmp_key, tmp_size, test.GetArgs().value, &value_size);
        if ( g_fut ) {
            if ( g_fut->GetPostRet() != Session::Status::eOk ) {
                cerr << "GetAsync failed sts: " << g_fut->GetPostRet() << endl;
                ret = -1;
                sess->DestroyFuture(g_fut);
                (void )test.DeleteSession();
                return ret;
            }
        } else {
            cerr << "GetAsync returned null future" << endl;
            ret = -1;
            (void )test.DeleteSession();
            return ret;
        }

        get_futures[i] = g_fut;
    }

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        Session::Status sts = get_futures[i]->Get();
        if ( sts != Session::Status::eOk || get_futures[i]->GetPostRet() != Session::Status::eOk ) {
            cerr << "GetAsync failed sts: " << sts << " port_ret: " << get_futures[i]->GetPostRet() << endl;
            ret = -1;
        } else {
            char tmp_value[128];
            memset(tmp_value, '\0', sizeof(tmp_value));
            memcpy(tmp_value, get_futures[i]->GetDupleValue().data(), MIN(sizeof(tmp_value) - 1, get_futures[i]->GetDupleValue().size()));
            char tmp_key[128];
            memset(tmp_key, '\0', sizeof(tmp_key));
            memcpy(tmp_key, get_futures[i]->GetDupleKey().data(), MIN(sizeof(tmp_key) - 1, get_futures[i]->GetDupleKey().size()));
            cout << "key(" << get_futures[i]->GetDupleKey().size() << "/" << tmp_key << ") value(" << get_futures[i]->GetDupleValue().size() << "/" << tmp_value << ")" << endl;
        }

        sess->DestroyFuture(get_futures[i]);
        delete [] keys[i];
    }

    (void )test.DeleteSession();
    return ret;
}


//
// ======================= AsyncDeleteTest ==============================
//
int AsyncDeleteTest(SetTest &test)
{
    int ret = 0;

    ret = test.OpenCreateSession();
    if ( ret ) {
        return ret;
    }

    // write records
    Session *sess = test.GetSession();
    if ( !sess ) {
        return ret;
    }

    DeleteFuture *delete_futures[100000];
    char *keys[100000];
    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        // put element
        keys[i] = new char[test.GetArgs().key_size + 32];
        const char *tmp_key;
        size_t tmp_size;
        if ( !i ) {
            tmp_key = test.GetArgs().key;
            tmp_size = test.GetArgs().key_size;
        } else {
            ::memset(keys[i], '\0', test.GetArgs().key_size + 32);
            ::memcpy(keys[i], test.GetArgs().key, test.GetArgs().key_size);
            ::strcat(keys[i], "_");
            ::strcat(keys[i], to_string(i).c_str());

            tmp_size = ::strlen(keys[i]);
            tmp_key = keys[i];
        }
        //cout << "key(" << tmp_key << "/" << tmp_size << ")" << endl;
        DeleteFuture *d_fut = sess->DeleteAsync(tmp_key, tmp_size);
        if ( d_fut ) {
            if ( d_fut->GetPostRet() != Session::Status::eOk ) {
                cerr << "DeleteAsync failed sts: " << d_fut->GetPostRet() << endl;
                ret = -1;
                sess->DestroyFuture(d_fut);
                (void )test.DeleteSession();
                return ret;
            }
        } else {
            cerr << "GetAsync returned null future" << endl;
            ret = -1;
            (void )test.DeleteSession();
            return ret;
        }

        delete_futures[i] = d_fut;
    }

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        Session::Status sts = delete_futures[i]->Get();
        if ( sts != Session::Status::eOk || delete_futures[i]->GetPostRet() != Session::Status::eOk ) {
            cerr << "DeleteAsync failed sts: " << sts << " post_ret: " << delete_futures[i]->GetPostRet() << endl;
            ret = -1;
        } else {
            cout << "DeleteAsync ok" << endl;
        }

        sess->DestroyFuture(delete_futures[i]);
        delete [] keys[i];
    }

    (void )test.DeleteSession();
    return ret;
}


//
// ======================= AsyncMergeTest ==============================
//
int AsyncMergeTest(SetTest &test)
{
    int ret = 0;

    ret = test.OpenCreateSession();
    if ( ret ) {
        return ret;
    }

    // write records
    Session *sess = test.GetSession();
    if ( !sess ) {
        return ret;
    }

    MergeFuture *merge_futures[100000];
    char *keys[100000];
    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        // put element
        keys[i] = new char[test.GetArgs().key_size + 32];
        const char *tmp_key;
        size_t tmp_size;
        if ( !i ) {
            tmp_key = test.GetArgs().key;
            tmp_size = test.GetArgs().key_size;
        } else {
            ::memset(keys[i], '\0', test.GetArgs().key_size + 32);
            ::memcpy(keys[i], test.GetArgs().key, test.GetArgs().key_size);
            ::strcat(keys[i], "_");
            ::strcat(keys[i], to_string(i).c_str());

            tmp_size = ::strlen(keys[i]);
            tmp_key = keys[i];
        }
        //cout << "key(" << tmp_key << "/" << tmp_size << ")" << endl;
        MergeFuture *m_fut = sess->MergeAsync(tmp_key, tmp_size, test.GetArgs().value, test.GetArgs().value_size);
        if ( m_fut ) {
            if ( m_fut->GetPostRet() != Session::Status::eOk ) {
                cerr << "MergeAsync failed sts: " << m_fut->GetPostRet() << endl;
                ret = -1;
                sess->DestroyFuture(m_fut);
                (void )test.DeleteSession();
                return ret;
            }
        } else {
            cerr << "MergeAsync returned null future" << endl;
            ret = -1;
            (void )test.DeleteSession();
            return ret;
        }

        merge_futures[i] = m_fut;
    }

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        Session::Status sts = merge_futures[i]->Get();
        if ( sts != Session::Status::eOk || merge_futures[i]->GetPostRet() != Session::Status::eOk ) {
            cerr << "MergeAsync failed sts: " << sts << " post_ret: " << merge_futures[i]->GetPostRet() << endl;
            ret = -1;
        } else {
            cout << "MergeAsync ok" << endl;
        }

        sess->DestroyFuture(merge_futures[i]);
        delete [] keys[i];
    }

    (void )test.DeleteSession();
    return ret;
}



static void GenerateKey(int random, int binary, const char *in_key, size_t in_key_size, string &out_key, uint32_t i, uint32_t j)
{
    if ( random ) {
        if ( !binary ) {
            out_key = string(in_key, in_key_size) + "_" + to_string(static_cast<uint32_t>(::rand())) + "_" + to_string(i) + "_" + to_string(j);
        } else {
            char tmp_key[1024];
            ::memset(tmp_key, '\0', sizeof(tmp_key));
            ::memcpy(&tmp_key[2], in_key, MIN(in_key_size, sizeof(tmp_key) - 2));
            uint32_t ran = static_cast<uint32_t>(::rand());
            if ( in_key_size + 2 + 3*sizeof(uint32_t) >= sizeof(tmp_key) ) {
                cerr << "Key too long" << endl;
                return;
            }
            ::memcpy(&tmp_key[2 + in_key_size], &ran, sizeof(uint32_t));
            ::memcpy(&tmp_key[2 + in_key_size + sizeof(uint32_t)], &i, sizeof(uint32_t));
            ::memcpy(&tmp_key[2 + in_key_size + 2*sizeof(uint32_t)], &j, sizeof(uint32_t));
            out_key = string(tmp_key, 2 + in_key_size + 3*sizeof(uint32_t));
        }
    } else {
        if ( !binary ) {
            out_key = string(in_key, in_key_size) + "_" + to_string(i) + "_" + to_string(j);
        } else {
            char tmp_key[1024];
            ::memset(tmp_key, '\0', sizeof(tmp_key));
            ::memcpy(&tmp_key[2], in_key, MIN(in_key_size, sizeof(tmp_key) - 2));
            if ( in_key_size + 2 + 2*sizeof(uint32_t) >= sizeof(tmp_key) ) {
                cerr << "Key too long" << endl;
                return;
            }
            ::memcpy(&tmp_key[2 + in_key_size], &i, sizeof(uint32_t));
            ::memcpy(&tmp_key[2 + in_key_size + sizeof(uint32_t)], &j, sizeof(uint32_t));
            out_key = string(tmp_key, 2 + in_key_size + 2*sizeof(uint32_t));
        }
    }
}

//
// ======================= AsyncPutSetTest ==============================
//
int AsyncPutSetTest(SetTest &test)
{
    int ret = 0;

    ret = test.OpenCreateSession();
    if ( ret ) {
        return ret;
    }

    // write records async
    if ( test.GetArgs().random_keys ) {
        ::srand(1);
    }

    Session *sess = test.GetSession();
    if ( !sess ) {
        return ret;
    }

    PutSetFuture *putset_futures[100000];
    vector<char *> *key_bufs[100000];
    vector<pair<Soe::Duple, Soe::Duple>> *putset_elements[100000];

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        vector<pair<Soe::Duple, Soe::Duple>> *elements = new vector<pair<Soe::Duple, Soe::Duple>>;
        vector<char *> *keys_s = new vector<char *>;

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            char *key_buf = new char[test.GetArgs().key_size + 64];
            string key;
            GenerateKey(test.GetArgs().random_keys, test.GetArgs().use_binary_keys, test.GetArgs().key, test.GetArgs().key_size, key, i, j);

            ::memcpy(key_buf, key.c_str(), key.size());
            key_buf[key.size()] = '\0';

            elements->push_back(pair<Soe::Duple, Soe::Duple>(Soe::Duple(key_buf, key.size()),
                Soe::Duple(test.GetArgs().value, test.GetArgs().value_size)));
            keys_s->push_back(key_buf);
        }

        // write elements
        size_t fail_pos;
        PutSetFuture *s_fut = sess->PutSetAsync(*elements, true, &fail_pos);
        if ( s_fut ) {
            if ( s_fut->GetPostRet() != Session::Status::eOk ) {
                if ( s_fut->GetPostRet() == Session::Status::eDuplicateKey ) {
                    cerr << "PutSetAsync duplicate key sts: " << s_fut->GetPostRet() << " fail_pos: " << fail_pos << endl;
                } else {
                    cerr << "PutSetAsync failed key sts: " << s_fut->GetPostRet() << endl;
                    ret = -1;
                    sess->DestroyFuture(s_fut);
                    (void )test.DeleteSession();
                    return ret;
                }
            } else {
                cout << "PutSetAsync ok" << endl;
            }
        } else {
            cerr << "PutAsyncSet returned null future" << endl;
            ret = -1;
            (void )test.DeleteSession();
            return ret;
        }

        putset_futures[i] = s_fut;
        putset_elements[i] = elements;
        key_bufs[i] = keys_s;
    }

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        Session::Status sts = putset_futures[i]->Get();
        if ( sts != Session::Status::eOk || putset_futures[i]->GetPostRet() != Session::Status::eOk ) {
            cerr << "PutSetFuture::Get() failed sts: " << sts << " post_ret: " << putset_futures[i]->GetPostRet() << " fail_pos: " << putset_futures[i]->GetFailPos() << endl;
            ret = -1;
        } else {
            cout << "PutSetFuture::Get() ok" << endl;
        }

        sess->DestroyFuture(putset_futures[i]);

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            delete [] (*key_bufs[i])[j];
        }

        key_bufs[i]->clear();
        delete key_bufs[i];
        putset_elements[i]->clear();
        delete putset_elements[i];
    }

    (void )test.DeleteSession();
    return ret;
}


//
// ======================= AsyncGetSetTest ==============================
//
int AsyncGetSetTest(SetTest &test)
{
    int ret = 0;

    ret = test.OpenCreateSession();
    if ( ret ) {
        return ret;
    }

    // read records async
    if ( test.GetArgs().random_keys ) {
        ::srand(1);
    }

    Session *sess = test.GetSession();
    if ( !sess ) {
        return ret;
    }

    GetSetFuture *getset_futures[100000];
    vector<char *> *key_bufs[100000];
    vector<pair<Soe::Duple, Soe::Duple>> *getset_elements[100000];

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        vector<pair<Soe::Duple, Soe::Duple>> *elements = new vector<pair<Soe::Duple, Soe::Duple>>;
        vector<char *> *keys_s = new vector<char *>;

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            char *key_buf = new char[test.GetArgs().key_size + 64];
            string key;
            GenerateKey(test.GetArgs().random_keys, test.GetArgs().use_binary_keys, test.GetArgs().key, test.GetArgs().key_size, key, i, j);

            ::memcpy(key_buf, key.c_str(), key.size());
            key_buf[key.size()] = '\0';

            elements->push_back(pair<Soe::Duple, Soe::Duple>(Soe::Duple(key_buf, key.size()),
                Soe::Duple(test.GetArgs().value, test.GetArgs().value_size)));
            keys_s->push_back(key_buf);
        }

        // get elements
        size_t fail_pos;
        GetSetFuture *s_fut = sess->GetSetAsync(*elements, true, &fail_pos);
        if ( s_fut ) {
            if ( s_fut->GetPostRet() != Session::Status::eOk ) {
                if ( s_fut->GetPostRet() == Session::Status::eNotFoundKey ) {
                    cerr << "GetSetAsync duplicate key sts: " << s_fut->GetPostRet() << " fail_pos: " << fail_pos << endl;
                } else {
                    cerr << "GetSetAsync failed key sts: " << s_fut->GetPostRet() << endl;
                    ret = -1;
                    sess->DestroyFuture(s_fut);
                    (void )test.DeleteSession();
                    return ret;
                }
            }
        } else {
            cerr << "GetSetAsync returned null future" << endl;
            ret = -1;
            (void )test.DeleteSession();
            return ret;
        }

        getset_futures[i] = s_fut;
        getset_elements[i] = elements;
        key_bufs[i] = keys_s;
    }

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        Session::Status sts = getset_futures[i]->Get();
        if ( sts != Session::Status::eOk || getset_futures[i]->GetPostRet() != Session::Status::eOk ) {
            cerr << "GetSetFuture::Get() failed sts: " << sts << " post_ret: " << getset_futures[i]->GetPostRet() << " fail_pos: " << getset_futures[i]->GetFailPos() << endl;
            ret = -1;
        } else {
            cout << "GetSetFuture::Get() ok" << endl;
        }

        if ( sts == Session::Status::eOk ) {
            vector<pair<Soe::Duple, Soe::Duple>> &results = getset_futures[i]->GetItems();
            size_t num_good = results.size();
            if ( getset_futures[i]->GetPostRet() == Session::Status::eNotFoundKey ) {
                num_good = static_cast<size_t>(getset_futures[i]->GetFailPos());
            }

            // print up to num_good items
            for ( size_t r_i = 0 ; r_i < num_good ; r_i++ ) {
                char tmp_value[128];
                memset(tmp_value, '\0', sizeof(tmp_value));
                memcpy(tmp_value, results[r_i].second.data(), MIN(sizeof(tmp_value) - 1, results[r_i].second.size()));
                char tmp_key[128];
                memset(tmp_key, '\0', sizeof(tmp_key));
                memcpy(tmp_key, results[r_i].first.data(), MIN(sizeof(tmp_key) - 1, results[r_i].first.size()));
                cout << "key(" << results[r_i].first.size() << "/" << tmp_key << ") value(" << results[r_i].second.size() << "/" << tmp_value << ")" << endl;
            }
        }

        sess->DestroyFuture(getset_futures[i]);

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            delete [] (*key_bufs[i])[j];
        }

        key_bufs[i]->clear();
        delete key_bufs[i];
        getset_elements[i]->clear();
        delete getset_elements[i];
    }

    (void )test.DeleteSession();
    return ret;
}


//
// ======================= AsyncDeleteSetTest ==============================
//
int AsyncDeleteSetTest(SetTest &test)
{
    int ret = 0;

    ret = test.OpenCreateSession();
    if ( ret ) {
        return ret;
    }

    // delete records async
    if ( test.GetArgs().random_keys ) {
        ::srand(1);
    }

    Session *sess = test.GetSession();
    if ( !sess ) {
        return ret;
    }

    DeleteSetFuture *deleteset_futures[100000];
    vector<char *> *key_bufs[100000];
    vector<Soe::Duple> *deleteset_elements[100000];

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        vector<Soe::Duple> *keys = new vector<Soe::Duple>;
        vector<char *> *keys_s = new vector<char *>;

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            char *key_buf = new char[test.GetArgs().key_size + 64];
            string key;
            GenerateKey(test.GetArgs().random_keys, test.GetArgs().use_binary_keys, test.GetArgs().key, test.GetArgs().key_size, key, i, j);

            ::memcpy(key_buf, key.c_str(), key.size());
            key_buf[key.size()] = '\0';

            keys->push_back(Soe::Duple(key_buf, key.size()));
            keys_s->push_back(key_buf);
        }

        // delete elements
        size_t fail_pos;
        DeleteSetFuture *s_fut = sess->DeleteSetAsync(*keys, true, &fail_pos);
        if ( s_fut ) {
            if ( s_fut->GetPostRet() != Session::Status::eOk ) {
                if ( s_fut->GetPostRet() == Session::Status::eNotFoundKey ) {
                    cerr << "DeleteSetAsync duplicate key sts: " << s_fut->GetPostRet() << " fail_pos: " << fail_pos << endl;
                } else {
                    cerr << "DeleteSetAsync failed key sts: " << s_fut->GetPostRet() << endl;
                    ret = -1;
                    sess->DestroyFuture(s_fut);
                    (void )test.DeleteSession();
                    return ret;
                }
            }
        } else {
            cerr << "DeleteSetAsync returned null future" << endl;
            ret = -1;
            (void )test.DeleteSession();
            return ret;
        }

        deleteset_futures[i] = s_fut;
        deleteset_elements[i] = keys;
        key_bufs[i] = keys_s;
    }

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        Session::Status sts = deleteset_futures[i]->Get();
        if ( sts != Session::Status::eOk || deleteset_futures[i]->GetPostRet() != Session::Status::eOk ) {
            cerr << "DeleteSetFuture::Get() failed sts: " << sts << " post_ret: " << deleteset_futures[i]->GetPostRet() << " fail_pos: " << deleteset_futures[i]->GetFailPos() << endl;
            ret = -1;
        } else {
            cout << "DeleteSetFuture::Get() ok" << endl;
        }

        sess->DestroyFuture(deleteset_futures[i]);

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            delete [] (*key_bufs[i])[j];
        }

        key_bufs[i]->clear();
        delete key_bufs[i];
        deleteset_elements[i]->clear();
        delete deleteset_elements[i];
    }

    (void )test.DeleteSession();
    return ret;
}


//
// ======================= AsyncMergeSetTest ==============================
//
int AsyncMergeSetTest(SetTest &test)
{
    int ret = 0;

    ret = test.OpenCreateSession();
    if ( ret ) {
        return ret;
    }

    // merge records async
    if ( test.GetArgs().random_keys ) {
        ::srand(1);
    }

    Session *sess = test.GetSession();
    if ( !sess ) {
        return ret;
    }

    MergeSetFuture *mergeset_futures[100000];
    vector<char *> *key_bufs[100000];
    vector<pair<Soe::Duple, Soe::Duple>> *mergeset_elements[100000];

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        vector<pair<Soe::Duple, Soe::Duple>> *elements = new vector<pair<Soe::Duple, Soe::Duple>>;
        vector<char *> *keys_s = new vector<char *>;

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            char *key_buf = new char[test.GetArgs().key_size + 64];
            string key;
            GenerateKey(test.GetArgs().random_keys, test.GetArgs().use_binary_keys, test.GetArgs().key, test.GetArgs().key_size, key, i, j);

            ::memcpy(key_buf, key.c_str(), key.size());
            key_buf[key.size()] = '\0';

            elements->push_back(pair<Soe::Duple, Soe::Duple>(Soe::Duple(key_buf, key.size()),
                Soe::Duple(test.GetArgs().value, test.GetArgs().value_size)));
            keys_s->push_back(key_buf);
        }

        // merge elements
        MergeSetFuture *s_fut = sess->MergeSetAsync(*elements);
        if ( s_fut ) {
            if ( s_fut->GetPostRet() != Session::Status::eOk ) {
                cerr << "MergeSetAsync failed key sts: " << s_fut->GetPostRet() << endl;
                ret = -1;
                sess->DestroyFuture(s_fut);
                (void )test.DeleteSession();
                return ret;
            }
        } else {
            cerr << "MergeSetAsync returned null future" << endl;
            ret = -1;
            (void )test.DeleteSession();
            return ret;
        }

        mergeset_futures[i] = s_fut;
        mergeset_elements[i] = elements;
        key_bufs[i] = keys_s;
    }

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        Session::Status sts = mergeset_futures[i]->Get();
        if ( sts != Session::Status::eOk || mergeset_futures[i]->GetPostRet() != Session::Status::eOk ) {
            cerr << "MergeSetFuture::Get() failed sts: " << sts << " post_ret: " << mergeset_futures[i]->GetPostRet() << " fail_pos: " << mergeset_futures[i]->GetFailPos() << endl;
            ret = -1;
        } else {
            cout << "MergeSetFuture::Get() ok" << endl;
        }

        sess->DestroyFuture(mergeset_futures[i]);

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            delete [] (*key_bufs[i])[j];
        }

        key_bufs[i]->clear();
        delete key_bufs[i];
        mergeset_elements[i]->clear();
        delete mergeset_elements[i];
    }

    (void )test.DeleteSession();
    return ret;
}

//
// ======================= AsyncConcurrentPutSetTest ==============================
//
static int AsyncConcurrentPutSetTestWorker(SetTest &test)
{
    int ret = 0;

    // write records async
    if ( test.GetArgs().random_keys ) {
        ::srand(1);
    }

    ret = test.OpenCreateSession();
    if ( ret ) {
        cerr << "OpenCreateSession failed, ret: " << ret << endl;
        return ret;
    }

    if ( test.GetArgs().destroy_store_before ) {
        ret = test.DestroyStore();
        if ( ret ) {
            cerr << "DestroyStore failed, ret: " << ret << endl;
            return ret;
        }

        ret = test.OpenCreateSession();
        if ( ret ) {
            cerr << "OpenCreateSession failed, ret: " << ret << endl;
            return ret;
        }
    }

    Session *sess = test.GetSession();
    if ( !sess ) {
        return ret;
    }

    if ( test.GetArgs().num_regressions > 100000 ) {
        (void )test.DeleteSession();
        cerr << "Num regressions too big, max=100000" << endl;
        return -1;
    }

    const uint32_t qone_shot = 20;
    PutSetFuture *putset_futures[qone_shot];
    vector<char *> *key_bufs[qone_shot];
    vector<pair<Soe::Duple, Soe::Duple>> *putset_elements[qone_shot];

    uint32_t reg_quant = test.GetArgs().num_regressions/qone_shot;
    uint32_t reg_quant_res = test.GetArgs().num_regressions%qone_shot;

    uint32_t global_cnt = 0;
    for ( uint32_t k = 0 ; k < reg_quant ; k++ ) {
        uint32_t i;
        for ( i = 0 ; i < qone_shot ; i++ ) {
            vector<pair<Soe::Duple, Soe::Duple>> *elements = new vector<pair<Soe::Duple, Soe::Duple>>;
            vector<char *> *keys_s = new vector<char *>;

            for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
                char *key_buf = new char[test.GetArgs().key_size + 64];
                string key;
                GenerateKey(test.GetArgs().random_keys, test.GetArgs().use_binary_keys, test.GetArgs().key, test.GetArgs().key_size, key, k*qone_shot + i, global_cnt++);

                ::memcpy(key_buf, key.c_str(), key.size());
                key_buf[key.size()] = '\0';

                elements->push_back(pair<Soe::Duple, Soe::Duple>(Soe::Duple(key_buf, key.size()),
                    Soe::Duple(test.GetArgs().value, test.GetArgs().value_size)));
                keys_s->push_back(key_buf);
            }

            // write elements
            size_t fail_pos;
            PutSetFuture *s_fut = sess->PutSetAsync(*elements, true, &fail_pos);
            if ( s_fut ) {
                if ( s_fut->GetPostRet() != Session::Status::eOk ) {
                    if ( s_fut->GetPostRet() == Session::Status::eDuplicateKey ) {
                        cerr << "PutSetAsync duplicate key sts: " << s_fut->GetPostRet() << " fail_pos: " << fail_pos << endl;
                    } else {
                        cerr << "PutSetAsync failed key sts: " << s_fut->GetPostRet() << endl;
                        ret = -1;
                        sess->DestroyFuture(s_fut);
                        ret = -1;
                        break;
                    }
                } else {
                    if ( !test.GetArgs().no_print ) {
                        cout << "PutSetAsync ok" << endl;
                    }
                }
            } else {
                cerr << "PutAsyncSet returned null future" << endl;
                ret = -1;
                break;
            }

            putset_futures[i] = s_fut;
            putset_elements[i] = elements;
            key_bufs[i] = keys_s;
        }

        for ( uint32_t ii = 0 ; ii < i ; ii++ ) {
            Session::Status sts = putset_futures[ii]->Get();
            if ( sts != Session::Status::eOk || putset_futures[ii]->GetPostRet() != Session::Status::eOk ) {
                cerr << "PutSetFuture::Get() failed sts: " << sts << " post_ret: " << putset_futures[ii]->GetPostRet() << " fail_pos: " << putset_futures[ii]->GetFailPos() << endl;
                ret = -1;
            } else {
                if ( !test.GetArgs().no_print ) {
                    cout << "PutSetFuture::Get() ok" << endl;
                }
            }

            sess->DestroyFuture(putset_futures[ii]);

            for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
                delete [] (*key_bufs[ii])[j];
            }

            key_bufs[ii]->clear();
            delete key_bufs[ii];
            putset_elements[ii]->clear();
            delete putset_elements[ii];
        }
        if ( ret ) {
            break;
        }
    }

    // run the rest of it
    uint32_t i;
    for ( i = 0 ; i < reg_quant_res ; i++ ) {
        vector<pair<Soe::Duple, Soe::Duple>> *elements = new vector<pair<Soe::Duple, Soe::Duple>>;
        vector<char *> *keys_s = new vector<char *>;

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            char *key_buf = new char[test.GetArgs().key_size + 64];
            string key;
            GenerateKey(test.GetArgs().random_keys, test.GetArgs().use_binary_keys, test.GetArgs().key, test.GetArgs().key_size, key, (qone_shot + 1)*reg_quant + i, global_cnt++);

            ::memcpy(key_buf, key.c_str(), key.size());
            key_buf[key.size()] = '\0';

            elements->push_back(pair<Soe::Duple, Soe::Duple>(Soe::Duple(key_buf, key.size()),
                Soe::Duple(test.GetArgs().value, test.GetArgs().value_size)));
            keys_s->push_back(key_buf);
        }

        // write elements
        size_t fail_pos;
        PutSetFuture *s_fut = sess->PutSetAsync(*elements, true, &fail_pos);
        if ( s_fut ) {
            if ( s_fut->GetPostRet() != Session::Status::eOk ) {
                if ( s_fut->GetPostRet() == Session::Status::eDuplicateKey ) {
                    cerr << "PutSetAsync duplicate key sts: " << s_fut->GetPostRet() << " fail_pos: " << fail_pos << endl;
                } else {
                    cerr << "PutSetAsync failed key sts: " << s_fut->GetPostRet() << endl;
                    ret = -1;
                    sess->DestroyFuture(s_fut);
                    ret = -1;
                    break;
                }
            } else {
                if ( !test.GetArgs().no_print ) {
                    cout << "PutSetAsync ok" << endl;
                }
            }
        } else {
            cerr << "PutAsyncSet returned null future" << endl;
            ret = -1;
            break;
        }

        putset_futures[i] = s_fut;
        putset_elements[i] = elements;
        key_bufs[i] = keys_s;
    }

    for ( uint32_t ii = 0 ; ii < i; ii++ ) {
        Session::Status sts = putset_futures[ii]->Get();
        if ( sts != Session::Status::eOk || putset_futures[ii]->GetPostRet() != Session::Status::eOk ) {
            cerr << "PutSetFuture::Get() failed sts: " << sts << " post_ret: " << putset_futures[ii]->GetPostRet() << " fail_pos: " << putset_futures[ii]->GetFailPos() << endl;
            ret = -1;
        } else {
            if ( !test.GetArgs().no_print ) {
                cout << "PutSetFuture::Get() ok" << endl;
            }
        }

        sess->DestroyFuture(putset_futures[ii]);

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            delete [] (*key_bufs[ii])[j];
        }

        key_bufs[ii]->clear();
        delete key_bufs[ii];
        putset_elements[ii]->clear();
        delete putset_elements[ii];
    }

    (void )test.DeleteSession();
    return ret;
}

int AsyncConcurrentPutSetTest(SetTest &test)
{
    vector<boost::thread *> worker_threads;
    vector<SetTest *> set_tests;
    worker_threads.reserve(test.GetArgs().thread_count);
    set_tests.reserve(test.GetArgs().thread_count);

    uint32_t w_cnt;
    for ( w_cnt = 0 ; w_cnt < test.GetArgs().thread_count ; w_cnt++ ) {
        set_tests[w_cnt] = new SetTest(test);
        string t_key = string(test.GetArgs().key) + "_" + to_string(w_cnt);
        set_tests[w_cnt]->GetArgs().key = new char[t_key.size()];
        set_tests[w_cnt]->GetArgs().key_size = t_key.size();
        ::memcpy(const_cast<char *>(set_tests[w_cnt]->GetArgs().key), t_key.data(), t_key.size());
        worker_threads[w_cnt] = new boost::thread(&AsyncConcurrentPutSetTestWorker, *set_tests[w_cnt]);
    }

    for ( uint32_t i = 0 ; i < w_cnt ; i++ ) {
        worker_threads[i]->join();
        delete worker_threads[i];
        delete set_tests[i];
    }

    return 0;
}

//
// ======================= AsyncGetSetTest ==============================
//
int AsyncConcurrentGetSetTest(SetTest &test)
{
    int ret = 0;

    ret = test.OpenCreateSession();
    if ( ret ) {
        return ret;
    }

    // read records async
    if ( test.GetArgs().random_keys ) {
        ::srand(1);
    }

    Session *sess = test.GetSession();
    if ( !sess ) {
        return ret;
    }

    GetSetFuture *getset_futures[100000];
    vector<char *> *key_bufs[100000];
    vector<pair<Soe::Duple, Soe::Duple>> *getset_elements[100000];

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        vector<pair<Soe::Duple, Soe::Duple>> *elements = new vector<pair<Soe::Duple, Soe::Duple>>;
        vector<char *> *keys_s = new vector<char *>;

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            char *key_buf = new char[test.GetArgs().key_size + 64];
            string key;
            GenerateKey(test.GetArgs().random_keys, test.GetArgs().use_binary_keys, test.GetArgs().key, test.GetArgs().key_size, key, i, j);

            ::memcpy(key_buf, key.c_str(), key.size());
            key_buf[key.size()] = '\0';

            elements->push_back(pair<Soe::Duple, Soe::Duple>(Soe::Duple(key_buf, key.size()),
                Soe::Duple(test.GetArgs().value, test.GetArgs().value_size)));
            keys_s->push_back(key_buf);
        }

        // get elements
        size_t fail_pos;
        GetSetFuture *s_fut = sess->GetSetAsync(*elements, true, &fail_pos);
        if ( s_fut ) {
            if ( s_fut->GetPostRet() != Session::Status::eOk ) {
                if ( s_fut->GetPostRet() == Session::Status::eNotFoundKey ) {
                    cerr << "GetSetAsync duplicate key sts: " << s_fut->GetPostRet() << " fail_pos: " << fail_pos << endl;
                } else {
                    cerr << "GetSetAsync failed key sts: " << s_fut->GetPostRet() << endl;
                    ret = -1;
                    sess->DestroyFuture(s_fut);
                    (void )test.DeleteSession();
                    return ret;
                }
            }
        } else {
            cerr << "GetSetAsync returned null future" << endl;
            ret = -1;
            (void )test.DeleteSession();
            return ret;
        }

        getset_futures[i] = s_fut;
        getset_elements[i] = elements;
        key_bufs[i] = keys_s;
    }

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        Session::Status sts = getset_futures[i]->Get();
        if ( sts != Session::Status::eOk || getset_futures[i]->GetPostRet() != Session::Status::eOk ) {
            cerr << "GetSetFuture::Get() failed sts: " << sts << " post_ret: " << getset_futures[i]->GetPostRet() << " fail_pos: " << getset_futures[i]->GetFailPos() << endl;
            ret = -1;
        } else {
            cout << "GetSetFuture::Get() ok" << endl;
        }

        if ( sts == Session::Status::eOk ) {
            vector<pair<Soe::Duple, Soe::Duple>> &results = getset_futures[i]->GetItems();
            size_t num_good = results.size();
            if ( getset_futures[i]->GetPostRet() == Session::Status::eNotFoundKey ) {
                num_good = static_cast<size_t>(getset_futures[i]->GetFailPos());
            }

            // print up to num_good items
            for ( size_t r_i = 0 ; r_i < num_good ; r_i++ ) {
                char tmp_value[128];
                memset(tmp_value, '\0', sizeof(tmp_value));
                memcpy(tmp_value, results[r_i].second.data(), MIN(sizeof(tmp_value) - 1, results[r_i].second.size()));
                char tmp_key[128];
                memset(tmp_key, '\0', sizeof(tmp_key));
                memcpy(tmp_key, results[r_i].first.data(), MIN(sizeof(tmp_key) - 1, results[r_i].first.size()));
                cout << "key(" << results[r_i].first.size() << "/" << tmp_key << ") value(" << results[r_i].second.size() << "/" << tmp_value << ")" << endl;
            }
        }

        sess->DestroyFuture(getset_futures[i]);

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            delete [] (*key_bufs[i])[j];
        }

        key_bufs[i]->clear();
        delete key_bufs[i];
        getset_elements[i]->clear();
        delete getset_elements[i];
    }

    (void )test.DeleteSession();
    return ret;
}


//
// ======================= AsyncDeleteSetTest ==============================
//
int AsyncConcurrentDeleteSetTest(SetTest &test)
{
    int ret = 0;

    ret = test.OpenCreateSession();
    if ( ret ) {
        return ret;
    }

    // delete records async
    if ( test.GetArgs().random_keys ) {
        ::srand(1);
    }

    Session *sess = test.GetSession();
    if ( !sess ) {
        return ret;
    }

    DeleteSetFuture *deleteset_futures[100000];
    vector<char *> *key_bufs[100000];
    vector<Soe::Duple> *deleteset_elements[100000];

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        vector<Soe::Duple> *keys = new vector<Soe::Duple>;
        vector<char *> *keys_s = new vector<char *>;

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            char *key_buf = new char[test.GetArgs().key_size + 64];
            string key;
            GenerateKey(test.GetArgs().random_keys, test.GetArgs().use_binary_keys, test.GetArgs().key, test.GetArgs().key_size, key, i, j);

            ::memcpy(key_buf, key.c_str(), key.size());
            key_buf[key.size()] = '\0';

            keys->push_back(Soe::Duple(key_buf, key.size()));
            keys_s->push_back(key_buf);
        }

        // delete elements
        size_t fail_pos;
        DeleteSetFuture *s_fut = sess->DeleteSetAsync(*keys, true, &fail_pos);
        if ( s_fut ) {
            if ( s_fut->GetPostRet() != Session::Status::eOk ) {
                if ( s_fut->GetPostRet() == Session::Status::eNotFoundKey ) {
                    cerr << "DeleteSetAsync duplicate key sts: " << s_fut->GetPostRet() << " fail_pos: " << fail_pos << endl;
                } else {
                    cerr << "DeleteSetAsync failed key sts: " << s_fut->GetPostRet() << endl;
                    ret = -1;
                    sess->DestroyFuture(s_fut);
                    (void )test.DeleteSession();
                    return ret;
                }
            }
        } else {
            cerr << "DeleteSetAsync returned null future" << endl;
            ret = -1;
            (void )test.DeleteSession();
            return ret;
        }

        deleteset_futures[i] = s_fut;
        deleteset_elements[i] = keys;
        key_bufs[i] = keys_s;
    }

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        Session::Status sts = deleteset_futures[i]->Get();
        if ( sts != Session::Status::eOk || deleteset_futures[i]->GetPostRet() != Session::Status::eOk ) {
            cerr << "DeleteSetFuture::Get() failed sts: " << sts << " post_ret: " << deleteset_futures[i]->GetPostRet() << " fail_pos: " << deleteset_futures[i]->GetFailPos() << endl;
            ret = -1;
        } else {
            cout << "DeleteSetFuture::Get() ok" << endl;
        }

        sess->DestroyFuture(deleteset_futures[i]);

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            delete [] (*key_bufs[i])[j];
        }

        key_bufs[i]->clear();
        delete key_bufs[i];
        deleteset_elements[i]->clear();
        delete deleteset_elements[i];
    }

    (void )test.DeleteSession();
    return ret;
}


//
// ======================= AsyncMergeSetTest ==============================
//
int AsyncConcurrentMergeSetTest(SetTest &test)
{
    int ret = 0;

    ret = test.OpenCreateSession();
    if ( ret ) {
        return ret;
    }

    // merge records async
    if ( test.GetArgs().random_keys ) {
        ::srand(1);
    }

    Session *sess = test.GetSession();
    if ( !sess ) {
        return ret;
    }

    MergeSetFuture *mergeset_futures[100000];
    vector<char *> *key_bufs[100000];
    vector<pair<Soe::Duple, Soe::Duple>> *mergeset_elements[100000];

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        vector<pair<Soe::Duple, Soe::Duple>> *elements = new vector<pair<Soe::Duple, Soe::Duple>>;
        vector<char *> *keys_s = new vector<char *>;

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            char *key_buf = new char[test.GetArgs().key_size + 64];
            string key;
            GenerateKey(test.GetArgs().random_keys, test.GetArgs().use_binary_keys, test.GetArgs().key, test.GetArgs().key_size, key, i, j);

            ::memcpy(key_buf, key.c_str(), key.size());
            key_buf[key.size()] = '\0';

            elements->push_back(pair<Soe::Duple, Soe::Duple>(Soe::Duple(key_buf, key.size()),
                Soe::Duple(test.GetArgs().value, test.GetArgs().value_size)));
            keys_s->push_back(key_buf);
        }

        // merge elements
        MergeSetFuture *s_fut = sess->MergeSetAsync(*elements);
        if ( s_fut ) {
            if ( s_fut->GetPostRet() != Session::Status::eOk ) {
                cerr << "MergeSetAsync failed key sts: " << s_fut->GetPostRet() << endl;
                ret = -1;
                sess->DestroyFuture(s_fut);
                (void )test.DeleteSession();
                return ret;
            }
        } else {
            cerr << "MergeSetAsync returned null future" << endl;
            ret = -1;
            (void )test.DeleteSession();
            return ret;
        }

        mergeset_futures[i] = s_fut;
        mergeset_elements[i] = elements;
        key_bufs[i] = keys_s;
    }

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        Session::Status sts = mergeset_futures[i]->Get();
        if ( sts != Session::Status::eOk || mergeset_futures[i]->GetPostRet() != Session::Status::eOk ) {
            cerr << "MergeSetFuture::Get() failed sts: " << sts << " post_ret: " << mergeset_futures[i]->GetPostRet() << " fail_pos: " << mergeset_futures[i]->GetFailPos() << endl;
            ret = -1;
        } else {
            cout << "MergeSetFuture::Get() ok" << endl;
        }

        sess->DestroyFuture(mergeset_futures[i]);

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            delete [] (*key_bufs[i])[j];
        }

        key_bufs[i]->clear();
        delete key_bufs[i];
        mergeset_elements[i]->clear();
        delete mergeset_elements[i];
    }

    (void )test.DeleteSession();
    return ret;
}

//
// ======================= AsyncSyncMixedSetOpenCloseTest ==============================
//
static int AsyncSyncMixedSetOpenCloseLoopTest(SetTest &test)
{
    int ret = 0;

    ret = test.OpenCreateSession();
    if ( ret ) {
        return ret;
    }

    // write records async
    if ( test.GetArgs().random_keys ) {
        ::srand(1);
    }

    Session *sess = test.GetSession();
    if ( !sess ) {
        return ret;
    }

    // async puts
    PutSetFuture *async_putset_futures[100000];
    vector<char *> *async_key_bufs[100000];
    vector<pair<Soe::Duple, Soe::Duple>> *async_putset_elements[100000];

    for ( uint32_t i = 0 ; i < test.GetArgs().num_loops && i < 100000 ; i++ ) {
        vector<pair<Soe::Duple, Soe::Duple>> *elements = new vector<pair<Soe::Duple, Soe::Duple>>;
        vector<char *> *keys_s = new vector<char *>;

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            char *key_buf = new char[test.GetArgs().key_size + 64];
            string key;
            GenerateKey(test.GetArgs().random_keys, test.GetArgs().use_binary_keys, test.GetArgs().key, test.GetArgs().key_size, key, i, j);

            ::memcpy(key_buf, key.c_str(), key.size());
            key_buf[key.size()] = '\0';

            elements->push_back(pair<Soe::Duple, Soe::Duple>(Soe::Duple(key_buf, key.size()),
                Soe::Duple(test.GetArgs().value, test.GetArgs().value_size)));
            keys_s->push_back(key_buf);
        }

        // write elements
        size_t fail_pos;
        PutSetFuture *s_fut = sess->PutSetAsync(*elements, true, &fail_pos);
        if ( s_fut ) {
            if ( s_fut->GetPostRet() != Session::Status::eOk ) {
                if ( s_fut->GetPostRet() == Session::Status::eDuplicateKey ) {
                    cerr << "PutSetAsync duplicate key sts: " << s_fut->GetPostRet() << " fail_pos: " << fail_pos << endl;
                } else {
                    cerr << "PutSetAsync failed key sts: " << s_fut->GetPostRet() << endl;
                    ret = -1;
                    sess->DestroyFuture(s_fut);
                    (void )test.DeleteSession();
                    return ret;
                }
            } else {
                cout << "PutSetAsync ok" << endl;
            }
        } else {
            cerr << "PutAsyncSet returned null future" << endl;
            ret = -1;
            (void )test.DeleteSession();
            return ret;
        }

        async_putset_futures[i] = s_fut;
        async_putset_elements[i] = elements;
        async_key_bufs[i] = keys_s;
    }

    // sync puts in the meantime, while asyns ones are being processed
    for ( uint32_t i = 0 ; i < test.GetArgs().num_loops && i < 100000 ; i++ ) {
        vector<pair<Soe::Duple, Soe::Duple>> elements;

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            char *key_buf = new char[test.GetArgs().key_size + 64];
            string key;
            GenerateKey(test.GetArgs().random_keys, test.GetArgs().use_binary_keys, test.GetArgs().key, test.GetArgs().key_size, key, i, j + 100000);

            ::memcpy(key_buf, key.c_str(), key.size());
            key_buf[key.size()] = '\0';

            elements.push_back(pair<Soe::Duple, Soe::Duple>(Soe::Duple(key_buf, key.size()),
                Soe::Duple(test.GetArgs().value, test.GetArgs().value_size)));
        }

        // write elements
        size_t fail_pos;
        Session::Status sts = sess->PutSet(elements, true, &fail_pos);
        if ( sts != Session::Status::eOk ) {
            if ( sts == Session::Status::eDuplicateKey ) {
                cerr << "PutSet duplicate key sts: " << sts << " fail_pos: " << fail_pos << endl;
            } else {
                cerr << "PutSet failed sts: " << sts << endl;
                ret = -1;
                break;
            }
        } else {
            cout << "PutSet ok" << endl;
        }

        elements.clear();
    }

    // get async completions
    for ( uint32_t i = 0 ; i < test.GetArgs().num_loops && i < 100000 ; i++ ) {
        Session::Status sts = async_putset_futures[i]->Get();
        if ( sts != Session::Status::eOk || async_putset_futures[i]->GetPostRet() != Session::Status::eOk ) {
            cerr << "PutSetFuture::Get() failed sts: " << sts << " post_ret: " << async_putset_futures[i]->GetPostRet() <<
                " fail_pos: " << async_putset_futures[i]->GetFailPos() << endl;
            ret = -1;
        } else {
            cout << "PutSetFuture::Get() ok" << endl;
        }

        sess->DestroyFuture(async_putset_futures[i]);

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            delete [] (*async_key_bufs[i])[j];
        }

        async_key_bufs[i]->clear();
        delete async_key_bufs[i];
        async_putset_elements[i]->clear();
        delete async_putset_elements[i];
    }

    (void )test.DeleteSession();
    return ret;
}

int AsyncSyncMixedSetOpenCloseTest(SetTest &test)
{
    int ret;
    char orig_key[10000];
    ::memset(orig_key, '\0', sizeof(orig_key));
    ::memcpy(orig_key, test.GetArgs().key, test.GetArgs().key_size);

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        char new_key[10000];
        sprintf(new_key, "%s_%u", orig_key, i);
        test.GetArgs().key = new_key;
        test.GetArgs().key_size = strlen(new_key);

        cout << "Doing ... key(" << test.GetArgs().key_size << "/" << test.GetArgs().key << ")" << endl;
        ret = AsyncSyncMixedSetOpenCloseLoopTest(test);
        if ( ret ) {
            break;
        }
    }

    return ret;
}

//
// ======================= AsyncSyncMixedSetTest ==============================
//
static int AsyncSyncMixedSetLoopTest(SetTest &test)
{
    int ret = 0;

    // write records async
    if ( test.GetArgs().random_keys ) {
        ::srand(1);
    }

    Session *sess = test.GetSession();
    if ( !sess ) {
        return ret;
    }

    // async puts
    PutSetFuture *async_putset_futures[100000];
    vector<char *> *async_key_bufs[100000];
    vector<pair<Soe::Duple, Soe::Duple>> *async_putset_elements[100000];

    uint32_t i, num_loops;
    for ( i = 0 ; i < test.GetArgs().num_loops && i < 100000 ; i++ ) {
        vector<pair<Soe::Duple, Soe::Duple>> *elements = new vector<pair<Soe::Duple, Soe::Duple>>;
        vector<char *> *keys_s = new vector<char *>;

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            char *key_buf = new char[test.GetArgs().key_size + 64];
            string key;
            GenerateKey(test.GetArgs().random_keys, test.GetArgs().use_binary_keys, test.GetArgs().key, test.GetArgs().key_size, key, i, j);

            ::memcpy(key_buf, key.c_str(), key.size());
            key_buf[key.size()] = '\0';

            elements->push_back(pair<Soe::Duple, Soe::Duple>(Soe::Duple(key_buf, key.size()),
                Soe::Duple(test.GetArgs().value, test.GetArgs().value_size)));
            keys_s->push_back(key_buf);
        }

        // write elements
        size_t fail_pos;
        PutSetFuture *s_fut = sess->PutSetAsync(*elements, true, &fail_pos);
        if ( s_fut ) {
            if ( s_fut->GetPostRet() != Session::Status::eOk ) {
                if ( s_fut->GetPostRet() == Session::Status::eDuplicateKey ) {
                    cerr << "PutSetAsync duplicate key sts: " << s_fut->GetPostRet() << " fail_pos: " << fail_pos << endl;
                } else {
                    cerr << "PutSetAsync failed key sts: " << s_fut->GetPostRet() << endl;
                    ret = -1;
                    break;
                }
            } else {
                cout << "PutSetAsync ok" << endl;
            }
        } else {
            cerr << "PutAsyncSet returned null future" << endl;
            ret = -1;
            return ret;
        }

        async_putset_futures[i] = s_fut;
        async_putset_elements[i] = elements;
        async_key_bufs[i] = keys_s;
    }
    num_loops = i;

    // sync puts in the meantime, while asyns ones are being processed
    for ( uint32_t i = 0 ; i < num_loops && i < 100000 ; i++ ) {
        vector<pair<Soe::Duple, Soe::Duple>> elements;

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            char *key_buf = new char[test.GetArgs().key_size + 64];
            string key;
            GenerateKey(test.GetArgs().random_keys, test.GetArgs().use_binary_keys, test.GetArgs().key, test.GetArgs().key_size, key, i, j + 100000);

            ::memcpy(key_buf, key.c_str(), key.size());
            key_buf[key.size()] = '\0';

            elements.push_back(pair<Soe::Duple, Soe::Duple>(Soe::Duple(key_buf, key.size()),
                Soe::Duple(test.GetArgs().value, test.GetArgs().value_size)));
        }

        // write elements
        size_t fail_pos;
        Session::Status sts = sess->PutSet(elements, true, &fail_pos);
        if ( sts != Session::Status::eOk ) {
            if ( sts == Session::Status::eDuplicateKey ) {
                cerr << "PutSet duplicate key sts: " << sts << " fail_pos: " << fail_pos << endl;
            } else {
                cerr << "PutSet failed sts: " << sts << endl;
                ret = -1;
                break;
            }
        } else {
            cout << "PutSet ok" << endl;
        }

        elements.clear();
    }

    // get async completions
    for ( uint32_t i = 0 ; i < num_loops && i < 100000 ; i++ ) {
        Session::Status sts = async_putset_futures[i]->Get();
        if ( sts != Session::Status::eOk || async_putset_futures[i]->GetPostRet() != Session::Status::eOk ) {
            cerr << "PutSetFuture::Get() failed sts: " << sts << " post_ret: " << async_putset_futures[i]->GetPostRet() <<
                " fail_pos: " << async_putset_futures[i]->GetFailPos() << endl;
            ret = -1;
        } else {
            cout << "PutSetFuture::Get() ok" << endl;
        }

        sess->DestroyFuture(async_putset_futures[i]);

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            delete [] (*async_key_bufs[i])[j];
        }

        async_key_bufs[i]->clear();
        delete async_key_bufs[i];
        async_putset_elements[i]->clear();
        delete async_putset_elements[i];
    }

    return ret;
}

int AsyncSyncMixedSetTest(SetTest &test)
{
    int ret;
    char orig_key[10000];
    ::memset(orig_key, '\0', sizeof(orig_key));
    ::memcpy(orig_key, test.GetArgs().key, test.GetArgs().key_size);


    ret = test.OpenCreateSession();
    if ( ret ) {
        return ret;
    }

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        char new_key[10000];
        sprintf(new_key, "%s_%u", orig_key, i);
        test.GetArgs().key = new_key;
        test.GetArgs().key_size = strlen(new_key);

        cout << "Doing ... " << i << " key(" << test.GetArgs().key_size << "/" << test.GetArgs().key << ")" << endl;
        ret = AsyncSyncMixedSetLoopTest(test);
        if ( ret ) {
            cout << "Failed at regression: " << i << endl;
            break;
        }
    }

    (void )test.DeleteSession();
    return ret;
}

//
// ======================= AsyncPrematureCloseSetTest ==============================
//
int AsyncPrematureCloseSetTest(SetTest &test)
{
    int ret = 0;

    ret = test.OpenCreateSession();
    if ( ret ) {
        return ret;
    }

    // write records async
    if ( test.GetArgs().random_keys ) {
        ::srand(1);
    }

    Session *sess = test.GetSession();
    if ( !sess ) {
        return ret;
    }

    PutSetFuture *putset_futures[100000];
    vector<char *> *key_bufs[100000];
    vector<pair<Soe::Duple, Soe::Duple>> *putset_elements[100000];

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        vector<pair<Soe::Duple, Soe::Duple>> *elements = new vector<pair<Soe::Duple, Soe::Duple>>;
        vector<char *> *keys_s = new vector<char *>;

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            char *key_buf = new char[test.GetArgs().key_size + 64];
            string key;
            GenerateKey(test.GetArgs().random_keys, test.GetArgs().use_binary_keys, test.GetArgs().key, test.GetArgs().key_size, key, i, j);

            ::memcpy(key_buf, key.c_str(), key.size());
            key_buf[key.size()] = '\0';

            elements->push_back(pair<Soe::Duple, Soe::Duple>(Soe::Duple(key_buf, key.size()),
                Soe::Duple(test.GetArgs().value, test.GetArgs().value_size)));
            keys_s->push_back(key_buf);
        }

        // write elements
        size_t fail_pos;
        PutSetFuture *s_fut = sess->PutSetAsync(*elements, true, &fail_pos);
        if ( s_fut ) {
            if ( s_fut->GetPostRet() != Session::Status::eOk ) {
                if ( s_fut->GetPostRet() == Session::Status::eDuplicateKey ) {
                    cerr << "PutSetAsync duplicate key sts: " << s_fut->GetPostRet() << " fail_pos: " << fail_pos << endl;
                } else {
                    cerr << "PutSetAsync failed key sts: " << s_fut->GetPostRet() << endl;
                    ret = -1;
                    sess->DestroyFuture(s_fut);
                    (void )test.DeleteSession();
                    return ret;
                }
            } else {
                cout << "PutSetAsync ok" << endl;
            }
        } else {
            cerr << "PutAsyncSet returned null future" << endl;
            ret = -1;
            (void )test.DeleteSession();
            return ret;
        }

        putset_futures[i] = s_fut;
        putset_elements[i] = elements;
        key_bufs[i] = keys_s;
    }

    (void )test.CloseSession();

    for ( uint32_t i = 0 ; i < test.GetArgs().num_regressions && i < 100000 ; i++ ) {
        Session::Status sts = putset_futures[i]->Get();
        if ( sts != Session::Status::eOk || putset_futures[i]->GetPostRet() != Session::Status::eOk ) {
            cerr << "PutSetFuture::Get() failed sts: " << sts << " post_ret: " << putset_futures[i]->GetPostRet() << " fail_pos: " << putset_futures[i]->GetFailPos() << endl;
            ret = -1;
        } else {
            cout << "PutSetFuture::Get() ok" << endl;
        }

        sess->DestroyFuture(putset_futures[i]);

        for ( uint32_t j = 0 ; j < test.GetArgs().num_ops ; j++ ) {
            delete [] (*key_bufs[i])[j];
        }

        key_bufs[i]->clear();
        delete key_bufs[i];
        putset_elements[i]->clear();
        delete putset_elements[i];
    }

    (void )test.DeleteSession();
    return ret;
}

//
// ======================= AsyncMixedPutGetTest ==============================
//

class MixedPutGet
{
    SetTest &parms;

public:
    uint32_t      th_id;

    MixedPutGet(SetTest &_parms)
        :parms(_parms),
         th_id(0)
    {}
    virtual ~MixedPutGet() {}

    SetTest &GetParms()
    {
        return parms;
    }
    virtual void start();
    static unsigned long GetThreadId();
};

unsigned long MixedPutGet::GetThreadId()
{
    string thread_id = boost::lexical_cast<string>(boost::this_thread::get_id());
    unsigned long thread_number = 0;
    sscanf(thread_id.c_str(), "%lx", &thread_number);
    return thread_number;
}

void MixedPutGet::start()
{
    unsigned long tid = MixedPutGet::GetThreadId();
    //cout << "MixedPutGet::" << __FUNCTION__ << " tid: " << tid << endl;

    int ret = GetParms().OpenCreateSession();
    if ( ret ) {
        cerr << "MixedPutGet::" << __FUNCTION__ << " can't get Soe session ret: " << ret << endl << flush;
        return;
    }

    // write records
    Session *sess = GetParms().GetSession();
    if ( !sess ) {
        cerr << "MixedPutGet::" << __FUNCTION__ << " can't get Soe session" << endl << flush;
        return;
    }

    char cdt_buf[128];
    string dt = CurrentDateTimeStr(cdt_buf);

    char get_buf[200000];
    char key[10000];
    for ( uint32_t reg_cnt = 0 ; reg_cnt < GetParms().GetArgs().num_loops ; reg_cnt++ ) {
        bool break_out = false;
        for ( uint32_t cnt = 0 ; cnt < GetParms().GetArgs().num_ops ; cnt++ ) {
            // put element
            sprintf(key, "%s_%ld_%s_%u_%u", dt.c_str(), tid, GetParms().GetArgs().key, reg_cnt, cnt);
            PutFuture *p_fut = sess->PutAsync(key, strlen(key), GetParms().GetArgs().value, GetParms().GetArgs().value_size);
            if ( p_fut ) {
                if ( p_fut->GetPostRet() != Session::Status::eOk ) {
                    cerr << "PutAsync failed sts: " << p_fut->GetPostRet() << endl << flush;
                    sess->DestroyFuture(p_fut);
                    break_out = true;
                    break;
                }
            } else {
                cerr << "PutAsync returned null future" << endl << flush;
                break_out = true;
                break;
            }

            // get element
            size_t get_buf_size = sizeof(get_buf);
            GetFuture *g_fut = sess->GetAsync(key, strlen(key), get_buf, &get_buf_size);
            if ( g_fut ) {
                if ( g_fut->GetPostRet() != Session::Status::eOk ) {
                    cerr << "GetAsync failed sts: " << g_fut->GetPostRet() << endl << flush;
                    sess->DestroyFuture(g_fut);
                    break_out = true;
                    break;
                }
            } else {
                cerr << "GetAsync returned null future" << endl << flush;
                break_out = true;
                break;
            }

            Session::Status sts = p_fut->Get();
            if ( sts != Session::Status::eOk ) {
                cerr << "PutAsync::Get() failed sts: " << sts << endl << flush;
                sess->DestroyFuture(p_fut);
                sess->DestroyFuture(g_fut);
                break_out = true;
                break;
            }

            if ( !GetParms().GetArgs().no_print ) {
                cout << "PutAsync sts " << p_fut->Status() << endl << flush;
            }
            sess->DestroyFuture(p_fut);

            sts = g_fut->Get();
            if ( sts != Session::Status::eOk && sts != Session::Status::eNotFoundKey ) {
                cerr << "GetAsync::Get() failed sts: " << sts << endl << flush;
                break_out = true;
                sess->DestroyFuture(g_fut);
                break;
            }

            if ( !GetParms().GetArgs().no_print ) {
                if ( !g_fut->GetDupleKey().size() ) {
                    cout << "GetAsync sts " << g_fut->Status() << " key(" << g_fut->GetDupleKey().size() << "/)" << endl << flush;
                } else {
                    char print_key_buf[100];
                    ::memcpy(print_key_buf, g_fut->GetDupleKey().data(), MIN(g_fut->GetDupleKey().size(), sizeof(print_key_buf) - 1));
                    print_key_buf[MIN(g_fut->GetDupleKey().size(), sizeof(print_key_buf) - 1)] = '\0';

                    if ( !g_fut->GetDupleValue().size() ) {
                        cout << "GetAsync sts " << g_fut->Status() << " key(" << g_fut->GetDupleKey().size() << "/" << print_key_buf << ")" <<
                            " value(" << g_fut->GetDupleValue().size() << "/)" << endl << flush;
                    } else {
                        char print_value_buf[1000];
                        ::memcpy(print_value_buf, g_fut->GetDupleValue().data(), MIN(g_fut->GetDupleValue().size(), sizeof(print_value_buf) - 1));
                        print_value_buf[MIN(g_fut->GetDupleValue().size(), sizeof(print_value_buf) - 1)] = '\0';


                        cout << "GetAsync sts " << g_fut->Status() << " key(" << g_fut->GetDupleKey().size() << "/" << print_key_buf << ")" <<
                            " value(" << g_fut->GetDupleValue().size() << "/" << print_value_buf << ")" << endl << flush;
                    }
                }
            }
            sess->DestroyFuture(g_fut);
        }

        if ( break_out == true ) {
            break;
        }
    }

    cout << "done tid: " << tid << endl;
    (void )GetParms().DeleteSession();

    return;
}

static int AsyncMixedPutGetTest(SetTest &test)
{
    vector<boost::thread *> worker_threads;
    vector<MixedPutGet *> worker_objs;
    worker_threads.reserve(test.GetArgs().thread_count);
    worker_objs.reserve(test.GetArgs().thread_count);

    MixedPutGet mixed_put_get(test);

    for ( uint32_t reg_cnt = 0 ; reg_cnt < test.GetArgs().num_regressions ; reg_cnt++ ) {
        for ( uint32_t i = 0 ; i < test.GetArgs().thread_count ; i++ ) {
            RegressionArgs *curr_a = new RegressionArgs;
            *curr_a = test.GetArgs();
            string *curr_store = new string(string(test.GetArgs().store_name) + "_" + to_string(i));
            curr_a->store_name = curr_store->c_str();

            SetTest *curr_t = new SetTest(*curr_a);
            worker_objs[i] = new MixedPutGet(*curr_t);
            worker_objs[i]->th_id = i;
            worker_threads[i] = new boost::thread(&MixedPutGet::start, worker_objs[i]);
        }

        for ( uint32_t i = 0 ; i < test.GetArgs().thread_count ; i++ ) {
            worker_threads[i]->join();
            delete worker_threads[i];
            delete worker_objs[i];
        }
    }

    return 0;
}

// ==============================================================


//
// usage()
//
static void usage(const char *prog)
{
    cerr <<
        "Usage " << prog << " [options]\n"
        "  -o, --cluster-name             Soe cluster name\n"
        "  -z, --space-name               Soe space name\n"
        "  -c, --store-name               Soe store name\n"
        "  -l, --transactional            Soe transactional store (default 0 - non-transactional)\n"
        "  -n, --num-ops                  Number of ops (default = 1)\n"
        "  -W, --num-loops                Number rof loop count in each regression (default =1)\n"
        "  -k, --key                      Key (default key_1)\n"
        "  -e, --end-key                  End key (default key_1)\n"
        "  -R, --destroy-store-before     Destroy store before test\n"
        "  -U, --test-binary-key          Test using binary keys with NULLs embedded\n"
        "  -t, --iterator-dir             Iterator dir (0 - forward(default), 1 - reverse)\n"
        "  -u, --default-first-iter-arg   Use iterator's First() args defaults\n"
        "  -v, --value                    Value (default value_1)\n"
        "  -d  --snap-backup-subset-name  Snapshot, backup or subset name (default snapshot_1, backup_1, subset_1)\n"
        "  -f  --snap-back-id             Snapshot or backup id (default 0)\n"
        "  -i, --sync-type                Sync type (0 - default, 1 - fdatasync, 2 - async WAL)\n"
        "  -m, --provider                 Store provider (0 - RCSDBEM, 1 - KVS, 2 - METADBCLI, 3 - POSTGRES, 4 - DEFAULT provider)\n"
        "  -j, --debug                    Debug (default 0)\n"
        "  -M, --hex-dump                 Do hex dump of read records (1 - hex print, 2 - pipe to stdout)\n"
        "  -X, --regression               IntegrityRegression test\n"
        "                                 1 - IntegrityRegression1 open/create/delete session/subset/sess_it/sub_it (default)\n"
        "                                 2 - IntegrityRegression2 write/read/verify session/subset records\n"
        "                                 3 - IntegrityRegression3 not yet defined\n"
        "                                 4 - AsyncPut() test\n"
        "                                 5 - AsyncGet() test\n"
        "                                 6 - AsyncDelete() test\n"
        "                                 7 - AsyncMerge() test\n"
        "                                 8 - AsyncPutSet() test\n"
        "                                 9 - AsyncGetSet() test\n"
        "                                 10 - AsyncDeleteSet() test\n"
        "                                 11 - AsyncMergeSet() test\n"
        "                                 12 - AsyncConcurrentPutSet() test\n"
        "                                 13 - AsyncConcurrentGetSet() test\n"
        "                                 14 - AsyncConcurrentDeleteSet() test\n"
        "                                 15 - AsyncConcurrentMergeSet() test\n"
        "                                 16 - AsyncSyncMixedSetOpenClose() test\n"
        "                                 17 - AsyncSyncMixedSet() test\n"
        "                                 18 - AsyncPrematureCloseSet() test\n"
        "                                 19 - mixed AsyncPut/AsyncGet() multi-threaded loop test\n"
        "  -N, --data-size                Set data size to an arbitrary value (will override -v)\n"
        "  -T, --sleep-usecs              Sleep usecs between tests\n"
        "  -b, --random-keys              Generate random instead of consecutive keys\n"
        "  -y, --regression-loop-cnt      Regression loop count\n"
        "  -r, --regression-thread-cnt    Regression thread count\n"
        "  -s, --wait_for_key             Wait for key press before exiting out\n"
        "  -p, --no-print                 No printing key/value (default print)\n"
        "  -x, --user-name                Run unser user-name\n"
        "  -h, --help                     Print this help\n" << endl;
}

static int soe_getopt(int argc, char *argv[],
    const char **cluster_name,
    const char **space_name,
    const char **store_name,
    int *provider,
    const char **key,
    size_t *key_size,
    const char **end_key,
    size_t *end_key_size,
    int *it_dir,
    int *default_first,
    char *value,
    size_t value_buf_size,
    size_t *value_size,
    uint32_t *num_ops,
    uint32_t *num_loops,
    uint32_t *num_regressions,
    uint32_t *regression_type,
    const char **snap_back_subset_name,
    uint32_t *snap_back_id,
    int *transactional,
    int *sync_type,
    TestType *test_type,
    int *wait_for_key,
    int *thread_count,
    int *hex_dump,
    char user_name[],
    useconds_t *sleep_usecs,
    int *no_print,
    int *group_write,
    int *delete_from_group,
    int *transaction_write,
    int *random_keys,
    int *destroy_store_before,
    int *use_binary_keys,
    int *debug)
{
    int c = 0;
    int ret = 3;

    while (1) {
        static char short_options[] = "o:z:c:t:m:n:k:e:v:d:f:y:r:i:j:x:g:w:M:X:N:T:W:suphlbABCDEFGHIJKLRU";
        static struct option long_options[] = {
            {"cluster",                 1, 0, 'o'},
            {"space",                   1, 0, 'z'},
            {"store",                   1, 0, 'c'},
            {"snap-backup-subset-name", 1, 0, 'd'},
            {"numops",                  1, 0, 'n'},
            {"numregressions",          1, 0, 'y'},
            {"key",                     1, 0, 'k'},
            {"value",                   1, 0, 'v'},
            {"snap_back_name",          1, 0, 'd'},
            {"snap_back_id",            1, 0, 'f'},
            {"write-group",             1, 0, 'g'},
            {"write-transaction",       1, 0, 'w'},
            {"delete-from-group",       1, 0, 's'},
            {"sync_type",               1, 0, 'i'},
            {"provider",                1, 0, 'm'},
            {"debug",                   1, 0, 'j'},
            {"iter-dir",                1, 0, 't'},
            {"default-first",           1, 0, 'u'},
            {"user-name",               1, 0, 'x'},
            {"value-size",              1, 0, 'N'},
            {"help",                    0, 0, 'h'},
            {0, 0, 0, 0}
        };

        if ( (c = getopt_long(argc, argv, short_options, long_options, NULL)) == -1 ) {
            break;
        }

        ret = 0;

        switch ( c ) {
        case 'o':
            *cluster_name = optarg;
            break;

        case 'z':
            *space_name = optarg;
            break;

        case 'c':
            *store_name = optarg;
            break;

        case 'l':
            *transactional = 1;
            break;

        case 'n':
            *num_ops = (uint32_t )atoi(optarg);
            break;

        case 'W':
            *num_loops = (uint32_t )atoi(optarg);
            break;

        case 'k':
            *key = optarg;
            *key_size = strlen(*key);
            break;

        case 'e':
            *end_key = optarg;
            *end_key_size = strlen(*end_key);
            break;

        case 't':
            *it_dir = atoi(optarg);
            break;

        case 'R':
            *destroy_store_before = 1;
            break;

        case 'U':
            *use_binary_keys = 1;
            break;

        case 'u':
            *default_first = 0;
            break;

        case 'v':
            *value_size = (size_t )strlen(optarg);
            strcpy(value, optarg);
            break;

        case 'd':
            *snap_back_subset_name = optarg;
            break;

        case 'f':
           *snap_back_id = (uint32_t )atoi(optarg);
            break;

        case 'g':
           *group_write = atoi(optarg);
            break;

        case 'w':
           *transaction_write = atoi(optarg);
            break;

        case 'i':
           *sync_type = atoi(optarg);
            break;

        case 'j':
           *debug = atoi(optarg);
            break;

        case 'm':
           *provider = atoi(optarg);
            break;

        case 'r':
           *thread_count = atoi(optarg);
            break;

        case 'b':
           *random_keys = 1;
            break;

        case 'p':
           *no_print = 1;
            break;

        case 'X':
            *regression_type = (uint32_t )atoi(optarg);
            if ( *regression_type == 1 ) {
                *test_type = eIntegrityRegression1;
            } else if ( *regression_type == 2 ) {
                *test_type = eIntegrityRegression2;
            } else if ( *regression_type == 3 ) {
                *test_type = eIntegrityRegression3;
            } else if ( *regression_type == 4 ) {
                *test_type = eAsyncPut;
            } else if ( *regression_type == 5 ) {
                *test_type = eAsyncGet;
            } else if ( *regression_type == 6 ) {
                *test_type = eAsyncDelete;
            } else if ( *regression_type == 7 ) {
                *test_type = eAsyncMerge;
            } else if ( *regression_type == 8 ) {
                *test_type = eAsyncPutSet;
            } else if ( *regression_type == 9 ) {
                *test_type = eAsyncGetSet;
            } else if ( *regression_type == 10 ) {
                *test_type = eAsyncDeleteSet;
            } else if ( *regression_type == 11 ) {
                *test_type = eAsyncMergeSet;
            } else if ( *regression_type == 12 ) {
                *test_type = eAsyncConcurrentPutSet;
            } else if ( *regression_type == 13 ) {
                *test_type = eAsyncConcurrentGetSet;
            } else if ( *regression_type == 14 ) {
                *test_type = eAsyncConcurrentDeleteSet;
            } else if ( *regression_type == 15 ) {
                *test_type = eAsyncConcurrentMergeSet;
            } else if ( *regression_type == 16 ) {
                *test_type = eAsyncSyncMixedOpenCloseSet;
            } else if ( *regression_type == 17 ) {
                *test_type = eAsyncSyncMixedSet;
            } else if ( *regression_type == 18 ) {
                *test_type = eAsyncPrematureCloseSet;
            } else if ( *regression_type == 19 ) {
                *test_type = eAsyncMixedPutGet;
            } else {
                *test_type = eIntegrityRegression1;
            }
            break;

        case 'M':
            *hex_dump = atoi(optarg);
            break;

        case 'N':
            *value_size = (size_t )atoi(optarg);
            break;

        case 'T':
            *sleep_usecs = (useconds_t )atoi(optarg);
            break;

        case 'y':
            *num_regressions = (uint32_t )atoi(optarg);
            break;

        case 'x':
            strcpy(user_name, optarg);
            break;

        case 's':
            *wait_for_key = 1;
            break;

        case 'h':
        case '?':
            usage(argv[0]);
            ret = 8;
            break;
        default:
            ret = -1;
            break;
        }
    }

    return ret;
}

static int SwitchUid(const char *username)
{
    int ret;
    size_t buffer_len;
    char pwd_buffer[1024];

    struct passwd pwd;
    struct passwd *pwd_res = 0;
    memset(&pwd, '\0', sizeof(struct passwd));

    buffer_len = sysconf(_SC_GETPW_R_SIZE_MAX) * sizeof(char);

    ret = getpwnam_r(username, &pwd, pwd_buffer, buffer_len, &pwd_res);
    if ( !pwd_res || ret ) {
        fprintf(stderr, "getpwnam_r failed to find requested entry.\n");
        return -3;
    }
    //printf("uid: %d\n", pwd.pw_uid);
    //printf("gid: %d\n", pwd.pw_gid);

    if ( getuid() == pwd_res->pw_uid ) {
        return 0;
    }

    ret = setegid(pwd_res->pw_gid);
    if ( ret ) {
        fprintf(stderr, "can't setegid() errno(%d)\n", errno);
        return -4;
    }
    ret = seteuid(pwd_res->pw_uid);
    if ( ret ) {
        fprintf(stderr, "can't seteuid() errno(%d)\n", errno);
        return -4;
    }

    return 0;
}

int main(int argc, char **argv)
{
    const char *cluster_name = "cluster_1";
    const char *space_name = "space_1";
    const char *store_name = "store_1";
    int provider = 0;
    const char *provider_name = 0;
    const char *key = "key_1";
    size_t key_size = (size_t )strlen(key);
    const char *end_key = "key_1";
    size_t end_key_size = (size_t )strlen(end_key);
    int it_dir = 0;
    int default_first = 1; // no default to 0
    const char *default_value = "value_1";
    char *value = 0;
    size_t value_buffer_size = 0;
    size_t value_size = 0;
    uint32_t num_ops = 1;
    uint32_t num_loops = 1;
    uint32_t num_regressions = 1;
    uint32_t regression_type = 1;
    const char *snap_back_subset_name = "embedded_in_store";
    uint32_t snap_back_id = 102;
    int transactional = 0;
    int sync_type = 0;
    int debug = 0;
    int wait_for_key = 0;
    TestType test_type = eIntegrityRegression1;
    int thread_count = 1;
    int hex_dump = 0;
    char user_name[256];
    useconds_t sleep_usecs = 0;
    int no_print = 0;
    int group_write = 0;
    int delete_from_group = 0;
    int transaction_write = 0;
    int random_keys = 0;
    int destroy_store_before = 0;
    int use_binary_keys = 0;

    value = new char[SOE_BIG_BUFFER];
    if ( !value ) {
        cerr << "Can't malloc big buffer(" << SOE_BIG_BUFFER << ")" << endl;
        return -1;
    }
    value_buffer_size = SOE_BIG_BUFFER;
    strcpy(value, default_value);
    value_size = (size_t )strlen(value);

    Session *soe_sess = 0;
    Subsetable *soe_sub = 0;
    eSessionSyncMode sess_sync_mode = eSyncDefault;
    memset(user_name, '\0', sizeof(user_name));

    int ret = soe_getopt(argc, argv,
        &cluster_name,
        &space_name,
        &store_name,
        &provider,
        &key,
        &key_size,
        &end_key,
        &end_key_size,
        &it_dir,
        &default_first,
        value,
        value_buffer_size,
        &value_size,
        &num_ops,
        &num_loops,
        &num_regressions,
        &regression_type,
        &snap_back_subset_name,
        &snap_back_id,
        &transactional,
        &sync_type,
        &test_type,
        &wait_for_key,
        &thread_count,
        &hex_dump,
        user_name,
        &sleep_usecs,
        &no_print,
        &group_write,
        &delete_from_group,
        &transaction_write,
        &random_keys,
        &destroy_store_before,
        &use_binary_keys,
        &debug);
    if ( ret ) {
        if ( ret != 8 ) {
            cerr << "Parse options failed" << endl;
            usage(argv[0]);
        }
        return ret;
    }

    if ( !*user_name ) {
        strcpy(user_name, "lzsystem");
    }
    if ( SwitchUid(user_name) < 0 ) {
        usage(argv[0]);
        cerr << "Run utility sudo integrity_test ...." << endl;
        return -1;
    }

    if ( value_buffer_size != SOE_BIG_BUFFER ) {
        if ( value_buffer_size >= SOE_BIG_BUFFER ) {
            value_buffer_size = SOE_BIG_BUFFER - 1;
        }
        value_size = value_buffer_size;
    }

    if ( value_size > strlen(value) ) {
        ::memset(value + strlen(value), 'D', value_size - strlen(value));
    }

    if ( num_ops > MAX_NUM_OPS ) {
        cerr << "number of ops too big" << endl;
        if ( value ) {
            free(value);
        }
        return -1;
    }

    if ( thread_count > MAX_THREAD_COUNT ) {
        thread_count = MAX_THREAD_COUNT;
    }

    RegressionArgs args;
    memset(&args, '\0', sizeof(RegressionArgs));
    args.cluster_name = cluster_name;
    args.space_name = space_name;
    args.store_name = store_name;
    args.provider = provider;
    args.provider_name = provider_name;
    args.key = key;
    args.key_size = key_size;
    args.end_key = end_key;
    args.end_key_size = end_key_size;
    args.it_dir = it_dir;
    args.value = value;
    args.value_buffer_size = value_buffer_size;
    args.value_size = value_size;
    args.num_ops = num_ops;
    args.num_loops = num_loops;
    args.num_regressions = num_regressions;
    args.snap_back_subset_name = snap_back_subset_name;
    args.snap_back_id = snap_back_id;
    args.transactional = transactional;
    args.sync_type = sync_type;
    args.debug = debug;
    args.wait_for_key = wait_for_key;
    args.sess = soe_sess;
    args.sub = soe_sub;
    args.sess_sync_mode = sess_sync_mode;
    args.hex_dump = hex_dump;
    args.sleep_usecs = sleep_usecs;
    args.no_print = no_print;
    args.io_counter = 0;
    args.default_first = default_first;
    args.thread_count = static_cast<uint32_t>(thread_count);
    args.random_keys = random_keys;
    args.destroy_store_before = destroy_store_before;
    args.use_binary_keys = use_binary_keys;


    //printf("pr(%s) key(%s) value(%s) value_size(%lu) num_ops(%u) num_regressions(%u) thread_count(%u) gr_w(%d) tr_nal(%d) tr_w(%d)\n",
        //provider_name, key, value, value_size, num_ops, num_regressions, thread_count, group_write, transactional, transaction_write);

    switch ( test_type ) {
    case eIntegrityRegression1:
    {
        IntegrityTest test(args);
        int ret = IntegrityRegression1(test);
        cout << "IntegrityRegression1(" << ret << ")" << endl;
    }
    break;
    case eIntegrityRegression2:
    {
        IntegrityTest test(args);
        int ret = IntegrityRegression2(test);
        cout << "IntegrityRegression2(" << ret << ")" << endl;
    }
    break;
    case eIntegrityRegression3:
    {
        IntegrityTest **tests = new IntegrityTest* [args.num_ops];

        for ( uint32_t i = 0 ; i < args.num_ops ; i++ ) {
            RegressionArgs *new_args = new RegressionArgs;
            *new_args = args;
            tests[i] = new IntegrityTest(*new_args);
            int ret = IntegrityRegression3(*tests[i]);
            cout << "IntegrityRegression3(" << ret << ")" << " for (" << i << ")" << endl;
        }

        for ( uint32_t i = 0 ; i < args.num_ops ; i++ ) {
            delete tests[i];
        }

        if ( args.wait_for_key ) {
            char something[1000];
            cin >> something;
        }
    }
    break;
    case eAsyncPut:
    {
        SetTest test(args);
        int ret = AsyncPutTest(test);
        cout << "AsyncPutTest(" << ret << ")" << endl;
    }
    break;
    case eAsyncGet:
    {
        SetTest test(args);
        int ret = AsyncGetTest(test);
        cout << "AsyncGetTest(" << ret << ")" << endl;
    }
    break;
    case eAsyncDelete:
    {
        SetTest test(args);
        int ret = AsyncDeleteTest(test);
        cout << "AsyncDeleteTest(" << ret << ")" << endl;
    }
    break;
    case eAsyncMerge:
    {
        SetTest test(args);
        int ret = AsyncMergeTest(test);
        cout << "AsyncMergeTest(" << ret << ")" << endl;
    }
    break;
    case eAsyncPutSet:
    {
        SetTest test(args);
        int ret = AsyncPutSetTest(test);
        cout << "AsyncPutSetTest(" << ret << ")" << endl;
    }
    break;
    case eAsyncGetSet:
    {
        SetTest test(args);
        int ret = AsyncGetSetTest(test);
        cout << "AsyncGetSetTest(" << ret << ")" << endl;
    }
    break;
    case eAsyncDeleteSet:
    {
        SetTest test(args);
        int ret = AsyncDeleteSetTest(test);
        cout << "AsyncDeleteSetTest(" << ret << ")" << endl;
    }
    break;
    case eAsyncMergeSet:
    {
        SetTest test(args);
        int ret = AsyncMergeSetTest(test);
        cout << "AsyncMergeSetTest(" << ret << ")" << endl;
    }
    break;
    case eAsyncConcurrentPutSet:
    {
        SetTest test(args);
        int ret = AsyncConcurrentPutSetTest(test);
        cout << "AsyncConcurrentPutSetTest(" << ret << ")" << endl;
    }
    break;
    case eAsyncConcurrentGetSet:
    {
        SetTest test(args);
        int ret = AsyncConcurrentGetSetTest(test);
        cout << "AsyncConcurrentGetSetTest(" << ret << ")" << endl;
    }
    break;
    case eAsyncConcurrentDeleteSet:
    {
        SetTest test(args);
        int ret = AsyncConcurrentDeleteSetTest(test);
        cout << "AsyncConcurrentDeleteSetTest(" << ret << ")" << endl;
    }
    break;
    case eAsyncConcurrentMergeSet:
    {
        SetTest test(args);
        int ret = AsyncConcurrentMergeSetTest(test);
        cout << "AsyncConcurrentMergeSetTest(" << ret << ")" << endl;
    }
    break;
    case eAsyncSyncMixedOpenCloseSet:
    {
        SetTest test(args);
        int ret = AsyncSyncMixedSetOpenCloseTest(test);
        cout << "AsyncSyncMixedSetOpenCloseTest(" << ret << ")" << endl;
    }
    break;
    case eAsyncSyncMixedSet:
    {
        SetTest test(args);
        int ret = AsyncSyncMixedSetTest(test);
        cout << "AsyncSyncMixedSetTest(" << ret << ")" << endl;
    }
    break;
    case eAsyncPrematureCloseSet:
    {
        SetTest test(args);
        int ret = AsyncPrematureCloseSetTest(test);
        cout << "AsyncPrematureCloseTest(" << ret << ")" << endl;
    }
    break;
    case eAsyncMixedPutGet:
    {
        SetTest test(args);
        int ret = AsyncMixedPutGetTest(test);
        cout << "AsyncMixedPutGetTest(" << ret << ")" << endl;
    }
    break;
    default:
        cout << "wrong option" << endl;
        usage(argv[0]);
        break;
    }

    return 0;
}
