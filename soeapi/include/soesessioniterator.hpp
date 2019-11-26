/**
 * @file    soesessioniterator.hpp
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
 * soesessioniterator.hpp
 *
 *  Created on: Jun 7, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOEAPI_INCLUDE_SOESESSIONITERATOR_HPP_
#define SOE_SOE_SOE_SOEAPI_INCLUDE_SOESESSIONITERATOR_HPP_

namespace Soe {

/**
 * @brief SessionIterator class
 *
 * This class allows iterating over instances of a store ({"Cluster", "Space", "Store", "Provider"}).
 *
 * A session iterator is created by invoking CreateIterator() method of Session class. This method
 * returns a pointer to Iterator base class. Subsequently, session iterator can be destroyed via
 * invoking DestroyIteraror(). Both session iterators and subset iterators are destroyed the same way,
 * i.e. via DestroyIterator().
 *
 */
class SessionIterator: public Iterator
{
friend class Session;

public:
    typedef enum _eIteratorDir {
        eIteratorDirForward  = 0,
        eIteratorDirBackward = 1
    } eIteratorDir;

private:
    std::shared_ptr<Session>       sess;
    Iterator::eIteratorDir         dir;
    LocalArgsDescriptor            args;
    LocalArgsDescriptor            status_args;

    SessionIterator(std::shared_ptr<Session> _sess,
        uint32_t _it = Iterator::invalid_iterator,
        Iterator::eIteratorDir _dir = Iterator::eIteratorDir::eIteratorDirForward);
    virtual ~SessionIterator();

    Iterator::eStatus TranslateStatus(Session::Status s_sts);

public:
    virtual Iterator::eStatus First(const char *key = 0, size_t key_size = 0) throw(std::runtime_error);
    virtual Iterator::eStatus Next() throw(std::runtime_error);
    virtual Iterator::eStatus Last() throw(std::runtime_error);

    virtual Duple Key() const;
    virtual Duple Value() const;

    virtual bool Valid() throw(std::runtime_error);

    Session *GetSession() { return sess.get(); }
};

}

#endif /* SOE_SOE_SOE_SOEAPI_INCLUDE_SOESESSIONITERATOR_HPP_ */
