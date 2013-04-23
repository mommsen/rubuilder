#ifndef _rubuilder_ru_RUinput_h_
#define _rubuilder_ru_RUinput_h_

#include <boost/scoped_ptr.hpp>

#include <stdint.h>

#include "log4cplus/logger.h"

#include "rubuilder/ru/InputHandler.h"
#include "rubuilder/utils/EvBid.h"
#include "rubuilder/utils/InfoSpaceItems.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdata/Boolean.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedInteger64.h"
#include "xdata/Vector.h"
#include "xgi/Output.h"


namespace rubuilder { namespace ru { // namespace rubuilder::ru

  /**
   * \ingroup xdaqApps
   * \brief Event fragment input handler of RU
   */
  
  class RUinput
  {
  
  public:

    RUinput(xdaq::Application*);

    virtual ~RUinput() {};

    /**
     * Notify the proxy of an input source change.
     * The input source is taken from the info space
     * parameter 'inputSource'.
     *
     * Valid input strings are:
     * FBO  : super fragments are sent from the FBO
     *        in form of a I2O_EVMRU_DATA_READY message
     * Local: dummy data is generated locally 
     */
    void inputSourceChanged();
    
    /**
     * Callback for I2O message received from frontend
     */
    void I2Ocallback(toolbox::mem::Reference*);

    /**
     * Fill the next complete super fragment into the Reference.
     * If no super fragment is ready, return false.
     */
    bool getData(const utils::EvBid&, toolbox::mem::Reference*&);
    
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
     * Return the logical number of I2O_EVMRU_DATA_READY messages
     * received since the last call to resetMonitoringCounters
     */
    uint64_t fragmentsCount() const;
 
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
     * Print the content of the block FIFO as HTML snipped
     */
    inline void printBlockFIFO(xgi::Output* out)
    { handler_->printBlockFIFO(out); }
 
    /**
     * Enable or disable the handling of incoming I2O messages
     */
    void acceptI2Omessages(const bool accept)
    { acceptI2Omessages_ = accept; }

  private:
    
    void dumpFragmentToLogger(toolbox::mem::Reference*) const;

    xdaq::Application* app_;
    log4cplus::Logger& logger_;
 
    boost::scoped_ptr<InputHandler> handler_;
    bool acceptI2Omessages_;
 
    utils::InfoSpaceItems inputParams_;
    xdata::UnsignedInteger32 blockFIFOCapacity_;
    xdata::String inputSource_;
    xdata::Boolean dropInputData_;
    xdata::Boolean generateDummySuperFragments_;
    xdata::Boolean dumpFragmentsToLogger_;
    xdata::Boolean usePlayback_;
    xdata::String playbackDataFile_;
    xdata::UnsignedInteger32 dummyBlockSize_;
    xdata::UnsignedInteger32 dummyFedPayloadSize_;
    xdata::UnsignedInteger32 dummyFedPayloadStdDev_;
    xdata::Vector<xdata::UnsignedInteger32> fedSourceIds_;

    xdata::UnsignedInteger32 lastEventNumberFromRUI_;
    xdata::UnsignedInteger64 i2oEVMRUDataReadyCount_;
  };
  
  
} } //namespace rubuilder::ru

#endif // _rubuilder_ru_RUinput_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
