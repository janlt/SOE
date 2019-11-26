/**
 * @file    soeduple.cpp
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
 * soeduple.cpp
 *
 *  Created on: Jun 8, 2017
 *      Author: Jan Lisowiec
 */

#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <new>

#include "soeduple.hpp"

using namespace std;

namespace Soe {

char toHex(unsigned char v)
{
    if ( v <= 9 ) {
        return '0' + v;
    }
    return 'A' + v - 10;
}

int fromHex(char c)
{
    // toupper:
    if ( c >= 'a' && c <= 'f' ) {
        c -= ('a' - 'A');  // space
    }

    // validation
    if ( c < '0' || (c > '9' && (c < 'A' || c > 'F')) ) {
        return -1;
    }

    if ( c <= '9' ) {
        return c - '0';
    }
    return c - 'A' + 10;
}

Duple::Duple(const DupleParts& parts, string *buf)
{
    size_t length = 0;
    for ( int i = 0 ; i < parts.num_parts ; ++i ) {
        length += parts.parts[i].size();
    }
    buf->reserve(length);

    for ( int i = 0 ; i < parts.num_parts ; ++i ) {
        buf->append(parts.parts[i].data(), parts.parts[i].size());
    }
    data_ptr = buf->data();
    data_size = buf->size();
}

string Duple::ToString(bool hex) const
{
    string result;  // RVO/NRVO/move
    if ( hex ) {
        result.reserve(2 * data_size);
        for ( size_t i = 0 ; i < data_size ; ++i ) {
            unsigned char c = data_ptr[i];
            result.push_back(toHex(c >> 4));
            result.push_back(toHex(c & 0xf));
        }
        return result;
    } else {
        result.assign(data_ptr, data_size);
        return result;
    }
}

bool Duple::DecodeHex(string *result) const
{
    string::size_type len = data_size;
    if ( len % 2 ) {
        return false;
    }

    if ( !result ) {
        return false;
    }

    result->clear();
    result->reserve(len / 2);

    for ( size_t i = 0 ; i < len ; ) {
        int h1 = fromHex(data_ptr[i++]);
        if ( h1 < 0 ) {
            return false;
        }

        int h2 = fromHex(data_ptr[i++]);
        if ( h2 < 0 ) {
            return false;
        }
        result->push_back((h1 << 4) | h2);
    }
    return true;
}

}


