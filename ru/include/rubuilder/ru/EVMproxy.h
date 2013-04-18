#ifndef _rubuilder_ru_EVMproxy_h_
#define _rubuilder_ru_EVMproxy_h_

#include <boost/thread/mutex.hpp>

#include <stdint.h>

#include "log4cplus/logger.h"

#include "rubuilder/utils/EvBid.h"
#include "rubuilder/utils/I2OMessages.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "rubuilder/utils/OneToOneQueue.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xgi/Output.h"


namespace rubuilder { namespace ru { // namespace rubuilder::ru

  /**
   * \ingroup xdaqApps
   * \brief Proxy for EVM-RU communication
   */
  
  class EVMproxy
  {
    
  public:

    EVMproxy
    (
      xdaq::Application*
    );

    virtual ~EVMproxy() {};
    
    /**
     * Callback for I2O message received from EVM
     */
    void I2Ocallback(toolbox::mem::Reference*);
    
    /**
     * Fill the next available event builder id
     * into the passed reference.
     * Return false if no id is available
     */
    bool getTrigEvBid(utils::EvBid&);

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
    void configure();

    /**
     * Remove all data
     */
    void clear();
  
    /**
     * Print monitoring/configuration as HTML snipped
     */
    void printHtml(xgi::Output*);

    /**
     * Print the content of the EvB id FIFO as HTML snipped
     */
    inline void printEvBidFIFO(xgi::Output* out)
    { evbIdFIFO_.printVerticalHtml(out); }


  private:
    
    void updateReadoutCounters(const msg::EvBidsMsg*);
    void handleReadoutMsg(const msg::EvBidsMsg*);

    xdaq::Application* app_;
    log4cplus::Logger& logger_;

    typedef utils::OneToOneQueue<utils::EvBid> EvBidFIFO;
    EvBidFIFO evbIdFIFO_;
        
    struct EVMMonitoring
    {
      uint64_t logicalCount;
      uint64_t payload;
      uint64_t i2oCount;
      uint32_t lastEventNumberFromEVM;
    } evmMonitoring_;
    boost::mutex evmMonitoringMutex_;
    
    utils::InfoSpaceItems evmParams_;
    xdata::UnsignedInteger32 evbIdFIFOCapacity_;
    
    xdata::UnsignedInteger32 lastEventNumberFromEVM_;
    xdata::UnsignedInteger64 i2oRUReadoutCount_;
  };
  
  
} } //namespace rubuilder::ru

#endif // _rubuilder_ru_EVMproxy_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
