#ifndef _rubuilder_utils_CreateStrings_h_
#define _rubuilder_utils_CreateStrings_h_

#include <string>

#include "xdaq/ApplicationDescriptor.h"

namespace rubuilder { namespace utils { // namespace rubuilder::utils

/**
 * Returns the name of the memory pool from which fast-control messages
 * shall be allocated.
 */
std::string generateFastControlMsgPoolName
(
    const std::string   appClassname,
    const unsigned int  appInstance
);

/**
 * Returns the name of work loop executing the application's
 * "self-driven" behaviour.
 */
std::string generateWorkLoopName
(
    const std::string   appClassname,
    const unsigned int  appInstance
);

/**
 * Returns the name of work loop action implementing the application's
 * "self-driven" behaviour.
 */
std::string generateWorkLoopActionName
(
    const std::string   appClassname,
    const unsigned int  appInstance
);

/**
 * Returns the name of the work loop that executes the action of updating
 * monitoring information.
 */
std::string generateMonitoringWorkLoopName
(
    const std::string   appClass,
    const unsigned int  appInstance
);

/**
 * Returns the name of the action that implements the updating of
 * monitioring information.
 */
std::string generateMonitoringActionName
(
    const std::string   appClass,
    const unsigned int  appInstance
);

/**
 * Returns the name of the info space that contains exported parameters
 * for monitoring.
 */
std::string generateMonitoringInfoSpaceName
(
    const std::string   appClass,
    const unsigned int  appInstance
);

/**
 * Returns the name of the work loop that executes the sending of old
 * fast-control messages.
 */
std::string generateOldMsgSenderWorkLoopName
(
    const std::string   appClass,
    const unsigned int  appInstance
);

/**
 * Returns the name of the action that implements the sending of old
 * fast-control messages.
 */
std::string generateOldMsgSenderActionName
(
    const std::string   appClass,
    const unsigned int  appInstance
);

/**
 * Returns the url of the specified application.
 */
std::string getUrl(xdaq::ApplicationDescriptor *appDescriptor);

/**
 * Returns the formatted identifier of the specified application.
 */
std::string getIdentifier(xdaq::ApplicationDescriptor*);

/**
 * Creates and returns the error message of an I2O exception by specifying
 * the source and destination involved.
 */
std::string createI2oErrorMsg
(
    xdaq::ApplicationDescriptor *source,
    xdaq::ApplicationDescriptor *destination
);

} } // namespace rubuilder::utils

#endif
