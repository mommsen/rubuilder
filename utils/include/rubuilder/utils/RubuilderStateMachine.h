#ifndef _rubuilder_utils_StateMachine_h_
#define _rubuilder_utils_StateMachine_h_

#ifdef RUBUILDER_BOOST
#include "rubuilder/boost/statechart/event_base.hpp"
#include "rubuilder/boost/statechart/state.hpp"
#include "rubuilder/boost/statechart/state_machine.hpp"
#include <boost/thread/mutex.hpp>
#else
#include <boost/pool/pool_alloc.hpp>
#include <boost/statechart/event_base.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#endif

#include "rubuilder/utils/Exception.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "xcept/tools.h"
#include "xdaq/Application.h"
#include "xdaq2rc/RcmsStateNotifier.h"
#include "xdata/String.h"

#include "log4cplus/logger.h"

#include <string>


namespace rubuilder { namespace utils { // namespace rubuilder::utils

  ///////////////////
  // Public events //
  ///////////////////
  class Configure : public boost::statechart::event<Configure> {};
  class Halt : public boost::statechart::event<Halt> {};
  class Enable : public boost::statechart::event<Enable> {};
  class Stop : public boost::statechart::event<Stop> {};

  ////////////////////////////////
  // Internal transition events //
  ////////////////////////////////

  class ConfigureDone: public boost::statechart::event<ConfigureDone> {};
  class ClearDone: public boost::statechart::event<ClearDone> {};

  class Fail : public boost::statechart::event<Fail>
  {
  public:
    Fail(xcept::Exception& exception) : exception_(exception) {};
    std::string getReason() const { return exception_.message(); }
    std::string getTraceback() const { return xcept::stdformat_exception_history(exception_); }
    xcept::Exception& getException() const { return exception_; }

  private:
    mutable xcept::Exception exception_;
  };

  struct StateName {
    virtual ~StateName() {};
    virtual std::string stateName() const = 0;
  };

  /////////////////////////////////
  // The rubuilder state machine //
  /////////////////////////////////
  
  template< class MostDerived, class InitialState >
  class RubuilderStateMachine :
    public boost::statechart::state_machine<MostDerived,InitialState,boost::fast_pool_allocator<MostDerived> >
  {
    
  public:
    
    RubuilderStateMachine(xdaq::Application*);
    
    void processSoapEvent(const std::string& event, std::string& newStateName);
    std::string processFSMEvent(const boost::statechart::event_base&);
    void processEvent(const boost::statechart::event_base&);
    
    void appendConfigurationItems(utils::InfoSpaceItems&);
    void appendMonitoringItems(utils::InfoSpaceItems&);
    void updateMonitoringItems();
    
    void notifyRCMS(const std::string& stateName);
    void failEvent(const Fail&);
    void unconsumed_event(const boost::statechart::event_base&);
    log4cplus::Logger& getLogger() const { return app_->getApplicationLogger(); }
    std::string getStateName() const { return stateName_; }
    
    typedef std::list<std::string> SoapFsmEvents;
    SoapFsmEvents getSoapFsmEvents() { return soapFsmEvents_; }

  protected:

    virtual void do_appendConfigurationItems(utils::InfoSpaceItems&) {};
    virtual void do_appendMonitoringItems(utils::InfoSpaceItems&) {};
    virtual void do_updateMonitoringItems() {};
    virtual void do_processSoapEvent(const std::string& event, std::string& newStateName);
    
    xdaq::Application* app_;
    xdaq2rc::RcmsStateNotifier rcmsStateNotifier_;

    SoapFsmEvents soapFsmEvents_;

  private:
    
    #ifdef RUBUILDER_BOOST
    boost::mutex eventMutex_;
    boost::mutex stateNameMutex_;
    #else
    boost::shared_mutex eventMutex_;
    boost::shared_mutex stateNameMutex_;
    #endif
    
    std::string stateName_;
    std::string reasonForFailed_;
    
    xdata::String monitoringStateName_;    
  };
  
  
  ////////////////////////////
  // Wrapper state template //
  ////////////////////////////
  
