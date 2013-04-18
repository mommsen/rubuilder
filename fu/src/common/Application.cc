#include "cgicc/HTTPHTMLHeader.h"
#include "cgicc/HTTPPlainHeader.h"
#include "i2o/Method.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/frl_header.h"
#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "rubuilder/fu/Application.h"
#include "rubuilder/fu/Constants.h"
#include "rubuilder/fu/ForceFailedEvent.h"
#include "rubuilder/fu/version.h"
#include "rubuilder/utils/CRC16.h"
#include "rubuilder/utils/XoapUtils.h"
#include "toolbox/utils.h"
#include "toolbox/fsm/FailedEvent.h"
#include "toolbox/mem/HeapAllocator.h"
#include "xcept/tools.h"
#include "xdaq/NamespaceURI.h"
#include "xdaq/exception/ApplicationNotFound.h"
#include "xdata/Double.h"
#include "xdata/InfoSpaceFactory.h"
#include "xgi/Method.h"
#include "xoap/domutils.h"
#include "xoap/MessageFactory.h"
#include "xoap/MessageReference.h"
#include "xoap/Method.h"
#include "xoap/SOAPBody.h"
#include "xoap/SOAPBodyElement.h"
#include "xoap/SOAPEnvelope.h"

#include <sstream>
#include <unistd.h>


rubuilder::fu::Application::Application(xdaq::ApplicationStub *s)
throw (xdaq::exception::Exception) :
xdaq::WebApplication(s),

//logger_(Logger::getInstance(generateLoggerName())),
logger_(getApplicationLogger()),
soapParameterExtractor_(this),

applicationBSem_(toolbox::BSem::FULL)
{
    tid_               = 0;
    i2oAddressMap_     = i2o::utils::getAddressMap();
    poolFactory_       = toolbox::mem::getMemoryPoolFactory();
    appInfoSpace_      = getApplicationInfoSpace();
    appDescriptor_     = getApplicationDescriptor();
    appContext_        = getApplicationContext();
    xmlClass_          = appDescriptor_->getClassName();
    instance_          = appDescriptor_->getInstance();
    urn_               = appDescriptor_->getURN();
    superFragmentHead_ = 0;
    superFragmentTail_ = 0;
    blockNb_           = 0;
    faultDetected_     = false;

    appDescriptor_->setAttribute("icon", APP_ICON);

    // Note that rubuilderTesterDescriptor_ will be zero if the
    // RUBuilderTester application is not found
    rubuilderTesterDescriptor_ = getRUBuilderTester();

    i2oExceptionHandler_ = toolbox::exception::bind
    (
        this,
        &rubuilder::fu::Application::onI2oException,
        "onI2oException"
    );

    i2oPoolName_ = createI2oPoolName(instance_);

    try
    {
        i2oPool_ = createHeapAllocatorMemoryPool(poolFactory_, i2oPoolName_);
    }
    catch(xcept::Exception &e)
    {
        std::string s;

        s = "Failed to create " + i2oPoolName_ + " pool";

        XCEPT_RETHROW(xdaq::exception::Exception, s, e);
    }

    buDescriptor_ = 0;
    buTid_        = 0;

    try
    {
        defineFsm();
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(xdaq::exception::Exception,
            "Failed to define finite state machine", e);
    }

    // Initialise and group parameters
    stdConfigParams_  = initAndGetStdConfigParams();
    stdMonitorParams_ = initAndGetStdMonitorParams();
    monitorCounters_  = getMonitorCounters();
    resetCounters(monitorCounters_);
    addCountersToParams(monitorCounters_, stdMonitorParams_);
    dbgMonitorParams_ = initAndGetDbgMonitorParams();

    // Create info space for monitoring
    monitoringInfoSpaceName_ =
        generateMonitoringInfoSpaceName(xmlClass_, instance_);
    toolbox::net::URN urn =
        this->createQualifiedInfoSpace(monitoringInfoSpaceName_);
    monitoringInfoSpace_ = xdata::getInfoSpaceFactory()->get(urn.toString());

    try
    {
        putParamsIntoInfoSpace(stdConfigParams_ , appInfoSpace_       );
        putParamsIntoInfoSpace(stdMonitorParams_, appInfoSpace_       );
        putParamsIntoInfoSpace(dbgMonitorParams_, appInfoSpace_       );

        putParamsIntoInfoSpace(stdMonitorParams_, monitoringInfoSpace_);
        putParamsIntoInfoSpace(dbgMonitorParams_, monitoringInfoSpace_);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(xdaq::exception::Exception,
            "Failed to put parameters into info spaces", e);
    }

    bindFsmSoapCallbacks();
    bindI2oCallbacks();
    bindXgiCallbacks();

    LOG4CPLUS_INFO(logger_, "End of constructor");
}


std::string rubuilder::fu::Application::generateLoggerName()
{
    xdaq::ApplicationDescriptor *appDescriptor = getApplicationDescriptor();
    std::string                 appClass       = appDescriptor->getClassName();
    unsigned int                appInstance    = appDescriptor->getInstance();
    std::stringstream           oss;


    oss << appClass << appInstance;

    return oss.str();
}


xdaq::ApplicationDescriptor
*rubuilder::fu::Application::getRUBuilderTester()
{
    xdaq::ApplicationDescriptor *appDescriptor = 0;


    try
    {
        appDescriptor =
            getApplicationContext()->
            getDefaultZone()->
            getApplicationDescriptor("rubuilder::tester::Application", 0);
    }
    catch(xcept::Exception &e)
    {
        appDescriptor = 0;
    }

    return appDescriptor;
}


std::string rubuilder::fu::Application::createI2oPoolName
(
    const unsigned int  fuInstance
)
{
    std::stringstream oss;


    oss << "FU" << fuInstance << "_i2oFragmentPool";

    return oss.str();
}


void rubuilder::fu::Application::defineFsm()
throw (rubuilder::fu::exception::Exception)
{
    try
    {
        // Define FSM states
        fsm_.addState('H', "Halted" , this,
            &rubuilder::fu::Application::stateChanged);
        fsm_.addState('R', "Ready"  , this,
            &rubuilder::fu::Application::stateChanged);
        fsm_.addState('E', "Enabled", this,
            &rubuilder::fu::Application::stateChanged);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(rubuilder::fu::exception::Exception,
            "Failed to define FSM states", e);
    }

    try
    {
        // Explicitly set the name of the Failed state
        fsm_.setStateName('F', "Failed");
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(rubuilder::fu::exception::Exception,
            "Failed to explicitly set the name of the Failed state", e);
    }

    try
    {
        // Define FSM transitions
        fsm_.addStateTransition('H', 'R', "Configure", this,
            &rubuilder::fu::Application::configureAction);
        fsm_.addStateTransition('R', 'E', "Enable"   , this,
            &rubuilder::fu::Application::enableAction);
        fsm_.addStateTransition('H', 'H', "Halt"     , this,
            &rubuilder::fu::Application::haltAction);
        fsm_.addStateTransition('R', 'H', "Halt"     , this,
            &rubuilder::fu::Application::haltAction);
        fsm_.addStateTransition('E', 'H', "Halt"     , this,
            &rubuilder::fu::Application::haltAction);

        fsm_.addStateTransition('H', 'F', "Fail"     , this,
            &rubuilder::fu::Application::failAction);
        fsm_.addStateTransition('R', 'F', "Fail"     , this,
            &rubuilder::fu::Application::failAction);
        fsm_.addStateTransition('E', 'F', "Fail"     , this,
            &rubuilder::fu::Application::failAction);

        fsm_.addStateTransition('F', 'F', "Fail"     , this,
            &rubuilder::fu::Application::failAction);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(rubuilder::fu::exception::Exception,
            "Failed to define FSM transitions", e);
    }

    try
    {
        fsm_.setFailedStateTransitionAction(this,
            &rubuilder::fu::Application::failAction);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(rubuilder::fu::exception::Exception,
            "Failed to set action for failed state transition", e);
    }

    try
    {
        fsm_.setFailedStateTransitionChanged(this,
            &rubuilder::fu::Application::stateChanged);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(rubuilder::fu::exception::Exception,
            "Failed to set state changed callback for failed state transition",
            e);
    }

    try
    {
        fsm_.setInitialState('H');
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(rubuilder::fu::exception::Exception,
            "Failed to set state initial state of FSM", e);
    }

    try
    {
        fsm_.reset();
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(rubuilder::fu::exception::Exception,
            "Failed to reset FSM", e);
    }
}


