#include "i2o/Method.h"
#include "interface/shared/i2ogevb2g.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "rubuilder/evm/Application.h"
#include "rubuilder/evm/BUproxy.h"
#include "rubuilder/evm/EoLSHandler.h"
#include "rubuilder/evm/EVM.h"
#include "rubuilder/evm/L1InfoHandler.h"
#include "rubuilder/evm/RUproxy.h"
#include "rubuilder/evm/SMproxy.h"
#include "rubuilder/evm/StateMachine.h"
#include "rubuilder/evm/TRGproxy.h"
#include "rubuilder/evm/version.h"
#include "toolbox/mem/Pool.h"

#include <iomanip>
#include <unistd.h>


rubuilder::evm::Application::Application(xdaq::ApplicationStub *s) :
utils::RubuilderApplication<StateMachine>(s,rubuilderevm::version,"/rubuilder/images/evm64x64.gif")
{
  toolbox::mem::Pool* fastCtrlMsgPool = getFastControlMsgPool();

  eolsHandler_.reset( new EoLSHandler(this, fastCtrlMsgPool) );
  l1InfoHandler_.reset( new L1InfoHandler(this, eolsHandler_, fastCtrlMsgPool) );
  trgProxy_.reset( new TRGproxy(this, fastCtrlMsgPool) );
  ruProxy_.reset( new RUproxy(this, fastCtrlMsgPool) );
  buProxy_.reset( new BUproxy(this, eolsHandler_, fastCtrlMsgPool) );
  smProxy_.reset( new SMproxy(this, eolsHandler_, fastCtrlMsgPool) );
  evm_.reset( new EVM(this, trgProxy_, ruProxy_, buProxy_, l1InfoHandler_) );
  stateMachine_.reset( new StateMachine(this, eolsHandler_, l1InfoHandler_,
      trgProxy_, ruProxy_, buProxy_, smProxy_, evm_) );

  evm_->registerStateMachine(stateMachine_);
  
  initialize();

  LOG4CPLUS_INFO(logger_, "End of constructor");
}


