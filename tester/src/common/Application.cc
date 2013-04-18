#include "cgicc/HTTPHTMLHeader.h"
#include "cgicc/HTTPPlainHeader.h"
#include "rubuilder/utils/Constants.h"
#include "rubuilder/utils/TimerManager.h"
#include "rubuilder/tester/Application.h"
#include "rubuilder/tester/Constants.h"
#include "xcept/tools.h"
#include "xdaq/NamespaceURI.h"
#include "xdaq/exception/ApplicationNotFound.h"
#include "xgi/Method.h"
#include "xgi/Utils.h"
#include "xoap/domutils.h"
#include "xoap/MessageFactory.h"
#include "xoap/MessageReference.h"
#include "xoap/Method.h"
#include "xoap/SOAPBody.h"
#include "xoap/SOAPBodyElement.h"
#include "xoap/SOAPEnvelope.h"

#include <algorithm>
#include <cmath>
#include <stdlib.h>


rubuilder::tester::Application::Application(xdaq::ApplicationStub *s)
throw (xdaq::exception::Exception) :
xdaq::WebApplication(s),

//logger_(Logger::getInstance(generateLoggerName()))
logger_(getApplicationLogger())

{
    i2oAddressMap_   = i2o::utils::getAddressMap();
    poolFactory_     = toolbox::mem::getMemoryPoolFactory();
    appInfoSpace_    = getApplicationInfoSpace();
    appDescriptor_   = getApplicationDescriptor();
    appContext_      = getApplicationContext();
    xmlClass_        = appDescriptor_->getClassName();
    instance_        = appDescriptor_->getInstance();
    urn_             = appDescriptor_->getURN();
    atcpsAreEnabled_ = false;

    appDescriptor_->setAttribute("icon", APP_ICON);

    // Initialise the map from application url to application descriptor
    initAppUrlToDescriptor();

    initAppDescriptors(atcpDescriptors_, "pt::atcp::PeerTransportATCP");
    initAppDescriptors(buDescriptors_  , "rubuilder::bu::Application"       );
    initAppDescriptors(evmDescriptors_ , "rubuilder::evm::Application"      );
    initAppDescriptors(fuDescriptors_  , "rubuilder::fu::Application"       );
    initAppDescriptors(ruDescriptors_  , "rubuilder::ru::Application"       );
    initAppDescriptors(ruiDescriptors_ , "rubuilder::rui::Application"      );
    initAppDescriptors(taDescriptors_  , "rubuilder::ta::Application"       );

    testStarted_ = false;

    bindXgiCallbacks();

    // Emulate RCMS state listener
    xoap::bind
    (
        this,
        &rubuilder::tester::Application::sendNotificationReply,
        "sendNotificationReply",
        "http://replycommandreceiver.ws.fm.rcms"
    );

    // Initialise and group parameters
    try
    {
        stdConfigParams_  = initAndGetStdConfigParams();
        stdMonitorParams_ = initAndGetStdMonitorParams();
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(xdaq::exception::Exception,
            "Failed to initialise and group parameters", e);
    }

    try
    {
        putParamsIntoInfoSpace(stdConfigParams_ , appInfoSpace_);
        putParamsIntoInfoSpace(stdMonitorParams_, appInfoSpace_);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(xdaq::exception::Exception,
            "Failed to put parameters into info spaces", e);
    }

    LOG4CPLUS_INFO(logger_, "End of constructor");
}


void rubuilder::tester::Application::initAppDescriptors
(
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    > &descriptors,
    const std::string className
)
{
    std::set<xdaq::ApplicationDescriptor*> descriptorsFromFramework;


    try
    {
        descriptorsFromFramework =
            getApplicationContext()->
            getDefaultZone()->
            getApplicationDescriptors(className);
    }
    catch(xcept::Exception e)
    {
        descriptorsFromFramework.clear();
    }


    std::set<xdaq::ApplicationDescriptor*>::const_iterator itor;

    for
    (
        itor=descriptorsFromFramework.begin();
        itor!=descriptorsFromFramework.end();
        itor++
    )
    {
        descriptors.insert(*itor);
    }
}


void rubuilder::tester::Application::initAppUrlToDescriptor()
{
    // Clear the application URL to descriptor map
    appUrlToDescriptor_.clear();

    // Create a vector of the RU builder classes
    std::vector<std::string> classes;
    classes.push_back("rubuilder::bu::Application");
    classes.push_back("rubuilder::evm::Application");
    classes.push_back("rubuilder::ru::Application");

    std::vector<std::string>::const_iterator cItor;

    // For each RU builder class
    for(cItor=classes.begin(); cItor!=classes.end(); cItor++)
    {
        std::set<xdaq::ApplicationDescriptor*> descriptors;

        // Get the application descriptors
        descriptors =
            getApplicationContext()->
            getDefaultZone()->
            getApplicationDescriptors(*cItor);

        std::set<xdaq::ApplicationDescriptor*>::const_iterator dItor;

        // Add the descriptors to the application URL to descriptor map
        for(dItor=descriptors.begin(); dItor!=descriptors.end(); dItor++)
        {
            appUrlToDescriptor_[getUrl(*dItor)] = *dItor;
        }
    }
}


xoap::MessageReference rubuilder::tester::Application::sendNotificationReply
(
    xoap::MessageReference msg
)
throw (xoap::exception::Exception)
{
    std::string sourceURL = "";
    std::string stateName = "";
    std::string className = "";
    std::string instance  = "";

    try
    {
        sourceURL = extractSourceURLFromSendNotificationReply(msg);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(xoap::exception::Exception,
            "Failed to extract sourceURL from sendNotificationReply", e);
    }

    try
    {
        stateName = extractStateNameFromSendNotificationReply(msg);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(xoap::exception::Exception,
            "Failed to extract stateName from sendNotificationReply", e);
    }

    // Try to find the application descriptor of the source
    std::map<std::string, xdaq::ApplicationDescriptor*>::const_iterator itor;
    itor = appUrlToDescriptor_.find(sourceURL);

    // If the application descriptor has been found
    if(itor != appUrlToDescriptor_.end())
    {
        className = itor->second->getClassName();

        if(itor->second->hasInstanceNumber())
        {
            std::stringstream oss;
            oss << itor->second->getInstance();
            instance = oss.str();
        }
        else
        {
            instance = "-";
        }
    }
    else
    {
        className = "UNKNOWN";
        instance  = "-";
    }

    std::string stateChangeNotification = sourceURL + " " + className +
        instance + " " + stateName;

    // Keep a count of the number of state notifications received
    nbStateChangeNotifications_.value_++;

    // Update the state notifications window
    std::stringstream oss;
    oss << nbStateChangeNotifications_.value_ << ": ";
    oss << stateChangeNotification;
    stateChangeNotificationsWindow_.push_back(oss.str());

    if(stateChangeNotificationsWindow_.size() >
        stateChangeNotificationsWindowSize_.value_)
    {
        stateChangeNotificationsWindow_.pop_front();
    }

    // Log the state change notification
    LOG4CPLUS_INFO(logger_,
        "Recieved state change notification. " << stateChangeNotification);

    // Reply to the state change notification
    std::string responseString = "sendNotificationReplyResponse";

    try
    {
        xoap::MessageReference message = xoap::createMessage();
        xoap::SOAPEnvelope envelope = message->getSOAPPart().getEnvelope();
        xoap::SOAPBody body = envelope.getBody();
        xoap::SOAPName responseName =
            envelope.createName(responseString, "xdaq", XDAQ_NS_URI);

        body.addBodyElement(responseName);

        return message;
    }
    catch(xcept::Exception e)
    {
        std::stringstream oss;

        oss << "Failed to create " << responseString << " message";

        XCEPT_RETHROW(xoap::exception::Exception, oss.str(), e);
    }
}


