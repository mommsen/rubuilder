#ifndef _rubuilder_ru_Application_h_
#define _rubuilder_ru_Application_h_

#include <boost/shared_ptr.hpp>

#include <string>

#include "rubuilder/ru/StateMachine.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "rubuilder/utils/RubuilderApplication.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/ApplicationStub.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xgi/Output.h"


namespace rubuilder { namespace ru { // namespace rubuilder::ru

class BUproxy;
class EVMproxy;
class RU;
class RUinput;
class SuperFragmentTable;

/**
 * \ingroup xdaqApps
 * \brief Readout unit (RU)
 */
class Application :
  public utils::RubuilderApplication<StateMachine>
{
public:
  
  /**
   * Define factory method for the instantion of RU applications.
   */
  XDAQ_INSTANTIATOR();
  
  /**
   * Constructor.
   */
  Application(xdaq::ApplicationStub*);
  
  
private:

  boost::shared_ptr<BUproxy> buProxy_;
  boost::shared_ptr<EVMproxy> evmProxy_;
  boost::shared_ptr<RU> ru_;
  boost::shared_ptr<RUinput> ruInput_;
  boost::shared_ptr<SuperFragmentTable> superFragmentTable_;

  xdata::UnsignedInteger64 i2oEVMRUDataReadyCount_;
  xdata::UnsignedInteger64 i2oBUCacheCount_;
  xdata::UnsignedInteger32 nbSuperFragmentsReady_;
  
  virtual void bindI2oCallbacks();
  inline void I2O_RU_READOUT_Callback(toolbox::mem::Reference*);
  inline void I2O_EVMRU_DATA_READY_Callback(toolbox::mem::Reference*);
  inline void I2O_RU_SEND_Callback(toolbox::mem::Reference*);
  inline void I2O_DATA_READY_Callback(toolbox::mem::Reference*);
  
  virtual void do_appendApplicationInfoSpaceItems(utils::InfoSpaceItems&);
  virtual void do_appendMonitoringInfoSpaceItems(utils::InfoSpaceItems&);
  virtual void do_updateMonitoringInfo();
  
  virtual void do_handleItemChangedEvent(const std::string& item);
  virtual void do_handleItemRetrieveEvent(const std::string& item);
  
  virtual void bindNonDefaultXgiCallbacks();
  virtual void do_defaultWebPage(xgi::Output*);
  void evbIdFIFOWebPage(xgi::Input*, xgi::Output*);
  void blockFIFOWebPage(xgi::Input*, xgi::Output*);

}; // class Application

} } // namespace rubuilder::ru

#endif


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
