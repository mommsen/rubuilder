#include "rubuilder/ru/BUproxy.h"
#include "rubuilder/ru/EVMproxy.h"
#include "rubuilder/ru/RU.h"
#include "rubuilder/ru/RUinput.h"
#include "rubuilder/ru/StateMachine.h"
#include "rubuilder/ru/States.h"
#include "rubuilder/utils/Exception.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>


rubuilder::ru::StateMachine::StateMachine
(
  xdaq::Application* app,
  boost::shared_ptr<BUproxy> buProxy,
  boost::shared_ptr<EVMproxy> evmProxy,
  boost::shared_ptr<RU> ru,
  boost::shared_ptr<RUinput> ruInput
):
RSM(app),
buProxy_(buProxy),
evmProxy_(evmProxy),
ru_(ru),
ruInput_(ruInput)
{
  // initiate FSM here to assure that the derived state machine class
  // has been fully constructed.
  this->initiate();
}


void rubuilder::ru::StateMachine::mismatchEvent(const MismatchDetected& evt)
{
  LOG4CPLUS_ERROR(app_->getApplicationLogger(), evt.getTraceback());
  
  rcmsStateNotifier_.stateChanged("MismatchDetectedBackPressuring", evt.getReason());

  app_->notifyQualified("error", evt.getException());
}


void rubuilder::ru::StateMachine::timedOutEvent(const TimedOut& evt)
{
  LOG4CPLUS_ERROR(app_->getApplicationLogger(), evt.getTraceback());
  
  rcmsStateNotifier_.stateChanged("TimedOutBackPressuring", evt.getReason());

  app_->notifyQualified("error", evt.getException());
}


void rubuilder::ru::StateMachine::ruReadout(toolbox::mem::Reference* bufRef)
{
  std::string msg = "Failed to process I2O_RU_READOUT";
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


void rubuilder::ru::StateMachine::dataReady(toolbox::mem::Reference* bufRef)
{
  std::string msg = "Failed to process I2O_DATA_READY";
  try
  {
    ruInput_->I2Ocallback(bufRef);
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


void rubuilder::ru::StateMachine::evmRuDataReady(toolbox::mem::Reference* bufRef)
{
  std::string msg = "Failed to process I2O_EVMRU_DATA_READY";
  try
  {
    ruInput_->I2Ocallback(bufRef);
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


void rubuilder::ru::StateMachine::ruSend(toolbox::mem::Reference* bufRef)
{
  std::string msg = "Failed to process I2O_RU_SEND";
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


void rubuilder::ru::Configuring::entryAction()
{
  doConfiguring_ = true;
  configuringThread_.reset(
    new boost::thread( boost::bind( &rubuilder::ru::Configuring::activity, this) )
  );
  #ifndef RUBUILDER_BOOST
  configuringThread_->detach();
  #endif
}


void rubuilder::ru::Configuring::activity()
{
  outermost_context_type& stateMachine = outermost_context();

  std::string msg = "Failed to configure the components";
  try
  {
    if (doConfiguring_) stateMachine.buProxy()->configure();
    if (doConfiguring_) stateMachine.evmProxy()->configure();
    if (doConfiguring_) stateMachine.ru()->configure();
    if (doConfiguring_) stateMachine.ruInput()->configure();
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


void rubuilder::ru::Configuring::exitAction()
{
  doConfiguring_ = false;
  configuringThread_->join();
}


void rubuilder::ru::Clearing::entryAction()
{
  doClearing_ = true;
  clearingThread_.reset(
    new boost::thread( boost::bind( &rubuilder::ru::Clearing::activity, this) )
  );
  #ifndef RUBUILDER_BOOST
  clearingThread_->detach();
  #endif
}


void rubuilder::ru::Clearing::activity()
{
  outermost_context_type& stateMachine = outermost_context();
  
  std::string msg = "Failed to clear the components";
  try
  {
    if (doClearing_) stateMachine.buProxy()->clear();
    if (doClearing_) stateMachine.evmProxy()->clear();
    if (doClearing_) stateMachine.ru()->clear();
    if (doClearing_) stateMachine.ruInput()->clear();
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


void rubuilder::ru::Clearing::exitAction()
{
  doClearing_ = false;
  clearingThread_->join();
}


void rubuilder::ru::Processing::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.buProxy()->resetMonitoringCounters();
  stateMachine.evmProxy()->resetMonitoringCounters();
  stateMachine.ru()->resetMonitoringCounters();
  stateMachine.ruInput()->resetMonitoringCounters();
}


void rubuilder::ru::Enabled::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.ru()->startProcessing();
  stateMachine.ruInput()->acceptI2Omessages(true);
}


void rubuilder::ru::Enabled::exitAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.ruInput()->acceptI2Omessages(false);
  stateMachine.ru()->stopProcessing();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
