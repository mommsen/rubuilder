#ifndef _rubuilder_ru_InputHandler_h_
#define _rubuilder_ru_InputHandler_h_

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <stdint.h>
#include <vector>

#include "i2o/shared/i2omsg.h"
#include "rubuilder/ru/SuperFragment.h"
#include "rubuilder/utils/EvBid.h"
#include "rubuilder/utils/EvBidFactory.h"
#include "rubuilder/utils/OneToOneQueue.h"
#include "rubuilder/utils/SuperFragmentGenerator.h"
#include "toolbox/mem/Pool.h"
#include "toolbox/mem/Reference.h"
#include "xdaq/Application.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/Vector.h"
#include "xgi/Output.h"


namespace rubuilder { namespace ru { // namespace rubuilder::ru

  class InputHandler
  {
  public:

    InputHandler(xdaq::Application* app) :
    app_(app) {};

    virtual ~InputHandler() {};
    
    /**
     * Callback for I2O message received from frontend
     */
    virtual void I2Ocallback(toolbox::mem::Reference*) = 0;

    /**
     * Fill the next complete super fragment into the Reference.
     * If no super fragment is ready, return false.
     */
    virtual bool getData(const utils::EvBid&, toolbox::mem::Reference*&) = 0;

    /**
     * Configure
     */
    struct Configuration
    {
      uint32_t blockFIFOCapacity;
      bool dropInputData;
      uint32_t dummyBlockSize;
      uint32_t dummyFedPayloadSize;
      uint32_t dummyFedPayloadStdDev;
      xdata::Vector<xdata::UnsignedInteger32> fedSourceIds;
      bool usePlayback;
      std::string playbackDataFile;
    };
    virtual void configure(const Configuration&) {};
    
    /**
     * Remove all data
     */
    virtual void clear() {};
  
    /**
     * Print monitoring information as HTML snipped
     */
    virtual void printHtml(xgi::Output*) = 0;
    
    /**
     * Print the content of the block FIFO as HTML snipped
     */
    virtual inline void printBlockFIFO(xgi::Output* out) {};
    
    /**
     * Return the last event number seen
     */
    inline uint32_t lastEventNumber() const
    { return inputMonitoring_.lastEventNumber; }
    
    /**
     * Return the number of received event fragments
     * since the last call to resetMonitoringCounters
     */
    inline uint64_t fragmentsCount() const
    { return inputMonitoring_.logicalCount; }
    
    /**
     * Reset the monitoring counters
     */
    inline void resetMonitoringCounters()
    {
      boost::mutex::scoped_lock sl(inputMonitoringMutex_);
      
      inputMonitoring_.payload = 0;
      inputMonitoring_.logicalCount = 0;
      inputMonitoring_.i2oCount = 0;
      inputMonitoring_.lastEventNumber = 0;
    }

  protected:

    xdaq::Application* app_;

    struct InputMonitoring
    {
      uint64_t logicalCount;
      uint64_t payload;
      uint64_t i2oCount;
      uint32_t lastEventNumber;
    } inputMonitoring_;
    boost::mutex inputMonitoringMutex_;
  };


  class FBOproxy : public InputHandler
  {
  public:
    
    FBOproxy(xdaq::Application*);
    virtual ~FBOproxy() {};

    virtual void I2Ocallback(toolbox::mem::Reference*);
    virtual bool getData(const utils::EvBid&, toolbox::mem::Reference*&);
    virtual void configure(const Configuration&);
    virtual void clear();
    virtual void printHtml(xgi::Output*);
    virtual inline void printBlockFIFO(xgi::Output* out)
    { blockFIFO_.printVerticalHtml(out); }

  private:
    
    void updateInputCounters(const I2O_MESSAGE_FRAME*);
    void appendBlockToSuperFragment(toolbox::mem::Reference*);
    void handleCompleteSuperFragment();

    utils::EvBidFactory evbIdFactory_;
    typedef utils::OneToOneQueue<toolbox::mem::Reference*> BlockFIFO;
    BlockFIFO blockFIFO_;
    bool dropInputData_;

    toolbox::mem::Reference* superFragmentHead_;
    toolbox::mem::Reference* superFragmentTail_;
  };


  class FEROLproxy : public InputHandler
  {
  public:
    
    FEROLproxy(xdaq::Application*);
    virtual ~FEROLproxy() {};

    virtual void I2Ocallback(toolbox::mem::Reference*);
    virtual bool getData(const utils::EvBid&, toolbox::mem::Reference*&);
    virtual void configure(const Configuration&);
    virtual void clear();
    virtual void printHtml(xgi::Output*);
    virtual inline void printBlockFIFO(xgi::Output* out)
    { blockFIFO_.printVerticalHtml(out); }

  private:
    
    toolbox::mem::Reference* copyDataIntoDataBlock(SuperFragmentPtr);
    void fillBlockInfo(toolbox::mem::Reference*, const utils::EvBid&, const uint32_t nbBlocks) const;

    SuperFragment::FEDlist fedList_;
    typedef std::map<utils::EvBid,SuperFragmentPtr> SuperFragmentMap;
    SuperFragmentMap superFragmentMap_;
    toolbox::mem::Pool* superFragmentPool_;
    
    typedef std::map<uint16_t,utils::EvBidFactory> EvBidFactories;
    EvBidFactories evbIdFactories_;

    typedef utils::OneToOneQueue<SuperFragmentPtr> BlockFIFO;
    BlockFIFO blockFIFO_;
    bool dropInputData_;
    uint32_t blockSize_;
  };


  class FEROL2proxy : public InputHandler
  {
  public:
    
    FEROL2proxy(xdaq::Application*);
    virtual ~FEROL2proxy() {};

    virtual void I2Ocallback(toolbox::mem::Reference*);
    virtual bool getData(const utils::EvBid&, toolbox::mem::Reference*&);
    virtual void configure(const Configuration&);
    virtual void clear();
    virtual void printHtml(xgi::Output*);

  private:
    
    toolbox::mem::Reference* getNextFragment(const utils::EvBid&);
    toolbox::mem::Reference* copyDataIntoDataBlock(toolbox::mem::Reference*, const utils::EvBid&) const;
    void fillBlockInfo(toolbox::mem::Reference*, const utils::EvBid&, const uint32_t nbBlocks) const;

    typedef utils::OneToOneQueue<toolbox::mem::Reference*> FragmentFIFO;
    typedef std::map<uint16_t,FragmentFIFO> FedFragmentFIFOs;
    FedFragmentFIFOs fedFragmentFIFOs_;
    
    SuperFragment::FEDlist fedList_;
    toolbox::mem::Pool* superFragmentPool_;
    
    typedef std::map<uint16_t,utils::EvBidFactory> EvBidFactories;
    EvBidFactories evbIdFactories_;

    bool dropInputData_;
    uint32_t blockSize_;
  };


  class DummyInputData : public InputHandler
  {
  public:
    
    DummyInputData(xdaq::Application*);
    virtual ~DummyInputData() {};

    virtual void I2Ocallback(toolbox::mem::Reference*);
    virtual bool getData(const utils::EvBid&, toolbox::mem::Reference*&);
    virtual void configure(const Configuration&);
    virtual void printHtml(xgi::Output*);

  private:
    
    toolbox::mem::Reference* generateDummySuperFrag(const uint32_t eventNumber);
    void setNbBlocksInSuperFragment
    (
      toolbox::mem::Reference*,
      const uint32_t nbBlocks
    );

    rubuilder::utils::SuperFragmentGenerator superFragmentGenerator_;
  };
  
  
} } //namespace rubuilder::ru

#endif // _rubuilder_ru_InputHandler_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
