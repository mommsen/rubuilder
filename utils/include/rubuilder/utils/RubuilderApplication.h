#ifndef _rubuilder_utils_RubuilderApplication_h_
#define _rubuilder_utils_RubuilderApplication_h_

#include <boost/shared_ptr.hpp>

#include <string>

#include "log4cplus/logger.h"

#include "rubuilder/utils/Exception.h"
#include "rubuilder/utils/CreateStrings.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "rubuilder/utils/RubuilderStateMachine.h"
#include "rubuilder/utils/TimerManager.h"
#include "toolbox/mem/HeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/net/URN.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WorkLoop.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/Exception.h"
#include "xcept/tools.h"
#include "xdaq/ApplicationDescriptor.h"
#include "xdaq/ApplicationStub.h"
#include "xdaq/NamespaceURI.h"
#include "xdaq/WebApplication.h"
#include "xdaq2rc/SOAPParameterExtractor.hh"
#include "xdata/ActionListener.h"
#include "xdata/InfoSpace.h"
#include "xdata/InfoSpaceFactory.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"
#include "xgi/Input.h"
#include "xgi/Method.h"
#include "xgi/Output.h"
#include "xoap/MessageFactory.h"
#include "xoap/MessageReference.h"
#include "xoap/Method.h"
#include "xoap/SOAPBody.h"
#include "xoap/SOAPEnvelope.h"
#include "xoap/SOAPName.h"
#include "xoap/SOAPPart.h"


namespace rubuilder { namespace utils { // namespace rubuilder::utils

/**
 * \ingroup xdaqApps
 * \brief Generic rubuilder application.
 */
template<class StateMachine>
class RubuilderApplication :
  public xdaq::WebApplication,
  public xdata::ActionListener
{
public:

  /**
   * Constructor.
   */
  RubuilderApplication
  (
    xdaq::ApplicationStub*,
    const std::string& codeVersion,
    const std::string& appIcon
  );


protected:

  void initialize();

  virtual void do_appendApplicationInfoSpaceItems(utils::InfoSpaceItems&) = 0;
  virtual void do_appendMonitoringInfoSpaceItems(utils::InfoSpaceItems&) = 0;
  virtual void do_updateMonitoringInfo() = 0;
  
  virtual void do_handleItemChangedEvent(const std::string& item) {};
  virtual void do_handleItemRetrieveEvent(const std::string& item) {};

  virtual void bindI2oCallbacks() {};

  virtual void bindNonDefaultXgiCallbacks() {};
  virtual void do_defaultWebPage(xgi::Output*) = 0;

  toolbox::mem::Pool* getFastControlMsgPool();
  xoap::MessageReference createFsmSoapResponseMsg
  (
    const std::string& event,
    const std::string& state
  );

  void webPageHeader(xgi::Output*, const std::string& name);
  void webPageBanner(xgi::Output*);
  void printWebPageIcon
  (
    xgi::Output*,
    const std::string& imgSrc,
    const std::string& label,
    const std::string& href
  );
  
  const std::string codeVersion_;
  const std::string appIcon_;
  log4cplus::Logger logger_;
  
  xdata::InfoSpace *monitoringInfoSpace_;

  boost::shared_ptr<StateMachine> stateMachine_;
  xdaq2rc::SOAPParameterExtractor soapParameterExtractor_;

  std::string urn_;
  std::string xmlClass_;
  xdata::UnsignedInteger32 instance_;
  xdata::String stateName_;
  
  xdata::UnsignedInteger32 monitoringSleepSec_;
  
  
private:

  void initApplicationInfoSpace();
  void initMonitoringInfoSpace();
  void startMonitoring();
  void bindSoapCallbacks();
  void bindXgiCallbacks();
  xoap::MessageReference processSoapFsmEvent(xoap::MessageReference msg);
  
  void startMonitoringWorkloop();
  bool updateMonitoringInfo(toolbox::task::WorkLoop*);

  void appendApplicationInfoSpaceItems(utils::InfoSpaceItems&);
  void appendMonitoringInfoSpaceItems(utils::InfoSpaceItems&);
  void actionPerformed(xdata::Event&);
  void handleItemChangedEvent(const std::string& item);
  void handleItemRetrieveEvent(const std::string& item);

  void defaultWebPage(xgi::Input*, xgi::Output*);

}; // template class RubuilderApplication

} } // namespace rubuilder::utils


