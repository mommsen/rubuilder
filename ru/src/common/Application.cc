#include "i2o/Method.h"
#include "interface/shared/i2ogevb2g.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "rubuilder/ru/Application.h"
#include "rubuilder/ru/BUproxy.h"
#include "rubuilder/ru/EVMproxy.h"
#include "rubuilder/ru/RU.h"
#include "rubuilder/ru/RUinput.h"
#include "rubuilder/ru/SuperFragmentTable.h"
#include "rubuilder/ru/version.h"


rubuilder::ru::Application::Application(xdaq::ApplicationStub* s) :
utils::RubuilderApplication<StateMachine>(s,rubuilderru::version,"/rubuilder/images/ru64x64.gif")
{
  superFragmentTable_.reset( new SuperFragmentTable() );
  
  evmProxy_.reset( new EVMproxy(this) );
  ruInput_.reset( new RUinput(this) );
  buProxy_.reset( new BUproxy(this, superFragmentTable_) );    
  ru_.reset( new RU(this, superFragmentTable_, buProxy_, evmProxy_, ruInput_) );
  stateMachine_.reset( new StateMachine(this, buProxy_, evmProxy_, ru_, ruInput_) );

  ru_->registerStateMachine(stateMachine_);
  superFragmentTable_->registerBUproxy(buProxy_);
  
  initialize();

  LOG4CPLUS_INFO(logger_, "End of constructor");
}


