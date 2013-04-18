#ifndef _rubuilder_bu_Application_h_
#define _rubuilder_bu_Application_h_

#include <boost/shared_ptr.hpp>

#include <string>

#include "rubuilder/bu/StateMachine.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "rubuilder/utils/RubuilderApplication.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/ApplicationStub.h"
#include "xdata/Integer32.h"
#include "xdata/UnsignedInteger32.h"
#include "xgi/Input.h"
#include "xgi/Output.h"


namespace rubuilder { namespace bu { // namespace rubuilder::bu

class BU;
class DiskWriter;
class EventTable;
class EVMproxy;
class FUproxy;
class RUproxy;


/**
 * \ingroup xdaqApps
 * \brief Builder Unit (BU).
 */
class Application :
  public utils::RubuilderApplication<StateMachine>
{
public:
  
  /**
   * Define factory method for the instantion of BU applications.
   */
  XDAQ_INSTANTIATOR();

  /**
   * Constructor.
   */
  Application(xdaq::ApplicationStub*);


private:
  
  boost::shared_ptr<BU> bu_;
  boost::shared_ptr<DiskWriter> diskWriter_;
  boost::shared_ptr<EventTable> eventTable_;
  boost::shared_ptr<EVMproxy> evmProxy_;
  boost::shared_ptr<RUproxy> ruProxy_;
  boost::shared_ptr<FUproxy> fuProxy_;
  
  xdata::UnsignedInteger32 nbEvtsUnderConstruction_;
  xdata::UnsignedInteger32 nbEventsInBU_;
  xdata::UnsignedInteger32 nbEvtsReady_;
  xdata::UnsignedInteger32 nbEvtsBuilt_;
  xdata::UnsignedInteger32 nbEvtsCorrupted_;
  xdata::UnsignedInteger32 nbFilesWritten_;
  xdata::Integer32 usedEvmInstance_;
 
  virtual void bindI2oCallbacks();
  inline void I2O_BU_CONFIRM_Callback(toolbox::mem::Reference*);
  inline void I2O_BU_CACHE_Callback(toolbox::mem::Reference*);
  inline void I2O_BU_ALLOCATE_Callback(toolbox::mem::Reference*);
  inline void I2O_BU_COLLECT_Callback(toolbox::mem::Reference*);
  inline void I2O_BU_DISCARD_Callback(toolbox::mem::Reference*);
  inline void I2O_EVM_LUMISECTION_Callback(toolbox::mem::Reference*);
  
  virtual void do_appendApplicationInfoSpaceItems(utils::InfoSpaceItems&);
  virtual void do_appendMonitoringInfoSpaceItems(utils::InfoSpaceItems&);
  virtual void do_updateMonitoringInfo();
  
  virtual void do_handleItemRetrieveEvent(const std::string& item);
  
  virtual void bindNonDefaultXgiCallbacks();
  virtual void do_defaultWebPage(xgi::Output*);
  void triggerFIFOWebPage(xgi::Input*, xgi::Output*);
  void completeEventsFIFOWebPage(xgi::Input*, xgi::Output*);
  void discardFIFOWebPage(xgi::Input*, xgi::Output*);
  void freeResourceIdFIFOWebPage(xgi::Input*, xgi::Output*);
  void requestFIFOWebPage(xgi::Input*, xgi::Output*);
  void blockFIFOWebPage(xgi::Input*, xgi::Output*);
  void eventFIFOWebPage(xgi::Input*, xgi::Output*);
  void eolsFIFOWebPage(xgi::Input*, xgi::Output*);

}; // class Application

} } // namespace rubuilder::bu

#endif


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
