#include "rubuilder/bu/BU.h"
#include "rubuilder/bu/DiskWriter.h"
#include "rubuilder/bu/EVMproxy.h"
#include "rubuilder/bu/FileHandler.h"
#include "rubuilder/bu/FUproxy.h"
#include "rubuilder/bu/RUproxy.h"
#include "rubuilder/bu/EventTable.h"
#include "rubuilder/bu/StateMachine.h"
#include "rubuilder/bu/States.h"
#include "rubuilder/utils/Constants.h"
#include "rubuilder/utils/Exception.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>


rubuilder::bu::StateMachine::StateMachine
(
  xdaq::Application* app,
  boost::shared_ptr<BU> bu,
  boost::shared_ptr<EVMproxy> evmProxy,
  boost::shared_ptr<FUproxy> fuProxy,
  boost::shared_ptr<RUproxy> ruProxy,
  boost::shared_ptr<DiskWriter> diskWriter,
  boost::shared_ptr<EventTable> eventTable
):
RSM(app),
bu_(bu),
evmProxy_(evmProxy),
fuProxy_(fuProxy),
ruProxy_(ruProxy),
diskWriter_(diskWriter),
eventTable_(eventTable)
{
  // initiate FSM here to assure that the derived state machine class
  // has been fully constructed.
  this->initiate();
}

void rubuilder::bu::StateMachine::do_appendConfigurationItems(utils::InfoSpaceItems& params)
{
  runNumber_ = 0;
  maxEvtsUnderConstruction_ = 64;
  msgAgeLimitDtMSec_ = utils::DEFAULT_MESSAGE_AGE_LIMIT_MSEC;
  
  params.add("runNumber", &runNumber_);
  params.add("maxEvtsUnderConstruction", &maxEvtsUnderConstruction_);
  params.add("msgAgeLimitDtMSec", &msgAgeLimitDtMSec_);
}

void rubuilder::bu::StateMachine::do_appendMonitoringItems(utils::InfoSpaceItems& items)
{
  items.add("runNumber", &runNumber_);
}

void rubuilder::bu::StateMachine::buConfirm(toolbox::mem::Reference* bufRef)
{
  std::string msg = "Failed to process I2O_BU_CONFIRM";
  try
  {
    evmProxy_->I2Ocallback(bufRef);
  }
  catch( xcept::Exception& e )
  {
    XCEPT_DECLARE_NESTED(exception::I2O,
      sentinelException, msg, e );
    process_event( utils::Fail(sentinelException) );
  }
  catch( std::exception& e )
  {
    msg += ": ";
    msg += e.what();
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( utils::Fail(sentinelException) );
  }
  catch(...)
  {
    msg += ": unknown exception";
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( utils::Fail(sentinelException) );
  }
}


void rubuilder::bu::StateMachine::buCache(toolbox::mem::Reference* bufRef)
{
  std::string msg = "Failed to process I2O_BU_CACHE";
  try
  {
    ruProxy_->I2Ocallback(bufRef);
  }
  catch( xcept::Exception& e )
  {
    XCEPT_DECLARE_NESTED(exception::I2O,
      sentinelException, msg, e );
    process_event( utils::Fail(sentinelException) );
  }
  catch( std::exception& e )
  {
    msg += ": ";
    msg += e.what();
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( utils::Fail(sentinelException) );
  }
  catch(...)
  {
    msg += ": unknown exception";
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( utils::Fail(sentinelException) );
  }
}


void rubuilder::bu::StateMachine::buAllocate(toolbox::mem::Reference* bufRef)
{
  std::string msg = "Failed to process I2O_BU_ALLOCATE";
  try
  {
    fuProxy_->allocateI2Ocallback(bufRef);
  }
  catch( xcept::Exception& e )
  {
    XCEPT_DECLARE_NESTED(exception::I2O,
      sentinelException, msg, e );
    process_event( utils::Fail(sentinelException) );
  }
  catch( std::exception& e )
  {
    msg += ": ";
    msg += e.what();
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( utils::Fail(sentinelException) );
  }
  catch(...)
  {
    msg += ": unknown exception";
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( utils::Fail(sentinelException) );
  }
}