void rubuilder::ru::Application::do_appendApplicationInfoSpaceItems
(
  utils::InfoSpaceItems& appInfoSpaceParams
)
{
  i2oEVMRUDataReadyCount_ = 0;
  i2oBUCacheCount_ = 0;
  nbSuperFragmentsReady_ = 0;

  appInfoSpaceParams.add("i2oEVMRUDataReadyCount", &i2oEVMRUDataReadyCount_, utils::InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("i2oBUCacheCount", &i2oBUCacheCount_, utils::InfoSpaceItems::retrieve);
  appInfoSpaceParams.add("nbSuperFragmentsReady", &nbSuperFragmentsReady_, utils::InfoSpaceItems::retrieve);

  buProxy_->appendConfigurationItems(appInfoSpaceParams);
  evmProxy_->appendConfigurationItems(appInfoSpaceParams);
  ru_->appendConfigurationItems(appInfoSpaceParams);
  ruInput_->appendConfigurationItems(appInfoSpaceParams);
  stateMachine_->appendConfigurationItems(appInfoSpaceParams);
}


void rubuilder::ru::Application::do_appendMonitoringInfoSpaceItems
(
  utils::InfoSpaceItems& monitoringParams
)
{
  buProxy_->appendMonitoringItems(monitoringParams);
  evmProxy_->appendMonitoringItems(monitoringParams);
  ru_->appendMonitoringItems(monitoringParams);
  ruInput_->appendMonitoringItems(monitoringParams);
  stateMachine_->appendMonitoringItems(monitoringParams);
}


void rubuilder::ru::Application::do_handleItemChangedEvent(const std::string& item)
{
  if (item == "inputSource")
  {
    ruInput_->inputSourceChanged();
  }
}


void rubuilder::ru::Application::do_handleItemRetrieveEvent(const std::string& item)
{
  if (item == "i2oEVMRUDataReadyCount")
  {
    try
    {
      i2oEVMRUDataReadyCount_.setValue( *(monitoringInfoSpace_->find("i2oEVMRUDataReadyCount")) );
    }
    catch(xdata::exception::Exception& e)
    {
      i2oEVMRUDataReadyCount_ = 0;
    }
  }
  else if (item == "i2oBUCacheCount")
  {
    try
    {
      i2oBUCacheCount_.setValue( *(monitoringInfoSpace_->find("i2oBUCacheCount")) );
    }
    catch(xdata::exception::Exception& e)
    {
      i2oBUCacheCount_ = 0;
    }
  }
  else if (item == "nbSuperFragmentsReady")
  {
    try
    {
      nbSuperFragmentsReady_.setValue( *(monitoringInfoSpace_->find("nbSuperFragmentsReady")) );
    }
    catch(xdata::exception::Exception& e)
    {
      nbSuperFragmentsReady_ = 0;
    }
  }
}


void rubuilder::ru::Application::bindI2oCallbacks()
{
  i2o::bind
    (
      this,
      &rubuilder::ru::Application::I2O_RU_READOUT_Callback,
      I2O_RU_READOUT,
      XDAQ_ORGANIZATION_ID
    );
  
  i2o::bind
    (
      this,
      &rubuilder::ru::Application::I2O_EVMRU_DATA_READY_Callback,
      I2O_DATA_READY,
      XDAQ_ORGANIZATION_ID
    );
  
  i2o::bind
    (
      this,
      &rubuilder::ru::Application::I2O_EVMRU_DATA_READY_Callback,
      I2O_EVMRU_DATA_READY,
      XDAQ_ORGANIZATION_ID
    );
  
  i2o::bind
    (
      this,
      &rubuilder::ru::Application::I2O_RU_SEND_Callback,
      I2O_RU_SEND,
      XDAQ_ORGANIZATION_ID
    );
  
  i2o::bind
    (
      this,
      &rubuilder::ru::Application::I2O_DATA_READY_Callback,
      I2O_DATA_READY,
      XDAQ_ORGANIZATION_ID
    );
}


void rubuilder::ru::Application::I2O_RU_READOUT_Callback
(
  toolbox::mem::Reference *bufRef
)
{
  stateMachine_->processEvent( RuReadout(bufRef) );
}


void rubuilder::ru::Application::I2O_EVMRU_DATA_READY_Callback
(
  toolbox::mem::Reference *bufRef
)
{
  stateMachine_->processEvent( EvmRuDataReady(bufRef) );
}


void rubuilder::ru::Application::I2O_RU_SEND_Callback
(
  toolbox::mem::Reference *bufRef
)
{
  stateMachine_->processEvent( RuSend(bufRef) );
}


void rubuilder::ru::Application::I2O_DATA_READY_Callback
(
  toolbox::mem::Reference *bufRef
)
{
  ruInput_->I2Ocallback( bufRef );
}


void rubuilder::ru::Application::do_updateMonitoringInfo()
{
  buProxy_->updateMonitoringItems();
  evmProxy_->updateMonitoringItems();
  ruInput_->updateMonitoringItems();
  ru_->updateMonitoringItems();
}


void rubuilder::ru::Application::bindNonDefaultXgiCallbacks()
{
  xgi::bind
    (
      this,
      &rubuilder::ru::Application::evbIdFIFOWebPage,
      "evbIdFIFO"
    );
  xgi::bind
    (
      this,
      &rubuilder::ru::Application::blockFIFOWebPage,
      "blockFIFO"
    );
}


void rubuilder::ru::Application::do_defaultWebPage
(
  xgi::Output *out
)
{
  *out << "<tr>"                                                << std::endl;
  *out << "<td class=\"component\">"                            << std::endl;
  ruInput_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_e.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"2\" class=\"component\">"              << std::endl;
  ru_->printHtml(out, monitoringSleepSec_);
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"2\">"                                  << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_e.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "<td rowspan=\"3\" class=\"component\">"              << std::endl;
  buProxy_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td class=\"component\">"                            << std::endl;
  evmProxy_->printHtml(out);
  *out << "</td>"                                               << std::endl;
  *out << "<td>"                                                << std::endl;
  *out << "<img src=\"/rubuilder/images/arrow_e.gif\" alt=\"\"/>"<< std::endl;
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
}


void rubuilder::ru::Application::evbIdFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "evbIdFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  evmProxy_->printEvBidFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


void rubuilder::ru::Application::blockFIFOWebPage
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
  ruInput_->printBlockFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


/**
 * Provides the factory method for the instantiation of RU applications.
 */
XDAQ_INSTANTIATOR_IMPL(rubuilder::ru::Application)



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
