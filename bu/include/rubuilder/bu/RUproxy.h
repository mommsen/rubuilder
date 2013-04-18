#ifndef _rubuilder_bu_RUproxy_h_
#define _rubuilder_bu_RUproxy_h_

#include <boost/thread/mutex.hpp>

#include <stdint.h>

#include "log4cplus/logger.h"

#include "rubuilder/utils/InfoSpaceItems.h"
#include "rubuilder/utils/OneToOneQueue.h"
#include "rubuilder/utils/RUbroadcaster.h"
#include "rubuilder/utils/TimerManager.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xgi/Output.h"


namespace rubuilder { namespace bu { // namespace rubuilder::bu

  /**
   * \ingroup xdaqApps
   * \brief Proxy for EVM-BU communication
   */
  
  class RUproxy : public utils::RUbroadcaster
  {
    
  public:

    RUproxy
    (
      xdaq::Application*,
      toolbox::mem::Pool*
    );

    virtual ~RUproxy() {};
    
    /**
     * Callback for I2O message received from EVM
     */
    void I2Ocallback(toolbox::mem::Reference*);
    
    /**
     * Fill the next available data block
     * into the passed buffer reference.
     * Return false if no trigger block is available
     */
    bool getDataBlock(toolbox::mem::Reference*&);

    /**
     * Send request for data fragments to the RUs for
     * the passed trigger message
     */
    void requestDataForTrigger(toolbox::mem::Reference*);

    /**
     * Send any old, but incomplete request message to the RUs.
     * Returns true if a message was sent.
     */
    bool sendOldRequests();

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
     * Reset the monitoring counters
     */
    void resetMonitoringCounters();
   
    /**
     * Configure
     */
    void configure(const int msgAgeLimitDtMSec);

    /**
     * Remove all data
     */
    void clear();
  
    /**
     * Print monitoring/configuration as HTML snipped
     */
    void printHtml(xgi::Output*);

    /**
     * Print the content of the data block FIFO as HTML snipped
     */
    inline void printBlockFIFO(xgi::Output* out)
    { blockFIFO_.printVerticalHtml(out); }
    
    
  private:
    
    void createRqstForFrags();
    uint32_t packRqstForFragsMsg(toolbox::mem::Reference*);
    void sendRqstForFragsToAllRUs();

    void updateBlockCounters(toolbox::mem::Reference*);
    void updateRequestCounters();
    
    uint32_t index_;

    typedef utils::OneToOneQueue<toolbox::mem::Reference*> BlockFIFO;
    BlockFIFO blockFIFO_;
    
    toolbox::mem::Reference* rqstForFragsBufRef_;
    boost::mutex rqstForFragsMutex_;
    size_t rqstForFragsBufSize_;
    
    utils::TimerManager timerManager_;
    const uint8_t timerId_;

    typedef std::map<uint32_t,uint64_t> CountsPerRU;
    struct BlockMonitoring
    {
      uint64_t logicalCount;
      uint64_t payload;
      uint64_t i2oCount;
      uint32_t lastEventNumberFromRUs;
      CountsPerRU logicalCountPerRU;
      CountsPerRU payloadPerRU;
    } blockMonitoring_;
    boost::mutex blockMonitoringMutex_;

    struct RequestMonitoring
    {
      uint64_t logicalCount;
      uint64_t payload;
      uint64_t i2oCount;
    } requestMonitoring_;
    boost::mutex requestMonitoringMutex_;
    
    utils::InfoSpaceItems ruParams_;
    xdata::UnsignedInteger32 blockFIFOCapacity_;
    xdata::UnsignedInteger32 I2O_RU_SEND_Packing_;

    xdata::UnsignedInteger32 lastEventNumberFromRUs_;
    xdata::UnsignedInteger64 i2oBUCacheCount_;
    xdata::UnsignedInteger64 i2oRUSendCount_;
  };
  
  
} } //namespace rubuilder::bu

#endif // _rubuilder_bu_RUproxy_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
