/**
 * @file    soeiterable.hpp
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
 * soeiterable.hpp
 *
 *  Created on: Jun 12, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOEAPI_INCLUDE_SOEITERABLE_HPP_
#define SOE_SOE_SOE_SOEAPI_INCLUDE_SOEITERABLE_HPP_

namespace Soe {

class Duple;

class Iterator: public Manageable
{
    friend class Subsetable;

public:
    typedef enum _eStatus {
        SOE_ITERATOR_STATUS()
    } eStatus;

    typedef enum _eIteratorDir {
        SOE_ITERATOR_DIR
    } eIteratorDir;

    typedef enum _eType {
        SOE_ITERATOR_TYPE
    } eType;

    uint32_t                   iterator_no;
    Iterator::eType            type;
    const static uint32_t      invalid_iterator = -1;
    int                        session_status;

    Iterator(uint32_t _it, Iterator::eType _type): iterator_no(_it), type(_type) {}
    virtual ~Iterator() {}

    virtual Iterator::eStatus First(const char *key = 0, size_t key_size = 0) throw(std::runtime_error) = 0;
    virtual Iterator::eStatus Next() throw(std::runtime_error) = 0;
    virtual Iterator::eStatus Last() throw(std::runtime_error) = 0;

    virtual Duple Key() const = 0;
    virtual Duple Value() const = 0;

    virtual bool Valid() throw(std::runtime_error) = 0;
};

}

#endif /* SOE_SOE_SOE_SOEAPI_INCLUDE_SOEITERABLE_HPP_ */
