#ifndef _rubuilder_utils_XoapUtils_h_
#define _rubuilder_utils_XoapUtils_h_

#include "xoap/MessageReference.h"

#include <string>

namespace rubuilder { namespace utils { // namespace rubuilder::utils

/**
 * Creates a SOAP response message for the specified FSM event and result
 * state.
 */
xoap::MessageReference createFsmSoapResponseMsg
(
    const std::string event,
    const std::string state
);


void printSoapMsgToStdOut
(
    xoap::MessageReference message
);

} } // namespace rubuilder::utils

#endif