////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class StateMachine>
rubuilder::utils::RubuilderApplication<StateMachine>::RubuilderApplication
(
  xdaq::ApplicationStub* s,
  const std::string& codeVersion,
  const std::string& appIcon
) :
xdaq::WebApplication(s),
codeVersion_(codeVersion),
appIcon_(appIcon),
logger_(getApplicationLogger()),
soapParameterExtractor_(this)
{
  xdaq::ApplicationDescriptor* appDescriptor = getApplicationDescriptor();
  xmlClass_ = appDescriptor->getClassName();
  instance_ = appDescriptor->getInstance();
  urn_ = appDescriptor->getURN();

  appDescriptor->setAttribute("icon", appIcon_);
}


template<class StateMachine>
void rubuilder::utils::RubuilderApplication<StateMachine>::initialize()
{
  initApplicationInfoSpace();
  initMonitoringInfoSpace();
  startMonitoring();
  
  bindSoapCallbacks();
  bindI2oCallbacks();
  bindXgiCallbacks();
}


template<class StateMachine>
void rubuilder::utils::RubuilderApplication<StateMachine>::initApplicationInfoSpace()
{
  try
  {
    InfoSpaceItems appInfoSpaceParams;
    xdata::InfoSpace *appInfoSpace = getApplicationInfoSpace();

    appendApplicationInfoSpaceItems(appInfoSpaceParams);
 
    appInfoSpaceParams.putIntoInfoSpace(appInfoSpace, this);
  }
  catch(xcept::Exception &e)
  {
    const std::string msg = "Failed to put parameters into application info space";
    LOG4CPLUS_ERROR(logger_,
      msg << xcept::stdformat_exception_history(e));
    
    // Notify the sentinel
    XCEPT_DECLARE_NESTED(exception::Monitoring,
      sentinelException, msg, e);
    notifyQualified("error",sentinelException);
  }
}


template<class StateMachine>
void rubuilder::utils::RubuilderApplication<StateMachine>::appendApplicationInfoSpaceItems
(
  InfoSpaceItems& params
)
{
  stateName_ = "Halted";
  monitoringSleepSec_ = 1;

  params.add("stateName", &stateName_, utils::InfoSpaceItems::retrieve);
  params.add("monitoringSleepSec", &monitoringSleepSec_);

  do_appendApplicationInfoSpaceItems(params);
}


template<class StateMachine>
void rubuilder::utils::RubuilderApplication<StateMachine>::initMonitoringInfoSpace()
{
  try
  {
    InfoSpaceItems monitoringParams;
    
    appendMonitoringInfoSpaceItems(monitoringParams);

    // Create info space for monitoring
    const std::string monitoringInfoSpaceName =
      generateMonitoringInfoSpaceName(xmlClass_, instance_);
    toolbox::net::URN urn =
      createQualifiedInfoSpace(monitoringInfoSpaceName);
    monitoringInfoSpace_ = xdata::getInfoSpaceFactory()->get(urn.toString());

    monitoringParams.putIntoInfoSpace(monitoringInfoSpace_, this);
  }
  catch(xcept::Exception &e)
  {
    const std::string msg = "Failed to put parameters into monitoring info space";
    LOG4CPLUS_ERROR(logger_,
      msg << xcept::stdformat_exception_history(e));
    
    // Notify the sentinel
    XCEPT_DECLARE_NESTED(exception::Monitoring,
      sentinelException, msg, e);
    notifyQualified("error",sentinelException);
  }
}


template<class StateMachine>
void rubuilder::utils::RubuilderApplication<StateMachine>::appendMonitoringInfoSpaceItems
(
  InfoSpaceItems& items
)
{
  do_appendMonitoringInfoSpaceItems(items);
}


