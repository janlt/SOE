/*
 * soe_bind_api.cpp
 *
 *  Created on: Nov 28, 2016
 *      Author: Jan Lisowiec
 */

#include <boost/function.hpp>
#include <boost/assign/list_of.hpp>

#include <boost/type_index.hpp>

#include <vector>
#include <iostream>
#include <fstream>
#include <string>

#include "soe_bind_api.hpp"

namespace SoeApi {

uint32_t ApiBase::fun_next = 100;
#ifdef __NO_STD_MAP__
boost::shared_ptr<ApiBase> ApiBase::functions[];
#endif

std::map<ApiFuncNum, ApiBase*> ApiBase::functions __attribute__((init_priority(10000)));
std::map<std::string, ApiFuncNum> ApiBase::function_names __attribute__((init_priority(10000)));
bool ApiBase::Initializer::initialized = false;
Lock ApiBase::instance_lock __attribute__((init_priority(10000)));

ApiBase::Initializer::Initializer()
{
    if ( Initializer::initialized == false ) {
#ifdef __NO_STD_MAP__
        for ( ApiFuncNum i = 0 ; i < apiFuncMax ; i++ ) {
            ApiBase::functions[i] = boost::shared_ptr<ApiBase>(NULL);
        }
#endif
    }
    Initializer::initialized = true;
}

ApiBase::ApiBase(const std::string &nm) throw(Exception)
    :name(nm)
{
    func_no = ApiBase::fun_next++;
#ifdef __NO_STD_MAP__
    for ( ApiFuncNum i = 0 ; i < apiFuncMax ; i++ ) {
        if ( ApiBase::functions[i].get() && ApiBase::functions[i]->name == nm ) {
            throw Exception("Duplicate name detected: name: " + name);
        }
    }

    for ( ApiFuncNum j = 0 ; j < apiFuncMax ; j++ ) {
        if ( ApiBase::functions[j] == boost::shared_ptr<ApiBase>(NULL) ) {
            func_no = j;
            return;
        }
    }

    throw Exception("No more slots available");
#endif
}

ApiFuncNum ApiBase::SearchFunctionNo(const std::string &nm) throw(Exception)
{
    WriteLock w_lock(ApiBase::instance_lock);

    std::map<std::string, ApiFuncNum>::iterator it_no = function_names.find(nm);
    if ( it_no == function_names.end() ) {
        throw Exception("No function for name: " + nm);
    }

    std::map<ApiFuncNum, ApiBase*>::iterator it = functions.find((*it_no).second);
    if ( it != functions.end() ) {
        return (*it).first;
    }

#ifdef __NO_STD_MAP__
    for ( ApiFuncNum i = 0 ; i < apiFuncMax ; i++ ) {
        if ( ApiBase::functions[i].get() && nm == ApiBase::functions[i]->name ) {
            return i;
        }
    }
#endif

    throw Exception("No function for name: " + nm);
}

}




