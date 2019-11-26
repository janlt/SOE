/**
 * @file    soesubsetscapicapi.hpp
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
 * soesubsetscapi.hpp
 *
 *  Created on: Jun 25, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOEAPI_INCLUDE_SOESUBSETSCAPI_HPP_
#define SOE_SOE_SOE_SOEAPI_INCLUDE_SOESUBSETSCAPI_HPP_

SOE_EXTERNC_BEGIN

SessionStatus DisjointSubsetPut(SubsetableHND s_hnd, const char *key, size_t key_size, const char *data, size_t data_size);
SessionStatus DisjointSubsetGet(SubsetableHND s_hnd, const char *key, size_t key_size, char *buf, size_t *buf_size);
SessionStatus DisjointSubsetMerge(SubsetableHND s_hnd, const char *key, size_t key_size, const char *data, size_t data_size);
SessionStatus DisjointSubsetDelete(SubsetableHND s_hnd, const char *key, size_t key_size);

SessionStatus DisjointSubsetGetSet(SubsetableHND s_hnd, CDPairVector *items, SoeBool fail_on_first_nfound, size_t *fail_pos);
SessionStatus DisjointSubsetPutSet(SubsetableHND s_hnd, CDPairVector *items, SoeBool fail_on_first_dup, size_t *fail_pos);
SessionStatus DisjointSubsetDeleteSet(SubsetableHND s_hnd, CDVector *keys, SoeBool fail_on_first_nfound, size_t *fail_pos);
SessionStatus DisjointSubsetMergeSet(SubsetableHND s_hnd, CDPairVector *items);

IteratorHND DisjointSubsetCreateIterator(SubsetableHND s_hnd,
    eIteratorDir dir,
    const char *start_key,
    size_t start_key_size,
    const char *end_key,
    size_t end_key_size);
SessionStatus DisjointSubsetDestroyIterator(SubsetableHND s_hnd, IteratorHND it);

SOE_EXTERNC_END

#endif /* SOE_SOE_SOE_SOEAPI_INCLUDE_SOESUBSETSCAPI_HPP_ */