  template< class MostDerived,
            class Context,
            class InnerInitial = boost::mpl::list<>,
            boost::statechart::history_mode historyMode = boost::statechart::has_no_history >
  class RubuilderState : public StateName,
                         public boost::statechart::state<MostDerived, Context, InnerInitial, historyMode>
  {
  public:
    std::string stateName() const
    { return stateName_; }

  protected:
    typedef boost::statechart::state<MostDerived, Context, InnerInitial, historyMode> boost_state;
    typedef RubuilderState my_state;
    
    RubuilderState(const std::string stateName, typename boost_state::my_context& c) :
    boost_state(c), stateName_(stateName) {};
    virtual ~RubuilderState() {};

    virtual void entryAction() {};
    virtual void exitAction() {};

    const std::string stateName_;

    void safeEntryAction()
    {
      std::string msg = "Failed to enter " + stateName_ + " state";
      try
      {
        entryAction();
      }
      catch( xcept::Exception& e )
      {
        XCEPT_DECLARE_NESTED(exception::FSM,
          sentinelException, msg, e );
        this->post_event( Fail(sentinelException) );
      }
      catch( std::exception& e )
      {
        msg += ": ";
        msg += e.what();
        XCEPT_DECLARE(exception::FSM,
          sentinelException, msg );
        this->post_event( Fail(sentinelException) );
      }
      catch(...)
      {
        msg += ": unknown exception";
        XCEPT_DECLARE(exception::FSM,
          sentinelException, msg );
        this->post_event( Fail(sentinelException) );
      }
    };

    void safeExitAction()
    {
      std::string msg = "Failed to leave " + stateName_ + " state";
      try
      {
        exitAction();
      }
      catch( xcept::Exception& e )
      {
        XCEPT_DECLARE_NESTED(exception::FSM,
          sentinelException, msg, e );
        this->post_event( Fail(sentinelException) );
      }
      catch( std::exception& e )
      {
        msg += ": ";
        msg += e.what();
        XCEPT_DECLARE(exception::FSM,
          sentinelException, msg );
        this->post_event( Fail(sentinelException) );
      }
      catch(...)
      {
        msg += ": unknown exception";
        XCEPT_DECLARE(exception::FSM,
          sentinelException, msg );
        this->post_event( Fail(sentinelException) );
      }
    };

  };

} } //namespace rubuilder::utils

////////////////////////////////////////////////////////////////////////////////
// Implementation follows                                                     //
////////////////////////////////////////////////////////////////////////////////

template<class MostDerived,class InitialState>
rubuilder::utils::RubuilderStateMachine<MostDerived,InitialState>::RubuilderStateMachine
(
  xdaq::Application* app
):
app_(app),
rcmsStateNotifier_
(
  app->getApplicationLogger(),
  app->getApplicationDescriptor(),
  app->getApplicationContext()
),
stateName_("Halted"),
reasonForFailed_("")
{
  xdata::InfoSpace *is = app->getApplicationInfoSpace();
  is->fireItemAvailable("rcmsStateListener",
    rcmsStateNotifier_.getRcmsStateListenerParameter() );
  is->fireItemAvailable("foundRcmsStateListener",
    rcmsStateNotifier_.getFoundRcmsStateListenerParameter() );
  rcmsStateNotifier_.findRcmsStateListener();
  rcmsStateNotifier_.subscribeToChangesInRcmsStateListener(is);

  soapFsmEvents_.push_back("Configure");
  soapFsmEvents_.push_back("Halt");
  soapFsmEvents_.push_back("Enable");
  soapFsmEvents_.push_back("Stop");
  soapFsmEvents_.push_back("Fail");
}


template <class MostDerived,class InitialState>
void rubuilder::utils::RubuilderStateMachine<MostDerived,InitialState>::processSoapEvent
(
  const std::string& soapEvent,
  std::string& newStateName
)
{
  do_processSoapEvent(soapEvent, newStateName);
}


template <class MostDerived,class InitialState>
void rubuilder::utils::RubuilderStateMachine<MostDerived,InitialState>::do_processSoapEvent
(
  const std::string& soapEvent,
  std::string& newStateName
)
{
  if ( soapEvent == "Configure" )
    newStateName = processFSMEvent( Configure() );
  else if ( soapEvent == "Enable" )
    newStateName = processFSMEvent( Enable() );
  else if ( soapEvent == "Halt" )
    newStateName = processFSMEvent( Halt() );
  else if ( soapEvent == "Stop" )
    newStateName = processFSMEvent( Stop() );
  else if ( soapEvent == "Fail" )
  {
    XCEPT_DECLARE(exception::SOAP,
      sentinelException, "Externally requested by SOAP command" );
    newStateName = processFSMEvent( Fail(sentinelException) );
  }
  else
  {
    XCEPT_DECLARE(exception::FSM, sentinelException,
      "Received an unknown state machine event '" + soapEvent + "'.");
    newStateName = processFSMEvent( Fail(sentinelException) );
  }
}


