/*
 * allocstrategy.hpp   (C) Copyright Jan Lisowiec, 2003-2017
 *
 */

/*
 * allocstrategy.hpp
 *
 *  Created on: Dec 21, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MEMPOOLS_INCLUDE_ALLOCSTRATEGY_HPP_
#define SOE_SOE_SOE_MEMPOOLS_INCLUDE_ALLOCSTRATEGY_HPP_


namespace MemMgmt {
class AllocStrategy
{
public:
    AllocStrategy();
    virtual ~AllocStrategy();
    virtual AllocStrategy * copy() const = 0;
    virtual unsigned int preallocateCount() const = 0;
    virtual unsigned int incrementQuant() const = 0;
    virtual unsigned int decrementQuant() const = 0;
    virtual unsigned int minIncQuant() const = 0;
    virtual unsigned int maxIncQuant() const = 0;
    virtual unsigned int minDecQuant() const = 0;
    virtual unsigned int maxDecQuant() const = 0;

private:
    AllocStrategy(const AllocStrategy &right);
    AllocStrategy & operator=(const AllocStrategy &right);
};
} // namespace MemMgmt


#endif /* SOE_SOE_SOE_MEMPOOLS_INCLUDE_ALLOCSTRATEGY_HPP_ */
