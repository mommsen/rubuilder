#ifndef _rubuilder_evm_Application_h_
#define _rubuilder_evm_Application_h_

#include <boost/shared_ptr.hpp>

#include <string>

#include "rubuilder/evm/StateMachine.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "rubuilder/utils/RubuilderApplication.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/ApplicationStub.h"
#include "xdata/Boolean.h"
#include "xdata/UnsignedInteger32.h"
#include "xgi/Input.h"
#include "xgi/Output.h"


namespace rubuilder { namespace evm { // namespace rubuilder::evm

class EVM;
class TRGproxy;
class BUproxy;
class RUproxy;
class SMproxy;
class L1InfoHandler;
class EoLSHandler;


/**
 * \ingroup xdaqApps
 * \brief Event manager (EVM).
 */
class Application :
  public utils::RubuilderApplication<StateMachine>
{
public:
  
  /**
   * Define factory method for the instantion of EVM applications.
   */
  XDAQ_INSTANTIATOR();

  /**
   * Constructor.
   */
  Application(xdaq::ApplicationStub*);


private:
  
  boost::shared_ptr<EVM> evm_;
  boost::shared_ptr<TRGproxy> trgProxy_;
  boost::shared_ptr<RUproxy> ruProxy_;
  boost::shared_ptr<BUproxy> buProxy_;
  boost::shared_ptr<SMproxy> smProxy_;
  boost::shared_ptr<L1InfoHandler> l1InfoHandler_;
  boost::shared_ptr<EoLSHandler> eolsHandler_;
  
  xdata::UnsignedInteger32 nbTriggers_;
  xdata::UnsignedInteger32 nbEvtsBuilt_;
  xdata::Boolean ruBuilderIsFlushed_;

  virtual void bindI2oCallbacks();
  inline void I2O_EVM_TRIGGER_Callback(toolbox::mem::Reference*);
  inline void I2O_EVM_ALLOCATE_CLEAR_Callback(toolbox::mem::Reference*);
  
  virtual void do_appendApplicationInfoSpaceItems(utils::InfoSpaceItems&);
  virtual void do_appendMonitoringInfoSpaceItems(utils::InfoSpaceItems&);
  virtual void do_updateMonitoringInfo();
  
  virtual void do_handleItemChangedEvent(const std::string& item);
  virtual void do_handleItemRetrieveEvent(const std::string& item);
  
  virtual void bindNonDefaultXgiCallbacks();
  virtual void do_defaultWebPage(xgi::Output*);
  void clearedEvbIdFIFOWebPage(xgi::Input*, xgi::Output*);
  void triggerFIFOWebPage(xgi::Input*, xgi::Output*);
  void eventFIFOWebPage(xgi::Input*, xgi::Output*);
  void requestFIFOsWebPage(xgi::Input*, xgi::Output*);
  void l1InfoFIFOWebPage(xgi::Input*, xgi::Output*);
  void eolsFIFOsWebPage(xgi::Input*, xgi::Output*);
  void lumiSectionInfoFIFOWebPage(xgi::Input*, xgi::Output*);
  void releasedEvbIdFIFOWebPage(xgi::Input*, xgi::Output*);
  void completedLumiSectionFIFOWebPage(xgi::Input*, xgi::Output*);
  void lastL1InfoWebPage(xgi::Input*, xgi::Output*);

}; // class Application

} } // namespace rubuilder::evm

#endif


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
