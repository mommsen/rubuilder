#ifndef _rubuilder_evm_RUproxy_h_
#define _rubuilder_evm_RUproxy_h_

#include <boost/thread/mutex.hpp>

#include <stdint.h>

#include "log4cplus/logger.h"

#include "rubuilder/utils/EvBid.h"
#include "rubuilder/utils/I2OMessages.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "rubuilder/utils/RUbroadcaster.h"
#include "rubuilder/utils/TimerManager.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xgi/Output.h"


namespace rubuilder { namespace evm { // namespace rubuilder::evm

  /**
   * \ingroup xdaqApps
   * \brief Proxy for EVM-RU communication
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
     * Add the EvBid to the EvBids message for the RU.
     * If the msg packing 'I2O_RU_READOUT_Packing' is reached,
     * send the message to the RUs.
     */
    void addEvBid(const utils::EvBid&);

    /**
     * Send any old, but incomplete EvBids message to the RUs.
     * Returns true if a message was sent.
     */
    bool sendOldEvBids();


  private:
    
    void sendEvBidsToAllRUs();
    uint32_t packEvBidsMsg(const utils::EvBid&);
    void createNewReadoutMsg();
    void updateCounters();
    
    toolbox::mem::Reference *ruReadoutBufRef_;
    boost::mutex ruReadoutMutex_;
    size_t ruReadoutBufSize_;

    utils::TimerManager timerManager_;
    const uint8_t timerId_;

    struct RUMonitoring
    {
      uint64_t msgCount;
      uint64_t payload;
      uint64_t i2oCount;
      uint32_t lastEventNumberToRUs;
    } ruMonitoring_;
    boost::mutex ruMonitoringMutex_;

    utils::InfoSpaceItems ruParams_;
    xdata::UnsignedInteger32 I2O_RU_READOUT_Packing_;
    xdata::UnsignedInteger32 msgAgeLimitDtMSec_;

    xdata::UnsignedInteger32 lastEventNumberToRUs_;
    xdata::UnsignedInteger64 i2oRUReadoutCount_;
  };
  
  
} } //namespace rubuilder::evm

#endif // _rubuilder_evm_RUproxy_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
