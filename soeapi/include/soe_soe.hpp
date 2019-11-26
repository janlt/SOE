/**
 * @file    soe_soe.hpp
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
 * soe_soe.hpp
 *
 *  Created on: Jan 16, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOE_INCLUDE_SOE_SOE_HPP_
#define SOE_SOE_SOE_SOE_INCLUDE_SOE_SOE_HPP_

#include "soecapi.hpp"

#include "soe_bind_api.hpp"
#include <boost/atomic.hpp>
#include <boost/thread/future.hpp>

#include "kvsargdefs.hpp"
#include "rcsdbargdefs.hpp"
#include "rcsdbfutureapi.hpp"
#include "metadbcliargdefs.hpp"
#include "kvsfacadeapi.hpp"
#include "rcsdbfacadeapi.hpp"
#include "metadbclifacadeapi.hpp"

#include "soeaccessors.hpp"
#include "soeprovider.hpp"
#include "soemanageable.hpp"
#include "soeiterable.hpp"
#include "soeduple.hpp"
#include "soesessionable.hpp"
#include "soesession.hpp"
#include "soefuturable.hpp"
#include "soefutures.hpp"
#include "soesubsetable.hpp"
#include "soesubsets.hpp"
#include "soesessioniterator.hpp"
#include "soesubsetiterator.hpp"
#include "soebackupengine.hpp"
#include "soesnapshotengine.hpp"
#include "soesessiongroup.hpp"
#include "soesessiontransaction.hpp"

namespace Soe {

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

//!
//! Translate vectors
//!
int CDPVectorToCPPDPVector(const CDPairVector *c_items, std::vector<std::pair<Soe::Duple, Soe::Duple>> *cpp_items);
int CPPDPVectorToCDPVector(const std::vector<std::pair<Soe::Duple, Soe::Duple>> *cpp_items, CDPairVector *c_items, bool alloc_c_vector = true);

int CDVectorToCPPDVector(const CDVector *c_items, std::vector<Soe::Duple> *cpp_items);
int CPDPVectorToCDVector(const std::vector<Soe::Duple> *cpp_items, CDVector *c_items, bool alloc_c_vector = true);


}

#endif /* SOE_SOE_SOE_SOE_INCLUDE_SOE_SOE_HPP_ */
