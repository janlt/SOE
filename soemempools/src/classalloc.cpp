/*
 * classalloc.cpp   (C) Copyright Jan Lisowiec, 2003-2017
 *
 */

/*
 * classalloc.cpp
 *
 *  Created on: Dec 21, 2016
 *      Author: Jan Lisowiec
 */

#include <stdlib.h>
#include "classalloc.hpp"

void *operator new(size_t size_, size_t inc_)
{
#ifdef _MM_DEBUG_
#ifndef _MM_GMP_ON_
    std::cout << "::new(size_t size_, size_t inc_) "
    << " size_: " << size_
    << " inc_: " << inc_
    << endl << flush;
#else
    fprintf(stderr, "::new(size_t size_, size_t inc_) size_: %d inc_: %d\n", size_, inc_);
    fflush(stderr);
#endif
#endif

#ifndef _MM_GMP_ON_
    return ::new char[size_ + inc_];
#else
    return (char *)::malloc(size_ + inc_) + inc_;
#endif
}

namespace MemMgmt {
} // namespace MemMgmt