std::string rubuilder::fu::Application::generateMonitoringInfoSpaceName
(
    const std::string   appClass,
    const unsigned int  appInstance
)
{
    std::stringstream oss;
    oss << appClass;
    return oss.str();
}


std::vector< std::pair<std::string, xdata::Serializable*> >
rubuilder::fu::Application::initAndGetStdConfigParams()
{
    std::vector< std::pair<std::string, xdata::Serializable*> > params;


    buClass_            = "rubuilder::bu::Application";
    buInstNb_           = 0;
    nbOutstandingRqsts_ = 80;
    sleepBetweenEvents_ = false;
    sleepIntervalUSec_  = 1000; // 1 millisecond
    nbEventsBeforeExit_ = 0;

    params.push_back(std::pair<std::string,xdata::Serializable *>
        ("buClass", &buClass_));
    params.push_back(std::pair<std::string,xdata::Serializable *>
        ("buInstNb", &buInstNb_));
    params.push_back(std::pair<std::string,xdata::Serializable *>
        ("nbOutstandingRqsts", &nbOutstandingRqsts_));
    params.push_back(std::pair<std::string,xdata::Serializable *>
        ("sleepBetweenEvents", &sleepBetweenEvents_));
    params.push_back(std::pair<std::string,xdata::Serializable *>
        ("sleepIntervalUSec" , &sleepIntervalUSec_));
    params.push_back(std::pair<std::string,xdata::Serializable *>
        ("nbEventsBeforeExit", &nbEventsBeforeExit_));

    return params;
}


std::vector< std::pair<std::string, xdata::Serializable*> >
rubuilder::fu::Application::initAndGetDbgConfigParams()
{
    std::vector< std::pair<std::string, xdata::Serializable*> > params;


    return params;
}


void rubuilder::fu::Application::resetCounters
(
    std::vector< std::pair<std::string, xdata::UnsignedInteger32*> > &counters
)
{
    std::vector
    <
        std::pair<std::string, xdata::UnsignedInteger32*>
    >::const_iterator itor;
    xdata::UnsignedInteger32 *counter = 0;


    for(itor=counters.begin(); itor!=counters.end(); itor++)
    {
        counter = (*itor).second;
        *counter = 0;
    }
}


void rubuilder::fu::Application::addCountersToParams
(
    std::vector
    <
        std::pair<std::string, xdata::UnsignedInteger32*>
    > &counters,
    std::vector
    <
        std::pair<std::string, xdata::Serializable*>
    > &params
)
{
    std::vector
    <
        std::pair<std::string, xdata::UnsignedInteger32*>
    >::const_iterator itor;


    for(itor=counters.begin(); itor!=counters.end(); itor++)
    {
        params.push_back(std::pair<std::string,xdata::Serializable *>
            ((*itor).first, (*itor).second));
    }
}


std::vector< std::pair<std::string, xdata::UnsignedInteger32*> >
rubuilder::fu::Application::getMonitorCounters()
{
    std::vector< std::pair<std::string, xdata::UnsignedInteger32*> > counters;


    counters.push_back(std::pair<std::string,xdata::UnsignedInteger32*>
        ("nbEventsProcessed", &nbEventsProcessed_));

    counters.push_back(std::pair<std::string,xdata::UnsignedInteger32 *>
        ("I2O_BU_ALLOCATE_Payload", &I2O_BU_ALLOCATE_Payload_));
    counters.push_back(std::pair<std::string,xdata::UnsignedInteger32 *>
        ("I2O_BU_ALLOCATE_LogicalCount", &I2O_BU_ALLOCATE_LogicalCount_));
    counters.push_back(std::pair<std::string,xdata::UnsignedInteger32 *>
        ("I2O_BU_ALLOCATE_I2oCount", &I2O_BU_ALLOCATE_I2oCount_));
    counters.push_back(std::pair<std::string,xdata::UnsignedInteger32 *>
        ("I2O_BU_DISCARD_Payload", &I2O_BU_DISCARD_Payload_));
    counters.push_back(std::pair<std::string,xdata::UnsignedInteger32 *>
        ("I2O_BU_DISCARD_LogicalCount", &I2O_BU_DISCARD_LogicalCount_));
    counters.push_back(std::pair<std::string,xdata::UnsignedInteger32 *>
        ("I2O_BU_DISCARD_I2oCount", &I2O_BU_DISCARD_I2oCount_));
    counters.push_back(std::pair<std::string,xdata::UnsignedInteger32 *>
        ("I2O_FU_TAKE_Payload", &I2O_FU_TAKE_Payload_));
    counters.push_back(std::pair<std::string,xdata::UnsignedInteger32 *>
        ("I2O_FU_TAKE_LogicalCount", &I2O_FU_TAKE_LogicalCount_));
    counters.push_back(std::pair<std::string,xdata::UnsignedInteger32 *>
        ("I2O_FU_TAKE_I2oCount", &I2O_FU_TAKE_I2oCount_));

    return counters;
}


std::vector< std::pair<std::string, xdata::Serializable*> >
rubuilder::fu::Application::initAndGetStdMonitorParams()
{
    std::vector< std::pair<std::string, xdata::Serializable*> > params;


    stateName_        = "Halted";
    faultDetected_    = false;

    params.push_back(std::pair<std::string,xdata::Serializable *>
        ("stateName", &stateName_));
    params.push_back(std::pair<std::string,xdata::Serializable *>
        ("faultDetected", &faultDetected_));

    return params;
}


std::vector< std::pair<std::string, xdata::Serializable*> >
rubuilder::fu::Application::initAndGetDbgMonitorParams()
{
    std::vector< std::pair<std::string, xdata::Serializable*> > params;


    nbSuperFragmentsInEvent_ = 0;

    params.push_back(std::pair<std::string,xdata::Serializable *>
        ("nbSuperFragmentsInEvent", &nbSuperFragmentsInEvent_));

    return params;
}


void rubuilder::fu::Application::putParamsIntoInfoSpace
(
    std::vector< std::pair<std::string, xdata::Serializable*> > &params,
    xdata::InfoSpace                                            *s
)
throw (rubuilder::fu::exception::Exception)
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

            XCEPT_RETHROW(rubuilder::fu::exception::Exception, oss.str(), e);
        }
    }
}


void rubuilder::fu::Application::stateChanged
(
    toolbox::fsm::FiniteStateMachine &fsm
)
throw (toolbox::fsm::exception::Exception)
{
    toolbox::fsm::State state = fsm.getCurrentState();


    try
    {
        stateName_ = fsm.getStateName(state);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(toolbox::fsm::exception::Exception,
            "Failed to set exported parameter stateName", e);
    }
}


void rubuilder::fu::Application::bindFsmSoapCallbacks()
{
    xoap::bind
    (
        this,
        &rubuilder::fu::Application::processSoapFsmEvent,
        "Configure",
        XDAQ_NS_URI
    );

    xoap::bind
    (
        this,
        &rubuilder::fu::Application::processSoapFsmEvent,
        "Enable",
        XDAQ_NS_URI
    );

    xoap::bind
    (
        this,
        &rubuilder::fu::Application::processSoapFsmEvent,
        "Halt",
        XDAQ_NS_URI
    );

    xoap::bind
    (
        this,
        &rubuilder::fu::Application::processSoapFsmEvent,
        "Fail",
        XDAQ_NS_URI
    );

    xoap::bind
    (
        this,
        &rubuilder::fu::Application::resetMonitoringCountersMsg,
        "resetMonitoringCounters",
        XDAQ_NS_URI
    );
}


