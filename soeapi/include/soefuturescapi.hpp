/**
 * @file    soefuturescapi.hpp
 * @author  Jan Lisowiec <jlisowiec@gmail.com>
 * @version 1.0
 * @brief   Soe Futures API
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
 * soefuturescapi.hpp
 *
 *  Created on: Dec 22, 2017
 *      Author: Jan Lisowiec
 */

#ifndef RENTCONTROL_SOE_SOE_SOE_SOEAPI_INCLUDE_SOEFUTURESCAPI_HPP_
#define RENTCONTROL_SOE_SOE_SOE_SOEAPI_INCLUDE_SOEFUTURESCAPI_HPP_

SOE_EXTERNC_BEGIN

/*
 * @brief Futures C API
 *
 * Futures C API
 */
FutureHND SessionPutAsync(SessionHND s_hnd, const char *key, size_t key_size, const char *data, size_t data_size);
FutureHND SessionGetAsync(SessionHND s_hnd, const char *key, size_t key_size, char *buf, size_t *buf_size);
FutureHND SessionDeleteAsync(SessionHND s_hnd, const char *key, size_t key_size);
FutureHND SessionMergeAsync(SessionHND s_hnd, const char *key, size_t key_size, const char *data, size_t data_size);

SetFutureHND SessionGetSetAsync(SessionHND s_hnd, CDPairVector *items, SoeBool fail_on_first_nfound, size_t *fail_pos);
SetFutureHND SessionPutSetAsync(SessionHND s_hnd, CDPairVector *items, SoeBool fail_on_first_dup, size_t *fail_pos);
SetFutureHND SessionDeleteSetAsync(SessionHND s_hnd, CDVector *keys, SoeBool fail_on_first_nfound, size_t *fail_pos);
SetFutureHND SessionMergeSetAsync(SessionHND s_hnd, CDPairVector *items);

void SessionDestroyFuture(SessionHND s_hnd, FutureHND f_hnd);

SessionStatus FutureGetPostRet(FutureHND f_hnd);
SessionStatus FutureGet(FutureHND f_hnd);

SessionStatus SetFutureGetFailDetect(SetFutureHND f_hnd, SoeBool *f_det);
SessionStatus SetFutureGetFailPos(SetFutureHND f_hnd, size_t *f_pos);

SessionStatus FutureGetKey(FutureHND f_hnd, char **key);
SessionStatus FutureGetKeySize(FutureHND f_hnd, size_t *key_size);
SessionStatus FutureGetValue(FutureHND f_hnd, char **value);
SessionStatus FutureGetValueSize(FutureHND f_hnd, size_t *value_size);
SessionStatus FutureGetCDupleKey(FutureHND f_hnd, CDuple *key_cduple);
SessionStatus FutureGetCDupleValue(FutureHND f_hnd, CDuple *value_cduple);

SessionStatus SetFutureGetItems(SetFutureHND s_fut, CDPairVector *items);
SessionStatus SetFutureGetKeys(SetFutureHND s_fut, CDVector *keys);

SOE_EXTERNC_END

#endif /* RENTCONTROL_SOE_SOE_SOE_SOEAPI_INCLUDE_SOEFUTURESCAPI_HPP_ */
