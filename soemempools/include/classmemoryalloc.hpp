/*
 * classmemoryalloc.hpp   (C) Copyright Jan Lisowiec, 2003-2017
 *
 */

/*
 * classmemoryalloc.hpp
 *
 *  Created on: Dec 21, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MEMPOOLS_INCLUDE_CLASSMEMORYALLOC_HPP_
#define SOE_SOE_SOE_MEMPOOLS_INCLUDE_CLASSMEMORYALLOC_HPP_


#include <cstddef>
#include <iostream>
#include <typeinfo>
using namespace std;
#include <stdlib.h>

namespace MemMgmt {
template <class W>
class ClassMemoryAlloc
{
public:
    static void * New (unsigned int padd_);
    static void Delete (void *obj_);
    static unsigned int objectSize ();
    static const char* getName ();

private:
    ClassMemoryAlloc(const ClassMemoryAlloc< W > &right);
    ClassMemoryAlloc< W > & operator=(const ClassMemoryAlloc< W > &right);
};

template <class W>
inline void * ClassMemoryAlloc<W>::New (unsigned int padd_)
{
#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    cout << " ClassMemoryAlloc<W>::New, padd_: " << padd_
    << " sizeof(W): " << sizeof(W)
    << endl << flush;
#else
    fprintf(stderr, " ClassMemoryAlloc<W>::New, padd_: %d sizeof(W): %d\n", padd_, sizeof(W));
    fflush(stderr);
#endif
#endif

#ifndef _MM_GMP_ON_
    return ::new char[padd_ + sizeof(W)];
#else
    return (char *)::malloc(padd_ + sizeof(W)) + padd_;
#endif
}

template <class W>
inline void ClassMemoryAlloc<W>::Delete (void *obj_)
{}

template <class W>
inline unsigned int ClassMemoryAlloc<W>::objectSize ()
{
    return (unsigned int )sizeof(W);
}

template <class W>
inline const char* ClassMemoryAlloc<W>::getName ()
{
    static const type_info &ti = typeid(W);

#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    cout << "NAME: " << ti.name() << endl << flush;
#else
    fprintf(stderr, "NAME: %s\n", ti.name());
#endif
#endif

    return ti.name();
}
} // namespace MemMgmt


#endif /* SOE_SOE_SOE_MEMPOOLS_INCLUDE_CLASSMEMORYALLOC_HPP_ */