void rubuilder::fu::Application::fsmWebPage
(
    xgi::Input  *in,
    xgi::Output *out
)
throw (xgi::exception::Exception)
{
    try
    {
         processFsmForm(in);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(xgi::exception::Exception,
            "Failed to process FSM form", e);
    }

    toolbox::fsm::State      state = fsm_.getCurrentState();
    std::vector<std::string> events;

    out->getHTTPResponseHeader().addHeader("Content-Type", "text/html");

    *out << "<html>"                                              << std::endl;
    *out << "<head>"                                              << std::endl;
    *out << "<link type=\"text/css\" rel=\"stylesheet\"";
    *out << " href=\"/" << urn_ << "/styles.css\"/>"              << std::endl;
    *out << "<title>"                                             << std::endl;
    *out << xmlClass_ << instance_ << " FSM"                      << std::endl;
    *out << "</title>"                                            << std::endl;
    *out << "</head>"                                             << std::endl;
    *out << "<body>"                                              << std::endl;

    rubuilder::utils::printWebPageTitleBar
    (
        out,
        rubuilder::fu::FSM_ICON,
        "FSM",
        xmlClass_,                   // appClass
        instance_,                   // appInstance
        stateName_,                  // appState
        urn_,                        // urn
        rubuilderfu::version,        // version
        "$Id:$", // cvs id
        rubuilder::fu::APP_ICON,
        rubuilder::fu::DBG_ICON,
        rubuilder::fu::FSM_ICON
    );

    *out << "<form method=\"get\" action=\"/" << urn_ << "/fsm\">";
    *out << std::endl;

    switch(state)
    {
    case 'H': // Halted
        events.push_back("Configure");
        break;
    case 'R': // Ready
        events.push_back("Enable");
        break;
    case 'E': // Enabled
        events.push_back("Halt");
        break;
    case 'F': // Failed
        // Nothing can be done
        break;
    default:
        std::stringstream oss;

        oss << "Unknown state: " << state;

        XCEPT_RAISE(xgi::exception::Exception, oss.str());
    }

    // If the application is not in the "Failed" state
    if(state != 'F')
    {
        *out << "<table width=\"100%\">"                          << std::endl;
        *out << "<tr align=\"left\" width=\"50%\">"               << std::endl;
        *out << "  <td>"                                          << std::endl;

        std::vector<std::string>::const_iterator itor;

        for(itor=events.begin(); itor!=events.end(); itor++)
        {
            *out << "    <input"                                  << std::endl;
            *out << "     type=\"submit\""                        << std::endl;
            *out << "     name=\"event\""                         << std::endl;
            *out << "     value=\"" << *itor << "\""              << std::endl;
            *out << "    />"                                      << std::endl;
        }

        *out << "  </td>"                                         << std::endl;
        *out << "  <td align=\"left\"width=\"50%\">"              << std::endl;
        *out << "    <font color=\"red\"><b>DEBUGGING ONLY!!!!</b></font>";
        *out                                                      << std::endl;
        *out << "    <input"                                      << std::endl;
        *out << "     type=\"submit\""                            << std::endl;
        *out << "     name=\"event\""                             << std::endl;
        *out << "     value=\"Fail\""                             << std::endl;
        *out << "    />"                                          << std::endl;
        *out << "  </td>"                                         << std::endl;
        *out << "</tr>"                                           << std::endl;
        *out << "</table>"                                        << std::endl;
    }

    *out << "</form>"                                             << std::endl;
    *out << "</body>"                                             << std::endl;

    *out << "</html>"                                             << std::endl;
}


void rubuilder::fu::Application::processFsmForm
(
    xgi::Input *in
)
throw (xgi::exception::Exception)
{
    cgicc::Cgicc         cgi(in);
    cgicc::form_iterator element = cgi.getElement("event");
    std::string          event   = "";


    // If their is an event from from the html form
    if(element != cgi.getElements().end())
    {
        event = (*element).getValue();

        try
        {
            toolbox::Event::Reference evtRef(new toolbox::Event(event, this));

            applicationBSem_.take();
            fsm_.fireEvent(evtRef);
            applicationBSem_.give();
        }
        catch(xcept::Exception &e)
        {
            applicationBSem_.give();

            std::stringstream oss;

            oss << "Failed to process FSM event: " << event;

            LOG4CPLUS_ERROR(logger_,
                oss.str() << " : " << xcept::stdformat_exception_history(e));
            XCEPT_RETHROW(xgi::exception::Exception, oss.str(), e);
        }
    }
}


xoap::MessageReference rubuilder::fu::Application::processSoapFsmEvent
(
    xoap::MessageReference msg
)
throw (xoap::exception::Exception)
{
    std::string event = "";


    try
    {
        event = soapParameterExtractor_.extractParameters(msg);
    }
    catch(xcept::Exception &e)
    {
        std::string s = "Failed to extract FSM event and parameters from SOAP message";

        LOG4CPLUS_ERROR(logger_,
            s << " : " << xcept::stdformat_exception_history(e));
        XCEPT_RETHROW(xoap::exception::Exception, s, e);
    }

    try
    {
        toolbox::Event::Reference evtRef(new toolbox::Event(event, this));

        applicationBSem_.take();
        fsm_.fireEvent(evtRef);
        applicationBSem_.give();
    }
    catch(xcept::Exception &e)
    {
        applicationBSem_.give();

        std::stringstream oss;

        oss << "Failed to process FSM event: " << event;

        LOG4CPLUS_ERROR(logger_,
            oss.str() << " : " << xcept::stdformat_exception_history(e));
        XCEPT_RETHROW(xoap::exception::Exception, oss.str(), e);
    }

    if(fsm_.getCurrentState() == 'F')
    {
        XCEPT_RAISE(xoap::exception::Exception, reasonForFailed_.getValue());
    }

    return  rubuilder::utils::createFsmSoapResponseMsg(event, stateName_.toString());
}


xoap::MessageReference
rubuilder::fu::Application::resetMonitoringCountersMsg
(
    xoap::MessageReference msg
)
throw (xoap::exception::Exception)
{
    applicationBSem_.take();
    resetCounters(monitorCounters_);
    applicationBSem_.give();

    std::string responseString = "resetMonitoringCountersResponse";

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
    catch(xcept::Exception &e)
    {
        std::stringstream oss;

        oss << "Failed to create " << responseString << " message";

        XCEPT_RETHROW(xoap::exception::Exception, oss.str(), e);
    }
}


void rubuilder::fu::Application::configureAction
(
    toolbox::Event::Reference e
)
throw (toolbox::fsm::exception::Exception)
{
    releaseSuperFragment();

    // Reset the current block number of the super-fragment under-construction.
    blockNb_ = 0;

    // Clean start
    faultDetected_ = false;
    faultDescription_.setValue("");
    fedSourceIdHistory_.clear();
    fedSourceIds_.clear();

    try
    {
        tid_ = i2oAddressMap_->getTid(appDescriptor_);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(toolbox::fsm::exception::Exception,
            "Failed to get the I2O TID of this application", e);
    }

    nbEventsProcessed_ = 0;

    try
    {
        buDescriptor_ =
            getApplicationContext()->
            getDefaultZone()->
            getApplicationDescriptor(buClass_, buInstNb_);
        buTid_        = i2oAddressMap_->getTid(buDescriptor_);
    }
    catch(xcept::Exception &e)
    {
        std::stringstream oss;

        oss << "Failed to get application descriptor and I2O for";
        oss << " BU" << instance_;

        XCEPT_RETHROW(toolbox::fsm::exception::Exception, oss.str(), e);
    }
    catch(...)
    {
        std::stringstream oss;

        oss << "Failed to get application descriptor and I2O for";
        oss << " BU" << instance_;
        oss << " : Unknown exception";

        XCEPT_RAISE(toolbox::fsm::exception::Exception, oss.str());
    }
}


