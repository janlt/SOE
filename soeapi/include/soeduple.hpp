/**
 * @file    soeduple.hpp
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
 * soeduple.hpp
 *
 *  Created on: Jun 8, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOEAPI_INCLUDE_SOEDUPLE_HPP_
#define SOE_SOE_SOE_SOEAPI_INCLUDE_SOEDUPLE_HPP_

namespace Soe {

class Duple
{
public:
    Duple()
        :data_ptr(""),
         data_size(0)
    {}

    Duple(const char* d, size_t n)
        :data_ptr(d),
         data_size(n)
    {}

    Duple(const std::string& s)
        :data_ptr(s.data()),
         data_size(s.size())
    {}

    Duple(const char* s)
        :data_ptr(s),
         data_size(strlen(s))
    {}

    Duple(const struct DupleParts& parts, std::string* buf);

    const char* data() const
    {
        return data_ptr;
    }

    char* buffer() const
    {
        return const_cast<char *>(data_ptr);
    }

    void set_size(size_t new_size)
    {
        data_size = new_size;
    }

    size_t size() const
    {
        return data_size;
    }

    size_t *buffer_size()
    {
        return &data_size;
    }

    bool empty() const
    {
        return data_size == 0 || !data_ptr;
    }

    char operator[](size_t n) const
    {
        assert(n < size());
        return data_ptr[n];
    }

    void clear()
    {
        data_ptr = "";
        data_size = 0;
    }

    // Drop the first "n" bytes from this slice.
    void remove_prefix(size_t n)
    {
        assert( n <= size() );
        data_ptr += n;
        data_size -= n;
    }

    void remove_suffix(size_t n)
    {
        assert( n <= size() );
        data_size -= n;
    }

    std::string ToString(bool hex = false) const;

    bool DecodeHex(std::string* result) const;

    //   <  0 iff "*this" <  "b",
    //   == 0 iff "*this" == "b",
    //   >  0 iff "*this" >  "b"
    int compare(const Duple& b) const;

    bool starts_with(const Duple& x) const
    {
        return ((data_size >= x.data_size) &&
            (!memcmp(data_ptr, x.data_ptr, x.data_size)));
    }

    bool ends_with(const Duple& x) const
    {
        return ((data_size >= x.data_size) &&
            (!memcmp(data_ptr + data_size - x.data_size, x.data_ptr, x.data_size)));
    }

    size_t difference_offset(const Duple& b) const;

private:
    const char *data_ptr;
    size_t      data_size;
};

struct DupleParts
{
    DupleParts(const Duple* _parts, int _num_parts)
        :parts(_parts),
         num_parts(_num_parts)
    {}
    DupleParts()
        :parts(nullptr),
         num_parts(0)
    {}

    const Duple *parts;
    int          num_parts;
};

inline bool operator==(const Duple& x, const Duple& y)
{
    return ((x.size() == y.size()) && (!memcmp(x.data(), y.data(), x.size())));
}

inline bool operator!=(const Duple& x, const Duple& y)
{
    return !(x == y);
}

inline int Duple::compare(const Duple& b) const
{
    const size_t min_len = (data_size < b.data_size) ? data_size : b.data_size;
    int r = memcmp(data_ptr, b.data_ptr, min_len);
    if ( !r ) {
        if ( data_size < b.data_size ) {
            r = -1;
        } else if (data_size > b.data_size) {
            r = +1;
        }
    }
    return r;
}

inline size_t Duple::difference_offset(const Duple& b) const
{
    size_t off = 0;
    const size_t len = (data_size < b.data_size) ? data_size : b.data_size;
    for ( ; off < len ; off++ ) {
        if ( data_ptr[off] != b.data_ptr[off] ) {
            break;
        }
    }
    return off;
}

}

#endif /* SOE_SOE_SOE_SOEAPI_INCLUDE_SOEDUPLE_HPP_ */
