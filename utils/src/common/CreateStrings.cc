#include "rubuilder/utils/CreateStrings.h"

#include <iostream>
#include <sstream>

std::string rubuilder::utils::generateFastControlMsgPoolName
(
    const std::string  appClassname,
    const unsigned int appInstance
)
{
    std::stringstream oss;


    oss << appClassname << appInstance << " fast control message pool";

    return oss.str();
}


std::string rubuilder::utils::generateWorkLoopName
(
    const std::string  appClassname,
    const unsigned int appInstance
)
{
    std::stringstream oss;


    oss << appClassname << appInstance << " work loop";

    return oss.str();
}


std::string rubuilder::utils::generateWorkLoopActionName
(
    const std::string  appClassname,
    const unsigned int appInstance
)
{
    std::stringstream oss;


    oss << appClassname << appInstance << " work loop action";

    return oss.str();
}



std::string rubuilder::utils::generateMonitoringWorkLoopName
(
    const std::string  appClass,
    const unsigned int appInstance
)
{
    std::stringstream oss;


    oss << appClass << appInstance << " monitoring work loop";

    return oss.str();
}


std::string rubuilder::utils::generateMonitoringActionName
(
    const std::string  appClass,
    const unsigned int appInstance
)
{
    std::stringstream oss;


    oss << appClass << appInstance << " monitoring action";

    return oss.str();
}


std::string rubuilder::utils::generateMonitoringInfoSpaceName
(
    const std::string  appClass,
    const unsigned int appInstance
)
{
    std::stringstream oss;

    oss << appClass;

    return oss.str();
}


std::string rubuilder::utils::generateOldMsgSenderWorkLoopName
(
    const std::string   appClass,
    const unsigned int  appInstance
)
{
    std::stringstream oss;

    oss << appClass << appInstance << " old message sender work loop";

    return oss.str();
}


std::string rubuilder::utils::generateOldMsgSenderActionName
(
    const std::string   appClass,
    const unsigned int  appInstance
)
{
    std::stringstream oss;

    oss << appClass << appInstance << " old message sender action";

    return oss.str();
}


std::string rubuilder::utils::getUrl
(
    xdaq::ApplicationDescriptor *appDescriptor
)
{
    std::string url;


    url  = appDescriptor->getContextDescriptor()->getURL();
    url += "/";
    url += appDescriptor->getURN();

    return url;
}


std::string rubuilder::utils::getIdentifier
(
    xdaq::ApplicationDescriptor *appDesc
)
{
    std::ostringstream identifier;
    identifier << appDesc->getClassName() << appDesc->getInstance() << "/";
    return identifier.str();
}


std::string rubuilder::utils::createI2oErrorMsg
(
    xdaq::ApplicationDescriptor *source,
    xdaq::ApplicationDescriptor *destination
)
{
    std::stringstream oss;


    oss << "I2O exception from ";
    oss << source->getClassName();
    oss << " instance ";
    oss << source->getInstance();
    oss << " to ";
    oss << destination->getClassName();
    oss << " instance ";
    oss << destination->getInstance();

    return oss.str();
}
