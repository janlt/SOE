/*
 * rcsdbfutureapi.hpp
 *
 *  Created on: Dec 14, 2017
 *      Author: Jan Lisowiec
 */

#ifndef RENTCONTROL_SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBFUTUREAPI_HPP_
#define RENTCONTROL_SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBFUTUREAPI_HPP_

namespace RcsdbFacade {

struct FutureSignal
{
    void                *future;
    LocalArgsDescriptor *args;
};

}

#endif /* RENTCONTROL_SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBFUTUREAPI_HPP_ */