void rubuilder::fu::Application::enableAction
(
    toolbox::Event::Reference e
)
throw (toolbox::fsm::exception::Exception)
{
    try
    {
        allocateNEvents(nbOutstandingRqsts_);
    }
    catch(xcept::Exception &e)
    {
        std::stringstream oss;

        oss << "Failed to allocate " << nbOutstandingRqsts_ << " events";

        XCEPT_RETHROW(toolbox::fsm::exception::Exception, oss.str(), e);
    }
}


void rubuilder::fu::Application::haltAction
(
    toolbox::Event::Reference e
)
throw (toolbox::fsm::exception::Exception)
{
    // Do nothing
}


void rubuilder::fu::Application::failAction
(
    toolbox::Event::Reference event
)
throw (toolbox::fsm::exception::Exception)
{
    std::string reason       = "";
    std::string xceptMessage = "";
    std::string errorMessage = "";


    // Determine reason for failure if there is one
    if(typeid(*event) == typeid(toolbox::fsm::FailedEvent))
    {
        toolbox::fsm::FailedEvent &failedEvent =
            dynamic_cast<toolbox::fsm::FailedEvent&>(*event);
        xcept::Exception exception = failedEvent.getException();

        std::stringstream oss;

        oss << "Failure occurred when performing transition from: ";
        oss << failedEvent.getFromState();
        oss <<  " to: ";
        oss << failedEvent.getToState();

        reason = oss.str();

        oss << " exception history: ";
        oss << xcept::stdformat_exception_history(exception);
	
	xceptMessage = oss.str();
    }
    else if(typeid(*event) == typeid(rubuilder::fu::ForceFailedEvent))
    {
        rubuilder::fu::ForceFailedEvent &forceFailedEvent =
            dynamic_cast<rubuilder::fu::ForceFailedEvent&>(*event);

        reason = forceFailedEvent.getReason();
    }

    // Update web page data
    reasonForFailed_.setValue(reason);

    // Prepare error message for logging and the sentinel
    if(reason != "")
    {
        errorMessage = "Moved to Failed state: " + reason;
    }
    else
    {
        errorMessage = "Moved to Failed state";
    }

    LOG4CPLUS_FATAL(logger_, errorMessage << xceptMessage);

    // Notify the sentinel
    XCEPT_DECLARE(rubuilder::fu::exception::Exception, sentinelException,
        errorMessage);
    this->notifyQualified("fatal", sentinelException);
}


void rubuilder::fu::Application::bindI2oCallbacks()
{
    i2o::bind
    (
        this,
        &rubuilder::fu::Application::I2O_FU_TAKE_Callback,
        I2O_FU_TAKE,
        XDAQ_ORGANIZATION_ID
    );

    i2o::bind
    (
        this,
        &rubuilder::fu::Application::I2O_EVM_LUMISECTION_Callback,
        I2O_EVM_LUMISECTION,
        XDAQ_ORGANIZATION_ID
    );
}


void rubuilder::fu::Application::bindXgiCallbacks()
{
    xgi::bind
    (
        this,
        &rubuilder::fu::Application::css,
        "styles.css"
    );

    xgi::bind
    (
        this,
        &rubuilder::fu::Application::defaultWebPage,
        "Default"
    );

    xgi::bind
    (
        this,
        &rubuilder::fu::Application::debugWebPage,
        "debug"
    );

    xgi::bind
    (
        this,
        &rubuilder::fu::Application::fsmWebPage,
        "fsm"
    );
}


void rubuilder::fu::Application::defaultWebPage
(
    xgi::Input  *in,
    xgi::Output *out
)
throw (xgi::exception::Exception)
{
    applicationBSem_.take();

    try
    {
        printDefaultWebPage(in, out);
    }
    catch(xcept::Exception &e)
    {
        applicationBSem_.give();

        XCEPT_RETHROW(xgi::exception::Exception,
            "Failed to print contents of default web page", e);
    }
    catch(std::exception &e)
    {
        applicationBSem_.give();

        std::stringstream oss;

        oss << "Failed to print contents of default web page: ";
        oss << e.what();

        XCEPT_RAISE(xgi::exception::Exception, oss.str());
    }
    catch(...)
    {
        applicationBSem_.give();

        XCEPT_RAISE(xgi::exception::Exception,
            "Failed to print contents of default web page: Unknown exception");
    }

    applicationBSem_.give();
}


void rubuilder::fu::Application::printDefaultWebPage
(
    xgi::Input  *in,
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
    *out << xmlClass_ << instance_ << " MAIN"                     << std::endl;
    *out << "</title>"                                            << std::endl;
    *out << "</head>"                                             << std::endl;

    *out << "<body>"                                              << std::endl;

    rubuilder::utils::printWebPageTitleBar
    (
        out,
        rubuilder::fu::APP_ICON,
        "Main",
        xmlClass_,                   // appClass
        instance_,                   // appInstance
        stateName_,                  // appState
        urn_,                        // urn
        rubuilderfu::version,        // version
        "$Id:$", // cvs id
        rubuilder::fu::APP_ICON,
        rubuilder::fu::DBG_ICON,
        rubuilder::fu::FSM_ICON
    );

    *out << "<table>"                                             << std::endl;
    *out << "<tr valign=\"top\">"                                 << std::endl;
    *out << "  <td>"                                              << std::endl;
    try
    {
        printParamsTable(in, out, "Standard configuration", stdConfigParams_);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(xgi::exception::Exception,
            "Failed to print standard configuration table", e);
    }
    *out << "  </td>"                                             << std::endl;
    *out << "  <td width=\"64\">"                                 << std::endl;
    *out << "  </td>"                                             << std::endl;
    *out << "  <td>"                                              << std::endl;
    try
    {
        printParamsTable(in, out, "Standard monitoring", stdMonitorParams_);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(xgi::exception::Exception,
            "Failed to print standard monitoring table", e);
    }
    *out << "  </td>"                                             << std::endl;
    *out << "</tr>"                                               << std::endl;
    *out << "</table>"                                            << std::endl;

    *out << "<p>"                                                 << std::endl;
    fedSourceIdHistory_.printHtml(out);
    *out << "</p>"                                                << std::endl;

    *out << "</body>"                                             << std::endl;

    *out << "</html>"                                             << std::endl;
}


void rubuilder::fu::Application::debugWebPage
(
    xgi::Input  *in,
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
    *out << xmlClass_ << instance_ << " DEBUG"                    << std::endl;
    *out << "</title>"                                            << std::endl;
    *out << "</head>"                                             << std::endl;
    *out << "<body>"                                              << std::endl;

    rubuilder::utils::printWebPageTitleBar
    (
        out,
        rubuilder::fu::DBG_ICON,
        "Debug",
        xmlClass_,                   // appClass
        instance_,                   // appInstance
        stateName_,                  // appState
        urn_,                        // urn
        rubuilderfu::version,        // version
        "$Id:$", // cvs id
        rubuilder::fu::APP_ICON,
        rubuilder::fu::DBG_ICON,
        rubuilder::fu::FSM_ICON
    );

    *out << "<table>"                                             << std::endl;
    *out << "<tr valign=\"top\">"                                 << std::endl;
    *out << "  <td>"                                              << std::endl;
    try
    {
        printParamsTable(in, out, "Debug configuration", dbgConfigParams_);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(xgi::exception::Exception,
            "Failed to print debug configuration table", e);
    }
    *out << "  </td>"                                             << std::endl;
    *out << "  <td width=\"64\">"                                 << std::endl;
    *out << "  </td>"                                             << std::endl;
    *out << "  <td>"                                              << std::endl;
    try
    {
        printParamsTable(in, out, "Debug monitoring", dbgMonitorParams_);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(xgi::exception::Exception,
            "Failed to print debug monitoring table", e);
    }
    *out << "  </td>"                                             << std::endl;
    *out << "</tr>"                                               << std::endl;
    *out << "</table>"                                            << std::endl;

    std::string reasonForFailed = reasonForFailed_.getValue();

    if(reasonForFailed == "")
    {
        reasonForFailed = "N/A";
    }

    *out << "<p>"                                                 << std::endl;
    *out << "<table frame=\"void\" rules=\"rows\" class=\"params\">";
    *out << std::endl;
    *out << "  <tr>"                                              << std::endl;
    *out << "    <th>Reason state machine moved to \"Failed\"</th>";
    *out << std::endl;
    *out << "  </tr>"                                             << std::endl;
    *out << "  <tr>"                                              << std::endl;
    *out << "    <td>" << reasonForFailed << "</td>"              << std::endl;
    *out << "  </tr>"                                             << std::endl;
    *out << "</table>"                                            << std::endl;
    *out << "</p>"                                                << std::endl;

    std::string faultDescription = faultDescription_.getValue();

    if(faultDescription == "")
    {
        faultDescription = "N/A";
    }

    *out << "<p>"                                                 << std::endl;
    *out << "<table frame=\"void\" rules=\"rows\" class=\"params\">";
    *out << std::endl;
    *out << "  <tr>"                                              << std::endl;
    *out << "    <th>Description of fault in event data</th>"     << std::endl;
    *out << "  </tr>"                                             << std::endl;
    *out << "  <tr>"                                              << std::endl;
    *out << "    <td>" << faultDescription << "</td>"             << std::endl;
    *out << "  </tr>"                                             << std::endl;
    *out << "</table>"                                            << std::endl;
    *out << "</p>"                                                << std::endl;

    *out << "</body>"                                             << std::endl;
    *out << "</html>"                                             << std::endl;
}


