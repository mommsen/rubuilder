#include "rubuilder/rui/RUI.h"
#include "rubuilder/rui/StateMachine.h"
#include "rubuilder/rui/States.h"
#include "rubuilder/utils/Exception.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>


rubuilder::rui::StateMachine::StateMachine
(
  xdaq::Application* app,
  boost::shared_ptr<RUI> rui
):
RSM(app),
rui_(rui)
{
  // initiate FSM here to assure that the derived state machine class
  // has been fully constructed.
  this->initiate();
}


void rubuilder::rui::Configuring::entryAction()
{
  doConfiguring_ = true;
  configuringThread_.reset(
    new boost::thread( boost::bind( &rubuilder::rui::Configuring::activity, this) )
  );
  #ifndef RUBUILDER_BOOST
  configuringThread_->detach();
  #endif
}


void rubuilder::rui::Configuring::activity()
{
  outermost_context_type& stateMachine = outermost_context();

  std::string msg = "Failed to configure the components";
  try
  {
    if (doConfiguring_) stateMachine.rui()->configure();
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


void rubuilder::rui::Configuring::exitAction()
{
  doConfiguring_ = false;
  configuringThread_->join();
}


void rubuilder::rui::Clearing::entryAction()
{
  doClearing_ = true;
  clearingThread_.reset(
    new boost::thread( boost::bind( &rubuilder::rui::Clearing::activity, this) )
  );
  #ifndef RUBUILDER_BOOST
  clearingThread_->detach();
  #endif
}


void rubuilder::rui::Clearing::activity()
{
  outermost_context_type& stateMachine = outermost_context();
  
  std::string msg = "Failed to clear the components";
  try
  {
    if (doClearing_) stateMachine.rui()->clear();
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


void rubuilder::rui::Clearing::exitAction()
{
  doClearing_ = false;
  clearingThread_->join();
}


void rubuilder::rui::Processing::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.rui()->resetMonitoringCounters();
}


void rubuilder::rui::Enabled::entryAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.rui()->startProcessing();
}


void rubuilder::rui::Enabled::exitAction()
{
  outermost_context_type& stateMachine = outermost_context();
  stateMachine.rui()->stopProcessing();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
