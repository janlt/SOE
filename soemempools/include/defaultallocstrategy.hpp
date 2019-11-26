/*
 * defaultallocstrategy.hpp   (C) Copyright Jan Lisowiec, 2003-2017
 *
 */

/*
 * defaultallocstrategy.hpp
 *
 *  Created on: Dec 21, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MEMPOOLS_INCLUDE_DEFAULTALLOCSTRATEGY_HPP_
#define SOE_SOE_SOE_MEMPOOLS_INCLUDE_DEFAULTALLOCSTRATEGY_HPP_


#include "allocstrategy.hpp"

namespace MemMgmt {
class DefaultAllocStrategy : public AllocStrategy
{
public:
    DefaultAllocStrategy();
    virtual ~DefaultAllocStrategy();
    virtual AllocStrategy * copy () const;
    virtual unsigned int preallocateCount () const;
    virtual unsigned int incrementQuant () const;
    virtual unsigned int decrementQuant () const;
    virtual unsigned int minIncQuant () const;
    virtual unsigned int maxIncQuant () const;
    virtual unsigned int minDecQuant () const;
    virtual unsigned int maxDecQuant () const;

private:
    DefaultAllocStrategy(const DefaultAllocStrategy &right);
    DefaultAllocStrategy & operator=(const DefaultAllocStrategy &right);
};
} // namespace MemMgmt


#endif /* SOE_SOE_SOE_MEMPOOLS_INCLUDE_DEFAULTALLOCSTRATEGY_HPP_ */