void rubuilder::fu::Application::printParamsTable
(
    xgi::Input                                                 *in,
    xgi::Output                                                *out,
    const std::string                                          name,
    std::vector< std::pair<std::string, xdata::Serializable*> > &params
)
throw (xgi::exception::Exception)
{
    std::vector
    <
        std::pair<std::string, xdata::Serializable*>
    >::const_iterator itor;

    *out << "<table frame=\"void\" rules=\"rows\" class=\"params\">";
    *out << std::endl;

    *out << "  <tr>"                                              << std::endl;
    *out << "    <th colspan=2>"                                  << std::endl;
    *out << "      " << name                                      << std::endl;
    *out << "    </th>"                                           << std::endl;
    *out << "  </tr>"                                             << std::endl;


    for(itor=params.begin(); itor!=params.end(); itor++)
    {
        *out << "  <tr>"                                          << std::endl;

        // Name
        *out << "    <td>"                                        << std::endl;
        *out << "      " << itor->first                           << std::endl;
        *out << "    </td>"                                       << std::endl;

        // Value
        *out << "    <td>"                                        << std::endl;

        std::string str;

        try
        {
            str = itor->second->toString();
        }
        catch(xcept::Exception &e)
        {
            str = e.what();
        }
        *out << "      " << str << std::endl;

        *out << "    </td>"                                       << std::endl;

        *out << "  </tr>"                                         << std::endl;
    }

    *out << "</table>"                                            << std::endl;
}


void rubuilder::fu::Application::I2O_FU_TAKE_Callback
(
    toolbox::mem::Reference *bufRef
)
{
    applicationBSem_.take();

    toolbox::fsm::State     state = fsm_.getCurrentState();
    toolbox::mem::Reference *next = 0;


    switch(state)
    {
    case 'H': // Halted
    case 'F': // Failed
        bufRef->release();
        break;
    case 'R': // Ready
    case 'E': // Enabled
        if(faultDetected_)
        {
            bufRef->release();
        }
        else
        {
            // bufRef might point to a chain - break the chain into its
            // separate blocks and process each block individually
            while(bufRef != 0)
            {
                // Break the current block from the chain, making sure the next
                // block is not lost
                next = bufRef->getNextReference();
                bufRef->setNextReference(0);

                // Process the current block
                try
                {
                    processDataBlock(bufRef);
                }
                catch(xcept::Exception &e)
                {
                    LOG4CPLUS_ERROR(logger_,
                        "Failed to process data block : "
                        << xcept::stdformat_exception_history(e));
                }

                // Move to the next block (could be 0!)
                bufRef = next;
            }
        }
        break;
    default:
        LOG4CPLUS_ERROR(logger_, "Unknown application state");
        bufRef->release();
    }

    applicationBSem_.give();
}


void rubuilder::fu::Application::I2O_EVM_LUMISECTION_Callback
(
    toolbox::mem::Reference *bufRef
)
{
    // just release the buffer for now
    bufRef->release();
}