std::string
rubuilder::tester::Application::extractSourceURLFromSendNotificationReply
(
    xoap::MessageReference msg
)
throw (xoap::exception::Exception)
{
    xoap::SOAPPart     soapPart        = msg->getSOAPPart();
    xoap::SOAPEnvelope envelope        = soapPart.getEnvelope();
    xoap::SOAPBody     body            = envelope.getBody();
    DOMNode            *bodyNode       = body.getDOMNode();
    DOMDocument        *document       = bodyNode->getOwnerDocument();
    DOMNodeList        *sourceUrlNodes =
        document->getElementsByTagNameNS(xoap::XStr("*"),
        xoap::XStr("sourceURL"));

    if(sourceUrlNodes->getLength() == 0)
    {
        XCEPT_RAISE(xoap::exception::Exception,
            "sourceUrl element not found");
    }

    DOMNode     *sourceUrlNode       = sourceUrlNodes->item(0);
    DOMNodeList *sourceUrlChildNodes = sourceUrlNode->getChildNodes();

    if(sourceUrlChildNodes->getLength() == 0)
    {
        XCEPT_RAISE(xoap::exception::Exception,
            "Value of sourceUrl not found");
    }

    DOMNode     *valueNode = sourceUrlChildNodes->item(0);
    std::string sourceUrl  = xoap::XMLCh2String(valueNode->getNodeValue());

    return sourceUrl;
}


std::string
rubuilder::tester::Application::extractStateNameFromSendNotificationReply
(
    xoap::MessageReference msg
)
throw (xoap::exception::Exception)
{
    xoap::SOAPPart     soapPart        = msg->getSOAPPart();
    xoap::SOAPEnvelope envelope        = soapPart.getEnvelope();
    xoap::SOAPBody     body            = envelope.getBody();
    DOMNode            *bodyNode       = body.getDOMNode();
    DOMDocument        *document       = bodyNode->getOwnerDocument();
    DOMNodeList        *stateNameNodes =
        document->getElementsByTagNameNS(xoap::XStr("*"),
        xoap::XStr("stateName"));

    if(stateNameNodes->getLength() == 0)
    {
        XCEPT_RAISE(xoap::exception::Exception,
            "stateName element not found");
    }

    DOMNode     *stateNameNode       = stateNameNodes->item(0);
    DOMNodeList *stateNameChildNodes = stateNameNode->getChildNodes();

    if(stateNameChildNodes->getLength() == 0)
    {
        XCEPT_RAISE(xoap::exception::Exception,
            "Value of stateName not found");
    }

    DOMNode     *valueNode = stateNameChildNodes->item(0);
    std::string stateName  = xoap::XMLCh2String(valueNode->getNodeValue());

    return stateName;
}


std::string
rubuilder::tester::Application::generateLoggerName()
{
    xdaq::ApplicationDescriptor *appDescriptor = getApplicationDescriptor();
    std::string                 appClass       = appDescriptor->getClassName();
    unsigned int                appInstance    = appDescriptor->getInstance();
    std::stringstream           oss;


    oss << appClass << appInstance;

    return oss.str();
}


void rubuilder::tester::Application::onException
(
    xcept::Exception &e
)
{
    std::string exceptionStr = xcept::stdformat_exception_history(e);


    // Keep a count of the number of exceptions received
    nbExceptions_.value_++;

    // Update the exceptions window
    std::stringstream oss;
    oss << nbExceptions_.value_ << ": " << exceptionStr;
    exceptionsWindow_.push_back(oss.str());

    if(exceptionsWindow_.size() > exceptionsWindowSize_.value_)
    {
        exceptionsWindow_.pop_front();
    }

    // Log the exception
    std::string s;
    s = "Received an exception from the sentinel: " + exceptionStr;
    LOG4CPLUS_INFO(logger_, s);
}


void rubuilder::tester::Application::bindXgiCallbacks()
{
    xgi::bind
    (
        this,
        &rubuilder::tester::Application::css,
        "styles.css"
    );

    xgi::bind
    (
        this,
        &rubuilder::tester::Application::defaultWebPage,
        "Default"
    );

    xgi::bind
    (
        this,
        &rubuilder::tester::Application::controlWebPage,
        "control"
    );

    xgi::bind
    (
        this,
        &rubuilder::tester::Application::messagesWebPage,
        "messages"
    );

    xgi::bind
    (
        this,
        &rubuilder::tester::Application::exceptionsWebPage,
        "exceptions"
    );

    xgi::bind
    (
        this,
        &rubuilder::tester::Application::stateChangeNotificationsWebPage,
        "stateChangeNotifications"
    );
}


void rubuilder::tester::Application::defaultWebPage
(
    xgi::Input *in,
    xgi::Output *out
)
throw (xgi::exception::Exception)
{
    out->getHTTPResponseHeader().addHeader("Content-Type", "text/html");

    *out << "<html>"                                              << std::endl;

    *out << "<head>"                                              << std::endl;
//  *out << "<meta http-equiv=\"Refresh\" content=\"1\">"         << std::endl;
    *out << "<link type=\"text/css\" rel=\"stylesheet\"";
    *out << " href=\"/" << urn_ << "/styles.css\"/>"              << std::endl;
    *out << "<title>"                                             << std::endl;
    *out << xmlClass_ << instance_ << " MAIN"                     << std::endl;
    *out << "</title>"                                            << std::endl;
    *out << "</head>"                                             << std::endl;

    *out << "<body>"                                              << std::endl;

    printWebPageTitleBar(out, rubuilder::tester::APP_ICON, "Main");


    try
    {
        printEVMTables(out, evmDescriptors_);
    }
    catch(xcept::Exception e)
    {
        XCEPT_RETHROW(xgi::exception::Exception,
            "Failed to print EVM tables", e);
    }

    *out << "<br/>"                                               << std::endl;

    try
    {
        printRUTables(out, ruDescriptors_);
    }
    catch(xcept::Exception e)
    {
        XCEPT_RETHROW(xgi::exception::Exception,
            "Failed to print RU tables", e);
    }

    *out << "<br/>"                                               << std::endl;

    try
    {
        printBUTables(out, buDescriptors_);
    }
    catch(xcept::Exception e)
    {
        XCEPT_RETHROW(xgi::exception::Exception,
            "Failed to print BU tables", e);
    }

    *out << "</body>"                                             << std::endl;

    *out << "</html>"                                             << std::endl;
}


void rubuilder::tester::Application::printAppInstanceLinks
(
    xgi::Output *out,
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    > &appDescriptors
)
{
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    >::const_iterator itor;


    for(itor=appDescriptors.begin(); itor!=appDescriptors.end(); itor++)
    {
        printAppLink(out, *itor);
    }
}


void rubuilder::tester::Application::printAppLink
(
    xgi::Output                 *out,
    xdaq::ApplicationDescriptor *appDescriptor
)
{
    *out << " <a href=\"" << getUrl(appDescriptor) << "\">";
    *out << appDescriptor->getInstance();
    *out << "</a>";
}


void rubuilder::tester::Application::printEVMTables
(
    xgi::Output *out,
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    > &evmDescriptors
)
throw (rubuilder::tester::exception::Exception)
{
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    >::const_iterator itor;


    *out << "<table border=\"0\">"                                << std::endl;
    *out << "<tr>"                                                << std::endl;

    for(itor=evmDescriptors.begin(); itor!=evmDescriptors.end(); itor++)
    {
        // Put space between EVM table
        if(itor != evmDescriptors.begin())
        {
            *out << "<td width=\"64\"></td>"                      << std::endl;
        }

        *out << "<td>";
        try
        {
            printEVMTable(out, *itor);
        }
        catch(xcept::Exception &e)
        {
            std::stringstream oss;

            oss << "Failed to print table for EVM";

            if((*itor)->hasInstanceNumber())
            {
                oss << (*itor)->getInstance();
            }
            else
            {
                oss << "-";
            }

            XCEPT_RETHROW(rubuilder::tester::exception::Exception, oss.str(), e);
        }
        *out << "</td>"                                           << std::endl;
    }

    *out << "</tr>"                                               << std::endl;
    *out << "</table>"                                            << std::endl;
}


void rubuilder::tester::Application::printEVMTable
(
    xgi::Output                 *out,
    xdaq::ApplicationDescriptor *evmDescriptor
)
throw (rubuilder::tester::exception::Exception)
{
    std::string eventNb;


    try
    {
        eventNb = getScalarParam(evmDescriptor, "lastEventNumberFromTrigger",
            "unsignedInt");
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(rubuilder::tester::exception::Exception,
            "Failed to get event number", e);
    }

    *out << "<table frame=\"void\" rules=\"rows\" class=\"params\">";
    *out << std::endl;

    *out << "<tr>"                                                << std::endl;
    *out << "  <th colspan=2 align=\"center\">"                   << std::endl;
    *out << "    <b>"                                             << std::endl;

    *out << "      <a href=\"" << getUrl(evmDescriptor) << "\">EVM";
    if(evmDescriptor->hasInstanceNumber())
    {
        *out << evmDescriptor->getInstance();
    }
    else
    {
        *out << "-";
    }
    *out << "</a>" << std::endl;

    *out << "    </b>"                                            << std::endl;
    *out << "  </th>"                                             << std::endl;
    *out << "</tr>"                                               << std::endl;

    *out << "<tr>"                                                << std::endl;
    *out << "  <td>"                                              << std::endl;
    *out << "    Event Nb."                                       << std::endl;
    *out << "  </td>"                                             << std::endl;
    *out << "  <td>"                                              << std::endl;
    *out << "    " << eventNb                                     << std::endl;
    *out << "  </td>"                                             << std::endl;
    *out << "</tr>"                                               << std::endl;

    *out << "</table>"                                            << std::endl;
}


