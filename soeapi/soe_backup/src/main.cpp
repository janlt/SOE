/**
 * @file    main.cpp
 * @author  Jan Lisowiec <jlisowiec@gmail.com>
 * @version 1.0
 * @brief   Soe backup utility
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
 *  Created on: Sep 20, 2018
 *      Author: Jan Lisowiec
 */

#include <pwd.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>

#include <string.h>
#include <dlfcn.h>

#include <string>
#include <vector>

#include <vector>
#include <map>
#include <fstream>
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
#include "soestatistics.hpp"
#include "soemgr.hpp"

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

// ==============================================================

//
// usage()
//
static void usage(const char *prog)
{
    cerr <<
        "Usage " << prog << " [options]\n"
        "  -d, --backup-dir               Backup root directory path\n"
        "  -B, --do-backup                Perform backup\n"
        "  -L, --list-dbs                 List databases\n"
        "  -M, --list-backup-dbs          List backup databases\n"
        "  -D, --do-drop                  Drop backup\n"
        "  -F, --do-verify                Verify backup\n"
        "  -R, --do-restore               Restore backup\n"
        //"  -o, --cluster-name             Soe cluster name\n"
        //"  -z, --space-name               Soe space name\n"
        //"  -c, --store-name               Soe store name\n"
        //"  -m, --provider                 Store provider (0 - RCSDBEM, 1 - KVS, 2 - METADBCLI, 3 - POSTGRES, 4 - DEFAULT provider)\n"
        "  -r, --no-overwrite             No overwrite duplicates\n"
        "  -j, --debug                    Debug (default 0)\n"
        "  -x, --user-name                Run under user-name\n"
        "  -h, --help                     Print this help\n" << endl;
}

typedef enum _eBackupOp {
    eDoBackup       = 1,
    eListDbs        = 2,
    eListBackupDbs  = 3,
    eDoDrop         = 4,
    eDoVerify       = 5,
    eDoRestore      = 6
} eBackupOp;

