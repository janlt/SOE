/**
 * @file    soesessioniteratorcapi.hpp
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
 * soesessioniteratorcapi.hpp
 *
 *  Created on: Jun 25, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOEAPI_INCLUDE_SOESESSIONITERATORCAPI_HPP_
#define SOE_SOE_SOE_SOEAPI_INCLUDE_SOESESSIONITERATORCAPI_HPP_

SOE_EXTERNC_BEGIN

SessionStatus SessionIteratorFirst(IteratorHND i_hnd, const char *key, size_t key_size);
SessionStatus SessionIteratorNext(IteratorHND i_hnd);
SessionStatus SessionIteratorLast(IteratorHND i_hnd);

CDuple SessionIteratorKey(IteratorHND i_hnd);
CDuple SessionIteratorValue(IteratorHND i_hnd);

SoeBool SessionIteratorValid(IteratorHND i_hnd);

SOE_EXTERNC_END

#endif /* SOE_SOE_SOE_SOEAPI_INCLUDE_SOESESSIONITERATORCAPI_HPP_ */