void rubuilder::tester::Application::printRUTables
(
    xgi::Output *out,
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    > &ruDescriptors
)
throw (rubuilder::tester::exception::Exception)
{
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    >::const_iterator itor;


    *out << "<table border=\"0\">"                                << std::endl;
    *out << "<tr>"                                                << std::endl;

    for(itor=ruDescriptors.begin(); itor!=ruDescriptors.end(); itor++)
    {
        // Put space between each table
        if(itor != ruDescriptors.begin())
        {
            *out << "<td width=\"64\"></td>"                      << std::endl;
        }

        *out << "<td>";
        try
        {
            printRUTable(out, *itor);
        }
        catch(xcept::Exception &e)
        {
            std::stringstream oss;

            oss << "Failed to print table for RU";

            if((*itor)->hasInstanceNumber())
            {
                oss << (*itor)->getInstance();
            }
            else
            {
                oss << "-";
            }

            XCEPT_RETHROW(rubuilder::tester::exception::Exception, oss.str(), e);
        }
        *out << "</td>"                                           << std::endl;
    }

    *out << "</tr>"                                               << std::endl;
    *out << "</table>"                                            << std::endl;
}


void rubuilder::tester::Application::printRUTable
(
    xgi::Output                 *out,
    xdaq::ApplicationDescriptor *ruDescriptor
)
throw (rubuilder::tester::exception::Exception)
{
    std::string eventNb;


    try
    {
        eventNb = getScalarParam(ruDescriptor, "lastEventNumberFromEVM",
            "unsignedInt");
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(rubuilder::tester::exception::Exception,
            "Failed to get event number", e);
    }

    *out << "<table frame=\"void\" rules=\"rows\" class=\"params\">";
    *out << std::endl;

    *out << "<tr>"                                                << std::endl;
    *out << "  <th colspan=2 align=\"center\">"                   << std::endl;
    *out << "    <b>"                                             << std::endl;

    *out << "      <a href=\"" << getUrl(ruDescriptor) << "\">RU";
    if(ruDescriptor->hasInstanceNumber())
    {
        *out << ruDescriptor->getInstance();
    }
    else
    {
        *out << "-";
    }
    *out << "</a>" << std::endl;

    *out << "    </b>"                                            << std::endl;
    *out << "  </th>"                                             << std::endl;
    *out << "</tr>"                                               << std::endl;

    *out << "<tr>"                                                << std::endl;
    *out << "  <td>"                                              << std::endl;
    *out << "    Event Nb."                                       << std::endl;
    *out << "  </td>"                                             << std::endl;
    *out << "  <td>"                                              << std::endl;
    *out << "    " << eventNb                                     << std::endl;
    *out << "  </td>"                                             << std::endl;
    *out << "</tr>"                                               << std::endl;

    std::vector< std::pair<std::string, std::string> > stats =
        getStats(ruDescriptor);
    std::vector< std::pair<std::string, std::string> >::const_iterator itor;

    for(itor=stats.begin(); itor!=stats.end(); itor++)
    {
        *out << "<tr>"                                            << std::endl;
        *out << "  <td>"                                          << std::endl;
        *out << "    " << itor->first                             << std::endl;
        *out << "  </td>"                                         << std::endl;
        *out << "  <td>"                                          << std::endl;
        *out << "    " << itor->second                            << std::endl;
        *out << "  </td>"                                         << std::endl;
        *out << "</tr>"                                           << std::endl;
    }

    *out << "</table>"                                            << std::endl;
}


void rubuilder::tester::Application::printBUTables
(
    xgi::Output *out,
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    > &buDescriptors
)
throw (rubuilder::tester::exception::Exception)
{
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    >::const_iterator itor;


    *out << "<table border=\"0\">"                                << std::endl;
    *out << "<tr>"                                                << std::endl;

    for(itor=buDescriptors.begin(); itor!=buDescriptors.end(); itor++)
    {
        // Put space between each table
        if(itor != buDescriptors.begin())
        {
            *out << "<td width=\"64\"></td>"                      << std::endl;
        }

        *out << "<td>";
        try
        {
            printBUTable(out, *itor);
        }
        catch(xcept::Exception &e)
        {
            std::stringstream oss;

            oss << "Failed to print table for BU";

            if((*itor)->hasInstanceNumber())
            {
                oss << (*itor)->getInstance();
            }
            else
            {
                oss << "-";
            }

            XCEPT_RETHROW(rubuilder::tester::exception::Exception, oss.str(), e);
        }
        *out << "</td>"                                           << std::endl;
    }

    *out << "</tr>"                                               << std::endl;
    *out << "</table>"                                            << std::endl;
}


void rubuilder::tester::Application::printBUTable
(
    xgi::Output                 *out,
    xdaq::ApplicationDescriptor *buDescriptor
)
throw (rubuilder::tester::exception::Exception)
{
    std::string eventNb;


    try
    {
        eventNb = getScalarParam(buDescriptor, "lastEventNumberFromEVM",
            "unsignedInt");
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(rubuilder::tester::exception::Exception,
            "Failed to get event number", e);
    }

    *out << "<table frame=\"void\" rules=\"rows\" class=\"params\">";
    *out << std::endl;

    *out << "<tr>"                                                << std::endl;
    *out << "  <th colspan=2 align=\"center\">"                   << std::endl;
    *out << "    <b>"                                             << std::endl;

    *out << "      <a href=\"" << getUrl(buDescriptor) << "\">BU";
    if(buDescriptor->hasInstanceNumber())
    {
        *out << buDescriptor->getInstance();
    }
    else
    {
        *out << "-";
    }
    *out << "</a>" << std::endl;

    *out << "    </b>"                                            << std::endl;
    *out << "  </th>"                                             << std::endl;
    *out << "</tr>"                                               << std::endl;

    *out << "<tr>"                                                << std::endl;
    *out << "  <td>"                                              << std::endl;
    *out << "    Event Nb."                                       << std::endl;
    *out << "  </td>"                                             << std::endl;
    *out << "  <td>"                                              << std::endl;
    *out << "    " << eventNb                                     << std::endl;
    *out << "  </td>"                                             << std::endl;
    *out << "</tr>"                                               << std::endl;

    std::vector< std::pair<std::string, std::string> > stats =
        getStats(buDescriptor);
    std::vector< std::pair<std::string, std::string> >::const_iterator itor;

    for(itor=stats.begin(); itor!=stats.end(); itor++)
    {
        *out << "<tr>"                                            << std::endl;
        *out << "  <td>"                                          << std::endl;
        *out << "    " << itor->first                             << std::endl;
        *out << "  </td>"                                         << std::endl;
        *out << "  <td>"                                          << std::endl;
        *out << "    " << itor->second                            << std::endl;
        *out << "  </td>"                                         << std::endl;
        *out << "</tr>"                                           << std::endl;
    }

    *out << "</table>"                                            << std::endl;
}


