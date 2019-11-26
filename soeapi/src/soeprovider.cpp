/**
 * @file    soeprovider.cpp
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
 * soeprovider.hpp
 *
 *  Created on: Feb 3, 2017
 *      Author: Jan Lisowiec
 */

#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <new>

#include "soe_soe.hpp"
#include "soeprovider.hpp"

using namespace std;
using namespace KvsFacade;
using namespace RcsdbFacade;

//#define SOE_DEFAULT_PROVIDER_RCSDB

namespace Soe {

void CapabilitySet::AddCapability(const string &cap)
{
    capabilities.push_back(cap);
}

vector<pair<string, ProviderCapabilities> > Provider::providers;
string Provider::default_provider;

#ifdef SOE_DEFAULT_PROVIDER_RCSDB
CreateLocalNameFun Provider::default_create_name_fun = RcsdbFacade::CreateLocalName;
#else
CreateLocalNameFun Provider::default_create_name_fun = MetadbcliFacade::CreateLocalName;
#endif
Lock     Provider::static_lock;

bool Provider::Initializer::initialized = false;
Provider::Initializer initialized;

Provider::Initializer::Initializer()
{
    WriteLock w_lock(Provider::static_lock);

    if ( Initializer::initialized == false ) {
        providers.push_back(pair<string, ProviderCapabilities>(RcsdbFacade::GetProvider(), ProviderCapabilities(RcsdbFacade::CreateLocalName)));
        providers.push_back(pair<string, ProviderCapabilities>(MetadbcliFacade::GetProvider(), ProviderCapabilities(MetadbcliFacade::CreateLocalName)));
        providers.push_back(pair<string, ProviderCapabilities>(KvsFacade::GetProvider(), ProviderCapabilities(KvsFacade::CreateLocalName)));

#ifdef SOE_DEFAULT_PROVIDER_RCSDB
        Provider::default_provider = RcsdbFacade::GetProvider();
        Provider::default_create_name_fun = RcsdbFacade::CreateLocalName;
#else
        string r_p = RcsdbFacade::GetProvider();
        string k_p = KvsFacade::GetProvider();
        Provider::default_provider = MetadbcliFacade::GetProvider();
        Provider::default_create_name_fun = MetadbcliFacade::CreateLocalName;
#endif
    }
    Initializer::initialized = true;
}

Provider::Initializer::~Initializer()
{
    WriteLock w_lock(Provider::static_lock);

    if ( Initializer::initialized == true ) {
        providers.clear();
        default_provider = "";
        default_create_name_fun = 0;

    }
    Initializer::initialized = false;
}

std::string Provider::GetName()
{
    return name;
}
std::string Provider::GetLocalName()
{
    return RcsdbFacade::GetProvider();
}

CreateLocalNameFun Provider::CheckProvider()
{
    auto p = pair<string, ProviderCapabilities>(name, ProviderCapabilities());

    for ( auto it = providers.begin() ; it != providers.end() ; it++ ) {
        if ( it->first == name ) {
          return it->second.create_name_fun;
        }
    }

    return 0;
}

void Provider::RegisterProvider(const string &service_locator, const ProviderCapabilities &pc)
{
    WriteLock w_lock(Provider::static_lock);

    providers.push_back(pair<string, ProviderCapabilities>(service_locator, pc));
}

void Provider::DeregisterProvider(const string &service_locator, const ProviderCapabilities &pc)
{
    WriteLock w_lock(Provider::static_lock);

    ProviderCapabilities  npc = const_cast<ProviderCapabilities &>(pc);
    for ( auto it = providers.begin() ; it != providers.end() ; it++ ) {
        if ( it->first == service_locator && it->second == npc ) {
            providers.erase(it);
            break;
        }
    }
}

void Provider::EnumProviders(vector<pair<string, ProviderCapabilities> > &_providers)
{
    WriteLock w_lock(Provider::static_lock);

    for ( auto it = providers.begin() ; it != providers.end() ; it++ ) {
        _providers.push_back(pair<string, ProviderCapabilities>(it->first, it->second));
    }
}

const string Provider::InvalidCreateLocalNameFun(const string &_cluster,
    const string &_space,
    const string &_store,
    uint32_t token,
    const string &_fun)
{
    return "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
}

}
