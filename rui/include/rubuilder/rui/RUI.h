#ifndef _rubuilder_rui_RUI_h_
#define _rubuilder_rui_RUI_h_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <stdint.h>

#include "rubuilder/ru/SuperFragmentTable.h"
#include "rubuilder/utils/ApplicationDescriptorAndTid.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "rubuilder/utils/OneToOneQueue.h"
#include "rubuilder/utils/PerformanceMonitor.h"
#include "rubuilder/utils/SuperFragmentGenerator.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xdaq/Application.h"
#include "xdata/Boolean.h"
#include "xdata/Double.h"
#include "xdata/Integer32.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xdata/Vector.h"
#include "xgi/Output.h"


namespace rubuilder { namespace rui { // namespace rubuilder::rui

  class StateMachine;

  /**
   * \ingroup xdaqApps
   * \brief Core RUI class
   */
  
  class RUI : public toolbox::lang::Class
  {
    
  public:

    RUI(xdaq::Application*);

    virtual ~RUI() {};
    
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
     * Print the content of the fragment FIFO as HTML snipped
     */
    inline void printFragmentFIFO(xgi::Output* out)
    { fragmentFIFO_.printVerticalHtml(out); }
    
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
     * Start generating and sending fragments
     */
    void startProcessing();

    /**
     * Stop generating and sending fragments
     */
    void stopProcessing();

  private:

    void getApplicationDescriptors();
    void startWorkLoops();
    bool generating(toolbox::task::WorkLoop*);
    bool sending(toolbox::task::WorkLoop*);
    void sendData(toolbox::mem::Reference*);
    void getPerformance(utils::PerformanceMonitor&);
    
    xdaq::Application* app_;
    uint32_t tid_;
    utils::ApplicationDescriptorAndTid ru_;
    
    volatile bool doProcessing_;
    volatile bool generatingActive_;
    volatile bool sendingActive_;

    toolbox::task::WorkLoop* generatingWL_;
    toolbox::task::WorkLoop* sendingWL_;
    toolbox::task::ActionSignature* generatingAction_;
    toolbox::task::ActionSignature* sendingAction_;

    typedef utils::OneToOneQueue<toolbox::mem::Reference*> FragmentFIFO;
    FragmentFIFO fragmentFIFO_;

    utils::SuperFragmentGenerator superFragmentGenerator_;

    struct DataMonitoring
    {
      uint64_t logicalCount;
      uint64_t i2oCount;
      uint64_t payload;
      uint64_t payloadSquared;
      uint32_t lastEventNumberToRU;
    } dataMonitoring_;
    mutable boost::mutex dataMonitoringMutex_;

    utils::PerformanceMonitor intervalStart_;
    utils::PerformanceMonitor delta_;
    mutable boost::mutex performanceMonitorMutex_;
    xdata::UnsignedInteger64 i2oEVMRUDataReadyCount_;

    utils::InfoSpaceItems ruiParams_;
    xdata::String destinationClass_;
    xdata::Integer32 destinationInstance_;
    xdata::Boolean usePlayback_;
    xdata::String playbackDataFile_;
    xdata::UnsignedInteger32 dummyBlockSize_;
    xdata::UnsignedInteger32 dummyFedPayloadSize_;
    xdata::UnsignedInteger32 dummyFedPayloadStdDev_;
    xdata::Vector<xdata::UnsignedInteger32> fedSourceIds_;
    xdata::UnsignedInteger32 fragmentFIFOCapacity_;
    xdata::UnsignedInteger32 maxFragmentsInMemory_;
  };
  
  
} } //namespace rubuilder::rui

#endif // _rubuilder_rui_RUI_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
