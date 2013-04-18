#ifndef _rubuilder_utils_RUbroadcaster_h_
#define _rubuilder_utils_RUbroadcaster_h_

#include <boost/thread/mutex.hpp>

#include <set>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

#include "log4cplus/logger.h"

#include "interface/evb/i2oEVBMsgs.h"
#include "rubuilder/utils/ApplicationDescriptorAndTid.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdata/Boolean.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/Vector.h"



namespace rubuilder { namespace utils { // namespace rubuilder::utils

  /**
   * \ingroup xdaqApps
   * \brief Send I2O messages to multiple RUs
   */
  
  class RUbroadcaster
  {
    
  public:

    RUbroadcaster
    (
      xdaq::Application*,
      toolbox::mem::Pool*
    );

    virtual ~RUbroadcaster() {};
    
    /**
     * Init the RU instances and append necessary
     * configuration parameters to the InfoSpaceItems
     */
    void initRuInstances(InfoSpaceItems&);
    
    /**
     * Find the application descriptors of the participating RUs.
     *
     * By default, all RUs in the zone will participate.
     * This can be overwritten by setting either the instances
     * in 'ruInstances' or using a fragment set. In the latter
     * case, 'useFragmentSet' has to be set to true and the 
     * 'fragmentSetsUrl' and 'fragmentSetId' need to be specified.
     */
    void getApplicationDescriptors();

    /**
     * Send the data contained in the reference to all RUs.
     */
    void sendToAllRUs
    (
      toolbox::mem::Reference*,
      const size_t bufSize
    );
    
    /**
     * Return the number of RUs participating in the event building
     */
    size_t getRuCount() const
    { return ruCount_; }

    
  protected:

    xdaq::Application* app_;
    toolbox::mem::Pool* fastCtrlMsgPool_;
    log4cplus::Logger& logger_;
    uint32_t tid_;
    
    typedef std::set<ApplicationDescriptorAndTid>
    RUDescriptorsAndTids;
    RUDescriptorsAndTids participatingRUs_;
    
  private:
    
    void getRuInstances();
    void fillParticipatingRUsUsingFragmentSet();
    void fillParticipatingRUsUsingRuInstances();
    void fillRUInstance(xdata::UnsignedInteger32);

    typedef xdata::Vector<xdata::UnsignedInteger32> RUInstances;
    RUInstances ruInstances_;
    boost::mutex ruInstancesMutex_;
    size_t ruCount_;

    xdata::Boolean useFragmentSet_;
    xdata::String fragmentSetsUrl_;
    xdata::UnsignedInteger32 fragmentSetId_;
  };
  
  
} } //namespace rubuilder::utils

#endif // _rubuilder_utils_RUbroadcaster_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
