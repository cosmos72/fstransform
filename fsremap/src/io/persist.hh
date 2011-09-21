/*
 * io/persist.hh
 *
 *  Created on: Mar 6, 2011
 *      Author: max
 */

#ifndef FSREMAP_PERSIST_HH
#define FSREMAP_PERSIST_HH

#include "../types.hh"   // for ft_uoff

FT_IO_NAMESPACE_BEGIN

class ft_persist
{
private:
    /** cannot call copy constructor */
    ft_persist(const ft_persist &);

    /** cannot call assignment operator */
    const ft_persist & operator=(const ft_persist &);

public:
    /** default constructor */
    ft_persist();

    /** destructor */
    ~ft_persist();
};


FT_IO_NAMESPACE_END

#endif /* FSREMAP_PERSIST_HH */