void rubuilder::fu::Application::processDataBlock
(
    toolbox::mem::Reference *bufRef
)
throw (rubuilder::fu::exception::Exception)
{
    I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME *block =
        (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
    I2O_MESSAGE_FRAME *stdMsg = (I2O_MESSAGE_FRAME*)block;
    bool superFragmentIsLastOfEvent =
        block->superFragmentNb == (block->nbSuperFragmentsInEvent - 1);
    bool blockIsLastOfSuperFragment =
        block->blockNb == (block->nbBlocksInSuperFragment-1);
    bool blockIsLastOfEvent =
        superFragmentIsLastOfEvent && blockIsLastOfSuperFragment;
    U32 buResourceId = block->buResourceId;

    // Update parameters showing message payloads and counts
    monitoringInfoSpace_->lock(); appInfoSpace_->lock();
    I2O_FU_TAKE_Payload_.value_ +=
        (stdMsg->MessageSize << 2) -
        sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
    I2O_FU_TAKE_I2oCount_.value_++;
    if(blockIsLastOfEvent)
    {
        I2O_FU_TAKE_LogicalCount_.value_++;
    }

    // For debugging
    nbSuperFragmentsInEvent_ = block->nbSuperFragmentsInEvent;
    monitoringInfoSpace_->unlock(); appInfoSpace_->unlock();

    // Check block as an individual
    try
    {
        if(block->superFragmentNb == 0)
        {
            checkTAPayload(bufRef);
        }
        else
        {
            checkRUIPayload(bufRef);
        }
    }
    catch(xcept::Exception &e)
    {
	std::string errorMessage = "Invalid block";

        faultDetected_ = true;
        faultDescription_.setValue(errorMessage);

        LOG4CPLUS_ERROR(logger_, errorMessage << " : " 
			<< xcept::stdformat_exception_history(e));

        // Notify the sentinel
        XCEPT_DECLARE(rubuilder::fu::exception::Exception, sentinelException,
        errorMessage);
        this->notifyQualified("error", sentinelException);
    }

    appendBlockToSuperFragment(bufRef);

    if(blockIsLastOfSuperFragment)
    {
        try
        {
            checkSuperFragment();
        }
        catch(xcept::Exception &e)
        {
	    std::string errorMessage = "Invalid super-fragment";

            faultDetected_ = true;
            faultDescription_.setValue(errorMessage);

            LOG4CPLUS_ERROR(logger_, errorMessage << " : " 
			<< xcept::stdformat_exception_history(e));

            // Notify the sentinel
            XCEPT_DECLARE(rubuilder::fu::exception::Exception,
                sentinelException, errorMessage);
            this->notifyQualified("error", sentinelException);
        }

        try
        {
            releaseSuperFragment();
        }
        catch(xcept::Exception &e)
        {
            std::stringstream oss;

            oss << "Failed to release super-fragment with";
            oss << " BU resource id: " << buResourceId;

            XCEPT_RETHROW(rubuilder::fu::exception::Exception, oss.str(), e);
        }
    }

    if(blockIsLastOfEvent)
    {
        nbEventsProcessed_.value_++;

        // Reset FED source IDs ready for next event
        fedSourceIds_.clear();

        // If FU is to emulate a crash
        if(nbEventsBeforeExit_.value_ > 0)
        {
            if(nbEventsProcessed_.value_ == nbEventsBeforeExit_.value_)
            {
                std::stringstream oss;

                oss << "Emulating crash after " << nbEventsBeforeExit_;
                oss << " events";

                LOG4CPLUS_FATAL(logger_, oss.str());
                exit(-1);
            }
        }

        if(sleepBetweenEvents_.value_)
        {
            ::usleep(sleepIntervalUSec_.value_);
        }

        try
        {
            discardEvent(buResourceId);
        }
        catch(xcept::Exception &e)
        {
            std::stringstream oss;

            oss << "Failed to discard event with";
            oss << " BU resource id: " << buResourceId;

            XCEPT_RETHROW(rubuilder::fu::exception::Exception, oss.str(), e);
        }

        try
        {
            allocateNEvents(1);
        }
        catch(xcept::Exception &e)
        {
            XCEPT_RETHROW(rubuilder::fu::exception::Exception,
                "Failed to allocate an event", e);
        }
    }
}


void rubuilder::fu::Application::appendBlockToSuperFragment
(
    toolbox::mem::Reference *bufRef
)
{
    if(superFragmentHead_ == 0)
    {
        superFragmentHead_ = bufRef;
        superFragmentTail_ = bufRef;
    }
    else
    {
        superFragmentTail_->setNextReference(bufRef);
        superFragmentTail_ = bufRef;
    }
}


void rubuilder::fu::Application::checkSuperFragment()
throw (rubuilder::fu::exception::Exception)
{
    unsigned int len  = getSumOfFedData(superFragmentHead_);
    unsigned char* buf = new unsigned char[len];


    try
    {
        fillBufferWithSuperFragment(buf, len, superFragmentHead_);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(rubuilder::fu::exception::Exception,
            "Failed to fill contiguous memory super-fragment", e);
    }

    try
    {
        checkFedTraversal(buf, len);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(rubuilder::fu::exception::Exception,
            "Failed to traverse FEDs", e);
    }

    try
    {
        checkFedSourceIds(buf, len);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(rubuilder::fu::exception::Exception,
            "Invalid source IDs", e);
    }

    delete[] buf;
}


unsigned int rubuilder::fu::Application::getSumOfFedData
(
    toolbox::mem::Reference *bufRef
)
{
    unsigned char *blockAddr     = 0;
    unsigned char *frlHeaderAddr = 0;
    frlh_t        *frlHeader     = 0;
    unsigned int  nbBytes        = 0;


    while(bufRef != 0)
    {
        blockAddr     = (unsigned char*)bufRef->getDataLocation();
        frlHeaderAddr = blockAddr + sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
        frlHeader     = (frlh_t*)frlHeaderAddr;

        nbBytes += frlHeader->segsize & FRL_SEGSIZE_MASK;

        bufRef = bufRef->getNextReference();
    }

    return nbBytes;
}


void rubuilder::fu::Application::fillBufferWithSuperFragment
(
    unsigned char           *buf,
    unsigned int            len,
    toolbox::mem::Reference *bufRef
)
throw (rubuilder::fu::exception::Exception)
{
    unsigned char *blockAddr     = 0;
    unsigned char *frlHeaderAddr = 0;
    unsigned char *fedAddr       = 0;
    frlh_t        *frlHeader     = 0;
    unsigned char *endPos        = 0;
    unsigned char *pos           = 0;
    unsigned int  nbBytes        = 0;


    endPos = buf + len;
    pos    = buf;

    while(bufRef != 0)
    {
        blockAddr     = (unsigned char*)bufRef->getDataLocation();
        frlHeaderAddr = blockAddr + sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
        fedAddr       = frlHeaderAddr + sizeof(frlh_t);
        frlHeader     = (frlh_t*)frlHeaderAddr;
        nbBytes       = frlHeader->segsize & FRL_SEGSIZE_MASK;

        // If inserting fed bytes would over-shoot end of buffer
        if((pos + nbBytes) > endPos)
        {
            XCEPT_RAISE(rubuilder::fu::exception::Exception, "Reached end of buffer");
        }

        memcpy(pos, fedAddr, nbBytes);

        pos += nbBytes;
        bufRef = bufRef->getNextReference();
    }
}


void rubuilder::fu::Application::checkFedTraversal
(
    unsigned char* buf,
    unsigned int len
)
throw (rubuilder::fu::exception::Exception)
{
    unsigned char *fedTrailerAddr = 0;
    unsigned char *fedHeaderAddr  = 0;
    fedt_t        *fedTrailer     = 0;
    fedh_t        *fedHeader      = 0;
    unsigned int  fedSize         = 0;
    unsigned int  sumOfFedSizes   = 0;


    fedTrailerAddr = buf + len - sizeof(fedt_t);

    while(fedTrailerAddr > buf)
    {
        fedTrailer = (fedt_t*)fedTrailerAddr;
        
        // check for fed trailer id
        if (FED_TCTRLID_EXTRACT(fedTrailer->eventsize) != FED_SLINK_END_MARKER) {
            std::ostringstream oss;
            oss << "Missing FED trailer id.";
            XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
        }
        
        fedSize = FED_EVSZ_EXTRACT(fedTrailer->eventsize) << 3;
        sumOfFedSizes += fedSize;
        
        fedHeaderAddr = fedTrailerAddr - fedSize + sizeof(fedt_t);
        
        // check that fed header is within buffer
        if (fedHeaderAddr < buf) {
            std::ostringstream oss;
            oss << "FED header address out-of-bounds.";
            XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
        }
        
        // check that payload starts within buffer
        if((fedHeaderAddr + sizeof(fedh_t)) > (buf + len))
        {
            std::ostringstream oss;
            oss << "FED payload out-of-bounds.";
            XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
        }
        
        fedHeader = (fedh_t*)fedHeaderAddr;
        
        // check for fed header id
        if (FED_HCTRLID_EXTRACT(fedHeader->eventid) != FED_SLINK_START_MARKER) {
            std::ostringstream oss;
            oss << "Missing FED header id.";
            XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
        }
        
        const uint32_t fedId = FED_SOID_EXTRACT(fedHeader->sourceid);

        const uint32_t conscheck = fedTrailer->conscheck;
        const uint32_t crc = FED_CRCS_EXTRACT(fedTrailer->conscheck);
        fedTrailer->conscheck = 0;
        const uint32_t crcChk = evf::compute_crc(fedHeaderAddr, fedSize);
        if (crc != crcChk) {
            std::ostringstream oss;
            oss << "crc check failed for fedid:" << fedId <<
                " found " << std::hex << crc <<
                " but calculated " << crcChk;
            XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
        }
        fedTrailer->conscheck = conscheck;

        // Move to the next FED trailer
        fedTrailerAddr = fedTrailerAddr - fedSize;
    }

    if((fedTrailerAddr + sizeof(fedt_t)) != buf)
    {
        std::stringstream oss;

        oss << "End of traversal does not land on the first FED";
        oss << " Super-fragment size from FED trailers: " << sumOfFedSizes;
        oss << " Super-fragment size from FRL headers: " << len;

        XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
    }
}


void rubuilder::fu::Application::checkFedSourceIds
(
    unsigned char* buf,
    unsigned int len
)
throw (rubuilder::fu::exception::Exception)
{
    unsigned char *fedTrailerAddr = 0;
    unsigned char *fedHeaderAddr  = 0;
    fedt_t   *fedTrailer     = 0;
    fedh_t   *fedHeader      = 0;
    size_t   fedSize         = 0;
    uint32_t fedSourceId     = 0;
    size_t   sumOfFedSizes   = 0;


    fedTrailerAddr = buf + len - sizeof(fedt_t);

    while(fedTrailerAddr > buf)
    {
        fedTrailer = (fedt_t*)fedTrailerAddr;

        fedSize = (fedTrailer->eventsize &
            ~(FED_SLINK_END_MARKER << FED_HCTRLID_SHIFT)) << 3;
        sumOfFedSizes += fedSize;

        fedHeaderAddr = fedTrailerAddr - fedSize + sizeof(fedt_t);

        if(fedHeaderAddr < buf)
        {
            XCEPT_RAISE(rubuilder::fu::exception::Exception,
                "Fell off front of super-fragment");
        }

        if((fedHeaderAddr + sizeof(fedh_t)) > (buf + len))
        {
            XCEPT_RAISE(rubuilder::fu::exception::Exception,
                "Fell off end of super-fragment");
        }

        fedHeader   = (fedh_t*)fedHeaderAddr;
        fedSourceId = FED_SOID_EXTRACT(fedHeader->sourceid);

        // Update FED source ID history
        fedSourceIdHistory_.insert(fedSourceId);

        // Check for duplicate FED source ID
        if(!fedSourceIds_.insert(fedSourceId))
        {
            std::stringstream oss;
            uint32_t eventid = FED_LVL1_EXTRACT(fedHeader->eventid);

            oss << "Duplicate FED sourceid=" << fedSourceId;
            oss << " eventid=" << eventid;

            XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
        }

        // Move to the next FED trailer
        fedTrailerAddr = fedTrailerAddr - fedSize;
    }

    if((fedTrailerAddr + sizeof(fedt_t)) != buf)
    {
        std::stringstream oss;

        oss << "End of traversal does not land on the first FED";
        oss << " Super-fragment size from FED trailers: " << sumOfFedSizes;
        oss << " Super-fragment size from FRL headers: " << len;

        XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
    }
}


void rubuilder::fu::Application::releaseSuperFragment()
{
    if(superFragmentHead_ != 0)
    {
        superFragmentHead_->release();

        superFragmentHead_ = 0;
        superFragmentTail_ = 0;
    }
}


void rubuilder::fu::Application::checkTAPayload
(
    toolbox::mem::Reference *bufRef
)
throw (rubuilder::fu::exception::Exception)
{
    unsigned int                       bufSize              = 0;
    unsigned int                       minimumBufSize       = 0;
    char                               *payload             = 0;
    frlh_t                             *frlHeader           = 0;
    I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME *block               = 0;
    unsigned int                       expectedSegSize      = 0;
    unsigned int                       segSize              = 0;
    fedh_t                             *fedHeader           = 0;
    fedt_t                             *fedTrailer          = 0;
    size_t                             expectedFedEventSize = 0;
    size_t                             fedEventSize         = 0;
    uint32_t                           eventid              = 0;


    bufSize = bufRef->getDataSize();
    minimumBufSize = sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME) +
                     sizeof(frlh_t) + sizeof(fedh_t) + sizeof(fedt_t);

    if(bufSize < minimumBufSize)
    {
        std::stringstream oss;

        oss << "Trigger message is too small.";
        oss << " Minimum size: " << minimumBufSize;
        oss << " Received: "     << bufSize;

        XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
    }

    payload = ((char*)bufRef->getDataLocation()) +
              sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);

    frlHeader = (frlh_t*)(payload);

    block = (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();

    if(frlHeader->trigno != block->eventNumber)
    {
        std::stringstream oss;

        oss << "Trigger FRL header \"trigno\" does not match";
        oss << " RU builder header \"eventNumber\"";
        oss << " trigno: " << frlHeader->trigno;
        oss << " eventNumber: " << block->eventNumber;

        XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
    }

    if(frlHeader->segno != 0)
    {
        std::stringstream oss;

        oss << "Trigger FRL header segment number is not 0.";
        oss << " Received: " << frlHeader->segno;

        XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
    }

    expectedSegSize = bufSize - sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME) -
                      sizeof(frlh_t);
    segSize = frlHeader->segsize & FRL_SEGSIZE_MASK;

    if(segSize != expectedSegSize)
    {
        std::stringstream oss;

        oss << "Trigger FRL header segment size is not as expected.";
        oss << " Expected: " << expectedSegSize;
        oss << " Received: " << segSize;

        XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
    }

    fedHeader   = (fedh_t*)(payload + sizeof(frlh_t));
    eventid     = FED_LVL1_EXTRACT(fedHeader->eventid);

    if(eventid != block->eventNumber)
    {
        std::stringstream oss;

        oss << "Trigger FED header \"eventid\" does not match";
        oss << " RU builder header \"eventNumber\"";
        oss << " eventid: " << eventid;
        oss << " eventNumber: " << block->eventNumber;

        XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
    }

    fedTrailer = (fedt_t*)(payload + sizeof(frlh_t) + segSize - sizeof(fedt_t));

    expectedFedEventSize = expectedSegSize >> 3;
    fedEventSize = fedTrailer->eventsize & FED_EVSZ_MASK;

    if(fedEventSize != expectedFedEventSize)
    {
        std::stringstream oss;

        oss << "Trigger FED trailer event size is not as expected.";
        oss << " Expected: " << expectedFedEventSize;
        oss << " Received: " << fedEventSize;

        XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
    }
}


