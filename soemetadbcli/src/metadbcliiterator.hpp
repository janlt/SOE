/**
 * @file    metadbcliiterator.hpp
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
 * metadbcliiterator.hpp
 *
 *  Created on: Aug 7, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_METADBCLI_SRC_METADBCLIITERATOR_HPP_
#define SOE_SOE_SOE_METADBCLI_SRC_METADBCLIITERATOR_HPP_

namespace MetadbcliFacade {

class StoreProxy;

class ProxyIterator
{
    friend class StoreProxy;

    uint32_t  id;
    char     *key_buf;
    size_t    key_buf_len;
    char     *buf_buf;
    size_t    buf_buf_len;

protected:
    ProxyIterator(uint32_t _id);
    ~ProxyIterator();
};

}



#endif /* SOE_SOE_SOE_METADBCLI_SRC_METADBCLIITERATOR_HPP_ */
