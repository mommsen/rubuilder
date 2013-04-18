#ifndef _rubuilder_evm_EoLSHandler_h_
#define _rubuilder_evm_EoLSHandler_h_

#include <boost/thread/mutex.hpp>

#include <stdint.h>

#include "log4cplus/logger.h"

#include "interface/evb/i2oEVBMsgs.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "rubuilder/utils/OneToOneQueue.h"
#include "toolbox/lang/Class.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xdaq/Application.h"
#include "xdata/UnsignedInteger32.h"


namespace rubuilder { namespace evm { // namespace rubuilder::evm

  class BUproxy;
  class SMproxy;

  /**
   * \ingroup xdaqApps
   * \brief Handler for L1 trigger information
   */
  
  class EoLSHandler : public toolbox::lang::Class
  {
    
  public:

    struct LumiSectionPair
    {
      uint32_t runNumber;
      uint32_t lumiSection;

      bool operator< (const LumiSectionPair& other) const
      {
        return runNumber == other.runNumber
          ? lumiSection < other.lumiSection
          : runNumber < other.runNumber;
      }
    };

    EoLSHandler
    (
      xdaq::Application*,
      toolbox::mem::Pool*
    );

    /**
     * Register the BU proxy used to send the EoLS signal
     */
    void registerBUproxy(BUproxy* buProxy)
    { buProxy_ = buProxy; }

    /**
     * Register the SM proxy used to send the EoLS signal
     */
    void registerSMproxy(SMproxy* smProxy)
    { smProxy_ = smProxy; }

    /**
     * Send the EoLS signal for the given lumiSection
     */
    void send(const LumiSectionPair&);

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
     * Return true if there is no more EoLS information to be processed
     */
    bool empty()
    { return ( completedLumiSectionFIFO_.empty() && idle_ ); }

    /**
     * Find the application descriptors
     */
    void getApplicationDescriptors();

    /**
     * Print monitoring/configuration as HTML snipped
     */
    void printHtml(xgi::Output*);

    /**
     * Print the content of the completed lumi section FIFO as HTML snipped
     */
    inline void printCompletedLumiSectionFIFO(xgi::Output* out)
    { completedLumiSectionFIFO_.printVerticalHtml(out); }


  private:

    void startWorkLoop();
    bool EoLSSignalSender(toolbox::task::WorkLoop*);
    void sendEoLSSignal(const LumiSectionPair&);
    
    xdaq::Application* app_;
    BUproxy* buProxy_;
    SMproxy* smProxy_;
    toolbox::mem::Pool* fastCtrlMsgPool_;
    log4cplus::Logger& logger_;
    uint32_t tid_;
    toolbox::task::WorkLoop* EoLSSignalSenderWL_;
    uint32_t localLastCompletedLS_;
    bool idle_;

    typedef utils::OneToOneQueue<LumiSectionPair> CompletedLumiSectionFIFO;
    CompletedLumiSectionFIFO completedLumiSectionFIFO_;
    boost::mutex completedLumiSectionFIFOMutex_;

    xdata::UnsignedInteger32 completedLumiSectionFIFOCapacity_;
    xdata::UnsignedInteger32 lastCompletedLS_;
  };
  
} } //namespace rubuilder::evm


inline std::ostream& operator<<
(
  std::ostream &s,
  rubuilder::evm::EoLSHandler::LumiSectionPair &lsPair
)
{
  s << "runNumber="   << lsPair.runNumber << " ";
  s << "lumiSection=" << lsPair.lumiSection;

  return s;
}

#endif // _rubuilder_evm_EoLSHandler_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