void rubuilder::fu::Application::checkRUIPayload
(
    toolbox::mem::Reference *bufRef
)
throw (rubuilder::fu::exception::Exception)
{
    char                               *blockAddr      = 0;
    char                               *frlHeaderAddr  = 0;
    I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME *block          = 0;
    frlh_t                             *frlHeader      = 0;
    unsigned int                       bufSize         = 0;
    unsigned int                       expectedSegSize = 0;
    unsigned int                       segSize         = 0;


    blockAddr     = (char *)bufRef->getDataLocation();
    frlHeaderAddr = blockAddr + sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);

    block     = (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)blockAddr;
    frlHeader = (frlh_t*)frlHeaderAddr;

    if(block->eventNumber != frlHeader->trigno)
    {
        std::stringstream oss;

        oss << "Event number of FU header does not match that of FRL header";
        oss << " block->eventNumber: " << block->eventNumber;
        oss << " frlHeader->trigno: " << frlHeader->trigno;

        XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
    }

    if(block->blockNb != frlHeader->segno)
    {
        std::stringstream oss;

        oss << "Block number of FU header does not match that of FRL header";
        oss << " block->blockNb: " << block->blockNb;
        oss << " frlHeader->segno: " << frlHeader->segno;

        XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
    }

    if(block->blockNb != blockNb_)
    {
        std::stringstream oss;

        oss << "Incorrect block number.";
        oss << " Expected: " << blockNb_;
        oss << " Received: " << block->blockNb;

        XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
    }

    bufSize = bufRef->getDataSize();
    expectedSegSize = bufSize - sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME) -
                      sizeof(frlh_t);
    segSize = frlHeader->segsize & FRL_SEGSIZE_MASK;

    if(segSize != expectedSegSize)
    {
        std::stringstream oss;

        oss << "RUI FRL header segment size is not as expected.";
        oss << " Expected: " << expectedSegSize;
        oss << " Received: " << segSize;

        XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
    }

    // Check that FU and FRL headers agree on end of super-fragment
    if
    (
        (block->blockNb == (block->nbBlocksInSuperFragment - 1)) !=
        ((frlHeader->segsize & FRL_LAST_SEGM) != 0)
    )
    {
        std::stringstream oss;

        oss << "End of super-fragment of FU header does not match FRL header.";
        oss << " FU header: ";
        oss << (block->blockNb == (block->nbBlocksInSuperFragment - 1));
        oss << " FRL header: ";
        oss << ((frlHeader->segsize & (~FRL_LAST_SEGM)) != 0);

        XCEPT_RAISE(rubuilder::fu::exception::Exception, oss.str());
    }

    // If end of super-fragment
    if(block->blockNb == (block->nbBlocksInSuperFragment - 1))
    {
        blockNb_ = 0;
    }
    else
    {
        blockNb_++;
    }
}


void rubuilder::fu::Application::allocateNEvents
(
    const int n
)
throw (rubuilder::fu::exception::Exception)
{
    toolbox::mem::Reference *bufRef = createBuAllocateMsg
    (
        poolFactory_,
        i2oPool_,
        tid_,
        buTid_,
        n
    );

    // Update parameters showing message payloads and counts
    monitoringInfoSpace_->lock(); appInfoSpace_->lock();
    I2O_BU_ALLOCATE_Payload_.value_ += n * sizeof(BU_ALLOCATE);
    I2O_BU_ALLOCATE_LogicalCount_.value_ += n;
    I2O_BU_ALLOCATE_I2oCount_.value_++;
    monitoringInfoSpace_->unlock(); appInfoSpace_->unlock();

    try
    {
        appContext_->postFrame
        (
            bufRef,
            appDescriptor_,
            buDescriptor_,
            i2oExceptionHandler_,
            buDescriptor_
        );
    }
    catch(xcept::Exception &e)
    {
        std::stringstream oss;

        oss << "Failed to send I2O_BU_ALLOCATE_MESSAGE_FRAME to";
        oss << " BU" << buInstNb_;

        XCEPT_RETHROW(rubuilder::fu::exception::Exception, oss.str(), e);
    }
}