template<class StateMachine>
void rubuilder::utils::RubuilderApplication<StateMachine>::actionPerformed(xdata::Event& ispaceEvent)
{
  try
  {
    if (ispaceEvent.type() == "ItemChangedEvent")
    {
      const std::string item =
        dynamic_cast<xdata::ItemChangedEvent&>(ispaceEvent).itemName();
      handleItemChangedEvent(item);
    }
    else if (ispaceEvent.type() == "ItemRetrieveEvent")
    {
      const std::string item =
        dynamic_cast<xdata::ItemRetrieveEvent&>(ispaceEvent).itemName();
      handleItemRetrieveEvent(item);
    }
  }
  catch(xcept::Exception &e)
  {
    std::ostringstream msg;
    msg << "Failed to perform action for " << ispaceEvent.type();
    XCEPT_DECLARE_NESTED(exception::Monitoring,
      sentinelException, msg.str(), e);

    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
}


template<class StateMachine>
void rubuilder::utils::RubuilderApplication<StateMachine>::handleItemChangedEvent(const std::string& item)
{
  do_handleItemChangedEvent(item);
}


template<class StateMachine>
void rubuilder::utils::RubuilderApplication<StateMachine>::handleItemRetrieveEvent(const std::string& item)
{
  if (item == "stateName")
  {
    stateName_ = stateMachine_->getStateName();
  }
  else
  {
    do_handleItemRetrieveEvent(item);
  }
}


template<class StateMachine>
void rubuilder::utils::RubuilderApplication<StateMachine>::bindSoapCallbacks()
{
  typename StateMachine::SoapFsmEvents soapFsmEvents =
    stateMachine_->getSoapFsmEvents();
  
  for (typename StateMachine::SoapFsmEvents::const_iterator it = soapFsmEvents.begin(),
         itEnd = soapFsmEvents.end(); it != itEnd; ++it)
  {
    xoap::bind(
      this,
      &rubuilder::utils::RubuilderApplication<StateMachine>::processSoapFsmEvent,
      *it,
      XDAQ_NS_URI
    );
  }
}


template<class StateMachine>
xoap::MessageReference rubuilder::utils::RubuilderApplication<StateMachine>::processSoapFsmEvent
(
  xoap::MessageReference msg
)
{
  std::string event = "";
  std::string newState = "unknown";  

  try
  {
    event = soapParameterExtractor_.extractParameters(msg);
  }
  catch(xcept::Exception &e)
  {
    std::ostringstream errorMsg;
    errorMsg << "Failed to extract FSM event and parameters from SOAP message: ";
    msg->writeTo(errorMsg);
    XCEPT_DECLARE_NESTED(exception::FSM, sentinelException, errorMsg.str(), e);
    newState = stateMachine_->processFSMEvent( Fail(sentinelException) );
  }

  stateMachine_->processSoapEvent(event, newState);

  return createFsmSoapResponseMsg(event, newState);
}


template<class StateMachine>
void rubuilder::utils::RubuilderApplication<StateMachine>::startMonitoring()
{
  try
  {
    startMonitoringWorkloop();
  }
  catch(xcept::Exception &e)
  {
    const std::string msg = "Failed to start monitoring work loop";
    XCEPT_DECLARE_NESTED(exception::Monitoring,
      sentinelException, msg, e);
    stateMachine_->processFSMEvent( Fail(sentinelException) );
  }
}


template<class StateMachine>
void rubuilder::utils::RubuilderApplication<StateMachine>::startMonitoringWorkloop()
{
  const std::string monitoringWorkLoopName =
    generateMonitoringWorkLoopName(xmlClass_, instance_);
  
  toolbox::task::WorkLoop* monitoringWorkLoop = toolbox::task::getWorkLoopFactory()->getWorkLoop
    (
      monitoringWorkLoopName,
      "waiting"
    );
  
  const std::string monitoringActionName =
    generateMonitoringActionName(xmlClass_, instance_);
  
  toolbox::task::ActionSignature* monitoringActionSignature =
    toolbox::task::bind
    (
      this,
      &rubuilder::utils::RubuilderApplication<StateMachine>::updateMonitoringInfo,
      monitoringActionName
    );
  
  try
  {
    monitoringWorkLoop->submit(monitoringActionSignature);
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::WorkLoop,
      "Failed to submit action to work loop: " + monitoringWorkLoopName,
      e);
  }
  
  try
  {
    monitoringWorkLoop->activate();
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::WorkLoop,
      "Failed to activate work loop: " + monitoringWorkLoopName, e);
  }
}


