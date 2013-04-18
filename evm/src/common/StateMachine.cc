#include "rubuilder/evm/BUproxy.h"
#include "rubuilder/evm/EoLSHandler.h"
#include "rubuilder/evm/EVM.h"
#include "rubuilder/evm/L1InfoHandler.h"
#include "rubuilder/evm/RUproxy.h"
#include "rubuilder/evm/SMproxy.h"
#include "rubuilder/evm/StateMachine.h"
#include "rubuilder/evm/States.h"
#include "rubuilder/evm/TRGproxy.h"
#include "rubuilder/utils/Exception.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>


rubuilder::evm::StateMachine::StateMachine
(
  xdaq::Application* app,
  boost::shared_ptr<EoLSHandler> eolsHandler,
  boost::shared_ptr<L1InfoHandler> l1InfoHandler,
  boost::shared_ptr<TRGproxy> trgProxy,
  boost::shared_ptr<RUproxy> ruProxy,
  boost::shared_ptr<BUproxy> buProxy,
  boost::shared_ptr<SMproxy> smProxy,
  boost::shared_ptr<EVM> evm
):
RSM(app),
eolsHandler_(eolsHandler),
l1InfoHandler_(l1InfoHandler),
trgProxy_(trgProxy),
ruProxy_(ruProxy),
buProxy_(buProxy),
smProxy_(smProxy),
evm_(evm)
{
  soapFsmEvents_.push_back("Blocked");
  soapFsmEvents_.push_back("Clear");

  // initiate FSM here to assure that the derived state machine class
  // has been fully constructed.
  this->initiate();
}


void rubuilder::evm::StateMachine::do_processSoapEvent
(
  const std::string& soapEvent,
  std::string& newStateName
)
{
  if ( soapEvent == "Blocked" )
    newStateName = processFSMEvent( Blocked() );
  else if ( soapEvent == "Clear" )
    newStateName = processFSMEvent( Clear() );
  else
    RSM::do_processSoapEvent(soapEvent,newStateName);
}


void rubuilder::evm::StateMachine::do_appendConfigurationItems(utils::InfoSpaceItems& params)
{
  drainingTimeOutSec_ = 30;

  params.add("drainingTimeOutSec", &drainingTimeOutSec_);
}


