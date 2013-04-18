#ifndef _rubuilder_utils_ApplicationDescriptorAndTid_h_
#define _rubuilder_utils_ApplicationDescriptorAndTid_h_

#include "i2o/i2oDdmLib.h"
#include "xdaq/ApplicationDescriptor.h"


namespace rubuilder { namespace utils { // namespace rubuilder::utils

/**
 * Keeps an application's descriptor together with its I2O TID.
 *
 * The main use of this structure is to reduce the number of calls to
 * i2o::utils::AddressMap.
 */
struct ApplicationDescriptorAndTid
{
    xdaq::ApplicationDescriptor *descriptor;
    I2O_TID                     tid;

    /**
     * Constrcutor which initialises both the application descriptor and
     * I2O_TID to 0.
     */
    ApplicationDescriptorAndTid()
    {
        descriptor = 0;
        tid        = 0;
    }

    friend bool operator<(
        const ApplicationDescriptorAndTid& a,
        const ApplicationDescriptorAndTid& b
    )
    {
        if ( a.descriptor != b.descriptor ) return ( a.descriptor < b.descriptor );
        return ( a.tid < b.tid );
    }
};

} } // namespace rubuilder::utils

#endif
