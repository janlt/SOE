/*
 * defaultstrategy.cpp   (C) Copyright Jan Lisowiec, 2003-2017
 *
 */

/*
 * defaultstrategy.cpp
 *
 *  Created on: Dec 21, 2016
 *      Author: Jan Lisowiec
 */

#include "defaultallocstrategy.hpp"

namespace MemMgmt
{
DefaultAllocStrategy::DefaultAllocStrategy()
{
}

DefaultAllocStrategy::DefaultAllocStrategy(const DefaultAllocStrategy &right)
{
}

DefaultAllocStrategy::~DefaultAllocStrategy()
{
}

AllocStrategy * DefaultAllocStrategy::copy() const
{
    return new DefaultAllocStrategy(*this);
}

unsigned int DefaultAllocStrategy::preallocateCount() const
{
    return 0;
}

unsigned int DefaultAllocStrategy::incrementQuant() const
{
    return 320;
}

unsigned int DefaultAllocStrategy::decrementQuant() const
{
    return 0;
}

unsigned int DefaultAllocStrategy::minIncQuant() const
{
    return 4;
}

unsigned int DefaultAllocStrategy::maxIncQuant() const
{
    return 10;
}

unsigned int DefaultAllocStrategy::minDecQuant() const
{
    return 0;
}

unsigned int DefaultAllocStrategy::maxDecQuant() const
{
    return 0;
}
} // namespace MemMgmt