std::vector< std::pair<std::string, std::string> >
rubuilder::tester::Application::getStats
(
    xdaq::ApplicationDescriptor *appDescriptor
)
{
    std::vector< std::pair<std::string, std::string> > stats;
    std::string  s                          = "";
    double       deltaT                     = 0.0;
    unsigned int deltaN                     = 0;
    double       deltaSumOfSquares          = 0.0;
    unsigned int deltaSumOfSizes            = 0;
    bool         retrievedDeltaT            = false;
    bool         retrievedDeltaN            = false;
    bool         retrievedDeltaSumOfSquares = false;
    bool         retrievedDeltaSumOfSizes   = false;


    try
    {
        s = getScalarParam(appDescriptor, "stateName", "string");
    }
    catch(xcept::Exception e)
    {
        s = "UNKNOWN";

        LOG4CPLUS_ERROR(logger_, "Failed to get state"
            << " : " << xcept::stdformat_exception_history(e));
    }
    stats.push_back
    (
        std::pair<std::string, std::string>("state", s)
    );

    try
    {
        s = getScalarParam(appDescriptor, "deltaT", "double");
        deltaT = atof(s.c_str());
        retrievedDeltaT = true;
    }
    catch(xcept::Exception e)
    {
        s = "UNKNOWN";
        retrievedDeltaT = false;

        LOG4CPLUS_ERROR(logger_, "Failed to get deltaT"
            << " : " << xcept::stdformat_exception_history(e));
    }
    stats.push_back
    (
        std::pair<std::string, std::string>("deltaT", s)
    );

    try
    {
        s = getScalarParam(appDescriptor, "deltaN", "unsignedInt");
        deltaN = atoi(s.c_str());
        retrievedDeltaN = true;
    }
    catch(xcept::Exception e)
    {
        s = "UNKNOWN";
        retrievedDeltaN = false;

        LOG4CPLUS_ERROR(logger_, "Failed to get deltaN"
            << " : " << xcept::stdformat_exception_history(e));
    }
    stats.push_back
    (
        std::pair<std::string, std::string>("deltaN", s)
    );


    try
    {
        s = getScalarParam(appDescriptor, "deltaSumOfSquares", "double");
        deltaSumOfSquares = atof(s.c_str());
        retrievedDeltaSumOfSquares = true;
    }
    catch(xcept::Exception e)
    {
        s = "UNKNOWN";
        retrievedDeltaSumOfSquares = false;

        LOG4CPLUS_ERROR(logger_, "Failed to get deltaSumOfSquares"
            << " : " << xcept::stdformat_exception_history(e));
    }
    stats.push_back
    (
        std::pair<std::string, std::string>("deltaSumOfSquares", s)
    );

    try
    {
        s = getScalarParam(appDescriptor, "deltaSumOfSizes", "unsignedInt");
        deltaSumOfSizes = atoi(s.c_str());
        retrievedDeltaSumOfSizes = true;
    }
    catch(xcept::Exception e)
    {
        s = "UNKNOWN";
        retrievedDeltaSumOfSizes = false;

        LOG4CPLUS_ERROR(logger_, "Failed to get deltaSumOfSizes"
            << " : " << xcept::stdformat_exception_history(e));
    }
    stats.push_back
    (
        std::pair<std::string, std::string>("deltaSumOfSizes", s)
    );

    if(retrievedDeltaSumOfSizes && retrievedDeltaT)
    {
        // Avoid divide by zero
        if(deltaT != 0.0)
        {
            double throughput = deltaSumOfSizes / deltaT / 1000000.0;

            std::stringstream oss;

            oss << throughput << " MB/s";

            stats.push_back
            (
                std::pair<std::string, std::string>("throughput", oss.str())
            );
        }
        else
        {
            stats.push_back
            (
                std::pair<std::string, std::string>("throughput", "DIV BY 0")
            );
        }
    }
    else
    {
        stats.push_back
        (
            std::pair<std::string, std::string>("throughput", "UNKNOWN")
        );
    }

    if(retrievedDeltaSumOfSizes && retrievedDeltaN)
    {
        // Avoid divide by zero
        if(deltaN != 0)
        {
            double average = deltaSumOfSizes / deltaN / 1000.0;

            std::stringstream oss;

            oss << average << " KB";

            stats.push_back
            (
                std::pair<std::string, std::string>("average", oss.str())
            );
        }
        else
        {
             stats.push_back
             (
                 std::pair<std::string, std::string>("average", "DIV BY 0")
             );
        }
    }
    else
    {
        stats.push_back
        (
            std::pair<std::string, std::string>("average", "UNKNOWN")
        );
    }

    if(retrievedDeltaN && retrievedDeltaT)
    {
        // Avoid divide by zero
        if(deltaT != 0.0)
        {
            double rate = ((double)deltaN) / deltaT / 1000.0;

            std::stringstream oss;

            oss << rate << " KHz";

            stats.push_back
            (
                std::pair<std::string, std::string>("rate", oss.str())
            );
        }
        else
        {
            stats.push_back
            (
                std::pair<std::string, std::string>("rate", "DIV BY 0")
            );
        }
    }
    else
    {
        stats.push_back
        (
            std::pair<std::string, std::string>("rate", "UNKNOWN")
        );
    }

    if(retrievedDeltaSumOfSquares && retrievedDeltaN &&
       retrievedDeltaSumOfSizes)
    {
        // Avoid divide by zero
        if(deltaN != 0)
        {
            double meanOfSquares = deltaSumOfSquares / ((double)deltaN);
            double mean          = ((double)deltaSumOfSizes) / ((double)deltaN);
            double squareOfMean  = mean * mean;
            double variance      = meanOfSquares - squareOfMean;

            // Variance maybe negative due to lack of precision
            if(variance < 0)
            {
                variance = 0.0;
            }

            double rms  = std::sqrt(variance) / 1000.0;

            std::stringstream oss;

            oss << rms << " KB";

            stats.push_back
            (
                std::pair<std::string, std::string>("rms", oss.str())
            );
        }
        else
        {
            stats.push_back
            (
                std::pair<std::string, std::string>("rms", "DIV BY 0")
            );
        }
    }
    else
    {
        stats.push_back
        (
            std::pair<std::string, std::string>("rms", "UNKNOWN")
        );
    }

    return stats;
}


void rubuilder::tester::Application::controlWebPage
(
    xgi::Input *in,
    xgi::Output *out
)
throw (xgi::exception::Exception)
{
    try
    {
        processControlForm(in);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(xgi::exception::Exception,
            "Failed to process control form", e);
    }

    out->getHTTPResponseHeader().addHeader("Content-Type", "text/html");

    *out << "<html>"                                              << std::endl;

    *out << "<head>"                                              << std::endl;
    *out << "<link type=\"text/css\" rel=\"stylesheet\"";
    *out << " href=\"/" << urn_ << "/styles.css\"/>"              << std::endl;
    *out << "<title>"                                             << std::endl;
    *out << xmlClass_ << instance_ << " CONTROL"                  << std::endl;
    *out << "</title>"                                            << std::endl;
    *out << "</head>"                                             << std::endl;

    *out << "<body>"                                              << std::endl;

    printWebPageTitleBar(out, rubuilder::tester::CTRL_ICON, "Main");

    *out << "<form method=\"get\" action=\"/" << urn_ << "/control\">";
    *out << std::endl;

    *out << "<input"                                              << std::endl;
    *out << " type=\"submit\""                                    << std::endl;
    *out << " name=\"command\""                                   << std::endl;

    if(testStarted_)
    {
        *out << " value=\"stop\""                                 << std::endl;
    }
    else
    {
        *out << " value=\"start\""                                << std::endl;
    }

    *out << "/>"                                                  << std::endl;

    *out << "</form>"                                             << std::endl;
    *out << "</body>"                                             << std::endl;

    *out << "</html>"                                             << std::endl;
}


void rubuilder::tester::Application::processControlForm
(
    xgi::Input *in
)
throw (xgi::exception::Exception)
{
    cgicc::Cgicc         cgi(in);
    cgicc::form_iterator cmdElement = cgi.getElement("command");
    std::string          cmdName    = "";


    // If their is a command from the html form
    if(cmdElement != cgi.getElements().end())
    {
        cmdName = (*cmdElement).getValue();

        if((cmdName == "start") && (!testStarted_))
        {
            try
            {
                startTest();
                testStarted_ = true;
            }
            catch(xcept::Exception e)
            {
                XCEPT_RETHROW(xgi::exception::Exception,
                    "Failed to start test", e);
            }
        }
        else if((cmdName == "stop") && testStarted_)
        {
            try
            {
                stopTest();
                testStarted_ = false;
            }
            catch(xcept::Exception e)
            {
                XCEPT_RETHROW(xgi::exception::Exception,
                    "Failed to stop test", e);
            }
        }
    }
}


std::string rubuilder::tester::Application::getUrl
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


