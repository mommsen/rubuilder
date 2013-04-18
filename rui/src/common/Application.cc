#include "i2o/Method.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "rubuilder/rui/Application.h"
#include "rubuilder/rui/RUI.h"
#include "rubuilder/rui/version.h"


rubuilder::rui::Application::Application(xdaq::ApplicationStub* s) :
utils::RubuilderApplication<StateMachine>(s,rubuilderrui::version,"/rubuilder/images/rui64x64.gif")
{
  rui_.reset( new RUI(this) );
  stateMachine_.reset( new StateMachine(this, rui_) );

  initialize();

  LOG4CPLUS_INFO(logger_, "End of constructor");
}


void rubuilder::rui::Application::do_appendApplicationInfoSpaceItems
(
  utils::InfoSpaceItems& appInfoSpaceParams
)
{
  i2oEVMRUDataReadyCount_ = 0;
  
  appInfoSpaceParams.add("i2oEVMRUDataReadyCount", &i2oEVMRUDataReadyCount_, utils::InfoSpaceItems::retrieve);
  
  rui_->appendConfigurationItems(appInfoSpaceParams);
  stateMachine_->appendConfigurationItems(appInfoSpaceParams);
}


void rubuilder::rui::Application::do_appendMonitoringInfoSpaceItems
(
  utils::InfoSpaceItems& monitoringParams
)
{
  rui_->appendMonitoringItems(monitoringParams);
  stateMachine_->appendMonitoringItems(monitoringParams);
}


void rubuilder::rui::Application::do_handleItemRetrieveEvent(const std::string& item)
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
}


void rubuilder::rui::Application::do_updateMonitoringInfo()
{
  rui_->updateMonitoringItems();
}


void rubuilder::rui::Application::bindNonDefaultXgiCallbacks()
{
  xgi::bind
    (
      this,
      &rubuilder::rui::Application::fragmentFIFOWebPage,
      "fragmentFIFO"
    );
}


void rubuilder::rui::Application::do_defaultWebPage
(
  xgi::Output *out
)
{
  *out << "<tr>"                                                << std::endl;
  *out << "<td class=\"component\">"                            << std::endl;
  rui_->printHtml(out,monitoringSleepSec_);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
}


void rubuilder::rui::Application::fragmentFIFOWebPage
(
  xgi::Input  *in,
  xgi::Output *out
)
{
  webPageHeader(out, "fragmentFIFO");

  *out << "<table class=\"layout\">"                            << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  webPageBanner(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "<tr>"                                                << std::endl;
  *out << "<td>"                                                << std::endl;
  rui_->printFragmentFIFO(out);
  *out << "</td>"                                               << std::endl;
  *out << "</tr>"                                               << std::endl;
  
  *out << "</table>"                                            << std::endl;
  
  *out << "</body>"                                             << std::endl;
  *out << "</html>"                                             << std::endl;
}


/**
 * Provides the factory method for the instantiation of RU applications.
 */
XDAQ_INSTANTIATOR_IMPL(rubuilder::rui::Application)



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
