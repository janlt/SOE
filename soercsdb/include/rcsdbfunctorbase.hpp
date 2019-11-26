/*
 * rcsdbfunctorbase.hpp
 *
 *  Created on: Aug 3, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBFUNCTORBASE_HPP_
#define SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBFUNCTORBASE_HPP_

using namespace boost;

namespace RcsdbFacade {

typedef SoeApi::ApiBase *ApiBasePtr;

class FunctorBase {
public:
    static const unsigned int max_functors = 1024;

    ApiBasePtr      functors[max_functors];
    unsigned int    num_functors;

    FunctorBase();
    virtual ~FunctorBase();

    int Register(ApiBasePtr fun);
    int Deregister(ApiBasePtr fun);
    void DeregisterAll();
};

}

#endif /* SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBFUNCTORBASE_HPP_ */
