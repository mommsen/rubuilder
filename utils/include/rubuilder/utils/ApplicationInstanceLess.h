#ifndef _rubuilder_utils_ApplicationInstanceLess_h_
#define _rubuilder_utils_ApplicationInstanceLess_h_

#include "xdaq/ApplicationDescriptor.h"


namespace rubuilder { namespace utils { // namespace rubuilder::utils

/**
 * Functor that can be used with the STL in order to determine if one
 * application descriptor is less than another based on the instance number
 * of the applications concerned.
 */
class ApplicationInstanceLess
{
public:

    /**
     * Returns true if the instance number of the application described by 
     * application descriptor a1 is less than that of a2.
     *
     * If one or both of the applications do not have an instance number, then
     * this method returns false.
     */
    bool operator()
    (
        xdaq::ApplicationDescriptor *a1,
        xdaq::ApplicationDescriptor *a2
    ) const;
};

} } // namespace rubuilder::utils

#endif
