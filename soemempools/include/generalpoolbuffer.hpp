/*
 * generalpoolbuffer.hpp   (C) Copyright Jan Lisowiec, 2003-2017
 *
 */

/*
 * generalpoolbuffer.hpp
 *
 *  Created on: Dec 21, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MEMPOOLS_INCLUDE_GENERALPOOLBUFFER_HPP_
#define SOE_SOE_SOE_MEMPOOLS_INCLUDE_GENERALPOOLBUFFER_HPP_


#include <iostream>
using namespace std;
#include <stdlib.h>
#include <string.h>

namespace MemMgmt {
const unsigned int MAX_BUFFER_SLOTS=1024;
const unsigned int OBJ_PADDING = sizeof(unsigned int);

const unsigned int BUCKET_SIZE_MASK = 0x00FFFFFF;
const unsigned int BUCKET_NUMBER_SHIFT = 24;
const unsigned int BUCKET_NUMBER_MASK = 0x000000FF;

template <class T, class Alloc>
class GeneralPoolBuffer
{
public:
    GeneralPoolBuffer (unsigned int bucketNum_);
    virtual ~GeneralPoolBuffer();
    static void * operator new(size_t size);
    static void operator delete(void *object);
    void rellocate (const GeneralPoolBuffer<T, Alloc> *buf_, unsigned int howMany_);
    void zeroOut (unsigned int howMany_);

    T *slots[MAX_BUFFER_SLOTS];
    unsigned int bucketNum;

private:
    GeneralPoolBuffer(const GeneralPoolBuffer< T,Alloc > &right);
    GeneralPoolBuffer< T,Alloc > & operator=(const GeneralPoolBuffer< T,Alloc > &right);
};

template <class T, class Alloc>
inline GeneralPoolBuffer<T,Alloc>::GeneralPoolBuffer (unsigned int bucketNum_)
    :bucketNum(bucketNum_)
{
    memset((void *)&slots[0], '\0', (size_t )(MAX_BUFFER_SLOTS*sizeof(T*)));
}

template <class T, class Alloc>
inline GeneralPoolBuffer<T,Alloc>::~GeneralPoolBuffer()
{}

template <class T, class Alloc>
inline void * GeneralPoolBuffer<T,Alloc>::operator new(size_t size)
{
#ifndef _MM_GMP_ON_
    return malloc(size);
#else
    return malloc(size);
#endif
}

template <class T, class Alloc>
inline void GeneralPoolBuffer<T,Alloc>::operator delete(void *object)
{
#ifndef _MM_GMP_ON_
    free(object);
#else
    free(object);
#endif
}

template <class T, class Alloc>
inline void GeneralPoolBuffer<T,Alloc>::rellocate (const GeneralPoolBuffer<T, Alloc> *buf_, unsigned int howMany_)
{
    memcpy((void *)&slots[0], (void *)&buf_->slots[0], (size_t )(howMany_*sizeof(T*)));
}

template <class T, class Alloc>
inline void GeneralPoolBuffer<T,Alloc>::zeroOut (unsigned int howMany_)
{
    memset((void *)slots, '\0', (size_t )(howMany_*sizeof(T*)));
}

} // namespace MemMgmt


#endif /* SOE_SOE_SOE_MEMPOOLS_INCLUDE_GENERALPOOLBUFFER_HPP_ */
