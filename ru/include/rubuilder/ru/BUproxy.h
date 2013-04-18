#ifndef _rubuilder_ru_BUproxy_h_
#define _rubuilder_ru_BUproxy_h_

#include <boost/thread/mutex.hpp>

#include <stdint.h>
#include <set>
#include <vector>

#include "log4cplus/logger.h"

#include "rubuilder/ru/SuperFragmentTable.h"
#include "rubuilder/utils/I2OMessages.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdaq/ApplicationDescriptor.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xdata/Vector.h"
#include "xgi/Output.h"


namespace rubuilder { namespace ru { // namespace rubuilder::ru

  /**
   * \ingroup xdaqApps
   * \brief Proxy for BU-RU communication
   */
  
  class BUproxy
  {
    
  public:

    BUproxy
    (
      xdaq::Application*,
      SuperFragmentTablePtr
    );

    virtual ~BUproxy() {};
    
    /**
     * Callback for I2O message received from EVM
     */
    void I2Ocallback(toolbox::mem::Reference*);
    
    /**
     * Send the data contained in the reference to the
     * BU specified in the request
     */
    void sendData(const SuperFragmentTable::Request&, toolbox::mem::Reference*);

    /**
     * Configure the BU proxy
     */
    void configure();

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
     * Remove all data
     */
    void clear();

    /**
     * Return the logical number of I2O_BU_CACHE messages
     * received since the last call to resetMonitoringCounters
     */
    uint64_t i2oBUCacheCount() const
    { return dataMonitoring_.logicalCount; }
  
    /**
     * Print monitoring/configuration as HTML snipped
     */
    void printHtml(xgi::Output*);


  private:
    
    void updateRequestCounters(const msg::RqstForFragsMsg*);
    void handleRequest(const msg::RqstForFragsMsg*);
    void getBuInstances();

    xdaq::Application* app_;
    log4cplus::Logger& logger_;
    SuperFragmentTablePtr superFragmentTable_;
    uint32_t tid_;
    uint32_t instance_;
    typedef std::set<uint32_t> BUInstances;
    BUInstances buInstances_;
    boost::mutex buInstancesMutex_;

    typedef std::map<uint32_t,uint64_t> CountsPerBU;
    struct RequestMonitoring
    {
      uint64_t logicalCount;
      uint64_t payload;
      uint64_t i2oCount;
      CountsPerBU logicalCountPerBU;
    } requestMonitoring_;
    boost::mutex requestMonitoringMutex_;

    struct DataMonitoring
    {
      uint32_t lastEventNumberToBUs;
      uint64_t logicalCount;
      uint64_t payload;
      uint64_t i2oCount;
      CountsPerBU payloadPerBU;
    } dataMonitoring_;
    boost::mutex dataMonitoringMutex_;

    xdata::UnsignedInteger32 lastEventNumberToBUs_;
    xdata::UnsignedInteger32 nbSuperFragmentsReady_;
    xdata::UnsignedInteger64 i2oBUCacheCount_;
    xdata::Vector<xdata::UnsignedInteger64> i2oRUSendCountBU_;
    xdata::Vector<xdata::UnsignedInteger64> i2oBUCachePayloadBU_;
  };
  
  
} } //namespace rubuilder::ru

#endif // _rubuilder_ru_BUproxy_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
