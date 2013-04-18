#ifndef _rubuilder_bu_LumiHandler_h_
#define _rubuilder_bu_LumiHandler_h_

#ifdef RUBUILDER_BOOST
#include "rubuilder/boost/filesystem/convenience.hpp"
#else
#include <boost/filesystem/convenience.hpp>
#endif
#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <vector>

#include "rubuilder/bu/FileHandler.h"
#include "rubuilder/bu/StateMachine.h"


namespace rubuilder { namespace bu { // namespace rubuilder::bu

  /**
   * \ingroup xdaqApps
   * \brief Write events belonging to the same lumi section
   */
 
  class LumiHandler
  {
  public:

    LumiHandler
    (
      const uint32_t buInstance,
      const boost::filesystem::path& rawDataDir,
      const boost::filesystem::path& metaDataDir,
      const uint32_t lumiSection,
      const uint32_t maxEventsPerFile,
      const uint32_t numberOfWriters
    );

    ~LumiHandler();
    
    /**
     * Return the next file handler to be used to write one event
     */
    FileHandlerPtr getFileHandler(boost::shared_ptr<StateMachine>);
        
    /**
     * Close the lumi section and do the bookkeeping
     */
    void close();
    
    
  private:

    void writeJSON() const;
    void defineJSON(const boost::filesystem::path&) const;

    const uint32_t buInstance_;
    const boost::filesystem::path rawDataDir_;
    const boost::filesystem::path metaDataDir_;
    const uint32_t lumiSection_;
    const uint32_t maxEventsPerFile_;
    const uint32_t numberOfWriters_;
    uint32_t index_;
    uint32_t nextFileHandler_;
    uint32_t eventsPerLS_;
    uint32_t filesPerLS_;
    
    typedef std::vector<FileHandlerPtr> FileHandlers;
    FileHandlers fileHandlers_;

  }; // LumiHandler
  
  typedef boost::shared_ptr<LumiHandler> LumiHandlerPtr;

} } // namespace rubuilder::bu

#endif // _rubuilder_bu_LumiHandler_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
