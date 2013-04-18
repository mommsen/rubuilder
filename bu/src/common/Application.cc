#include "i2o/Method.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "rubuilder/bu/Application.h"
#include "rubuilder/bu/BU.h"
#include "rubuilder/bu/DiskWriter.h"
#include "rubuilder/bu/EventTable.h"
#include "rubuilder/bu/EVMproxy.h"
#include "rubuilder/bu/FUproxy.h"
#include "rubuilder/bu/RUproxy.h"
#include "rubuilder/bu/StateMachine.h"
#include "rubuilder/bu/version.h"
#include "rubuilder/utils/Exception.h"
#include "toolbox/mem/Pool.h"
#include "xgi/Method.h"

#include <iomanip>
#include <unistd.h>


rubuilder::bu::Application::Application(xdaq::ApplicationStub *s) :
utils::RubuilderApplication<StateMachine>(s,rubuilderbu::version,"/rubuilder/images/bu64x64.gif")
{
  toolbox::mem::Pool* fastCtrlMsgPool = getFastControlMsgPool();

  diskWriter_.reset( new DiskWriter(this) );
  evmProxy_.reset( new EVMproxy(this, fastCtrlMsgPool) );
  ruProxy_.reset( new RUproxy(this, fastCtrlMsgPool) );
  fuProxy_.reset( new FUproxy(this, fastCtrlMsgPool) );
  eventTable_.reset( new EventTable(this, diskWriter_, evmProxy_, fuProxy_) );
  bu_.reset( new BU(this, evmProxy_, ruProxy_, eventTable_) );
  stateMachine_.reset( new StateMachine(this, bu_,
      evmProxy_, fuProxy_, ruProxy_, diskWriter_, eventTable_) );

  fuProxy_->registerEventTable(eventTable_);
  diskWriter_->registerEventTable(eventTable_);
  bu_->registerStateMachine(stateMachine_);
  diskWriter_->registerStateMachine(stateMachine_);
  eventTable_->registerStateMachine(stateMachine_);
  
  initialize();

  LOG4CPLUS_INFO(logger_, "End of constructor");
}