static int soe_getopt(int argc, char *argv[],
    const char **backup_dir,
    const char **cluster_name,
    const char **space_name,
    const char **store_name,
    int *provider,
    char user_name[],
    int *op,
    bool *overwrite,
    int *debug)
{
    int c = 0;
    int ret = 3;

    while (1) {
        //static char short_options[] = "d:o:z:c:m:x:BFDR";
        static char short_options[] = "d:m:x:BLMFDRrhj";
        static struct option long_options[] = {
            {"do-backup",               0, 0, 'B'},
            {"list-dbs",                0, 0, 'L'},
            {"list-backup-dbs",         0, 0, 'M'},
            {"do-verify",               0, 0, 'F'},
            {"do-drop",                 0, 0, 'D'},
            {"do-restore",              0, 0, 'R'},
            {"backup-dir",              1, 0, 'd'},
            {"no-overwrite",            0, 0, 'r'},
            //{"cluster",                 1, 0, 'o'},
            //{"space",                   1, 0, 'z'},
            //{"store",                   1, 0, 'c'},
            //{"provider",                1, 0, 'm'},
            {"user-name",               1, 0, 'x'},
            {"debug",                   0, 0, 'j'},
            {"help",                    0, 0, 'h'},
            {0, 0, 0, 0}
        };

        if ( (c = getopt_long(argc, argv, short_options, long_options, NULL)) == -1 ) {
            break;
        }

        ret = 0;

        switch ( c ) {
        case 'B':
            *op = eDoBackup;
            break;

        case 'D':
            *op = eDoDrop;
            break;

        case 'F':
            *op = eDoVerify;
            break;

        case 'R':
            *op = eDoRestore;
            break;

        case 'L':
            *op = eListDbs;
            break;

        case 'M':
            *op = eListBackupDbs;
            break;

        case 'd':
            *backup_dir = optarg;
            break;

        //case 'o':
            //*cluster_name = optarg;
            //break;

        //case 'z':
            //*space_name = optarg;
            //break;

        //case 'c':
            //*store_name = optarg;
            //break;

        //case 'm':
           //*provider = atoi(optarg);
            //break;

        case 'r':
            *overwrite = false;
            break;

        case 'x':
            strcpy(user_name, optarg);
            break;

        case 'h':
        case '?':
            usage(argv[0]);
            ret = 8;
            break;
        case 'j':
            *debug = 1;
            break;
        default:
            ret = -1;
            break;
        }

        if ( ret ) {
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
    const char *backup_dir = "";
    const char *cluster_name = "";
    const char *space_name = "";
    const char *store_name = "";
    int provider = 2;
    char user_name[256];
    int debug = 0;
    int op = -1; // 1 - backup, 2 - restore
    bool overwrite = true;

    const char *error_msg = "Either -B (backup), -D )drop), -F (verify) or -R (restore) option is required";

    memset(user_name, '\0', sizeof(user_name));

    int ret = soe_getopt(argc, argv,
        &backup_dir,
        &cluster_name,
        &space_name,
        &store_name,
        &provider,
        user_name,
        &op,
        &overwrite,
        &debug);
    if ( ret ) {
        if ( ret != 8 ) {
            cerr << "Parse options failed" << endl;
            usage(argv[0]);
        }
        return ret;
    }

    if ( debug ) {
        cout << "op: " << op << " backup_dir: " << backup_dir << " provider: " << provider << " user_name: " << user_name << " overwrite: " << overwrite << " debug: " << debug << endl;
    }

    if ( (op != eDoBackup && op != eDoRestore && op != eDoVerify && op != eDoDrop && op != eListDbs && op != eListBackupDbs) ) {
        cerr << error_msg << endl;
        usage(argv[0]);
        return -1;
    }

    if ( op != eListDbs && !*backup_dir ) {
        cerr << error_msg << endl;
        usage(argv[0]);
        return -1;
    }

    if ( !*user_name ) {
        strcpy(user_name, "lzsystem");
    }

    if ( debug ) {
        setenv("RC_LOGLVL", "ffffffffffffffff", 1);
        setenv("RC_SYSLOG", "stderr", 1);
    }

    if ( SwitchUid(user_name) < 0 ) {
        usage(argv[0]);
        cerr << "Run utility sudo integrity_test ...." << endl;
        return -1;
    }

    string pr = GetProviderName(provider);

    switch ( op ) {
    case eDoBackup:
    {
        // start backup
        Session::Status sts = Manager::getManager()->Backup(backup_dir, pr, false);
        cout << "Backup ret(" << Session::StatusToText(sts) << ")" << endl;
    }
    break;
    case eListDbs:
    {
        // start dbs
        ostringstream str_buf;
        Session::Status sts = Manager::getManager()->ListDbs(str_buf, pr);
        cout << str_buf.str() << flush;
        cout << "ListDBs ret(" << Session::StatusToText(sts) << ")" << endl;
    }
    break;
    case eListBackupDbs:
    {
        // start dbs
        ostringstream str_buf;
        string backup_path = string(backup_dir);
        Session::Status sts = Manager::getManager()->ListBackupDbs(backup_path, str_buf, pr);
        cout << str_buf.str() << flush;
        cout << "ListBackupDBs ret(" << Session::StatusToText(sts) << ")" << endl;
    }
    break;
    case eDoDrop:
    {
        // start verify
        Session::Status sts = Manager::getManager()->Drop(backup_dir, pr);
        cout << "Drop ret(" << Session::StatusToText(sts) << ")" << endl;
    }
    break;
    case eDoVerify:
    {
        // start verify
        Session::Status sts = Manager::getManager()->Verify(backup_dir, pr);
        cout << "Verify ret(" << Session::StatusToText(sts) << ")" << endl;
    }
    break;
    case eDoRestore:
    {
        // start restore
        Session::Status sts = Manager::getManager()->Restore(backup_dir, pr, true);
        cout << "Restore ret(" << Session::StatusToText(sts) << ")" << endl;
    }
    break;
    default:
        cerr << error_msg << endl;
        usage(argv[0]);
        return -1;
    }

    return 0;
}
