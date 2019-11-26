/**
 * @file    soesessiongroupcapi.hpp
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
 * soesessiongroupcapi.hpp
 *
 *  Created on: Jun 25, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOEAPI_INCLUDE_SOESESSIONGROUPCAPI_HPP_
#define SOE_SOE_SOE_SOEAPI_INCLUDE_SOESESSIONGROUPCAPI_HPP_

SOE_EXTERNC_BEGIN

SessionStatus GroupPut(GroupHND g_hnd, const char *key, size_t key_size, const char *data, size_t data_size);
SessionStatus GroupMerge(GroupHND g_hnd, const char *key, size_t key_size, const char *data, size_t data_size);
SessionStatus GroupDelete(GroupHND g_hnd, const char *key, size_t key_size);

SOE_EXTERNC_END

#endif /* SOE_SOE_SOE_SOEAPI_INCLUDE_SOESESSIONGROUPCAPI_HPP_ */
