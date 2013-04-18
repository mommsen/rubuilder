#ifndef _rubuilder_evm_SMproxy_h_
#define _rubuilder_evm_SMproxy_h_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <set>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

#include "log4cplus/logger.h"

#include "interface/evb/i2oEVBMsgs.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdata/Boolean.h"
#include "xdata/Serializable.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/Vector.h"

#include "rubuilder/utils/ApplicationDescriptorAndTid.h"


namespace rubuilder { namespace evm { // namespace rubuilder::evm

  class EoLSHandler;

  /**
   * \ingroup xdaqApps
   * \brief Proxy for EVM-SM communication
   */
  
  class SMproxy
  {
    
  public:

    SMproxy
    (
      xdaq::Application*,
      boost::shared_ptr<EoLSHandler>,
      toolbox::mem::Pool*
    );

    virtual ~SMproxy() {};
    
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
    void configure() {};

    /**
     * Remove all data
     */
    void clear() {};

    /**
     * Print monitoring/configuration as HTML snipped
     */
    void printHtml(xgi::Output*);

    /**
     * Find the application descriptors of the participating SMs
     */
    void getApplicationDescriptors();

    /**
     * Send the end-of-lumi-section message to all SMs.
     */
    void sendEoLSmsg(const I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME*);


  private:

    void discoverParticipatingSMs();
    void fillParticipatingSMsUsingSMInstances();

    xdaq::Application* app_;
    toolbox::mem::Pool* fastCtrlMsgPool_;
    log4cplus::Logger& logger_;
    uint32_t tid_;

    utils::InfoSpaceItems smParams_;

    typedef xdata::Vector<xdata::UnsignedInteger32> SMInstances;
    SMInstances smInstances_;

    typedef std::set<utils::ApplicationDescriptorAndTid> ParticipatingSMs;
    ParticipatingSMs participatingSMs_;

    xdata::Boolean autoDiscoverSM_;

    struct EoLSMonitoring
    {
      uint64_t payload;
      uint64_t msgCount;
      uint64_t i2oCount;
    } EoLSMonitoring_;
    boost::mutex EoLSMonitoringMutex_;
  };
  
  
} } //namespace rubuilder::evm

#endif // _rubuilder_evm_SMproxy_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