void rubuilder::bu::Application::do_appendApplicationInfoSpaceItems
(
  utils::InfoSpaceItems& appInfoSpaceParams
)
{
  nbEvtsUnderConstruction_ = 0;
  nbEventsInBU_ = 0;
  nbEvtsReady_ = 0;
  nbEvtsBuilt_ = 0;
  nbEvtsCorrupted_ = 0;
  nbFilesWritten_ = 0;
  usedEvmInstance_ = -1;
  
  appInfoSpaceParams.add("nbEvtsUnderConstruction", &nbEvtsUnderConstruction_, utils::InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbEventsInBU", &nbEventsInBU_, utils::InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbEvtsReady", &nbEvtsReady_, utils::InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbEvtsBuilt", &nbEvtsBuilt_, utils::InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbEvtsCorrupted", &nbEvtsCorrupted_, utils::InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbFilesWritten", &nbFilesWritten_, utils::InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("usedEvmInstance", &usedEvmInstance_, utils::InfoSpaceItems::retrieve);

  bu_->appendConfigurationItems(appInfoSpaceParams);
  evmProxy_->appendConfigurationItems(appInfoSpaceParams);
  ruProxy_->appendConfigurationItems(appInfoSpaceParams);
  fuProxy_->appendConfigurationItems(appInfoSpaceParams);
  diskWriter_->appendConfigurationItems(appInfoSpaceParams);
  eventTable_->appendConfigurationItems(appInfoSpaceParams); 
  stateMachine_->appendConfigurationItems(appInfoSpaceParams);
}


void rubuilder::bu::Application::do_appendMonitoringInfoSpaceItems
(
  utils::InfoSpaceItems& monitoringParams
)
{
  bu_->appendMonitoringItems(monitoringParams);
  evmProxy_->appendMonitoringItems(monitoringParams);
  ruProxy_->appendMonitoringItems(monitoringParams);
  fuProxy_->appendMonitoringItems(monitoringParams);
  diskWriter_->appendMonitoringItems(monitoringParams);
  eventTable_->appendMonitoringItems(monitoringParams); 
  stateMachine_->appendMonitoringItems(monitoringParams);
}


void rubuilder::bu::Application::do_handleItemRetrieveEvent(const std::string& item)
{
  if (item == "nbEvtsUnderConstruction")
  {
    try
    {
      nbEvtsUnderConstruction_.setValue( *(monitoringInfoSpace_->find("nbEvtsUnderConstruction")) );
    }
    catch(xdata::exception::Exception& e)
    {
      nbEvtsUnderConstruction_ = 0;
    }
  }
  else if (item == "nbEventsInBU")
  {
    try
    {
      nbEventsInBU_.setValue( *(monitoringInfoSpace_->find("nbEventsInBU")) );
    }
    catch(xdata::exception::Exception& e)
    {
      nbEventsInBU_ = 0;
    }
  }
  else if (item == "nbEvtsReady")
  {
    try
    {
      nbEvtsReady_.setValue( *(monitoringInfoSpace_->find("nbEvtsReady")) );
    }
    catch(xdata::exception::Exception& e)
    {
      nbEvtsReady_ = 0;
    }
  }
  else if (item == "nbEvtsBuilt")
  {
    try
    {
      nbEvtsBuilt_.setValue( *(monitoringInfoSpace_->find("nbEvtsBuilt")) );
    }
    catch(xdata::exception::Exception& e)
    {
      nbEvtsBuilt_ = 0;
    }
  }
  else if (item == "nbEvtsCorrupted")
  {
    try
    {
      nbEvtsCorrupted_.setValue( *(monitoringInfoSpace_->find("nbEvtsCorrupted")) );
    }
    catch(xdata::exception::Exception& e)
    {
      nbEvtsCorrupted_ = 0;
    }
  }
  else if (item == "nbFilesWritten")
  {
    try
    {
      nbFilesWritten_.setValue( *(monitoringInfoSpace_->find("nbFilesWritten")) );
    }
    catch(xdata::exception::Exception& e)
    {
      nbFilesWritten_ = 0;
    }
  }
  else if (item == "usedEvmInstance")
  {
    if ( evmProxy_.get() )
      usedEvmInstance_ = evmProxy_->getEvmInstance();
    else
      usedEvmInstance_ = -1;
  }
}


void rubuilder::bu::Application::bindI2oCallbacks()
{
  i2o::bind
    (
      this,
      &rubuilder::bu::Application::I2O_BU_CONFIRM_Callback,
      I2O_BU_CONFIRM,
      XDAQ_ORGANIZATION_ID
    );

  i2o::bind
    (
      this,
      &rubuilder::bu::Application::I2O_BU_CACHE_Callback,
      I2O_BU_CACHE,
      XDAQ_ORGANIZATION_ID
    );

  i2o::bind
    (
      this,
      &rubuilder::bu::Application::I2O_BU_ALLOCATE_Callback,
      I2O_BU_ALLOCATE,
      XDAQ_ORGANIZATION_ID
    );

  i2o::bind
    (
      this,
      &rubuilder::bu::Application::I2O_BU_COLLECT_Callback,
      I2O_BU_COLLECT,
      XDAQ_ORGANIZATION_ID
    );

  i2o::bind
    (
      this,
      &rubuilder::bu::Application::I2O_BU_DISCARD_Callback,
      I2O_BU_DISCARD,
      XDAQ_ORGANIZATION_ID
    );

  i2o::bind
    (
      this,
      &rubuilder::bu::Application::I2O_EVM_LUMISECTION_Callback,
      I2O_EVM_LUMISECTION,
      XDAQ_ORGANIZATION_ID
    );

}


void rubuilder::bu::Application::I2O_BU_CONFIRM_Callback
(
  toolbox::mem::Reference *bufRef
)
{
  stateMachine_->processEvent( BuConfirm(bufRef) );
}


void rubuilder::bu::Application::I2O_BU_CACHE_Callback
(
  toolbox::mem::Reference * bufRef
)
{
  stateMachine_->processEvent( BuCache(bufRef) );
}


void rubuilder::bu::Application::I2O_BU_ALLOCATE_Callback
(
  toolbox::mem::Reference * bufRef
)
{
  stateMachine_->processEvent( BuAllocate(bufRef) );
}


void rubuilder::bu::Application::I2O_BU_COLLECT_Callback
(
  toolbox::mem::Reference * bufRef
)
{
  stateMachine_->processEvent( BuCollect(bufRef) );
}


void rubuilder::bu::Application::I2O_BU_DISCARD_Callback
(
  toolbox::mem::Reference * bufRef
)
{
  stateMachine_->processEvent( BuDiscard(bufRef) );
}


void rubuilder::bu::Application::I2O_EVM_LUMISECTION_Callback
(
  toolbox::mem::Reference * bufRef
)
{
  stateMachine_->processEvent( EvmLumisection(bufRef) );
}


void rubuilder::bu::Application::bindNonDefaultXgiCallbacks()
{
  xgi::bind
    (
      this,
      &rubuilder::bu::Application::triggerFIFOWebPage,
      "triggerFIFO"
    );
  
  xgi::bind
    (
      this,
      &rubuilder::bu::Application::completeEventsFIFOWebPage,
      "completeEventsFIFO"
    );
  
  xgi::bind
    (
      this,
      &rubuilder::bu::Application::discardFIFOWebPage,
      "discardFIFO"
    );
  
  xgi::bind
    (
      this,
      &rubuilder::bu::Application::freeResourceIdFIFOWebPage,
      "freeResourceIdFIFO"
    );
  
  xgi::bind
    (
      this,
      &rubuilder::bu::Application::requestFIFOWebPage,
      "requestFIFO"
    );
  
  xgi::bind
    (
      this,
      &rubuilder::bu::Application::blockFIFOWebPage,
      "blockFIFO"
    );
  
  xgi::bind
    (
      this,
      &rubuilder::bu::Application::eventFIFOWebPage,
      "eventFIFO"
    );
  
  xgi::bind
    (
      this,
      &rubuilder::bu::Application::eolsFIFOWebPage,
      "eolsFIFO"
    );
}


void rubuilder::bu::Application::do_updateMonitoringInfo()
{
  diskWriter_->updateMonitoringItems();
  eventTable_->updateMonitoringItems();
  evmProxy_->updateMonitoringItems();
  ruProxy_->updateMonitoringItems();
  fuProxy_->updateMonitoringItems();
  bu_->updateMonitoringItems();
}


void rubuilder::bu::Application::do_defaultWebPage
(
  xgi::Output *out
)
{
  *out << "<tr>"                                                << std::endl;
  *out << "<td class=\"component\">"                            << std::endl;
  evmProxy_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_e.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"3\" class=\"component\">"              << std::endl;
  bu_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"2\">"                                  << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_ew.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"2\" class=\"component\">"              << std::endl;
  fuProxy_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td rowspan=\"3\" class=\"component\">"              << std::endl;
  ruProxy_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"2\">"                                  << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_e.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_ew.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"2\" class=\"component\">"              << std::endl;
  diskWriter_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
}


void rubuilder::bu::Application::triggerFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "triggerFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  evmProxy_->printTriggerFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void rubuilder::bu::Application::completeEventsFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "completeEventsFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  eventTable_->printCompleteEventsFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void rubuilder::bu::Application::discardFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "discardFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  eventTable_->printDiscardFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void rubuilder::bu::Application::freeResourceIdFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "freeResourceIdFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  eventTable_->printFreeResourceIdFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void rubuilder::bu::Application::eventFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "eventFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  diskWriter_->printEventFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void rubuilder::bu::Application::eolsFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "eolsFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  diskWriter_->printEoLSFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void rubuilder::bu::Application::requestFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "requestFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  fuProxy_->printRequestFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void rubuilder::bu::Application::blockFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "blockFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  ruProxy_->printBlockFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
 
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


/**
 * Provides the factory method for the instantiation of BU applications.
 */
XDAQ_INSTANTIATOR_IMPL(rubuilder::bu::Application)



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
