#ifndef _rubuilder_evm_TRGproxyHandlers_h_
#define _rubuilder_evm_TRGproxyHandlers_h_

#include <stdint.h>
#include <string>

#include "rubuilder/evm/TriggerFIFO.h"
#include "rubuilder/utils/ApplicationDescriptorAndTid.h"
#include "rubuilder/utils/EvBidFactory.h"
#include "rubuilder/utils/SuperFragmentGenerator.h"
#include "rubuilder/utils/TimerManager.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/Vector.h"
#include "xgi/Output.h"


namespace rubuilder { namespace evm { // namespace rubuilder::evm

  namespace TRGproxyHandlers {
    
    struct Configuration
    {
      bool dropInputData;
      uint32_t dummyBlockSize;
      uint32_t dummyFedPayloadSize;
      uint32_t dummyFedPayloadStdDev;
      xdata::Vector<xdata::UnsignedInteger32> fedSourceIds;
      uint32_t orbitsPerLS;
      uint32_t I2O_TA_CREDIT_Packing;
      std::string taClass;
      uint32_t taInstance;
      bool usePlayback;
      std::string playbackDataFile;
    };
    
    class TriggerHandler
    {
    public:
      
      TriggerHandler() {};
      
      virtual ~TriggerHandler() {};
      
      /**
       * Fill the next available trigger message into the Reference.
       * If no trigger is available, return false.
       */
      virtual bool getNextTrigger(toolbox::mem::Reference*&) = 0;
      
      /**
       * Configure
       */
      virtual void configure(const Configuration&) {};
      
      /**
       * Print monitoring information as HTML snipped
       */
      virtual void printHtml(xgi::Output*) {};
      
      /**
       * Prepare for a new run
       */
      virtual void reset() {};
      
      /**
       * Send any stale messages to the trigger
       */
      virtual void sendOldTriggerMessage() {};

      /**
       * Stop requesting any new triggers
       */
      virtual void stopRequestingTriggers() {};
    
    };
    
    
    class FEROLhandler : public TriggerHandler
    {
    public:
      
      FEROLhandler(TriggerFIFO& triggerFIFO, const std::string& urn) :
      TriggerHandler(), triggerFIFO_(triggerFIFO), superFragmentGenerator_(urn) {};
      
      virtual ~FEROLhandler() {};
      
      virtual void configure(const Configuration&);
      virtual bool getNextTrigger(toolbox::mem::Reference*& bufRef);
      virtual void reset();
      
    private:
      TriggerFIFO& triggerFIFO_;
      utils::EvBidFactory evbIdFactory_;
      utils::SuperFragmentGenerator superFragmentGenerator_;
      uint32_t orbitsPerLS_;
    };
    
    
    class GTPhandler : public TriggerHandler
    {
    public:
      
      GTPhandler(TriggerFIFO& triggerFIFO) :
      TriggerHandler(), triggerFIFO_(triggerFIFO) {};
      
      virtual ~GTPhandler() {};
      
      virtual bool getNextTrigger(toolbox::mem::Reference*& bufRef)
      { return triggerFIFO_.deq(bufRef); }
      
    private:
      TriggerFIFO& triggerFIFO_;
    };
    
    
    class GTPehandler : public TriggerHandler
    {
    public:
      
      GTPehandler(TriggerFIFO& triggerFIFO, const std::string& urn) :
      TriggerHandler(), triggerFIFO_(triggerFIFO), superFragmentGenerator_(urn) {};
      
      virtual ~GTPehandler() {};
      
      virtual void configure(const Configuration&);
      virtual bool getNextTrigger(toolbox::mem::Reference*&);
      virtual void reset();
      
    private:
      TriggerFIFO& triggerFIFO_;
      utils::EvBidFactory evbIdFactory_;
      utils::SuperFragmentGenerator superFragmentGenerator_;
      uint32_t orbitsPerLS_;
    };
    
    
    class TAhandler : public TriggerHandler
    {
    public:
      
      TAhandler(xdaq::Application*, toolbox::mem::Pool*, TriggerFIFO&);
      
      virtual ~TAhandler() {};
      
      virtual void configure(const Configuration&);
      virtual bool getNextTrigger(toolbox::mem::Reference*&);
      virtual void stopRequestingTriggers();
      
      virtual void sendOldTriggerMessage();
      virtual void printHtml(xgi::Output*);
      virtual void reset();
      
    private:
      
      void findTA(const std::string& taClass, const uint32_t taInstance);
      void requestTriggers(const uint32_t&);
      void sendTrigCredits(const uint32_t&);
      
      xdaq::Application* app_;
      toolbox::mem::Pool* fastCtrlMsgPool_;
      TriggerFIFO& triggerFIFO_;
      uint32_t tid_;
      utils::ApplicationDescriptorAndTid ta_;
      utils::TimerManager timerManager_;
      const uint8_t timerId_;
      uint32_t I2O_TA_CREDIT_Packing_;
      uint32_t nbCreditsToBeSent_;
      bool doRequestTriggers_;
      bool firstRequest_;
      
      struct TAMonitoring
      {
        uint64_t triggersRequested;
        uint64_t payload;
        uint64_t i2oCount;
      } taMonitoring_;
    };
    
    
    class DummyTrigger : public TriggerHandler
    {
    public:
      
      DummyTrigger(const std::string& urn) :
      TriggerHandler(), superFragmentGenerator_(urn) {};
      
      virtual ~DummyTrigger() {};
      
      virtual void configure(const Configuration&);
      virtual bool getNextTrigger(toolbox::mem::Reference*&);
      virtual void stopRequestingTriggers();
      virtual void reset();
      
    private:
      
      utils::SuperFragmentGenerator superFragmentGenerator_;
      bool doRequestTriggers_;
    };
    
  } // namespace TRGproxyHandlers
    
} } // namespace rubuilder::evm

#endif // _rubuilder_evm_TRGproxyHandlers_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
