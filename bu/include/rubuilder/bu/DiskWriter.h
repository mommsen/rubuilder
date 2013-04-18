#ifndef _rubuilder_bu_DiskWriter_h_
#define _rubuilder_bu_DiskWriter_h_

#ifdef RUBUILDER_BOOST
#include "rubuilder/boost/filesystem/convenience.hpp"
#else
#include <boost/filesystem/convenience.hpp>
#endif
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <map>
#include <stdint.h>

#include "rubuilder/bu/DiskUsage.h"
#include "rubuilder/bu/Event.h"
#include "rubuilder/bu/FileHandler.h"
#include "rubuilder/bu/LumiHandler.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "rubuilder/utils/OneToOneQueue.h"
#include "toolbox/lang/Class.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/task/Action.h"
#include "toolbox/task/WaitingWorkLoop.h"
#include "xdaq/Application.h"
#include "xdata/Boolean.h"
#include "xdata/Double.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"


namespace rubuilder { namespace bu { // namespace rubuilder::bu
  
  class EventTable;
  class StateMachine;

  /**
   * \ingroup xdaqApps
   * \brief Write events to disk
   */
 
  class DiskWriter : public toolbox::lang::Class
  {
  public:

    DiskWriter
    (
      xdaq::Application*
    );
    
    ~DiskWriter();
    
    /**
     * Write the event given as argument to disk
     */
    void writeEvent(const EventPtr);
        
    /**
     * Close the given lumi section
     */
    void closeLS(const uint32_t lumiSection);

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
    void configure(const uint32_t maxEvtsUnderConstruction);

    /**
     * Return true if events are written to disk
     */
    bool enabled() const
    { return writeEventsToDisk_.value_; }
    
    /**
     * Remove all data
     */
    void clear();
    
    /**
     * Register the state machine
     */
    void registerStateMachine(boost::shared_ptr<StateMachine> stateMachine)
    { stateMachine_ = stateMachine; }

    /**
     * Register the event table
     */
    void registerEventTable(boost::shared_ptr<EventTable> eventTable)
    { eventTable_ = eventTable; }

    /**
     * Start processing messages
     */
    void startProcessing(const uint32_t runNumber);

    /**
     * Stop processing messages
     */
    void stopProcessing();
  
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
     * Print the content of the EoLS FIFO as HTML snipped
     */
    inline void printEoLSFIFO(xgi::Output* out)
    { eolsFIFO_.printVerticalHtml(out); }
    
    
  private:

    void startProcessingWorkLoop();
    bool process(toolbox::task::WorkLoop*);
    bool handleEvents();
    bool handleEoLS();
    LumiHandlerPtr getLumiHandler(const uint32_t lumiSection);
    bool writing(toolbox::task::WorkLoop*);
    bool resourceMonitoring(toolbox::task::WorkLoop*);
    bool checkDiskSize(DiskUsagePtr);
    void writeJSON();
    void defineJSON(const boost::filesystem::path&) const;
    void createWritingWorkLoops();
    
    xdaq::Application* app_;
    boost::shared_ptr<EventTable> eventTable_;
    boost::shared_ptr<StateMachine> stateMachine_;
    
    const uint32_t buInstance_;
    uint32_t runNumber_;
    uint32_t index_;
    
    boost::filesystem::path buRawDataDir_;
    boost::filesystem::path runRawDataDir_;
    boost::filesystem::path buMetaDataDir_;
    boost::filesystem::path runMetaDataDir_;
    DiskUsagePtr rawDataDiskUsage_;
    DiskUsagePtr metaDataDiskUsage_;

    typedef std::map<uint32_t,LumiHandlerPtr> LumiHandlers;
    LumiHandlers lumiHandlers_;
    boost::mutex lumiHandlersMutex_;
    
    toolbox::task::WorkLoop* processingWL_;
    toolbox::task::ActionSignature* processingAction_;
    toolbox::task::WorkLoop* resourceMonitoringWL_;
    toolbox::task::ActionSignature* resourceMonitoringAction_;
 
    typedef std::vector<toolbox::task::WorkLoop*> WorkLoops;
    WorkLoops writingWorkLoops_;
    toolbox::task::ActionSignature* writingAction_;
    
    typedef utils::OneToOneQueue<EventPtr> EventFIFO;
    EventFIFO eventFIFO_;
    typedef utils::OneToOneQueue<uint32_t> EoLSFIFO;
    EoLSFIFO eolsFIFO_;
    
    struct FileHandlerAndEvent {
      const FileHandlerPtr fileHandler;
      const EventPtr event;
      
      FileHandlerAndEvent(const FileHandlerPtr fileHandler, const EventPtr event)
      : fileHandler(fileHandler), event(event) {};
    };
    typedef boost::shared_ptr<FileHandlerAndEvent> FileHandlerAndEventPtr;
    typedef utils::OneToOneQueue<FileHandlerAndEventPtr> FileHandlerAndEventFIFO;
    FileHandlerAndEventFIFO fileHandlerAndEventFIFO_;
    boost::mutex fileHandlerAndEventFIFOmutex_;

    volatile bool writingActive_;
    volatile bool doProcessing_;
    volatile bool processActive_;

    utils::InfoSpaceItems diskWriterParams_;
    xdata::Boolean writeEventsToDisk_;
    xdata::UnsignedInteger32 numberOfWriters_;
    xdata::String rawDataDir_;
    xdata::String metaDataDir_;
    xdata::Double rawDataHighWaterMark_;
    xdata::Double rawDataLowWaterMark_;
    xdata::Double metaDataHighWaterMark_;
    xdata::Double metaDataLowWaterMark_;
    xdata::UnsignedInteger32 maxEventsPerFile_;
    xdata::UnsignedInteger32 eolsFIFOCapacity_;
    xdata::Boolean tolerateCorruptedEvents_;

    struct DiskWriterMonitoring
    {
      uint32_t nbFiles;
      uint32_t nbEventsWritten;
      uint32_t nbLumiSections;
      uint32_t lastEventNumberWritten;
      uint32_t currentLumiSection;
      uint32_t lastEoLS;
      uint32_t nbEventsCorrupted;
    } diskWriterMonitoring_;
    boost::mutex diskWriterMonitoringMutex_;

    xdata::UnsignedInteger32 nbEvtsWritten_;
    xdata::UnsignedInteger32 nbFilesWritten_;
    xdata::UnsignedInteger32 nbEvtsCorrupted_;

  };
  
} } // namespace rubuilder::bu

#endif // _rubuilder_bu_DiskWriter_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