void rubuilder::bu::StateMachine::buCollect(toolbox::mem::Reference* bufRef)
{
  std::string msg = "Failed to process I2O_BU_COLLECT";
  try
  {
    fuProxy_->collectI2Ocallback(bufRef);
  }
  catch( xcept::Exception& e )
  {
    XCEPT_DECLARE_NESTED(exception::I2O,
      sentinelException, msg, e );
    process_event( utils::Fail(sentinelException) );
  }
  catch( std::exception& e )
  {
    msg += ": ";
    msg += e.what();
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( utils::Fail(sentinelException) );
  }
  catch(...)
  {
    msg += ": unknown exception";
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( utils::Fail(sentinelException) );
  }
}


void rubuilder::bu::StateMachine::buDiscard(toolbox::mem::Reference* bufRef)
{
  std::string msg = "Failed to process I2O_BU_DISCARD";
  try
  {
    fuProxy_->discardI2Ocallback(bufRef);
  }
  catch( xcept::Exception& e )
  {
    XCEPT_DECLARE_NESTED(exception::I2O,
      sentinelException, msg, e );
    process_event( utils::Fail(sentinelException) );
  }
  catch( std::exception& e )
  {
    msg += ": ";
    msg += e.what();
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( utils::Fail(sentinelException) );
  }
  catch(...)
  {
    msg += ": unknown exception";
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( utils::Fail(sentinelException) );
  }
}


void rubuilder::bu::StateMachine::evmLumisection(toolbox::mem::Reference* bufRef)
{
  std::string msg = "Failed to process I2O_EVM_LUMISECTION";
  try
  {
    I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME *msg =
      (I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME*)bufRef->getDataLocation();

    fuProxy_->handleEoLSmsg(msg);
    diskWriter_->closeLS(msg->lumiSection);
    
    // Request another event id
    msg::EvtIdRqstAndOrRelease rqstAndOrRelease;
    rqstAndOrRelease.requestType = msg::EvtIdRqstAndOrRelease::REQUEST;
    rqstAndOrRelease.resourceId  = msg->buResourceId;
    evmProxy_->sendEvtIdRqstAndOrRelease(rqstAndOrRelease);
    
    bufRef->release();
  }
  catch( xcept::Exception& e )
  {
    XCEPT_DECLARE_NESTED(exception::I2O,
      sentinelException, msg, e );
    process_event( utils::Fail(sentinelException) );
  }
  catch( std::exception& e )
  {
    msg += ": ";
    msg += e.what();
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( utils::Fail(sentinelException) );
  }
  catch(...)
  {
    msg += ": unknown exception";
    XCEPT_DECLARE(exception::I2O,
      sentinelException, msg );
    process_event( utils::Fail(sentinelException) );
  }
}


void rubuilder::bu::Configuring::entryAction()
{
  doConfiguring_ = true;
  configuringThread_.reset(
    new boost::thread( boost::bind( &rubuilder::bu::Configuring::activity, this) )
  );
  #ifndef RUBUILDER_BOOST
  configuringThread_->detach();
  #endif
}


