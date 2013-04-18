#ifndef _rubuilder_evm_L1InfoHandler_h_
#define _rubuilder_evm_L1InfoHandler_h_

#include <list>
#include <stdint.h>
#include <string>
#include <vector>

#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "log4cplus/logger.h"

#include "interface/evb/i2oEVBMsgs.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xdaq/Application.h"
#include "xdaq2rc/RcmsNotificationSender.h"
#include "xdata/Boolean.h"
#include "xdata/Serializable.h"
#include "xdata/Table.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/Vector.h"
#include "xgi/Output.h"

#include "rubuilder/utils/Constants.h"
#include "rubuilder/utils/EventUtils.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "rubuilder/utils/OneToOneQueue.h"


namespace rubuilder { namespace evm { // namespace rubuilder::evm

class EoLSHandler;

  /**
   * \ingroup xdaqApps
   * \brief Handler for L1 trigger information
   */
  
  class L1InfoHandler : public toolbox::lang::Class
  {
    
  public:

    L1InfoHandler
    (
      xdaq::Application*,
      boost::shared_ptr<EoLSHandler>,
      toolbox::mem::Pool*
    );

    virtual ~L1InfoHandler() {};
    
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
     * Return true if there is no more L1 information to be processed
     */
    bool empty()
    { return ( l1InfoFIFO_.empty() && lumiSectionInfoFIFO_.empty() && idle_ ); }
    
    /**
     * Send the current lumi-section information
     */
    void enqCurrentLumiSectionInfo();

    /**
     * Extract the L1 trigger information from the trigger message.
     * Return the encoded offline lumi section number or
     * 0 if the message cannot be decoded.
     */
    uint32_t extractL1Info(toolbox::mem::Reference*, const uint32_t runNumber);

    /**
     * Print monitoring/configuration as HTML snipped
     */
    void printHtml(xgi::Output*);

    /**
     * Print a HTML snipped to the output stream containing the last completed
     * L1 scaler information as sent to the HLTSG
     */
    void printL1Scalers(xgi::Output*);

    /**
     * Print the content of the L1 info FIFO as HTML snipped
     */
    inline void printL1InfoFIFO(xgi::Output* out)
    { l1InfoFIFO_.printVerticalHtml(out); }

    /**
     * Print the content of the lumi section info FIFO as HTML snipped
     */
    inline void printLumiSectionInfoFIFO(xgi::Output* out)
    { lumiSectionInfoFIFO_.printVerticalHtml(out); }

  private:

    typedef boost::array<uint32_t,utils::TRIGGER_BITS_COUNT> L1TriggerBitsArray;
    
    struct LumiSectionInfo
    {
      uint32_t runNumber;
      uint32_t lsNumber;
      uint32_t skippedLumiSections;
      uint32_t eventCount;
      uint32_t physicsTriggers;
      uint32_t calibrationTriggers;
      uint32_t randomTriggers;
      L1TriggerBitsArray l1Technical;
      L1TriggerBitsArray l1Decision_0_63;
      L1TriggerBitsArray l1Decision_64_127;
    };

    toolbox::mem::Reference* getNewLumiSectionInfo
    (
      uint32_t runNumber,
      uint32_t lumiSection,
      uint32_t previousLumiSection
    );
    toolbox::mem::Reference* getNewL1Information();
    void createL1ScalersInfoSpace();
    void initializeL1ScalersTable();
    void initializeNotificationReceiver();
    utils::InfoSpaceItems initAndGetL1ScalersParams();
    bool fillL1Info(toolbox::mem::Reference* trigMsg, utils::L1Information*);
    bool extractInfoFromGTP(const unsigned char* fedptr, utils::L1Information*);
    void verifyLumiSection(const utils::L1Information*);

    void startL1InfoWorkLoop();
    void startL1ScalersWorkLoop();

    bool processL1Info(toolbox::task::WorkLoop*);
    void processAllAvailableL1Infos();
    void addLumiSectionInfo(const utils::L1Information*);
    void addEventCounts(LumiSectionInfo*, const utils::L1Information*) const;
    void addTriggerBits(LumiSectionInfo*, const utils::L1Information*) const;
    void sumBits(const uint64_t& triggerBits, L1TriggerBitsArray& sums) const;
    void addSkippedLumiSectionsToFIFO(const LumiSectionInfo*);

    bool sendL1Scalers(toolbox::task::WorkLoop*);
    void sendAllAvailableL1Scalers();
    void updateL1Scalers(const LumiSectionInfo*);
    void sendL1ScalersMsg();
    
    xdaq::Application* app_;
    boost::shared_ptr<EoLSHandler> eolsHandler_;
    toolbox::mem::Pool* fastCtrlMsgPool_;
    log4cplus::Logger& logger_;
    xdaq2rc::RcmsNotificationSender rcmsNotifier_;
    toolbox::task::WorkLoop* l1InfoWL_;
    toolbox::task::WorkLoop* l1ScalersWL_;
    bool idle_;
    
    typedef utils::OneToOneQueue<toolbox::mem::Reference*> L1InfoFIFO;
    L1InfoFIFO l1InfoFIFO_;
    xdata::UnsignedInteger32 l1InfoFIFOCapacity_;

    typedef utils::OneToOneQueue<toolbox::mem::Reference*> LumiSectionInfoFIFO;
    LumiSectionInfoFIFO lumiSectionInfoFIFO_;
    xdata::UnsignedInteger32 lumiSectionInfoFIFOCapacity_;
    
    uint32_t lastSeenLumiSection_;   
    toolbox::mem::Reference* currentLumiSectionInfo_;
    boost::mutex currentLumiSectionInfoMutex_;

    xdata::InfoSpace *l1ScalersInfoSpace_;
    utils::InfoSpaceItems l1ScalersParams_;
    std::list<std::string> l1ScalersParamNames_;
    xdata::Table l1ScalersTable_;
    xdata::UnsignedInteger32 instance_;
    xdata::UnsignedInteger32 runnumber_;
    xdata::UnsignedInteger32 lsnumber_;
    xdata::UnsignedInteger32 eventcount_;
    xdata::UnsignedInteger32 physicstriggers_;
    xdata::UnsignedInteger32 calibrationtriggers_;
    xdata::UnsignedInteger32 randomtriggers_;
    xdata::UnsignedInteger32 skippedlumisections_;
    xdata::Vector<xdata::UnsignedInteger32> l1technical_;
    xdata::Vector<xdata::UnsignedInteger32> l1decision_0_63_;
    xdata::Vector<xdata::UnsignedInteger32> l1decision_64_127_;

    uint32_t localLastL1InfoForLS_;
    std::string lastL1decodeError_;

    xdata::UnsignedInteger32 lastL1InfoForLS_;

    xdata::Boolean enableL1Info_;
    xdata::Boolean sendL1FlashList_;
  };
  
  
} } //namespace rubuilder::evm

#endif // _rubuilder_evm_L1InfoHandler_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
