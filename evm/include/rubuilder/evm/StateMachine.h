#ifndef _rubuilder_evm_StateMachine_h_
#define _rubuilder_evm_StateMachine_h_

#include <boost/shared_ptr.hpp>

#include <stdint.h>

#ifdef RUBUILDER_BOOST
#include "rubuilder/boost/statechart/event_base.hpp"
#else
#include <boost/statechart/event_base.hpp>
#endif

#include "rubuilder/evm/TRGproxy.h"
#include "rubuilder/utils/RubuilderStateMachine.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdata/UnsignedInteger32.h"


namespace rubuilder { namespace evm { // namespace rubuilder::evm

  class BUproxy;
  class EoLSHandler;
  class EVM;
  class L1InfoHandler;
  class RUproxy;
  class SMproxy;
  class StateMachine;
  class Outermost;

  ///////////////////////
  // Transition events //
  ///////////////////////
  class DrainingDone : public boost::statechart::event<DrainingDone> {};
  class Blocked : public boost::statechart::event<Blocked> {};
  class Clear : public boost::statechart::event<Clear> {};

  class EvmTrigger : public boost::statechart::event<EvmTrigger> 
  {
  public:
    EvmTrigger(toolbox::mem::Reference* bufRef) : bufRef_(bufRef) {};

    toolbox::mem::Reference* getTriggerMsg() const { return bufRef_; };

  private:
    toolbox::mem::Reference* bufRef_;
  };

  class EvmAllocateClear : public boost::statechart::event<EvmAllocateClear> 
  {
  public:
    EvmAllocateClear(toolbox::mem::Reference* bufRef) : bufRef_(bufRef) {};

    toolbox::mem::Reference* getAllocateClearMsg() const { return bufRef_; };

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
      boost::shared_ptr<EoLSHandler>,
      boost::shared_ptr<L1InfoHandler>,
      boost::shared_ptr<TRGproxy>,
      boost::shared_ptr<RUproxy>,
      boost::shared_ptr<BUproxy>,
      boost::shared_ptr<SMproxy>,
      boost::shared_ptr<EVM>
    );
    
    
    void evmTrigger(toolbox::mem::Reference*);
    void evmAllocateClear(toolbox::mem::Reference*);

    boost::shared_ptr<EoLSHandler> eolsHandler() const { return eolsHandler_; }
    boost::shared_ptr<L1InfoHandler> l1InfoHandler() const { return l1InfoHandler_; }
    boost::shared_ptr<TRGproxy> trgProxy() const { return trgProxy_; }
    boost::shared_ptr<RUproxy> ruProxy() const { return ruProxy_; }
    boost::shared_ptr<BUproxy> buProxy() const { return buProxy_; }
    boost::shared_ptr<SMproxy> smProxy() const { return smProxy_; }
    boost::shared_ptr<EVM> evm() const { return evm_; }
    
    uint32_t drainingTimeOutSec() const { return drainingTimeOutSec_.value_; }
    
    
  private:

    virtual void do_appendConfigurationItems(utils::InfoSpaceItems&);
    
    virtual void do_processSoapEvent(const std::string& event, std::string& newStateName);
    
    boost::shared_ptr<EoLSHandler> eolsHandler_;
    boost::shared_ptr<L1InfoHandler> l1InfoHandler_;
    boost::shared_ptr<TRGproxy> trgProxy_;
    boost::shared_ptr<RUproxy> ruProxy_;
    boost::shared_ptr<BUproxy> buProxy_;
    boost::shared_ptr<SMproxy> smProxy_;
    boost::shared_ptr<EVM> evm_;
    
    xdata::UnsignedInteger32 drainingTimeOutSec_;
    
  };
  
} } //namespace rubuilder::evm

#endif //_rubuilder_evm_StateMachine_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