void rubuilder::bu::Configuring::activity()
{
  outermost_context_type& stateMachine = outermost_context();
  
  std::string msg = "Failed to get descriptors of peer applications";
  try
  {
    if (doConfiguring_) stateMachine.evmProxy()->getApplicationDescriptors();
    if (doConfiguring_) stateMachine.fuProxy()->getApplicationDescriptors();
    if (doConfiguring_) stateMachine.ruProxy()->getApplicationDescriptors();
  }
  catch( xcept::Exception& e )
  {
    XCEPT_DECLARE_NESTED(exception::Configuration,
      sentinelException, msg, e);
    stateMachine.processFSMEvent( utils::Fail(sentinelException) );
  }
  catch( std::exception& e )
  {
    msg += ": ";
    msg += e.what();
    XCEPT_DECLARE(exception::Configuration,
      sentinelException, msg );
    stateMachine.processFSMEvent( utils::Fail(sentinelException) );
  }
  catch(...)
  {
    msg += ": unknown exception";
    XCEPT_DECLARE(exception::Configuration,
      sentinelException, msg );
    stateMachine.processFSMEvent( utils::Fail(sentinelException) );
  }


  msg = "Failed to configure the components";
  try
  {  
    const uint32_t msgAgeLimitDtMSec = stateMachine.msgAgeLimitDtMSec();
    const uint32_t maxEvtsUnderConstruction = stateMachine.maxEvtsUnderConstruction();
    
    if (doConfiguring_) stateMachine.bu()->configure();
    if (doConfiguring_) stateMachine.evmProxy()->configure(msgAgeLimitDtMSec);
    if (doConfiguring_) stateMachine.ruProxy()->configure(msgAgeLimitDtMSec);
    if (doConfiguring_) stateMachine.fuProxy()->configure();
    if (doConfiguring_) stateMachine.diskWriter()->configure(maxEvtsUnderConstruction);
    if (doConfiguring_) stateMachine.eventTable()->configure(maxEvtsUnderConstruction);
    
    if (doConfiguring_) stateMachine.processFSMEvent( utils::ConfigureDone() );
  }
  catch( xcept::Exception& e )
  {
    XCEPT_DECLARE_NESTED(exception::Configuration,
      sentinelException, msg, e);
    stateMachine.processFSMEvent( utils::Fail(sentinelException) );
  }
  catch( std::exception& e )
  {
    msg += ": ";
    msg += e.what();
    XCEPT_DECLARE(exception::Configuration,
      sentinelException, msg );
    stateMachine.processFSMEvent( utils::Fail(sentinelException) );
  }
  catch(...)
  {
    msg += ": unknown exception";
    XCEPT_DECLARE(exception::Configuration,
      sentinelException, msg );
    stateMachine.processFSMEvent( utils::Fail(sentinelException) );
  }
}


void rubuilder::bu::Configuring::exitAction()
{
  doConfiguring_ = false;
  configuringThread_->join();
}


void rubuilder::bu::Clearing::entryAction()
{
  doClearing_ = true;
  clearingThread_.reset(
    new boost::thread( boost::bind( &rubuilder::bu::Clearing::activity, this) )
  );
  #ifndef RUBUILDER_BOOST
  clearingThread_->detach();
  #endif
}


void rubuilder::bu::Clearing::activity()
{
  outermost_context_type& stateMachine = outermost_context();
  
  std::string msg = "Failed to clear the components";
  try
  {
    if (doClearing_) stateMachine.bu()->clear();
    if (doClearing_) stateMachine.evmProxy()->clear();
    if (doClearing_) stateMachine.ruProxy()->clear();
    if (doClearing_) stateMachine.fuProxy()->clear();
    if (doClearing_) stateMachine.diskWriter()->clear();
    if (doClearing_) stateMachine.eventTable()->clear();
    
    if (doClearing_) stateMachine.processFSMEvent( utils::ClearDone() );
  }
  catch( xcept::Exception& e )
  {
    XCEPT_DECLARE_NESTED(exception::FSM,
      sentinelException, msg, e);
    stateMachine.processFSMEvent( utils::Fail(sentinelException) );
  }
  catch( std::exception& e )
  {
    msg += ": ";
    msg += e.what();
    XCEPT_DECLARE(exception::FSM,
      sentinelException, msg );
    stateMachine.processFSMEvent( utils::Fail(sentinelException) );
  }
  catch(...)
  {
    msg += ": unknown exception";
    XCEPT_DECLARE(exception::FSM,
      sentinelException, msg );
    stateMachine.processFSMEvent( utils::Fail(sentinelException) );
  }
}


void rubuilder::bu::Clearing::exitAction()
{
  doClearing_ = false;
  clearingThread_->join();
}


void rubuilder::bu::Processing::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();

  stateMachine.bu()->resetMonitoringCounters();
  stateMachine.evmProxy()->resetMonitoringCounters();
  stateMachine.fuProxy()->resetMonitoringCounters();
  stateMachine.ruProxy()->resetMonitoringCounters();
  stateMachine.diskWriter()->resetMonitoringCounters();
  stateMachine.eventTable()->resetMonitoringCounters();
}


void rubuilder::bu::Enabled::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();
  
  const uint32_t runNumber = stateMachine.runNumber();

  stateMachine.diskWriter()->startProcessing(runNumber);
  stateMachine.bu()->startProcessing(runNumber);
  stateMachine.eventTable()->startProcessing();
}


void rubuilder::bu::Enabled::exitAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.bu()->stopProcessing();
  stateMachine.eventTable()->stopProcessing();
  stateMachine.diskWriter()->stopProcessing();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