void rubuilder::evm::StateMachine::evmTrigger(toolbox::mem::Reference* bufRef)
{
  std::string msg = "Failed to process I2O_EVM_TRIGGER";
  try
  {
    trgProxy_->I2Ocallback(bufRef);
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


void rubuilder::evm::StateMachine::evmAllocateClear(toolbox::mem::Reference* bufRef)
{
  std::string msg = "Failed to process I2O_EVM_ALLOCATE_CLEAR";
  try
  {
    buProxy_->I2Ocallback(bufRef);
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


void rubuilder::evm::Configuring::entryAction()
{
  doConfiguring_ = true;
  configuringThread_.reset(
    new boost::thread( boost::bind( &rubuilder::evm::Configuring::activity, this) )
  );
  #ifndef RUBUILDER_BOOST
  configuringThread_->detach();
  #endif
}


void rubuilder::evm::Configuring::activity()
{
  outermost_context_type& stateMachine = outermost_context();
  
  std::string msg = "Failed to get descriptors of peer applications";
  try
  {
    if (doConfiguring_) stateMachine.ruProxy()->getApplicationDescriptors();
    if (doConfiguring_) stateMachine.buProxy()->getApplicationDescriptors();
    if (doConfiguring_) stateMachine.smProxy()->getApplicationDescriptors();
    if (doConfiguring_) stateMachine.eolsHandler()->getApplicationDescriptors();
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
    if (doConfiguring_) stateMachine.eolsHandler()->configure();
    if (doConfiguring_) stateMachine.l1InfoHandler()->configure();
    if (doConfiguring_) stateMachine.trgProxy()->configure();
    if (doConfiguring_) stateMachine.ruProxy()->configure();
    if (doConfiguring_) stateMachine.buProxy()->configure();
    if (doConfiguring_) stateMachine.smProxy()->configure();
    
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


void rubuilder::evm::Configuring::exitAction()
{
  doConfiguring_ = false;
  configuringThread_->join();
}


void rubuilder::evm::Clearing::entryAction()
{
  doClearing_ = true;
  clearingThread_.reset(
    new boost::thread( boost::bind( &rubuilder::evm::Clearing::activity, this) )
  );
  #ifndef RUBUILDER_BOOST
  clearingThread_->detach();
  #endif
}


void rubuilder::evm::Clearing::activity()
{
  outermost_context_type& stateMachine = outermost_context();
  
  std::string msg = "Failed to clear the components";
  try
  {
    if (doClearing_) stateMachine.eolsHandler()->clear();
    if (doClearing_) stateMachine.l1InfoHandler()->clear();
    if (doClearing_) stateMachine.trgProxy()->clear();
    if (doClearing_) stateMachine.ruProxy()->clear();
    if (doClearing_) stateMachine.buProxy()->clear();
    if (doClearing_) stateMachine.smProxy()->clear();
    
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


void rubuilder::evm::Clearing::exitAction()
{
  doClearing_ = false;
  clearingThread_->join();
}


void rubuilder::evm::Processing::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();
  
  stateMachine.eolsHandler()->resetMonitoringCounters();
  stateMachine.l1InfoHandler()->resetMonitoringCounters();
  stateMachine.trgProxy()->resetMonitoringCounters();
  stateMachine.ruProxy()->resetMonitoringCounters();
  stateMachine.buProxy()->resetMonitoringCounters();
  stateMachine.smProxy()->resetMonitoringCounters();
  stateMachine.evm()->resetMonitoringCounters();

  stateMachine.evm()->startProcessing();
}


void rubuilder::evm::Processing::exitAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.evm()->stopProcessing();
  stateMachine.l1InfoHandler()->enqCurrentLumiSectionInfo();
}


void rubuilder::evm::Enabled::exitAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.trgProxy()->stopRequestingTriggers();
}


void rubuilder::evm::Draining::entryAction()
{
  doDraining_ = true;
  drainingThread_.reset(
    new boost::thread( boost::bind( &rubuilder::evm::Draining::activity, this) )
  );
  #ifndef RUBUILDER_BOOST
  drainingThread_->detach();
  #endif
}


void rubuilder::evm::Draining::exitAction()
{ 
  doDraining_ = false;
  drainingThread_->join();
}


void rubuilder::evm::Draining::activity()
{
  outermost_context_type& stateMachine = outermost_context();
  
  std::string msg = "Failed to drain the rubuilder";
  try
  {  
    const uint32_t drainingTimeOutSec = outermost_context().drainingTimeOutSec();
    timerManager_.initTimer(timerId_, drainingTimeOutSec*1000);
    timerManager_.restartTimer(timerId_);
    
    drainingStartTime_ = time(0);
    previousConfirmCount_ = 0;
    
    while ( doDraining_ && isDraining() ) { ::sleep(1); }
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


bool rubuilder::evm::Draining::isDraining()
{
  outermost_context_type& stateMachine = outermost_context();

  if (
    stateMachine.evm()->ruBuilderIsFlushed() &&
    stateMachine.l1InfoHandler()->empty() &&
    stateMachine.eolsHandler()->empty()
  )
  {
    stateMachine.processFSMEvent( DrainingDone() );
    return false;
  }

  const uint64_t currentConfirmCount = stateMachine.buProxy()->getConfirmLogicalCount();
  if (previousConfirmCount_ == currentConfirmCount)
  {
    if (timerManager_.isFired(timerId_))
    {
      std::ostringstream msg;
      msg << "FlushFailed after " << difftime(time(0),drainingStartTime_) << "s: ";
      msg << stateMachine.evm()->getReasonForNotFlushed();
      if ( ! stateMachine.l1InfoHandler()->empty() ) msg << " L1InfoHandler not empty";
      if ( ! stateMachine.eolsHandler()->empty() ) msg << " EoLSHandler not empty";
      LOG4CPLUS_WARN(stateMachine.getLogger(), msg.str());
      stateMachine.processFSMEvent( Blocked() );
      return false;
    }
  }
  else
  {
    previousConfirmCount_ = currentConfirmCount;
    timerManager_.restartTimer(timerId_);
  }
  return true;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
