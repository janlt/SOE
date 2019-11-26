/*
 * classalloc.hpp   (C) Copyright Jan Lisowiec, 2003-2017
 *
 */

/*
 * classalloc.hpp
 *
 *  Created on: Dec 21, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MEMPOOLS_INCLUDE_CLASSALLOC_HPP_
#define SOE_SOE_SOE_MEMPOOLS_INCLUDE_CLASSALLOC_HPP_


#include <cstddef>
#include <iostream>
#include <typeinfo>
using namespace std;

void *operator new (size_t siz_, size_t inc_);

namespace MemMgmt {
template <class Z>
class ClassAlloc
{
public:
    static Z * New (unsigned int padd_);
    static void Delete (Z *obj_);
    static unsigned int objectSize ();
    static const char* getName ();

private:
    ClassAlloc(const ClassAlloc< Z > &right);
    ClassAlloc< Z > & operator=(const ClassAlloc< Z > &right);
};

template <class Z>
inline Z * ClassAlloc<Z>::New (unsigned int padd_)
{
#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    cout << " ClassAlloc<Z>::New, padd_: " << padd_
    << " sizeof(Z): " << sizeof(Z)
    << endl << flush;
#else
    fprintf(stderr, " ClassAlloc<Z>::New, padd_: %d sizeof(Z): %d\n", padd_, sizeof(Z));
    fflush(stderr);
#endif
#endif

    return ::new(padd_) Z;
}

template <class Z>
inline void ClassAlloc<Z>::Delete (Z *obj_)
{}

template <class Z>
inline unsigned int ClassAlloc<Z>::objectSize ()
{
    return (unsigned int )sizeof(Z);
}

template <class Z>
inline const char* ClassAlloc<Z>::getName ()
{
    static const type_info &ti = typeid(Z);

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


#endif /* SOE_SOE_SOE_MEMPOOLS_INCLUDE_CLASSALLOC_HPP_ */