toolbox::mem::Reference *rubuilder::fu::Application::createBuAllocateMsg
(
    toolbox::mem::MemoryPoolFactory *poolFactory,
    toolbox::mem::Pool              *pool,
    const I2O_TID                   taTid,
    const I2O_TID                   buTid,
    const int                       nbEvents
)
throw (rubuilder::fu::exception::Exception)
{
    toolbox::mem::Reference       *bufRef = 0;
    I2O_MESSAGE_FRAME             *stdMsg = 0;
    I2O_PRIVATE_MESSAGE_FRAME     *pvtMsg = 0;
    I2O_BU_ALLOCATE_MESSAGE_FRAME *msg    = 0;
    size_t                        msgSize = 0;


    msgSize = sizeof(I2O_BU_ALLOCATE_MESSAGE_FRAME) +
             (nbEvents - 1) * sizeof(BU_ALLOCATE);

    try
    {
        bufRef = poolFactory->getFrame(pool, msgSize);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(rubuilder::fu::exception::Exception,
            "Failed to get an I2O_BU_ALLOCATE_MESSAGE_FRAME", e);
    }
    bufRef->setDataSize(msgSize);

    stdMsg = (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
    pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
    msg    = (I2O_BU_ALLOCATE_MESSAGE_FRAME*)stdMsg;

    stdMsg->MessageSize      = msgSize >> 2;
    stdMsg->InitiatorAddress = taTid;
    stdMsg->TargetAddress    = buTid;
    stdMsg->Function         = I2O_PRIVATE_MESSAGE;
    stdMsg->VersionOffset    = 0;
    stdMsg->MsgFlags         = 0;  // Point-to-point

    pvtMsg->XFunctionCode    = I2O_BU_ALLOCATE;
    pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;

    msg->n                   = nbEvents;

    // The FU can specify a transaction id for each individual event requested
    for(int i=0; i<nbEvents; i++)
    {
        msg->allocate[i].fuTransactionId = 1234; // Dummy value
        msg->allocate[i].fset            = 0;    // IGNORED!!!
    }

    return bufRef;
}


void rubuilder::fu::Application::discardEvent
(
    const U32 buResourceId
)
throw (rubuilder::fu::exception::Exception)
{
    toolbox::mem::Reference *bufRef = createBuDiscardMsg
    (
        poolFactory_,
        i2oPool_,
        tid_,
        buTid_,
        buResourceId
    );

    // Update parameters showing message payloads and counts
    monitoringInfoSpace_->lock(); appInfoSpace_->lock();
    I2O_BU_DISCARD_Payload_.value_ += sizeof(U32);
    I2O_BU_DISCARD_LogicalCount_.value_++;
    I2O_BU_DISCARD_I2oCount_.value_++;
    monitoringInfoSpace_->unlock(); appInfoSpace_->unlock();

    try
    {
        appContext_->postFrame
        (
            bufRef,
            appDescriptor_,
            buDescriptor_,
            i2oExceptionHandler_,
            buDescriptor_
        );
    }
    catch(xcept::Exception &e)
    {
        std::stringstream oss;

        oss << "Failed to send I2O_BU_DISCARD_MESSAGE_FRAME to";
        oss << " BU" << buInstNb_;

        XCEPT_RETHROW(rubuilder::fu::exception::Exception, oss.str(), e);
    }
}


toolbox::mem::Reference *rubuilder::fu::Application::createBuDiscardMsg
(
    toolbox::mem::MemoryPoolFactory *poolFactory,
    toolbox::mem::Pool              *pool,
    const I2O_TID                   taTid,
    const I2O_TID                   buTid,
    const U32                       buResourceId
)
throw (rubuilder::fu::exception::Exception)
{
    toolbox::mem::Reference      *bufRef  = 0;
    I2O_MESSAGE_FRAME            *stdMsg  = 0;
    I2O_PRIVATE_MESSAGE_FRAME    *pvtMsg  = 0;
    I2O_BU_DISCARD_MESSAGE_FRAME *msg     = 0;
    size_t                        msgSize = 0;


    msgSize = sizeof(I2O_BU_DISCARD_MESSAGE_FRAME);

    try
    {
        bufRef = poolFactory->getFrame(pool, msgSize);
    }
    catch(xcept::Exception &e)
    {
        XCEPT_RETHROW(rubuilder::fu::exception::Exception,
            "Failed to get an I2O_BU_DISCARD_MESSAGE_FRAME", e);
    }
    bufRef->setDataSize(msgSize);

    stdMsg = (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
    pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
    msg    = (I2O_BU_DISCARD_MESSAGE_FRAME*)stdMsg;

    stdMsg->MessageSize      = msgSize >> 2;
    stdMsg->InitiatorAddress = taTid;
    stdMsg->TargetAddress    = buTid;
    stdMsg->Function         = I2O_PRIVATE_MESSAGE;
    stdMsg->VersionOffset    = 0;
    stdMsg->MsgFlags         = 0;  // Point-to-point

    pvtMsg->XFunctionCode    = I2O_BU_DISCARD;
    pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;

    // It is possible to discard more than one event at a time, hence the
    // reason for the array of BU resource ids
    msg->n                   = 1;
    msg->buResourceId[0]     = buResourceId;

    return bufRef;
}


toolbox::mem::Pool
*rubuilder::fu::Application::createHeapAllocatorMemoryPool
(
    toolbox::mem::MemoryPoolFactory *poolFactory,
    const std::string               poolName
)
throw (rubuilder::fu::exception::Exception)
{
    try
    {
        toolbox::net::URN urn("toolbox-mem-pool", poolName);
        toolbox::mem::HeapAllocator* a = new toolbox::mem::HeapAllocator();
        toolbox::mem::Pool *pool = poolFactory->createPool(urn, a);

        return pool;
    }
    catch(xcept::Exception &e)
    {
        std::string s = "Failed to create pool: " + poolName;

        XCEPT_RETHROW(rubuilder::fu::exception::Exception, s, e);
    }
    catch(...)
    {
        std::string s = "Failed to create pool: " + poolName +
                   " : Unknown exception";

        XCEPT_RAISE(rubuilder::fu::exception::Exception, s);
    }
}


std::string rubuilder::fu::Application::getUrl
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


bool rubuilder::fu::Application::onI2oException
(
    xcept::Exception &exception, void *context
)
{
    xdaq::ApplicationDescriptor *destDescriptor =
        (xdaq::ApplicationDescriptor *)context;
    std::string errorMessage =
        createI2oErrorMsg(appDescriptor_, destDescriptor);


    XCEPT_DECLARE_NESTED(rubuilder::fu::exception::Exception,
        sentinelException, errorMessage, exception);
    this->notifyQualified("error", sentinelException);

    LOG4CPLUS_ERROR(logger_,
        errorMessage << " : " << xcept::stdformat_exception_history(exception));

    return true;
}


std::string rubuilder::fu::Application::createI2oErrorMsg
(
    xdaq::ApplicationDescriptor *source,
    xdaq::ApplicationDescriptor *destination
)
{
    std::stringstream oss;


    oss << "I2O exception from ";
    oss << source->getClassName();
    oss << " instance " << source->getInstance();
    oss << " to ";
    oss << destination->getClassName();
    oss << " instance " << destination->getInstance();

    return oss.str();
}


/**
 * Provides the factory method for the instantiation of CLASS applications.
 */
XDAQ_INSTANTIATOR_IMPL(rubuilder::fu::Application)