void rubuilder::tester::Application::messagesWebPage
(
    xgi::Input *in,
    xgi::Output *out
)
throw (xgi::exception::Exception)
{
    out->getHTTPResponseHeader().addHeader("Content-Type", "text/html");

    *out << "<html>"                                              << std::endl;

    *out << "<head>"                                              << std::endl;
    *out << "<link type=\"text/css\" rel=\"stylesheet\"";
    *out << " href=\"/" << urn_ << "/styles.css\"/>"              << std::endl;
    *out << "<title>"                                             << std::endl;
    *out << xmlClass_ << instance_ <<  " MESSAGES"                << std::endl;
    *out << "</title>"                                            << std::endl;
    *out << "</head>"                                             << std::endl;

    *out << "<body>"                                              << std::endl;

    printWebPageTitleBar(out, rubuilder::tester::MSGS_ICON, "Messages");

    *out << "<p>"                                                 << std::endl;
    *out << "<table frame=\"void\" class=\"params\" width=\"100%\">";
    *out << std::endl;
    *out << "<tr><th>Exception messages</th></tr>"                << std::endl;
    *out << "</table>"                                            << std::endl;
    *out << "<iframe";
    *out << " src=\"/" << urn_ << "/exceptions\"";
    *out << " height=\"30%\"";
    *out << " width=\"100%\"";
    *out << ">"                                                   << std::endl;
    *out << "Your browser does not support inline frames<br/>"    << std::endl;
    *out << "<a href=\"/" << urn_ << "/exceptions\">"             << std::endl;
    *out << "  Frame contents"                                    << std::endl;
    *out << "</a>"                                                << std::endl;
    *out << "</iframe>"                                           << std::endl;
    *out << "</p>"                                                << std::endl;

    *out << "<p>"                                                 << std::endl;
    *out << "<table frame=\"void\" class=\"params\" width=\"100%\">";
    *out << std::endl;
    *out << "<tr><th>State change notification messages</th></tr>";
    *out << std::endl;
    *out << "</table>"                                            << std::endl;
    *out << "<iframe";
    *out << " src=\"/" << urn_ << "/stateChangeNotifications\"";
    *out << " height=\"30%\"";
    *out << " width=\"100%\"";
    *out << ">"                                                   << std::endl;
    *out << "Your browser does not support inline frames"         << std::endl;
    *out << "<a href=\"/" << urn_ << "/stateChangeNotifications\">";
    *out << std::endl;
    *out << "  Frame contents"                                    << std::endl;
    *out << "</a>"                                                << std::endl;
    *out << "</iframe>"                                           << std::endl;
    *out << "</p>"                                                << std::endl;

    *out << "</body>"                                             << std::endl;
    *out << "</html>"                                             << std::endl;
}


void rubuilder::tester::Application::exceptionsWebPage
(
    xgi::Input  *in,
    xgi::Output *out
)
throw (xgi::exception::Exception)
{
    out->getHTTPResponseHeader().addHeader("Content-Type", "text/plain");

    std::list<std::string>::const_iterator itor;

    for
    (
        itor=exceptionsWindow_.begin();
        itor!=exceptionsWindow_.end();
        itor++
    )
    {
        *out << (*itor) << "\n";
    }
}


void rubuilder::tester::Application::stateChangeNotificationsWebPage
(
    xgi::Input *in,
    xgi::Output *out
)
throw (xgi::exception::Exception)
{
    out->getHTTPResponseHeader().addHeader("Content-Type", "text/plain");

    std::list<std::string>::const_iterator itor;

    for
    (
        itor=stateChangeNotificationsWindow_.begin();
        itor!=stateChangeNotificationsWindow_.end();
        itor++
    )
    {
        *out << (*itor) << "\n";
    }
}


void rubuilder::tester::Application::startTest()
throw (rubuilder::tester::exception::Exception)
{
    bool atcpIsBeingUsed            = atcpDescriptors_.size() > 0;
    bool evmGenerateDummyTriggers   = taDescriptors_.size()  == 0;
    bool rusGenerateDummySuperFrags = ruiDescriptors_.size() == 0;
    bool busDropEvents              = fuDescriptors_.size()  == 0;


    try
    {
        checkThereIsARuBuilder();
    }
    catch(xcept::Exception e)
    {
        XCEPT_RETHROW(rubuilder::tester::exception::Exception,
            "Not enough applications to make a RU builder", e);
    }

    if(atcpIsBeingUsed)
    {
        if(!atcpsAreEnabled_)
        {
            try
            {
                configureAndEnableAtcps();
                atcpsAreEnabled_ = true;
            }
            catch(xcept::Exception e)
            {
                XCEPT_RETHROW(rubuilder::tester::exception::Exception,
                    "Failed to configure and enable atcps", e);
            }
        }
    }

    try
    {
        setEVMGenerateDummyTriggers(evmGenerateDummyTriggers);
    }
    catch(xcept::Exception e)
    {
        XCEPT_RETHROW(rubuilder::tester::exception::Exception,
            "Failed to tell EVM whether or not to generate dummy triggers", e);
    }

    try
    {
        setRUsGenerateDummySuperFrags(rusGenerateDummySuperFrags);
    }
    catch(xcept::Exception e)
    {
        XCEPT_RETHROW(rubuilder::tester::exception::Exception,
         "Failed to tell RUs whether or not to generate dummy super-fragments",
         e);
    }

    try
    {
        setBUsDropEvents(busDropEvents);
    }
    catch(xcept::Exception e)
    {
        XCEPT_RETHROW(rubuilder::tester::exception::Exception,
            "Failed to tell BUs whether or not drop events", e);
    }

    // If the TA is present then start it as an imaginary trigger
    if(taDescriptors_.size() > 0)
    {
        try
        {
            startTrigger();
        }
        catch(xcept::Exception e)
        {
            XCEPT_RETHROW(rubuilder::tester::exception::Exception,
                "Failed to start trigger", e);
        }
    }

    try
    {
        startRuBuilder();
    }
    catch(xcept::Exception e)
    {
        XCEPT_RETHROW(rubuilder::tester::exception::Exception,
            "Failed to start RU builder", e);
    }

    // If RUIs are present then start them as an imaginary FED builder
    if(ruiDescriptors_.size() > 0)
    {
        try
        {
            startFedBuilder();
        }
        catch(xcept::Exception e)
        {
            XCEPT_RETHROW(rubuilder::tester::exception::Exception,
                "Failed to start FED builder", e);
        }
    }

    // If FUs are present then start them as an imafinary filter farm
    if(fuDescriptors_.size() > 0)
    {
        try
        {
            startFilterFarm();
        }
        catch(xcept::Exception e)
        {
            XCEPT_RETHROW(rubuilder::tester::exception::Exception,
                "Failed to start filter farm", e);
        }
    }
}


void rubuilder::tester::Application::checkThereIsARuBuilder()
throw (rubuilder::tester::exception::Exception)
{
    if(buDescriptors_.size() < 1)
    {
        XCEPT_RAISE(rubuilder::tester::exception::Exception, "No BUs");
    }

    if(ruDescriptors_.size() < 1)
    {
        LOG4CPLUS_WARN(logger_, "There are no RUs");
    }
}


void rubuilder::tester::Application::configureAndEnableAtcps()
throw (rubuilder::tester::exception::Exception)
{
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    >::const_iterator itor;


    for(itor=atcpDescriptors_.begin(); itor!=atcpDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Configure", *itor);
    }
    for(itor=atcpDescriptors_.begin(); itor!=atcpDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Enable"   , *itor);
    }
}


void rubuilder::tester::Application::setEVMGenerateDummyTriggers
(
    const bool value
)
throw (rubuilder::tester::exception::Exception)
{
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    >::const_iterator itor;


    for(itor=evmDescriptors_.begin(); itor!=evmDescriptors_.end(); itor++)
    {
        xdaq::ApplicationDescriptor *appDescriptor = *itor;

        try
        {
            if (value)
            {
                setScalarParam(appDescriptor, "triggerSource", "string", "Local");
                setScalarParam(appDescriptor, "generateDummyTriggers",
                    "boolean", "true");
            }
        }
        catch(xcept::Exception e)
        {
            std::stringstream oss;

            oss << "Failed to set generateDummyTriggers of ";
            oss << appDescriptor->getClassName();
            if(appDescriptor->hasInstanceNumber())
            {
                oss << appDescriptor->getInstance();
            }
            else
            {
                oss << "-";
            }
            oss << " to true";

            XCEPT_RETHROW(rubuilder::tester::exception::Exception, oss.str(), e);
        }
    }
}