template<class StateMachine>
bool rubuilder::utils::RubuilderApplication<StateMachine>::updateMonitoringInfo
(
  toolbox::task::WorkLoop* wl
)
{
  std::string errorMsg = "Failed to update monitoring counters: ";
  
  try
  {
    monitoringInfoSpace_->lock();
    
    stateMachine_->updateMonitoringItems();
    do_updateMonitoringInfo();
    
    monitoringInfoSpace_->unlock();
  }
  catch(xcept::Exception &e)
  {
    monitoringInfoSpace_->unlock();
    
    LOG4CPLUS_ERROR(logger_,
      errorMsg << xcept::stdformat_exception_history(e));
    
    XCEPT_DECLARE_NESTED(exception::Monitoring,
      sentinelException, errorMsg, e);
    notifyQualified("error",sentinelException);
  }
  catch(std::exception &e)
  {
    monitoringInfoSpace_->unlock();
    
    errorMsg += e.what();
    
    LOG4CPLUS_ERROR(logger_, errorMsg);
    
    XCEPT_DECLARE(exception::Monitoring,
      sentinelException, errorMsg );
    notifyQualified("error",sentinelException);
  }
  catch(...)
  {
    monitoringInfoSpace_->unlock();
    
    errorMsg += "Unknown exception";

    LOG4CPLUS_ERROR(logger_, errorMsg);
    
    XCEPT_DECLARE(exception::Monitoring,
      sentinelException, errorMsg );
    notifyQualified("error",sentinelException);
  }
  
  ::sleep(monitoringSleepSec_.value_);
  
  // Reschedule this action code
  return true;
}


template<class StateMachine>
void rubuilder::utils::RubuilderApplication<StateMachine>::bindXgiCallbacks()
{
  xgi::bind
    (
      this,
      &rubuilder::utils::RubuilderApplication<StateMachine>::defaultWebPage,
      "Default"
    );

  bindNonDefaultXgiCallbacks();
}


template<class StateMachine>
void rubuilder::utils::RubuilderApplication<StateMachine>::defaultWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "MAIN");
  
  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<colgroup>"                                          << std::endl;
  *out << "<col/>"                                              << std::endl;
  *out << "<col class=\"arrow\"/>"                              << std::endl;
  *out << "<col/>"                                              << std::endl;
  *out << "<col class=\"arrow\"/>"                              << std::endl;
  *out << "<col/>"                                              << std::endl;
  *out << "</colgroup>"                                         << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td colspan=\"5\">"                                  << std::endl;
  
  webPageBanner(out);
  
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;

  do_defaultWebPage(out);
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


template<class StateMachine>
xoap::MessageReference rubuilder::utils::RubuilderApplication<StateMachine>::createFsmSoapResponseMsg
(
    const std::string& event,
    const std::string& state
)
{
  try
  {
    xoap::MessageReference message = xoap::createMessage();
    xoap::SOAPEnvelope envelope = message->getSOAPPart().getEnvelope();
    xoap::SOAPBody body = envelope.getBody();
    std::string responseString = event + "Response";
    xoap::SOAPName responseName =
      envelope.createName(responseString, "xdaq", XDAQ_NS_URI);
    xoap::SOAPBodyElement responseElement =
      body.addBodyElement(responseName);
    xoap::SOAPName stateName =
      envelope.createName("state", "xdaq", XDAQ_NS_URI);
    xoap::SOAPElement stateElement =
      responseElement.addChildElement(stateName);
    xoap::SOAPName attributeName =
      envelope.createName("stateName", "xdaq", XDAQ_NS_URI);
    
    stateElement.addAttribute(attributeName, state);
    
    return message;
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::SOAP,
      "Failed to create FSM SOAP response message for event:" +
      event + " and result state:" + state,  e);
  }
}


template<class StateMachine>
toolbox::mem::Pool* rubuilder::utils::RubuilderApplication<StateMachine>::getFastControlMsgPool()
{
  toolbox::mem::Pool* fastCtrlMsgPool = 0;
  
  try
  {
    toolbox::net::URN urn("toolbox-mem-pool", "udapl");
    fastCtrlMsgPool = toolbox::mem::getMemoryPoolFactory()->findPool(urn);
  }
  catch (toolbox::mem::exception::MemoryPoolNotFound)
  {
    const std::string fastCtrlMsgPoolName =
      generateFastControlMsgPoolName(xmlClass_,instance_);
    
    toolbox::net::URN urn("toolbox-mem-pool", fastCtrlMsgPoolName);
    toolbox::mem::HeapAllocator* a = new toolbox::mem::HeapAllocator();
    
    fastCtrlMsgPool = toolbox::mem::getMemoryPoolFactory()->createPool(urn, a);
  }
  catch (toolbox::mem::exception::Exception e)
  {
    XCEPT_RETHROW(exception::OutOfMemory, "Failed to create fast control message memory pool", e);
  }
  
  return fastCtrlMsgPool;
}


