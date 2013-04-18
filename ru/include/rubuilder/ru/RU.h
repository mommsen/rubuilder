#ifndef _rubuilder_ru_RU_h_
#define _rubuilder_ru_RU_h_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <stdint.h>

#include "rubuilder/ru/SuperFragmentTable.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "rubuilder/utils/PerformanceMonitor.h"
#include "rubuilder/utils/TimerManager.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xdaq/Application.h"
#include "xdata/Double.h"
#include "xdata/Integer32.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xgi/Output.h"


namespace rubuilder { namespace ru { // namespace rubuilder::ru

  class BUproxy;
  class EVMproxy;
  class RUinput;
  class StateMachine;

  /**
   * \ingroup xdaqApps
   * \brief Core RU class
   */
  
  class RU : public toolbox::lang::Class
  {
    
  public:

    RU
    (
      xdaq::Application*,
      SuperFragmentTablePtr,
      boost::shared_ptr<BUproxy>,
      boost::shared_ptr<EVMproxy>,
      boost::shared_ptr<RUinput>
    );

    virtual ~RU() {};
    
    /**
     * Register the state machine
     */
    void registerStateMachine(boost::shared_ptr<StateMachine> stateMachine)
    { stateMachine_ = stateMachine; }
    
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
     * Configure
     */
    void configure();

    /**
     * Remove all data
     */
    void clear();
    
    /**
     * Start (local) triggers and process messages
     */
    void startProcessing();

    /**
     * Stop processing messages
     */
    void stopProcessing();

  private:

    void startProcessingWorkLoop();
    bool process(toolbox::task::WorkLoop*);
    void updateSuperFragmentCounters(toolbox::mem::Reference*);
    void getPerformance(utils::PerformanceMonitor&);

    xdaq::Application* app_;
    SuperFragmentTablePtr superFragmentTable_;
    boost::shared_ptr<BUproxy> buProxy_;
    boost::shared_ptr<EVMproxy> evmProxy_;
    boost::shared_ptr<RUinput> ruInput_;
    boost::shared_ptr<StateMachine> stateMachine_;

    volatile bool doProcessing_;
    volatile bool processActive_;

    toolbox::task::WorkLoop* processingWL_;
    toolbox::task::ActionSignature* processingAction_;

    utils::TimerManager timerManager_;
    const uint8_t timerId_;

    struct SuperFragmentMonitoring
    {
      uint64_t count;
      uint64_t payload;
      uint64_t payloadSquared;
    } superFragmentMonitoring_;
    boost::mutex superFragmentMonitoringMutex_;

    utils::PerformanceMonitor intervalStart_;
    utils::PerformanceMonitor delta_;
    boost::mutex performanceMonitorMutex_;

    xdata::Double deltaT_;
    xdata::UnsignedInteger32 deltaN_;
    xdata::UnsignedInteger64 deltaSumOfSquares_;
    xdata::UnsignedInteger32 deltaSumOfSizes_;

    xdata::UnsignedInteger32 runNumber_;
    xdata::Integer32 maxPairAgeMSec_;
    
    xdata::UnsignedInteger32 monitoringRunNumber_;
    xdata::UnsignedInteger32 nbSuperFragmentsInRU_;
  };
  
  
} } //namespace rubuilder::ru

#endif // _rubuilder_ru_RU_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
