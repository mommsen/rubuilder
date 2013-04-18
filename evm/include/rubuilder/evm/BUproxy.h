#ifndef _rubuilder_evm_BUproxy_h_
#define _rubuilder_evm_BUproxy_h_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <set>
#include <stdint.h>
#include <string>
#include <utility>

#include "i2o/i2oDdmLib.h"
#include "log4cplus/logger.h"

#include "interface/evb/i2oEVBMsgs.h"
#include "rubuilder/evm/EoLSHandler.h"
#include "rubuilder/evm/LumiSectionTable.h"
#include "rubuilder/utils/EvBid.h"
#include "rubuilder/utils/I2OMessages.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "rubuilder/utils/OneToOneQueue.h"
#include "rubuilder/utils/OneToOneQueueCollection.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdata/Boolean.h"
#include "xdata/Serializable.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xgi/Output.h"


namespace rubuilder { namespace evm { // namespace rubuilder::evm

  /**
   * \ingroup xdaqApps
   * \brief Proxy for EVM-BU communication
   */
  
  class BUproxy
  {
    
  public:

    struct EventFifoElement
    {
      toolbox::mem::Reference *trigBufRef;
      utils::EvBid evbId;
      uint32_t runNumber;
      uint32_t lumiSection;
    };

    struct RqstFifoElement
    {
      I2O_TID buTid;
      uint32_t buIndex;
      uint32_t resourceId;
    };
    
    struct ReleasedEvtIdFifoElement
    {
      utils::EvBid evbId;
      uint32_t buIndex;
      uint32_t resourceId;
    };


    BUproxy
    (
      xdaq::Application*,
      boost::shared_ptr<EoLSHandler>,
      toolbox::mem::Pool*
    );

    virtual ~BUproxy() {};

    /**
     * Callback for I2O message received from BU
     */
    void I2Ocallback(toolbox::mem::Reference*);

    /**
     * Add the TriggerFifoElement to the event message queue
     * for the BU. If there's a request from the BU,
     * use it to satisfy the request.
     */
    void addEvent(const EventFifoElement&);

    /**    
     * Service a single BU request if possible.
     * Returns true if an event was served.
     */
    bool serviceBuRqst();

    /**
     * Process the next released event.
     * Return true if an event was released.
     */
    bool processNextReleasedEvent();

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
     * Find the application descriptors of the participating BUs
     */
    void getApplicationDescriptors();

    /**
     * Send the end-of-lumi-section message to all BUs.
     */
    void sendEoLSmsg(const I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME*);

    /**
     * Return the logical count of confirm messages sent to the BUs
     */
    inline uint64_t getConfirmLogicalCount() const
    { return confirmCounters_.logicalCount; }

    /**
     * Print monitoring/configuration as HTML snipped
     */
    void printHtml(xgi::Output*);

    /**
     * Print the content of the event FIFO as HTML snipped
     */
    inline void printEventFIFO(xgi::Output* out)
    { eventFIFO_.printVerticalHtml(out); }

    /**
     * Print the content of the request FIFO as HTML snipped
     */
    inline void printRequestFIFOs(xgi::Output* out)
    { requestFIFOs_.printVerticalHtml(out); }

    /**
     * Print the content of the released event id FIFO as HTML snipped
     */
    inline void printReleasedEvbIdFIFO(xgi::Output* out)
    { releasedEvbIdFIFO_.printVerticalHtml(out); }

    /**
     * Print the content of the released event id FIFO as HTML snipped
     */
    inline void printEoLSFIFOs(xgi::Output* out)
    { eolsFIFOs_.printVerticalHtml(out); }


  private:

    void pushRequestOntoFIFOs(const msg::EvtIdRqstsAndOrReleasesMsg*, const I2O_TID&);
    void requestEvent(const RqstFifoElement&);
    void releaseEvent(const ReleasedEvtIdFifoElement&);
    void updateAllocateClearCounters(const uint32_t& nbElements);
    void updateConfirmCounters(const I2O_MESSAGE_FRAME*);
    void sendEoLStoBU(const RqstFifoElement&, toolbox::mem::Reference*);

    xdaq::Application* app_;
    LumiSectionTable lumiSectionTable_;
    toolbox::mem::Pool* fastCtrlMsgPool_;
    log4cplus::Logger& logger_;
    uint32_t tid_;

    typedef std::map<utils::EvBid,EoLSHandler::LumiSectionPair> EvBidMap;
    EvBidMap evbIdMap_;

    typedef utils::OneToOneQueue<EventFifoElement> EventFIFO;
    EventFIFO eventFIFO_;

    typedef utils::OneToOneQueueCollection<RqstFifoElement> RequestFIFOs;
    RequestFIFOs requestFIFOs_;
    boost::mutex requestFIFOsMutex_;
    
    typedef utils::OneToOneQueue<ReleasedEvtIdFifoElement> ReleasedEvbIdFIFO;
    ReleasedEvbIdFIFO releasedEvbIdFIFO_;

    typedef utils::OneToOneQueueCollection<toolbox::mem::Reference*> EolsFIFOs;
    EolsFIFOs eolsFIFOs_;
    typedef std::set<xdaq::ApplicationDescriptor*> BUdescriptors;
    BUdescriptors buDescriptors_;
    
    struct AllocateClearCounters
    {
      uint64_t payload;
      uint64_t logicalCount;
      uint64_t i2oCount;
    } allocateClearCounters_;
    boost::mutex allocateClearCountersMutex_;

    struct ConfirmCounters
    {
      uint64_t payload;
      uint64_t logicalCount;
      uint64_t i2oCount;
      uint32_t lastEventNumberToBUs;
    } confirmCounters_;
    boost::mutex confirmCountersMutex_;

    struct EoLSMonitoring
    {
      uint64_t payload;
      uint64_t msgCount;
      uint64_t i2oCount;
    } EoLSMonitoring_;
    boost::mutex EoLSMonitoringMutex_;
    
    utils::InfoSpaceItems buParams_;
    xdata::UnsignedInteger32 eventFIFOCapacity_;
    xdata::UnsignedInteger32 requestFIFOCapacity_;
    xdata::UnsignedInteger32 releasedEvbIdFIFOCapacity_;
    xdata::UnsignedInteger32 eolsFIFOCapacity_;
    xdata::Boolean assignRoundRobin_;
    
    xdata::UnsignedInteger32 lastEventNumberToBUs_;
    xdata::UnsignedInteger64 i2oEVMAllocClearCount_;
    xdata::UnsignedInteger64 i2oBUConfirmLogicalCount_;

  };

} } // namespace rubuilder::evm

inline std::ostream& operator<<
(
  std::ostream& s,
  const rubuilder::evm::BUproxy::EventFifoElement& element
)
{
  s << element.evbId  << " ";
  s << "lumiSection=" << element.lumiSection << " ";
  s << "runNumber="   << element.runNumber;
  
  return s;
}

inline std::ostream& operator<<
(
  std::ostream& s,
  const rubuilder::evm::BUproxy::RqstFifoElement& element
)
{
  s << "buTid="      << element.buTid   << " ";
  s << "buIndex="    << element.buIndex << " ";
  s << "resourceId=" << element.resourceId;
  
  return s;
}

inline std::ostream& operator<<
(
  std::ostream& s,
  const rubuilder::evm::BUproxy::ReleasedEvtIdFifoElement& element
)
{
  s << element.evbId  << " ";
  s << "buIndex="     << element.buIndex     << " ";
  s << "resourceId="  << element.resourceId;
  
  return s;
}

#endif // _rubuilder_evm_BUproxy_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
