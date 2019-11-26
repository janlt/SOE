/*
 * rcsdbidgener.cpp
 *
 *  Created on: Apr 19, 2018
 *      Author: Jan Lisowiec
 */

#include <sys/time.h>
#include <chrono>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/random.hpp>
#include <rcsdbidgener.hpp>

namespace RcsdbFacade {

MonotonicId64Gener::MonotonicId64Gener(uint64_t _offset)
    :IdGener<uint64_t>(_offset),
     counter(0)
{
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long mills = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    uint64_t tmp_offset = IdGener<uint64_t>::GetOffset() + static_cast<uint64_t>(mills);
    IdGener<uint64_t>::SetOffset(tmp_offset);
}

MonotonicId64Gener::~MonotonicId64Gener()
{}

uint64_t MonotonicId64Gener::GetId()
{
    WritePlainLock w_lock(gener_lock);
    counter++;
    uint64_t n_id = IdGener<uint64_t>::GetOffset() + counter;
    return n_id;
}

MonotonicId64Gener &MonotonicId64Gener::operator = (const MonotonicId64Gener &_r)
{
    if ( &_r == this ) {
        return *this;
    }
    IdGener<uint64_t>::operator = (_r);
    counter = _r.counter;
    return *this;
}

std::random_device RandomUniformInt64Gener::rd;
boost::mt19937     RandomUniformInt64Gener::rng(RandomUniformInt64Gener::rd());

RandomUniformInt64Gener::RandomUniformInt64Gener(int64_t _offset, int _die_start, int _die_end)
    :IdGener<int64_t>(_offset)
{
    die = new boost::uniform_int<>(_die_start,_die_end);
}

RandomUniformInt64Gener::~RandomUniformInt64Gener()
{
    delete die;
}

int64_t RandomUniformInt64Gener::GetId()
{
    WritePlainLock w_lock(gener_lock);
    boost::variate_generator<boost::mt19937&, boost::uniform_int<> > tmp_die(RandomUniformInt64Gener::rng, *die);

    int64_t rnd_id = tmp_die() + IdGener<int64_t>::GetOffset();
    return rnd_id;
}

}
