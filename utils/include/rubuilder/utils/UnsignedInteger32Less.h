#ifndef _rubuilder_utils_UnsignedInteger32Less_h_
#define _rubuilder_utils_UnsignedInteger32Less_h_

#include "xdata/UnsignedInteger32.h"


namespace rubuilder { namespace utils { // namespace rubuilder::utils

/**
 * Functor that can be used with the STL in order to determine if one
 * xdata::UnsignedInteger32 is less than another.
 */
class UnsignedInteger32Less
{
public:

    /**
     * Returns true if ul1 is less than ul2.
     */
    bool operator()
    (
        const xdata::UnsignedInteger32 &ul1,
        const xdata::UnsignedInteger32 &ul2
    )
    const;
};

} } // namespace rubuilder::utils

#endif