void rubuilder::tester::Application::setRUsGenerateDummySuperFrags
(
    const bool value
)
throw (rubuilder::tester::exception::Exception)
{
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    >::const_iterator itor;


    for(itor = ruDescriptors_.begin(); itor != ruDescriptors_.end(); itor++)
    {
        xdaq::ApplicationDescriptor *appDescriptor = *itor;

        try
        {
            if (value)
            {
                setScalarParam(appDescriptor, "inputSource", "string", "Local");
                setScalarParam(appDescriptor, "generateDummySuperFragments",
                "boolean", "true");
            }
        }
        catch(xcept::Exception e)
        {
            std::stringstream oss;

            oss << "Failed to set generateDummySuperFragments of ";
            oss << appDescriptor->getClassName();
            oss << appDescriptor->getInstance();
            oss << " to true";

            XCEPT_RETHROW(rubuilder::tester::exception::Exception, oss.str(), e);
        }
    }
}


void rubuilder::tester::Application::setBUsDropEvents
(
    const bool value
)
throw (rubuilder::tester::exception::Exception)
{
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    >::const_iterator itor;


    for(itor = buDescriptors_.begin(); itor != buDescriptors_.end(); itor++)
    {
        xdaq::ApplicationDescriptor *appDescriptor = *itor;

        try
        {
            if (value)
            {
                setScalarParam(appDescriptor, "dropEventData", "boolean", "true");
            }
        }
        catch(xcept::Exception e)
        {
            std::stringstream oss;

            oss << "Failed to set dropEventData of ";
            oss << appDescriptor->getClassName();
            oss << appDescriptor->getInstance();
            oss << " to true";

            XCEPT_RETHROW(rubuilder::tester::exception::Exception, oss.str(), e);
        }
    }
}


void rubuilder::tester::Application::startTrigger()
throw (rubuilder::tester::exception::Exception)
{
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    >::const_iterator itor;


    //////////////////////////////
    // Configure and enable TAs //
    //////////////////////////////

    for(itor=taDescriptors_.begin(); itor!=taDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Configure", *itor);
    }

    for(itor=taDescriptors_.begin(); itor!=taDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Enable", *itor);
    }
}


void rubuilder::tester::Application::startRuBuilder()
throw (rubuilder::tester::exception::Exception)
{
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    >::const_iterator itor;


    ////////////////////
    // Configure EVMs //
    ////////////////////

    for(itor=evmDescriptors_.begin(); itor!=evmDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Configure", *itor);
    }


    ///////////////////
    // Configure BUs //
    ///////////////////

    for(itor=buDescriptors_.begin(); itor!= buDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Configure", *itor);
    }


    ///////////////////
    // Configure RUs //
    ///////////////////

    for(itor=ruDescriptors_.begin(); itor!=ruDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Configure", *itor);
    }

    ::sleep(1);

    ////////////////
    // Enable RUs //
    ////////////////

    for(itor=ruDescriptors_.begin(); itor!= ruDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Enable", *itor);
    }


    /////////////////
    // Enable EVMs //
    /////////////////

    for(itor=evmDescriptors_.begin(); itor!=evmDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Enable", *itor);
    }


    ////////////////
    // Enable BUs //
    ////////////////

    for(itor=buDescriptors_.begin(); itor!=buDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Enable", *itor);
    }
}


void rubuilder::tester::Application::startFedBuilder()
throw (rubuilder::tester::exception::Exception)
{
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    >::const_iterator itor;


    ////////////////////
    // Configure RUIs //
    ////////////////////

    for(itor=ruiDescriptors_.begin(); itor!=ruiDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Configure", *itor);
    }

    ::sleep(1);

    /////////////////
    // Enable RUIs //
    /////////////////

    for(itor=ruiDescriptors_.begin(); itor!=ruiDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Enable", *itor);
    }
}


void rubuilder::tester::Application::startFilterFarm()
throw (rubuilder::tester::exception::Exception)
{
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    >::const_iterator itor;


    ///////////////////
    // Configure FUs //
    ///////////////////

    for(itor=fuDescriptors_.begin(); itor!=fuDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Configure", *itor);
    }


    ////////////////
    // Enable FUs //
    ////////////////

    for(itor=fuDescriptors_.begin(); itor!=fuDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Enable", *itor);
    }
}


void rubuilder::tester::Application::stopTest()
throw (rubuilder::tester::exception::Exception)
{
    // If imaginary FED builder was started
    if(ruiDescriptors_.size() > 0)
    {
        try
        {
            stopFedBuilder();
        }
        catch(xcept::Exception e)
        {
            XCEPT_RETHROW(rubuilder::tester::exception::Exception,
                "Failed to stop FED builder", e);
        }
    }

    try
    {
        stopRuBuilder();
    }
    catch(xcept::Exception e)
    {
        XCEPT_RETHROW(rubuilder::tester::exception::Exception,
            "Failed to stop RU builder", e);
    }

    // If imaginary trigger was started
    if(taDescriptors_.size() > 0)
    {
        try
        {
            stopTrigger();
        }
        catch(xcept::Exception e)
        {
            XCEPT_RETHROW(rubuilder::tester::exception::Exception,
                "Failed to stop trigger", e);
        }
    }

    // If imaginary filter farm was started
    if(fuDescriptors_.size() > 0)
    {
        try
        {
            stopFilterFarm();
        }
        catch(xcept::Exception e)
        {
            XCEPT_RETHROW(rubuilder::tester::exception::Exception,
                "Failed to stop filter farm", e);
        }
    }
}


void rubuilder::tester::Application::stopRuBuilder()
throw (rubuilder::tester::exception::Exception)
{
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    >::const_iterator itor;


    ///////////////
    // Halt EVMs //
    ///////////////

    for(itor=evmDescriptors_.begin(); itor!=evmDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Halt", *itor);
    }


    //////////////
    // Halt BUs //
    //////////////

    for(itor=buDescriptors_.begin(); itor!=buDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Halt", *itor);
    }


    //////////////
    // Halt RUs //
    //////////////

    for(itor=ruDescriptors_.begin(); itor!=ruDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Halt", *itor);
    }
}


void rubuilder::tester::Application::stopFedBuilder()
throw (rubuilder::tester::exception::Exception)
{
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    >::const_iterator itor;


    ///////////////
    // Halt RUIs //
    ///////////////

    for(itor=ruiDescriptors_.begin(); itor!=ruiDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Halt", *itor);
    }
}


void rubuilder::tester::Application::stopTrigger()
throw (rubuilder::tester::exception::Exception)
{
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    >::const_iterator itor;


    //////////////
    // Halt TAs //
    //////////////

    for(itor=taDescriptors_.begin(); itor!=taDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Halt", *itor);
    }
}


void rubuilder::tester::Application::stopFilterFarm()
throw (rubuilder::tester::exception::Exception)
{
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    >::const_iterator itor;


    ///////////////
    // Halt RUIs //
    ///////////////

    for(itor=fuDescriptors_.begin(); itor!=fuDescriptors_.end(); itor++)
    {
        sendFSMEventToApp("Halt", *itor);
    }
}


void rubuilder::tester::Application::sendFSMEventToApp
(
    const std::string            eventName,
    xdaq::ApplicationDescriptor* destAppDescriptor
)
throw (rubuilder::tester::exception::Exception)
{
    try
    {
        xoap::MessageReference msg = createSimpleSOAPCmdMsg(eventName);
        xoap::MessageReference reply =
            appContext_->postSOAP(msg, *appDescriptor_, *destAppDescriptor);

        // Check if the reply indicates a fault occurred
        xoap::SOAPBody replyBody =
            reply->getSOAPPart().getEnvelope().getBody();

        if(replyBody.hasFault())
        {
            std::stringstream oss;

            oss << "Received fault reply from ";
            oss << destAppDescriptor->getClassName();
            oss << destAppDescriptor->getInstance();
            oss << " : " << replyBody.getFault().getFaultString();

            XCEPT_RAISE(rubuilder::tester::exception::Exception, oss.str());
        }
    }
    catch(xcept::Exception e)
    {
        std::stringstream oss;

        oss << "Failed to send FSM event \"" << eventName << "\" to ";
        oss << destAppDescriptor->getClassName();
        oss << destAppDescriptor->getInstance();
        oss << " and receive a valid reply";

        XCEPT_RETHROW(rubuilder::tester::exception::Exception, oss.str(), e);
    }
}


xoap::MessageReference
rubuilder::tester::Application::createSimpleSOAPCmdMsg
(
    const std::string cmdName
)
throw (rubuilder::tester::exception::Exception)
{
    try
    {
        xoap::MessageReference message = xoap::createMessage();
        xoap::SOAPPart soapPart = message->getSOAPPart();
        xoap::SOAPEnvelope envelope = soapPart.getEnvelope();
        xoap::SOAPBody body = envelope.getBody();
        xoap::SOAPName cmdSOAPName =
            envelope.createName(cmdName, "xdaq", "urn:xdaq-soap:3.0");

        body.addBodyElement(cmdSOAPName);

        return message;
    }
    catch(xcept::Exception e)
    {
        XCEPT_RETHROW(rubuilder::tester::exception::Exception,
            "Failed to create simple SOAP command message for cmdName " +
            cmdName, e);
    }
}