template<class StateMachine>
void rubuilder::utils::RubuilderApplication<StateMachine>::webPageHeader
(
  xgi::Output* out,
  const std::string& name
)
{
  *out << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\""          << std::endl;
  *out << "    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"          << std::endl;

  *out << "<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"en\" xml:lang=\"en\">" << std::endl;
  *out << "<head>"                                                                    << std::endl;
  *out << "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\"/>"   << std::endl;
  *out << "<link type=\"text/css\" rel=\"stylesheet\"";
  *out << " href=\"/rubuilder/html/rubuilder.css\"/>"                                 << std::endl;
  *out << "<title>"                                                                   << std::endl;
  *out << xmlClass_ << " " << instance_ << " - " << name                              << std::endl;
  *out << "</title>"                                                                  << std::endl;
  *out << "</head>"                                                                   << std::endl;
  *out << "<body>"                                                                    << std::endl;
}


template<class StateMachine>
void rubuilder::utils::RubuilderApplication<StateMachine>::webPageBanner
(
  xgi::Output* out
)  
{
  *out << "<div class=\"header\">"                                                    << std::endl;
  *out << "<table border=\"0\" width=\"100%\">"                                       << std::endl;
  *out << "<tr>"                                                                      << std::endl;
  *out << "<td class=\"icon\">"                                                       << std::endl;
  *out << "<img src=\"" << appIcon_ << "\" alt=\"EVM\"/>"                             << std::endl;
  *out << "<p>created by "
    <<"<a href=\"mailto:Remigius.Mommsen@cern.ch\">R.K.&nbsp;Mommsen</a>"
    << " &amp;&nbsp;S.&nbsp;Murray</p>"                                               << std::endl;
  *out << "</td>"                                                                     << std::endl;

  *out << "<td>"                                                                      << std::endl;
  *out << "<table width=\"100%\" border=\"0\">"                                       << std::endl;
  *out << "<tr>"                                                                      << std::endl;
  *out << "<td>"                                                                      << std::endl;
  *out << "<b>" << xmlClass_ << " " << instance_ << "</b>"                            << std::endl;
  *out << "</td>"                                                                     << std::endl;
  *out << "<td align=\"right\">"                                                      << std::endl;
  *out << "<b>" << stateMachine_->getStateName() << "</b>"                            << std::endl;
  *out << "</td>"                                                                     << std::endl;
  *out << "</tr>"                                                                     << std::endl;
  *out << "<tr>"                                                                      << std::endl;
  *out << "<td>"                                                                      << std::endl;
  *out << "Version " << codeVersion_                                                  << std::endl;
  *out << "</td>"                                                                     << std::endl;
  *out << "<td></td>"                                                                 << std::endl;
  *out << "</tr>"                                                                     << std::endl;
  *out << "<tr>"                                                                      << std::endl;
  *out << "<td>"                                                                      << std::endl;
  *out << "Page last updated: " << getCurrentTimeUTC() << " UTC"                      << std::endl;
  *out << "</td>"                                                                     << std::endl;
  *out << "<td align=\"right\">"                                                      << std::endl;
  *out << "<a href=\"/" << urn_ << "/ParameterQuery\">XML</a>"                        << std::endl;
  *out << "</td>"                                                                     << std::endl;
  *out << "</tr>"                                                                     << std::endl;
  *out << "</table>"                                                                  << std::endl;
  *out << "</td>"                                                                     << std::endl;
  *out << "<td class=\"app_links\">"                                                  << std::endl;
  printWebPageIcon(out, "/hyperdaq/images/HyperDAQ.jpg",
    "HyperDAQ", "/urn:xdaq-application:service=hyperdaq");
  *out << "</td>"                                                                     << std::endl;
  
  *out << "<td class=\"app_links\">"                                                  << std::endl;
  printWebPageIcon(out, appIcon_, "Main", "/" + urn_ + "/");
  *out << "</td>"                                                                     << std::endl;
  
  *out << "</tr>"                                                                     << std::endl;
  *out << "</table>"                                                                  << std::endl;
  *out << "</div>"                                                                    << std::endl;
}


template<class StateMachine>
void rubuilder::utils::RubuilderApplication<StateMachine>::printWebPageIcon
(
  xgi::Output* out,
  const std::string& imgSrc,
  const std::string& label,
  const std::string& href
)
{
  *out << "<a href=\"" << href << "\">";
  *out << "<img style=\"border-style:none\"";
  *out << " src=\"" << imgSrc << "\"";
  *out << " alt=\"" << label << "\"";
  *out << " width=\"64\"";
  *out << " height=\"64\"/>";
  *out << "</a><br/>" << std::endl;
  *out << "<a href=\"" << href << "\">" << label << "</a>";
  *out << std::endl;
}


#endif


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