void rubuilder::evm::Application::do_appendApplicationInfoSpaceItems
(
  utils::InfoSpaceItems& appInfoSpaceParams
)
{
  nbTriggers_ = 0;
  nbEvtsBuilt_ = 0;
  ruBuilderIsFlushed_ = true;
  
  appInfoSpaceParams.add("nbTriggers", &nbTriggers_, utils::InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbEvtsBuilt", &nbEvtsBuilt_, utils::InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("ruBuilderIsFlushed", &ruBuilderIsFlushed_, utils::InfoSpaceItems::retrieve);

  evm_->appendConfigurationItems(appInfoSpaceParams);
  trgProxy_->appendConfigurationItems(appInfoSpaceParams);
  buProxy_->appendConfigurationItems(appInfoSpaceParams);
  ruProxy_->appendConfigurationItems(appInfoSpaceParams);
  smProxy_->appendConfigurationItems(appInfoSpaceParams);
  l1InfoHandler_->appendConfigurationItems(appInfoSpaceParams);
  eolsHandler_->appendConfigurationItems(appInfoSpaceParams);
  stateMachine_->appendConfigurationItems(appInfoSpaceParams);
}


void rubuilder::evm::Application::do_appendMonitoringInfoSpaceItems
(
  utils::InfoSpaceItems& monitoringParams
)
{
  evm_->appendMonitoringItems(monitoringParams);
  trgProxy_->appendMonitoringItems(monitoringParams);
  buProxy_->appendMonitoringItems(monitoringParams);
  ruProxy_->appendMonitoringItems(monitoringParams);
  smProxy_->appendMonitoringItems(monitoringParams);
  l1InfoHandler_->appendMonitoringItems(monitoringParams);
  eolsHandler_->appendMonitoringItems(monitoringParams);
  stateMachine_->appendMonitoringItems(monitoringParams);
}


void rubuilder::evm::Application::do_handleItemChangedEvent(const std::string& item)
{
  if (item == "triggerSource")
  {
    trgProxy_->triggerSourceChanged();
  }
}


void rubuilder::evm::Application::do_handleItemRetrieveEvent(const std::string& item)
{
  if (item == "nbTriggers")
  {
    try
    {
      const uint64_t i2oEVMTriggerCount =
        static_cast<xdata::UnsignedInteger64*>(monitoringInfoSpace_->find("i2oEVMTriggerCount"))->value_;
      nbTriggers_ = static_cast<uint32_t>(i2oEVMTriggerCount);
    }
    catch(xdata::exception::Exception& e)
    {
      nbTriggers_ = 0;
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
  else if (item == "ruBuilderIsFlushed")
  {
    try
    {
      ruBuilderIsFlushed_.setValue( *(monitoringInfoSpace_->find("ruBuilderIsFlushed")) );
    }
    catch(xdata::exception::Exception& e)
    {
      ruBuilderIsFlushed_ = false;
    }
  }
}


void rubuilder::evm::Application::bindI2oCallbacks()
{
  i2o::bind
    (
      this,
      &rubuilder::evm::Application::I2O_EVM_TRIGGER_Callback,
      I2O_EVM_TRIGGER,
      XDAQ_ORGANIZATION_ID
    );
  
  i2o::bind
    (
      this,
      &rubuilder::evm::Application::I2O_EVM_TRIGGER_Callback,
      I2O_EVMRU_DATA_READY,
      XDAQ_ORGANIZATION_ID
    );
  
  i2o::bind
    (
      this,
      &rubuilder::evm::Application::I2O_EVM_TRIGGER_Callback,
      I2O_DATA_READY,
      XDAQ_ORGANIZATION_ID
    );
  
  i2o::bind
    (
      this,
      &rubuilder::evm::Application::I2O_EVM_ALLOCATE_CLEAR_Callback,
      I2O_EVM_ALLOCATE_CLEAR,
      XDAQ_ORGANIZATION_ID
    );
}


void rubuilder::evm::Application::I2O_EVM_TRIGGER_Callback
(
  toolbox::mem::Reference *bufRef
)
{
  stateMachine_->processEvent( EvmTrigger(bufRef) );
}


void rubuilder::evm::Application::I2O_EVM_ALLOCATE_CLEAR_Callback
(
  toolbox::mem::Reference * bufRef
)
{
  stateMachine_->processEvent( EvmAllocateClear(bufRef) );
}


void rubuilder::evm::Application::bindNonDefaultXgiCallbacks()
{
  xgi::bind
    (
      this,
      &rubuilder::evm::Application::triggerFIFOWebPage,
      "triggerFIFO"
    );
  
  xgi::bind
    (
      this,
      &rubuilder::evm::Application::eventFIFOWebPage,
      "eventFIFO"
    );
  
  xgi::bind
    (
      this,
      &rubuilder::evm::Application::releasedEvbIdFIFOWebPage,
      "releasedEvbIdFIFO"
    );
  
  xgi::bind
    (
      this,
      &rubuilder::evm::Application::requestFIFOsWebPage,
      "requestFIFOs"
    );
  
  xgi::bind
    (
      this,
      &rubuilder::evm::Application::eolsFIFOsWebPage,
      "eolsFIFOs"
    );
  
  xgi::bind
    (
      this,
      &rubuilder::evm::Application::l1InfoFIFOWebPage,
      "l1InfoFIFO"
    );
  
  xgi::bind
    (
      this,
      &rubuilder::evm::Application::lumiSectionInfoFIFOWebPage,
      "lumiSectionInfoFIFO"
    );
  
  xgi::bind
    (
      this,
      &rubuilder::evm::Application::completedLumiSectionFIFOWebPage,
      "completedLumiSectionFIFO"
    );
  
  xgi::bind
    (
      this,
      &rubuilder::evm::Application::lastL1InfoWebPage,
      "lastL1Info"
    );
}


void rubuilder::evm::Application::do_updateMonitoringInfo()
{
  eolsHandler_->updateMonitoringItems();
  l1InfoHandler_->updateMonitoringItems();
  trgProxy_->updateMonitoringItems();
  ruProxy_->updateMonitoringItems();
  buProxy_->updateMonitoringItems();
  smProxy_->updateMonitoringItems();
  evm_->updateMonitoringItems();
}


void rubuilder::evm::Application::do_defaultWebPage
(
  xgi::Output *out
)
{
  *out << "<tr>"                                                << std::endl;
  *out << "<td rowspan=\"4\" class=\"component\">"              << std::endl;
  trgProxy_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"2\">"                                  << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_e.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"2\" class=\"component\">"              << std::endl;
  evm_->printHtml(out, monitoringSleepSec_);
  *out << "</td>"                                               << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_e.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td class=\"component\">"                            << std::endl;
  ruProxy_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_ew.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"6\" class=\"component\">"              << std::endl;
  buProxy_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td></td>"                                           << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_s.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td></td>"                                           << std::endl;
  *out << "<td rowspan=\"2\" class=\"component\">"              << std::endl;
  l1InfoHandler_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td rowspan=\"3\" class=\"component\">"              << std::endl;
  smProxy_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td></td>"                                           << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_s.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_w.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td class=\"component\">"                            << std::endl;
  eolsHandler_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_ew.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
}


void rubuilder::evm::Application::triggerFIFOWebPage
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
  trgProxy_->printTriggerFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void rubuilder::evm::Application::eventFIFOWebPage
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
  buProxy_->printEventFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void rubuilder::evm::Application::requestFIFOsWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "requestFIFOs");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  buProxy_->printRequestFIFOs(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void rubuilder::evm::Application::l1InfoFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "l1InfoFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  l1InfoHandler_->printL1InfoFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
 
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void rubuilder::evm::Application::lumiSectionInfoFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "lumiSectionInfoFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  l1InfoHandler_->printLumiSectionInfoFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void rubuilder::evm::Application::releasedEvbIdFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "releasedEvbIdFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  buProxy_->printReleasedEvbIdFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void rubuilder::evm::Application::eolsFIFOsWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "eolsFIFOs");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  buProxy_->printEoLSFIFOs(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void rubuilder::evm::Application::completedLumiSectionFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "completedLumiSectionFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  eolsHandler_->printCompletedLumiSectionFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void rubuilder::evm::Application::lastL1InfoWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "lastL1Info");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  l1InfoHandler_->printL1Scalers(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


/**
 * Provides the factory method for the instantiation of EVM applications.
 */
XDAQ_INSTANTIATOR_IMPL(rubuilder::evm::Application)



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