std::string rubuilder::tester::Application::getScalarParam
(
    xdaq::ApplicationDescriptor* destAppDescriptor,
    const std::string            paramName,
    const std::string            paramType
)
throw (rubuilder::tester::exception::Exception)
{
    std::string appClass = destAppDescriptor->getClassName();
    std::string value    = "";


    try
    {
        xoap::MessageReference msg =
            createParameterGetSOAPMsg(appClass, paramName, paramType);

        xoap::MessageReference reply =
            appContext_->postSOAP(msg, *appDescriptor_, *destAppDescriptor);

        // Check if the reply indicates a fault occurred
        xoap::SOAPBody replyBody =
            reply->getSOAPPart().getEnvelope().getBody();

        if(replyBody.hasFault())
        {
            std::stringstream oss;

            oss << "Received fault reply: ";
            oss << replyBody.getFault().getFaultString();

            XCEPT_RAISE(rubuilder::tester::exception::Exception, oss.str());
        }

        value = extractScalarParameterValueFromSoapMsg(reply, paramName);
    }
    catch(xcept::Exception e)
    {
        std::string s = "Failed to get scalar parameter from application";

        XCEPT_RETHROW(rubuilder::tester::exception::Exception, s, e);
    }

    return value;
}


void rubuilder::tester::Application::setScalarParam
(
    xdaq::ApplicationDescriptor* destAppDescriptor,
    const std::string            paramName,
    const std::string            paramType,
    const std::string            paramValue
)
throw (rubuilder::tester::exception::Exception)
{
    std::string appClass = destAppDescriptor->getClassName();


    try
    {
        xoap::MessageReference msg = createParameterSetSOAPMsg(appClass,
                                     paramName, paramType, paramValue);

        xoap::MessageReference reply =
            appContext_->postSOAP(msg, *appDescriptor_, *destAppDescriptor);

        // Check if the reply indicates a fault occurred
        xoap::SOAPBody replyBody =
            reply->getSOAPPart().getEnvelope().getBody();

        if(replyBody.hasFault())
        {
            std::stringstream oss;

            oss << "Received fault reply: ";
            oss << replyBody.getFault().getFaultString();

            XCEPT_RAISE(rubuilder::tester::exception::Exception, oss.str());
        }
    }
    catch(xcept::Exception e)
    {
        std::string s = "Failed to set scalar parameter";

        XCEPT_RETHROW(rubuilder::tester::exception::Exception, s, e);
    }
}


xoap::MessageReference
rubuilder::tester::Application::createParameterGetSOAPMsg
(
    const std::string appClass,
    const std::string paramName,
    const std::string paramType
)
throw (rubuilder::tester::exception::Exception)
{
    std::string appNamespace = "urn:xdaq-application:" + appClass;
    std::string paramXsdType = "xsd:" + paramType;

    try
    {
        xoap::MessageReference message = xoap::createMessage();
        xoap::SOAPPart soapPart = message->getSOAPPart();
        xoap::SOAPEnvelope envelope = soapPart.getEnvelope();
        envelope.addNamespaceDeclaration("xsi",
            "http://www.w3.org/2001/XMLSchema-instance");
        envelope.addNamespaceDeclaration("xsd",
            "http://www.w3.org/2001/XMLSchema");
        envelope.addNamespaceDeclaration("soapenc",
            "http://schemas.xmlsoap.org/soap/encoding/");
        xoap::SOAPBody body = envelope.getBody();
        xoap::SOAPName cmdName =
            envelope.createName("ParameterGet", "xdaq", "urn:xdaq-soap:3.0");
        xoap::SOAPBodyElement cmdElement =
            body.addBodyElement(cmdName);
        xoap::SOAPName propertiesName =
            envelope.createName("properties", "p", appNamespace);
        xoap::SOAPElement propertiesElement =
            cmdElement.addChildElement(propertiesName);
        xoap::SOAPName propertiesTypeName =
            envelope.createName("type", "xsi",
             "http://www.w3.org/2001/XMLSchema-instance");
        propertiesElement.addAttribute(propertiesTypeName, "soapenc:Struct");
        xoap::SOAPName propertyName =
            envelope.createName(paramName, "p", appNamespace);
        xoap::SOAPElement propertyElement =
            propertiesElement.addChildElement(propertyName);
        xoap::SOAPName propertyTypeName =
             envelope.createName("type", "xsi",
             "http://www.w3.org/2001/XMLSchema-instance");

        propertyElement.addAttribute(propertyTypeName, paramXsdType);

        return message;
    }
    catch(xcept::Exception e)
    {
        XCEPT_RETHROW(rubuilder::tester::exception::Exception,
            "Failed to create ParameterGet SOAP message for parameter " +
            paramName + " of type " + paramType, e);
    }
}


xoap::MessageReference
rubuilder::tester::Application::createParameterSetSOAPMsg
(
    const std::string appClass,
    const std::string paramName,
    const std::string paramType,
    const std::string paramValue
)
throw (rubuilder::tester::exception::Exception)
{
    std::string appNamespace = "urn:xdaq-application:" + appClass;
    std::string paramXsdType = "xsd:" + paramType;

    try
    {
        xoap::MessageReference message = xoap::createMessage();
        xoap::SOAPPart soapPart = message->getSOAPPart();
        xoap::SOAPEnvelope envelope = soapPart.getEnvelope();
        envelope.addNamespaceDeclaration("xsi",
            "http://www.w3.org/2001/XMLSchema-instance");
        envelope.addNamespaceDeclaration("xsd",
            "http://www.w3.org/2001/XMLSchema");
        envelope.addNamespaceDeclaration("soapenc",
            "http://schemas.xmlsoap.org/soap/encoding/");
        xoap::SOAPBody body = envelope.getBody();
        xoap::SOAPName cmdName =
            envelope.createName("ParameterSet", "xdaq", "urn:xdaq-soap:3.0");
        xoap::SOAPBodyElement cmdElement =
            body.addBodyElement(cmdName);
        xoap::SOAPName propertiesName =
            envelope.createName("properties", "p", appNamespace);
        xoap::SOAPElement propertiesElement =
            cmdElement.addChildElement(propertiesName);
        xoap::SOAPName propertiesTypeName =
            envelope.createName("type", "xsi",
             "http://www.w3.org/2001/XMLSchema-instance");
        propertiesElement.addAttribute(propertiesTypeName, "soapenc:Struct");
        xoap::SOAPName propertyName =
            envelope.createName(paramName, "p", appNamespace);
        xoap::SOAPElement propertyElement =
            propertiesElement.addChildElement(propertyName);
        xoap::SOAPName propertyTypeName =
             envelope.createName("type", "xsi",
             "http://www.w3.org/2001/XMLSchema-instance");

        propertyElement.addAttribute(propertyTypeName, paramXsdType);

        propertyElement.addTextNode(paramValue);

        return message;
    }
    catch(xcept::Exception e)
    {
        XCEPT_RETHROW(rubuilder::tester::exception::Exception,
            "Failed to create ParameterSet SOAP message for parameter " +
            paramName + " of type " + paramType + " with value " + paramValue,
            e);
    }
}


std::string
rubuilder::tester::Application::extractScalarParameterValueFromSoapMsg
(
    xoap::MessageReference msg,
    const std::string      paramName
)
throw (rubuilder::tester::exception::Exception)
{
    try
    {
        xoap::SOAPPart part = msg->getSOAPPart();
        xoap::SOAPEnvelope env = part.getEnvelope();
        xoap::SOAPBody body = env.getBody();
        DOMNode *bodyNode = body.getDOMNode();
        DOMNodeList *bodyList = bodyNode->getChildNodes();
        DOMNode *responseNode = findNode(bodyList, "ParameterGetResponse");
        DOMNodeList *responseList = responseNode->getChildNodes();
        DOMNode *propertiesNode = findNode(responseList, "properties");
        DOMNodeList *propertiesList = propertiesNode->getChildNodes();
        DOMNode *paramNode = findNode(propertiesList, paramName);
        DOMNodeList *paramList = paramNode->getChildNodes();
        DOMNode *valueNode = paramList->item(0);
        std::string paramValue = xoap::XMLCh2String(valueNode->getNodeValue());

        return paramValue;
    }
    catch(xcept::Exception e)
    {
        XCEPT_RETHROW(rubuilder::tester::exception::Exception,
            "Parameter " + paramName + " not found", e);
    }
    catch(...)
    {
        XCEPT_RAISE(rubuilder::tester::exception::Exception,
            "Parameter " + paramName + " not found");
    }
}


