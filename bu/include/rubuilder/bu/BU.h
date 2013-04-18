#ifndef _rubuilder_bu_BU_h_
#define _rubuilder_bu_BU_h_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "rubuilder/utils/InfoSpaceItems.h"
#include "rubuilder/utils/OneToOneQueue.h"
#include "rubuilder/utils/PerformanceMonitor.h"
#include "toolbox/lang/Class.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xdaq/Application.h"
#include "xdata/Double.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xgi/Output.h"


namespace rubuilder { namespace bu { // namespace rubuilder::bu

  class EVMproxy;
  class RUproxy;
  class EventTable;
  class StateMachine;

  /**
   * \ingroup xdaqApps
   * \brief Core BU class
   */
  
  class BU : public toolbox::lang::Class
  {
    
  public:

    BU
    (
      xdaq::Application*,
      boost::shared_ptr<EVMproxy>,
      boost::shared_ptr<RUproxy>,
      boost::shared_ptr<EventTable>
    );

    virtual ~BU() {};
    
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
    void printHtml(xgi::Output*);
    
    /**
     * Reset the monitoring counters
     */
    void resetMonitoringCounters();
   
    /**
     * Configure
     */
    void configure();

    /**
     * Remove all data
     */
    void clear();
    
    /**
     * Register the state machine
     */
    void registerStateMachine(boost::shared_ptr<StateMachine> stateMachine)
    { stateMachine_ = stateMachine; }

    /**
     * Start processing messages
     */
    void startProcessing(const uint32_t runNumber);

    /**
     * Stop processing messages
     */
    void stopProcessing();
    
    
  private:

    void startProcessingWorkLoop();
    void startOldMsgSenderSchedulerWorkLoop();
    bool process(toolbox::task::WorkLoop*);
    bool oldMsgSenderScheduler(toolbox::task::WorkLoop*);
    bool sendOldMessages(toolbox::task::WorkLoop*);
    bool doWork();

    xdaq::Application* app_;
    boost::shared_ptr<EVMproxy> evmProxy_;
    boost::shared_ptr<RUproxy> ruProxy_;
    boost::shared_ptr<EventTable> eventTable_;
    boost::shared_ptr<StateMachine> stateMachine_;

    uint32_t runNumber_;
    volatile bool doProcessing_;
    volatile bool processActive_;

    volatile bool sendOldMessagesActionPending_;
    boost::mutex sendOldMessagesActionPendingMutex_;

    toolbox::task::WorkLoop* processingWL_;
    toolbox::task::WorkLoop* oldMsgSenderSchedulerWL_;
    toolbox::task::ActionSignature* processingAction_;
    toolbox::task::ActionSignature* sendOldMessagesAction_;

    xdata::UnsignedInteger32 oldMessageSenderSleepUSec_;

    utils::PerformanceMonitor intervalStart_;
    utils::PerformanceMonitor delta_;
    boost::mutex performanceMonitorMutex_;

    xdata::Double deltaT_;
    xdata::UnsignedInteger32 deltaN_;
    xdata::UnsignedInteger64 deltaSumOfSquares_;
    xdata::UnsignedInteger32 deltaSumOfSizes_;
  };
  
  
} } //namespace rubuilder::bu

#endif // _rubuilder_bu_BU_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
