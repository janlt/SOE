/*
 * globalmemoryalloc.hpp   (C) Copyright Jan Lisowiec, 2003-2017
 *
 */

/*
 * globalmemoryalloc.hpp
 *
 *  Created on: Dec 21, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MEMPOOLS_INCLUDE_GLOBALMEMORYALLOC_HPP_
#define SOE_SOE_SOE_MEMPOOLS_INCLUDE_GLOBALMEMORYALLOC_HPP_


#include <cstddef>
#include <iostream>
#include <typeinfo>
using namespace std;
#include <stdlib.h>

namespace MemMgmt {
template <class Y>
class GlobalMemoryAlloc
{
public:
    virtual ~GlobalMemoryAlloc();
    static void * New (unsigned int padd_);
    static void Delete (void *obj_);
    static unsigned int objectSize ();
    static const char* getName ();

protected:
    GlobalMemoryAlloc(const GlobalMemoryAlloc< Y > &right);
    GlobalMemoryAlloc< Y > & operator=(const GlobalMemoryAlloc< Y > &right);
};

template <class Y>
inline GlobalMemoryAlloc<Y>::~GlobalMemoryAlloc()
{}

template <class Y>
inline void * GlobalMemoryAlloc<Y>::New (unsigned int padd_)
{
#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    cout << " GlobalMemoryAlloc<Y>::New, padd_: " << padd_
    << " sizeof(Y): " << sizeof(Y)
    << endl << flush;
#else
    fprintf(stderr, " GlobalMemoryAlloc<Y>::New, padd_: %d sizeof(Y): %d\n", padd_, sizeof(Y));
    fflush(stderr);
#endif
#endif

#ifndef _MM_GMP_ON_
    return ::new char[padd_ + sizeof(Y)];
#else
    return (char *)::malloc(padd_ + sizeof(Y)) + padd_;
#endif
}

template <class Y>
inline void GlobalMemoryAlloc<Y>::Delete (void *obj_)
{}

template <class Y>
inline unsigned int GlobalMemoryAlloc<Y>::objectSize ()
{
    return (unsigned int )sizeof(Y);
}

template <class Y>
inline const char* GlobalMemoryAlloc<Y>::getName ()
{
    static const type_info &ti = typeid(Y);

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


#endif /* SOE_SOE_SOE_MEMPOOLS_INCLUDE_GLOBALMEMORYALLOC_HPP_ */