DOMNode *rubuilder::tester::Application::findNode
(
    DOMNodeList       *nodeList,
    const std::string nodeLocalName
)
throw (rubuilder::tester::exception::Exception)
{
    DOMNode      *node = 0;
    std::string  name  = "";
    unsigned int i     = 0;


    for(i=0; i<nodeList->getLength(); i++)
    {
        node = nodeList->item(i);

        if(node->getNodeType() == DOMNode::ELEMENT_NODE)
        {
            name = xoap::XMLCh2String(node->getLocalName());

            if(name == nodeLocalName)
            {
                return node;
            }
        }
    }

    XCEPT_RAISE(rubuilder::tester::exception::Exception,
        "Failed to find node with local name: " + nodeLocalName);
}


std::vector< std::pair<std::string, xdata::Serializable*> >
rubuilder::tester::Application::initAndGetStdConfigParams()
throw (rubuilder::tester::exception::Exception)
{
    std::vector< std::pair<std::string, xdata::Serializable*> > params;


    exceptionsWindowSize_         = 100;
    stateChangeNotificationsWindowSize_ = 100;

    params.push_back(std::pair<std::string,xdata::Serializable*>
        ("exceptionsWindowSize", &exceptionsWindowSize_));
    params.push_back(std::pair<std::string,xdata::Serializable*>
        ("stateChangeNotificationsWindowSize",
        &stateChangeNotificationsWindowSize_));

    return params;
}


std::vector< std::pair<std::string, xdata::Serializable*> >
rubuilder::tester::Application::initAndGetStdMonitorParams()
throw (rubuilder::tester::exception::Exception)
{
    std::vector< std::pair<std::string, xdata::Serializable*> > params;


    stateName_                  = "Enabled";
    nbExceptions_               = 0;
    nbStateChangeNotifications_ = 0;

    params.push_back(std::pair<std::string,xdata::Serializable*>
        ("stateName", &stateName_));
    params.push_back(std::pair<std::string,xdata::Serializable*>
        ("nbExceptions", &nbExceptions_));
    params.push_back(std::pair<std::string,xdata::Serializable*>
        ("nbStateChangeNotifications", &nbStateChangeNotifications_));

    return params;
}


void rubuilder::tester::Application::putParamsIntoInfoSpace
(
    std::vector< std::pair<std::string, xdata::Serializable*> > &params,
    xdata::InfoSpace                                            *s
)
throw (rubuilder::tester::exception::Exception)
{
    std::vector
    <
         std::pair<std::string, xdata::Serializable*>
    >::const_iterator itor;


    for(itor=params.begin(); itor!=params.end(); itor++)
    {
        try
        {
            s->fireItemAvailable(itor->first, itor->second);
        }
        catch(xcept::Exception &e)
        {
            std::stringstream oss;

            oss << "Failed to put " << itor->first;
            oss << " into infospace " << s->name();

            XCEPT_RETHROW(rubuilder::tester::exception::Exception, oss.str(), e);
        }
    }
}


void rubuilder::tester::Application::printWebPageTitleBar
(
    xgi::Output       *out,
    const std::string pageIconSrc,
    const std::string pageIconAlt
)
throw (xgi::exception::Exception)
{
    *out << "<table border=\"0\" width=\"100%\">"                 << std::endl;
    *out << "<tr>"                                                << std::endl;
    *out << "  <td width=\"64\">"                                 << std::endl;
    *out << "    <img"                                            << std::endl;
    *out << "     align=\"middle\""                               << std::endl;
    *out << "     src=\"" << pageIconSrc << "\""                  << std::endl;
    *out << "     alt=\"" << pageIconAlt << "\""                  << std::endl;
    *out << "     width=\"64\""                                   << std::endl;
    *out << "     height=\"64\""                                  << std::endl;
    *out << "     border=\"\"/>"                                  << std::endl;
    *out << "  </td>"                                             << std::endl;
    *out << "  <td align=\"left\">"                               << std::endl;
    *out << "    <b>" << xmlClass_ << instance_ << " " << stateName_.toString();
    *out << "</b><br/>"                                           << std::endl;
    *out << "    <a href=\"/" << urn_ << "/ParameterQuery\">XML</a>";
    *out << "<br/>"                                               << std::endl;
    *out << "    Page last updated: ";
    *out << rubuilder::utils::getCurrentTimeUTC() << " UTC"       << std::endl;
    *out << "  </td>"                                             << std::endl;

    *out << "  <td class=\"app_links\">"                          << std::endl;
    *out << "    TA ";
    printAppInstanceLinks(out, taDescriptors_);
    *out << "<br/>"                                               << std::endl;
    *out << "    EVM ";
    printAppInstanceLinks(out, evmDescriptors_);
    *out << "<br/>"                                               << std::endl;
    *out << "    RUI ";
    printAppInstanceLinks(out, ruiDescriptors_);
    *out << "<br/>"                                               << std::endl;
    *out << "    RU ";
    printAppInstanceLinks(out, ruDescriptors_);
    *out << "<br/>"                                               << std::endl;
    *out << "    BU ";
    printAppInstanceLinks(out, buDescriptors_);
    *out << "<br/>"                                               << std::endl;
    *out << "    FU ";
    printAppInstanceLinks(out, fuDescriptors_);
    *out                                                          << std::endl;
    *out << "  </td>"                                             << std::endl;

    *out << "  <td class=\"app_links\" align=\"center\" width=\"70\">";
    *out << std::endl;
    printWebPageIcon(out, rubuilder::utils::HYPERDAQ_ICON, "HyperDAQ",
        "/urn:xdaq-application:service=hyperdaq");
    *out << "  </td>"                                             << std::endl;

    *out << "  <td width=\"32\">"                                 << std::endl;
    *out << "  </td>"                                             << std::endl;
    *out << "  <td class=\"app_links\" align=\"center\" width=\"64\">";
    *out << std::endl;
    printWebPageIcon(out, rubuilder::tester::CTRL_ICON, "Control",
        "/" + urn_ + "/control");
    *out << "  </td>"                                             << std::endl;

    *out << "  <td width=\"32\">"                                 << std::endl;
    *out << "  </td>"                                             << std::endl;
    *out << "  <td class=\"app_links\" align=\"center\" width=\"64\">";
    *out << std::endl;
    printWebPageIcon(out, rubuilder::tester::MSGS_ICON, "Messages",
        "/" + urn_ + "/messages");
    *out << "  </td>"                                             << std::endl;

    *out << "  <td width=\"32\">"                                 << std::endl;
    *out << "  </td>"                                             << std::endl;
    *out << "  <td class=\"app_links\" align=\"center\" width=\"64\">";
    *out << std::endl;
    printWebPageIcon(out, rubuilder::tester::APP_ICON, "Main", "/" + urn_);
    *out << "  </td>"                                             << std::endl;

    *out << "</tr>"                                               << std::endl;
    *out << "</table>"                                            << std::endl;

    *out << "<hr/>"                                               << std::endl;
}


void rubuilder::tester::Application::printWebPageIcon
(
    xgi::Output       *out,
    const std::string imgSrc,
    const std::string label,
    const std::string href
)
throw (xgi::exception::Exception)
{
    *out << "<a href=\"" << href << "\">"                         << std::endl;
    *out << "  <img"                                              << std::endl;
    *out << "    src=\"" << imgSrc << "\""                        << std::endl;
    *out << "    alt=\"" << label << "\""                         << std::endl;
    *out << "    width=\"64\""                                    << std::endl;
    *out << "    height=\"64\""                                   << std::endl;
    *out << "    border=\"\"/>"                                   << std::endl;
    *out << "</a><br/>"                                           << std::endl;
    *out << "<a href=\"" << href << "\">"                         << std::endl;
    *out << "  " << label                                         << std::endl;
    *out << "</a>"                                                << std::endl;
}


/**
 * Provides the factory method for the instantiation of RUBuilderTester
 * applications.
 */
XDAQ_INSTANTIATOR_IMPL(rubuilder::tester::Application)
