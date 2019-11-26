/*
 * rcsdbidgener.hpp
 *
 *  Created on: Apr 19, 2018
 *      Author: Jan Lisowiec
 */

#ifndef RENTCONTROL_SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBIDGENER_HPP_
#define RENTCONTROL_SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBIDGENER_HPP_

#include <random> // std::random_device

namespace RcsdbFacade {

typedef boost::shared_mutex           Lock;
typedef boost::mutex                  PlainLock;
typedef boost::unique_lock<PlainLock> WritePlainLock;

template <class T>
class IdGener
{
    T   offset;

public:
    IdGener(T _start = 0);
    virtual ~IdGener();

    virtual T GetId() = 0;
    T GetOffset();
    void SetOffset(T _offset);
    IdGener &operator = (const IdGener &_r);
};

template <class T>
IdGener<T>::IdGener(T _offset)
    :offset(_offset)
{}

template <class T>
IdGener<T>::~IdGener()
{}

template <class T>
T IdGener<T>::GetOffset()
{
    return offset;
}

template <class T>
void IdGener<T>::SetOffset(T _offset)
{
    offset = _offset;
}

template <class T>
IdGener<T> &IdGener<T>::operator = (const IdGener<T> &_r)
{
    if ( &_r == this ) {
        return *this;
    }
    offset = _r.offset;
    return *this;
}

class MonotonicId64Gener: public IdGener<uint64_t>
{
    PlainLock     gener_lock;
    uint64_t      counter;

public:
    MonotonicId64Gener(uint64_t _offset = 0);
    virtual ~MonotonicId64Gener();

    virtual uint64_t GetId();
    MonotonicId64Gener &operator = (const MonotonicId64Gener &_r);
};

class RandomUniformInt64Gener: public IdGener<int64_t>
{
    PlainLock                   gener_lock;
    boost::uniform_int<>       *die;

    static std::random_device   rd;
    static boost::mt19937       rng;

public:
    RandomUniformInt64Gener(int64_t _offset, int _die_start, int _die_end);
    virtual ~RandomUniformInt64Gener();

    virtual int64_t GetId();
    RandomUniformInt64Gener &operator = (const RandomUniformInt64Gener &_r);
};

}

#endif /* RENTCONTROL_SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBIDGENER_HPP_ */
