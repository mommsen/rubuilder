#ifndef _rubuilder_bu_StateMachine_h_
#define _rubuilder_bu_StateMachine_h_

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
#include "xdata/UnsignedInteger32.h"


namespace rubuilder { namespace bu { // namespace rubuilder::bu
  
  class BU;
  class EVMproxy;
  class FUproxy;
  class RUproxy;
  class DiskWriter;
  class EventTable;
  class StateMachine;
  class Outermost;

  ///////////////////////
  // Transition events //
  ///////////////////////

  class BuConfirm : public boost::statechart::event<BuConfirm> 
  {
  public:
    BuConfirm(toolbox::mem::Reference* bufRef) : bufRef_(bufRef) {};

    toolbox::mem::Reference* getConfirmMsg() const { return bufRef_; };

  private:
    toolbox::mem::Reference* bufRef_;
  };

  class BuCache : public boost::statechart::event<BuCache> 
  {
  public:
    BuCache(toolbox::mem::Reference* bufRef) : bufRef_(bufRef) {};

    toolbox::mem::Reference* getCacheMsg() const { return bufRef_; };

  private:
    toolbox::mem::Reference* bufRef_;
  };

  class BuAllocate : public boost::statechart::event<BuAllocate> 
  {
  public:
    BuAllocate(toolbox::mem::Reference* bufRef) : bufRef_(bufRef) {};

    toolbox::mem::Reference* getAllocateMsg() const { return bufRef_; };

  private:
    toolbox::mem::Reference* bufRef_;
  };

  class BuCollect : public boost::statechart::event<BuCollect> 
  {
  public:
    BuCollect(toolbox::mem::Reference* bufRef) : bufRef_(bufRef) {};

    toolbox::mem::Reference* getCollectMsg() const { return bufRef_; };

  private:
    toolbox::mem::Reference* bufRef_;
  };

  class BuDiscard : public boost::statechart::event<BuDiscard> 
  {
  public:
    BuDiscard(toolbox::mem::Reference* bufRef) : bufRef_(bufRef) {};

    toolbox::mem::Reference* getDiscardMsg() const { return bufRef_; };

  private:
    toolbox::mem::Reference* bufRef_;
  };

  class EvmLumisection : public boost::statechart::event<EvmLumisection> 
  {
  public:
    EvmLumisection(toolbox::mem::Reference* bufRef) : bufRef_(bufRef) {};

    toolbox::mem::Reference* getLumisectionMsg() const { return bufRef_; };

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
      boost::shared_ptr<BU>,
      boost::shared_ptr<EVMproxy>,
      boost::shared_ptr<FUproxy>,
      boost::shared_ptr<RUproxy>,
      boost::shared_ptr<DiskWriter>,
      boost::shared_ptr<EventTable>
    );
    
    void buAllocate(toolbox::mem::Reference*);
    void buConfirm(toolbox::mem::Reference*);
    void buCache(toolbox::mem::Reference*);
    void buCollect(toolbox::mem::Reference*);
    void buDiscard(toolbox::mem::Reference*);
    void evmLumisection(toolbox::mem::Reference*);
    
    boost::shared_ptr<BU> bu() const { return bu_; }
    boost::shared_ptr<EVMproxy> evmProxy() const { return evmProxy_; }
    boost::shared_ptr<FUproxy> fuProxy() const { return fuProxy_; }
    boost::shared_ptr<RUproxy> ruProxy() const { return ruProxy_; }
    boost::shared_ptr<DiskWriter> diskWriter() const { return diskWriter_; }
    boost::shared_ptr<EventTable> eventTable() const { return eventTable_; }
    
    uint32_t runNumber() const { return runNumber_.value_; }
    uint32_t maxEvtsUnderConstruction() const { return maxEvtsUnderConstruction_.value_; }
    uint32_t msgAgeLimitDtMSec() const { return msgAgeLimitDtMSec_.value_; }
    
  private:
    
    virtual void do_appendConfigurationItems(utils::InfoSpaceItems&);
    virtual void do_appendMonitoringItems(utils::InfoSpaceItems&);
    
    boost::shared_ptr<BU> bu_;
    boost::shared_ptr<EVMproxy> evmProxy_;
    boost::shared_ptr<FUproxy> fuProxy_;
    boost::shared_ptr<RUproxy> ruProxy_;
    boost::shared_ptr<DiskWriter> diskWriter_;
    boost::shared_ptr<EventTable> eventTable_;

    xdata::UnsignedInteger32 runNumber_;
    xdata::UnsignedInteger32 maxEvtsUnderConstruction_;
    xdata::UnsignedInteger32 msgAgeLimitDtMSec_;

  };
  
} } //namespace rubuilder::bu

#endif //_rubuilder_bu_StateMachine_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
