#ifndef _rubuilder_evm_EVM_h_
#define _rubuilder_evm_EVM_h_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "rubuilder/utils/EvBidFactory.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "rubuilder/utils/PerformanceMonitor.h"
#include "toolbox/lang/Class.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xdaq/Application.h"
#include "xdata/Boolean.h"
#include "xdata/Double.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xgi/Output.h"


namespace rubuilder { namespace evm { // namespace rubuilder::evm

  class TRGproxy;
  class RUproxy;
  class BUproxy;
  class L1InfoHandler;
  class StateMachine;

  /**
   * \ingroup xdaqApps
   * \brief Core EVM class
   */
  
  class EVM : public toolbox::lang::Class
  {
    
  public:

    EVM
    (
      xdaq::Application*,
      boost::shared_ptr<TRGproxy>,
      boost::shared_ptr<RUproxy>,
      boost::shared_ptr<BUproxy>,
      boost::shared_ptr<L1InfoHandler>
    );

    virtual ~EVM() {};
    
    /**
     * Append the info space parameters used for the
     * configuration to the InfoSpaceItems
     */
    void appendConfigurationItems(utils::InfoSpaceItems&);
    
    /**
     * Append the info space items to be published in the 
     * monitoring info space to the InfoSpaceItems
     */
    void appendMonitoringItems(utils::InfoSpaceItems&);
    
    /**
     * Update all values of the items put into the monitoring
     * info space. The caller has to make sure that the info
     * space where the items reside is locked and properly unlocked
     * after the call.
     */
    void updateMonitoringItems();

    /**
     * Print monitoring/configuration as HTML snipped
     */
    void printHtml
    (
      xgi::Output*,
      const uint32_t monitoringSleepSec
    );
    
    /**
     * Reset the monitoring counters
     */
    void resetMonitoringCounters();

    /**
     * Register the state machine
     */
    void registerStateMachine(boost::shared_ptr<StateMachine> stateMachine)
    { stateMachine_ = stateMachine; }

    /**
     * Start (local) triggers and process messages
     */
    void startProcessing();

    /**
     * Stop processing messages
     */
    void stopProcessing();

    /**
     * Return true if no events are in the RUbuilder
     */
    inline bool ruBuilderIsFlushed()
    { return ruBuilderIsFlushed_; }

    /**
     * Return the reason why the EVM cannot be flushed
     */
    inline std::string getReasonForNotFlushed() const
    { return reasonForNotFlushed_; }

  private:

    void startProcessingWorkLoop();
    void startOldMsgSenderSchedulerWorkLoop();
    bool process(toolbox::task::WorkLoop*);
    bool oldMsgSenderScheduler(toolbox::task::WorkLoop*);
    bool sendOldMessages(toolbox::task::WorkLoop*);
    bool doWork();

    xdaq::Application* app_;
    boost::shared_ptr<TRGproxy> trgProxy_;
    boost::shared_ptr<RUproxy> ruProxy_;
    boost::shared_ptr<BUproxy> buProxy_;
    boost::shared_ptr<L1InfoHandler> l1InfoHandler_;
    boost::shared_ptr<StateMachine> stateMachine_;
    utils::EvBidFactory evbIdFactory_;

    volatile bool doProcessing_;
    volatile bool processActive_;
    volatile bool sendOldMessagesActionPending_;
    boost::mutex sendOldMessagesActionPendingMutex_;

    toolbox::task::WorkLoop* processingWL_;
    toolbox::task::WorkLoop* oldMsgSenderSchedulerWL_;
    toolbox::task::ActionSignature* processingAction_;
    toolbox::task::ActionSignature* sendOldMessagesAction_;

    xdata::UnsignedInteger32 nbEvtIdsInBuilder_;
    xdata::UnsignedInteger32 oldMessageSenderSleepUSec_;

    std::string reasonForNotFlushed_;
    utils::PerformanceMonitor intervalStart_;
    utils::PerformanceMonitor delta_;
    boost::mutex performanceMonitorMutex_;

    xdata::UnsignedInteger32 nbTriggersInEVM_;
    xdata::UnsignedInteger32 nbEvtsBuilt_;
    xdata::UnsignedInteger32 nbEvtsInBuilder_;

    xdata::Double deltaT_;
    xdata::UnsignedInteger32 deltaN_;
    xdata::UnsignedInteger64 deltaSumOfSquares_;
    xdata::UnsignedInteger32 deltaSumOfSizes_;

    xdata::UnsignedInteger32 runNumber_;
    xdata::UnsignedInteger32 monitoringRunNumber_;
    xdata::Boolean ruBuilderIsFlushed_;
  };
  
  
} } //namespace rubuilder::evm

#endif // _rubuilder_evm_EVM_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