template <class MostDerived,class InitialState>
std::string rubuilder::utils::RubuilderStateMachine<MostDerived,InitialState>::processFSMEvent
(
  const boost::statechart::event_base& event
)
{
  #ifdef RUBUILDER_BOOST
  boost::mutex::scoped_lock eventLock(eventMutex_);
  #else
  boost::unique_lock<boost::shared_mutex> eventUniqueLock(eventMutex_);
  #endif

  this->process_event(event);

  #ifdef RUBUILDER_BOOST
  boost::mutex::scoped_lock stateNameLock(stateNameMutex_);
  #else
  boost::unique_lock<boost::shared_mutex> stateNameUniqueLock(stateNameMutex_);
  #endif

  stateName_ = this->template state_cast<const StateName&>().stateName();
  return stateName_;
}


template <class MostDerived,class InitialState>
void rubuilder::utils::RubuilderStateMachine<MostDerived,InitialState>::processEvent
(
  const boost::statechart::event_base& event
)
{
  #ifdef RUBUILDER_BOOST
  boost::mutex::scoped_lock eventLock(eventMutex_);
  #else
  boost::shared_lock<boost::shared_mutex> eventSharedLock(eventMutex_);
  #endif
  
  this->process_event(event);
}


template <class MostDerived,class InitialState>
void rubuilder::utils::RubuilderStateMachine<MostDerived,InitialState>::appendConfigurationItems
(
  utils::InfoSpaceItems& params
)
{
  do_appendConfigurationItems(params);
}


template <class MostDerived,class InitialState>
void rubuilder::utils::RubuilderStateMachine<MostDerived,InitialState>::appendMonitoringItems
(
  utils::InfoSpaceItems& items
)
{
  monitoringStateName_ = "Halted";

  items.add("stateName", &monitoringStateName_);

  do_appendMonitoringItems(items);
}


template <class MostDerived,class InitialState>
void rubuilder::utils::RubuilderStateMachine<MostDerived,InitialState>::updateMonitoringItems()
{
  #ifdef RUBUILDER_BOOST
  boost::mutex::scoped_lock stateNameLock(stateNameMutex_);
  #else
  boost::shared_lock<boost::shared_mutex> stateNameSharedLock(stateNameMutex_);
  #endif
  
  monitoringStateName_ = stateName_;

  do_updateMonitoringItems();
}


template <class MostDerived,class InitialState>
void rubuilder::utils::RubuilderStateMachine<MostDerived,InitialState>::notifyRCMS
(
  const std::string& stateName
)
{
  rcmsStateNotifier_.stateChanged(stateName, "New state is " + stateName);
}


template <class MostDerived,class InitialState>
void rubuilder::utils::RubuilderStateMachine<MostDerived,InitialState>::failEvent(const Fail& evt)
{
  reasonForFailed_ = evt.getTraceback();

  LOG4CPLUS_FATAL(app_->getApplicationLogger(),
    "Failed: " << evt.getReason() << ". " << reasonForFailed_);

  rcmsStateNotifier_.stateChanged("Failed", evt.getReason());

  app_->notifyQualified("fatal", evt.getException());

  #ifdef RUBUILDER_BOOST
  boost::mutex::scoped_lock stateNameLock(stateNameMutex_);
  #else
  boost::unique_lock<boost::shared_mutex> stateNameUniqueLock(stateNameMutex_);
  #endif

  stateName_ = "Failed";
}


template <class MostDerived,class InitialState>
void rubuilder::utils::RubuilderStateMachine<MostDerived,InitialState>::unconsumed_event
(
  const boost::statechart::event_base& evt
)
{
  #ifdef RUBUILDER_BOOST
  boost::mutex::scoped_lock stateNameLock(stateNameMutex_);
  #else
  boost::shared_lock<boost::shared_mutex> stateNameSharedLock(stateNameMutex_);
  #endif
  
  LOG4CPLUS_ERROR(app_->getApplicationLogger(),
    "The " << typeid(evt).name()
    << " event is not supported from the "
    << stateName_ << " state!");
}


#endif //_rubuilder_utils_StateMachine_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
