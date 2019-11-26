/*
 * kvsfacadeapi.hpp
 *
 *  Created on: Dec 7, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_KVS_INCLUDE_KVSFACADEAPI_HPP_
#define SOE_SOE_SOE_KVS_INCLUDE_KVSFACADEAPI_HPP_

namespace KvsFacade {

class Exception
{
    std::string error;

public:
    Exception(const std::string err = "Know nothing"): error(err) {}
    ~Exception() {}
    const std::string &what() const { return error; }
};

}



#endif /* SOE_SOE_SOE_KVS_INCLUDE_KVSFACADEAPI_HPP_ */
