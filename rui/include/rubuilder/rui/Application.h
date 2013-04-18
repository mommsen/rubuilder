#ifndef _rubuilder_rui_Application_h_
#define _rubuilder_rui_Application_h_

#include <boost/shared_ptr.hpp>

#include <string>

#include "rubuilder/rui/StateMachine.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "rubuilder/utils/RubuilderApplication.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/ApplicationStub.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xgi/Output.h"


namespace rubuilder { namespace rui { // namespace rubuilder::rui

class BUproxy;
class EVMproxy;
class RUI;
class RUinput;
class SuperFragmentTable;

/**
 * \ingroup xdaqApps
 * \brief Readout unit interface (RUI)
 */
class Application :
  public utils::RubuilderApplication<StateMachine>
{
public:
  
  /**
   * Define factory method for the instantion of RUI applications.
   */
  XDAQ_INSTANTIATOR();
  
  /**
   * Constructor.
   */
  Application(xdaq::ApplicationStub*);
  
  
private:

  boost::shared_ptr<RUI> rui_;
  
  xdata::UnsignedInteger64 i2oEVMRUDataReadyCount_;
  
  virtual void do_appendApplicationInfoSpaceItems(utils::InfoSpaceItems&);
  virtual void do_appendMonitoringInfoSpaceItems(utils::InfoSpaceItems&);
  virtual void do_updateMonitoringInfo();
  virtual void do_handleItemRetrieveEvent(const std::string& item);
  
  virtual void bindNonDefaultXgiCallbacks();
  virtual void do_defaultWebPage(xgi::Output*);
  void fragmentFIFOWebPage(xgi::Input*, xgi::Output*);

}; // class Application

} } // namespace rubuilder::rui

#endif


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
