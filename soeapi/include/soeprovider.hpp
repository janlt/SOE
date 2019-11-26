/**
 * @file    soeprovider.hpp
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

#ifndef SOE_SOE_SOE_SOE_INCLUDE_SOEPROVIDER_HPP_
#define SOE_SOE_SOE_SOE_INCLUDE_SOEPROVIDER_HPP_

namespace Soe {

typedef const std::string (*CreateLocalNameFun)(const std::string &cluster,
        const std::string &space,
        const std::string &store,
        uint32_t token,
        const std::string &fun);

#define NullCreateLocalNameFun reinterpret_cast<CreateLocalNameFun>(0)

class CapabilitySet
{
public:
    std::vector<std::string> capabilities;

    CapabilitySet() {}
    virtual ~CapabilitySet() {}

    void AddCapability(const std::string &cap);
};

class ProviderCapabilities: public CapabilitySet
{
public:
    CreateLocalNameFun   create_name_fun;

public:
    ProviderCapabilities(const CreateLocalNameFun fun = RcsdbFacade::CreateLocalName): create_name_fun(fun) {}
    virtual ~ProviderCapabilities() {}

    bool operator==(const ProviderCapabilities &right) const
    {
        return create_name_fun == right.create_name_fun ? true : false;
    }
};

class Provider
{
    std::string                   name;
    static Lock                   static_lock;
    static std::vector<std::pair<std::string, ProviderCapabilities> > providers;

public:
    static std::string            default_provider;
    static CreateLocalNameFun     default_create_name_fun;

    Provider(const std::string &_name = default_provider): name(_name) {}
    ~Provider() {}

    bool operator==(const Provider &right) const
    {
        return name == right.name ? true : false;
    }

    Provider &operator=(const Provider &right)
    {
        name = right.name;
        return *this;
    }

    Provider(const Provider &right)
    {
        name = right.name;
    }

    std::string GetName();
    static std::string GetLocalName();
    CreateLocalNameFun CheckProvider();
    static void RegisterProvider(const std::string &service_locator, const ProviderCapabilities &pc);
    static void DeregisterProvider(const std::string &service_locator, const ProviderCapabilities &pc);
    static void EnumProviders(std::vector<std::pair<std::string, ProviderCapabilities> > &providers);

    class Initializer {
    public:
        static bool initialized;
        Initializer();
        ~Initializer();
    };
    static Initializer initalized;

    static const std::string InvalidCreateLocalNameFun(const std::string &_cluster,
        const std::string &_space,
        const std::string &_store,
        uint32_t token,
        const std::string &_fun);
};

}

#endif /* SOE_SOE_SOE_SOE_INCLUDE_SOEPROVIDER_HPP_ */
