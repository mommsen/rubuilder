#ifndef _rubuilder_ru_StateMachine_h_
#define _rubuilder_ru_StateMachine_h_

#include <boost/shared_ptr.hpp>

#ifdef RUBUILDER_BOOST
#include "rubuilder/boost/statechart/event_base.hpp"
#else
#include <boost/statechart/event_base.hpp>
#endif

#include "rubuilder/utils/Exception.h"
#include "rubuilder/utils/RubuilderStateMachine.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"


namespace rubuilder { namespace ru { // namespace rubuilder::ru
  
  class BUproxy;
  class EVMproxy;
  class RU;
  class RUinput;
  class StateMachine;
  class Outermost;

  ///////////////////////
  // Transition events //
  ///////////////////////
  
  class MismatchDetected : public boost::statechart::event<MismatchDetected>
  {
  public:
    MismatchDetected(exception::MismatchDetected& exception) : exception_(exception) {};
    std::string getReason() const { return exception_.message(); }
    std::string getTraceback() const { return xcept::stdformat_exception_history(exception_); }
    xcept::Exception& getException() const { return exception_; }

  private:
    mutable xcept::Exception exception_;
  };

  class TimedOut : public boost::statechart::event<TimedOut>
  {
  public:
    TimedOut(exception::TimedOut& exception) : exception_(exception) {};
    std::string getReason() const { return exception_.message(); }
    std::string getTraceback() const { return xcept::stdformat_exception_history(exception_); }
    xcept::Exception& getException() const { return exception_; }

  private:
    mutable xcept::Exception exception_;
  };

  class RuReadout : public boost::statechart::event<RuReadout> 
  {
  public:
    RuReadout(toolbox::mem::Reference* bufRef) : bufRef_(bufRef) {};

    toolbox::mem::Reference* getReadoutMsg() const { return bufRef_; };

  private:
    toolbox::mem::Reference* bufRef_;
  };

  class EvmRuDataReady : public boost::statechart::event<EvmRuDataReady> 
  {
  public:
    EvmRuDataReady(toolbox::mem::Reference* bufRef) : bufRef_(bufRef) {};

    toolbox::mem::Reference* getDataReadyMsg() const { return bufRef_; };

  private:
    toolbox::mem::Reference* bufRef_;
  };

  class RuSend : public boost::statechart::event<RuSend> 
  {
  public:
    RuSend(toolbox::mem::Reference* bufRef) : bufRef_(bufRef) {};

    toolbox::mem::Reference* getSendMsg() const { return bufRef_; };

  private:
    toolbox::mem::Reference* bufRef_;
  };
  
  ///////////////////////
  // The state machine //
  ///////////////////////

  typedef utils::RubuilderStateMachine<StateMachine,Outermost> RSM;
  class StateMachine: public RSM
  {
    
  public:
    
    StateMachine
    (
      xdaq::Application*,
      boost::shared_ptr<BUproxy>,
      boost::shared_ptr<EVMproxy>,
      boost::shared_ptr<RU>,
      boost::shared_ptr<RUinput>
    );

    void mismatchEvent(const MismatchDetected&);
    void timedOutEvent(const TimedOut&);
    
    void ruReadout(toolbox::mem::Reference*);
    void evmRuDataReady(toolbox::mem::Reference*);
    void ruSend(toolbox::mem::Reference*);

    boost::shared_ptr<BUproxy> buProxy() const { return buProxy_; }
    boost::shared_ptr<EVMproxy> evmProxy() const { return evmProxy_; }
    boost::shared_ptr<RU> ru() const { return ru_; }
    boost::shared_ptr<RUinput> ruInput() const { return ruInput_; }

  private:
    
    boost::shared_ptr<BUproxy> buProxy_;
    boost::shared_ptr<EVMproxy> evmProxy_;
    boost::shared_ptr<RU> ru_;
    boost::shared_ptr<RUinput> ruInput_;

  };
  
} } //namespace rubuilder::ru

#endif //_rubuilder_ru_StateMachine_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
