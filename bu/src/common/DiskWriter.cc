#include <sstream>
#include <iomanip>

#include "rubuilder/bu/DiskWriter.h"
#include "rubuilder/bu/EventTable.h"
#include "rubuilder/bu/StateMachine.h"
#include "rubuilder/utils/CreateStrings.h"
#include "rubuilder/utils/Exception.h"
#include "toolbox/task/WorkLoopFactory.h"


rubuilder::bu::DiskWriter::DiskWriter
(
  xdaq::Application* app
) :
app_(app),
buInstance_( app_->getApplicationDescriptor()->getInstance() ),
eventFIFO_("eventFIFO"),
eolsFIFO_("eolsFIFO"),
fileHandlerAndEventFIFO_("fileHandlerAndEventFIFO"),
writingActive_(false),
doProcessing_(false),
processActive_(false)
{
  resetMonitoringCounters();
  startProcessingWorkLoop();
}


rubuilder::bu::DiskWriter::~DiskWriter()
{
}


void rubuilder::bu::DiskWriter::writeEvent(const EventPtr event)
{
  if ( ! writeEventsToDisk_ ) return;
  
  while ( ! eventFIFO_.enq(event) ) { ::usleep(1000); }
}


void rubuilder::bu::DiskWriter::closeLS(const uint32_t lumiSection)
{
  if ( ! writeEventsToDisk_ ) return;

  while ( ! eolsFIFO_.enq(lumiSection) ) { ::usleep(1000); }
}


void rubuilder::bu::DiskWriter::startProcessingWorkLoop()
{
  try
  {
    const std::string identifier = utils::getIdentifier(app_->getApplicationDescriptor());
    
    processingWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( identifier + "DiskWriterProcessing", "waiting" );
    
    if ( ! processingWL_->isActive() )
    {
      processingAction_ =
        toolbox::task::bind(this, &rubuilder::bu::DiskWriter::process,
          identifier + "diskWriterProcess");
    
      processingWL_->activate();
    }
    
    resourceMonitoringWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( identifier + "DiskWriterResourceMonitoring", "waiting" );
    
    if ( ! resourceMonitoringWL_->isActive() )
    {
      resourceMonitoringAction_ =
        toolbox::task::bind(this, &rubuilder::bu::DiskWriter::resourceMonitoring,
          identifier + "resourceMonitoringProcess");
    
      resourceMonitoringWL_->activate();
    }
    
    writingAction_ =
      toolbox::task::bind(this, &rubuilder::bu::DiskWriter::writing,
        identifier + "diskWriting");
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloops.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


void rubuilder::bu::DiskWriter::startProcessing(const uint32_t runNumber)
{
  if ( ! writeEventsToDisk_ ) return;

  runNumber_ = runNumber;
  
  std::ostringstream runDir;
  runDir << "run" << std::setfill('0') << std::setw(8) << runNumber_;
  
  runRawDataDir_ = buRawDataDir_ / runDir.str() / "open";
  if ( ! boost::filesystem::create_directories(runRawDataDir_) )
  {
    std::ostringstream oss;
    oss << "Failed to create directory " << runRawDataDir_.string();
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }
  
  runMetaDataDir_ = buMetaDataDir_ / runDir.str();
  if ( ! boost::filesystem::exists(runMetaDataDir_) &&
    ( ! boost::filesystem::create_directories(runMetaDataDir_) ) )
  {
    std::ostringstream oss;
    oss << "Failed to create directory " << runMetaDataDir_.string();
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }
  
  doProcessing_ = true;
  processingWL_->submit(processingAction_);
  resourceMonitoringWL_->submit(resourceMonitoringAction_);

  for (uint32_t i=0; i < numberOfWriters_; ++i)
  {
    writingWorkLoops_.at(i)->submit(writingAction_);
  }
}


void rubuilder::bu::DiskWriter::stopProcessing()
{
  if ( ! writeEventsToDisk_ ) return;
  
  doProcessing_ = false;

  while ( processActive_ || writingActive_ ) ::usleep(1000);
  
  for (LumiHandlers::const_iterator it = lumiHandlers_.begin(), itEnd = lumiHandlers_.end();
       it != itEnd; ++it)
  {
    it->second->close();
  }
  lumiHandlers_.clear();
  
  if ( boost::filesystem::exists(runRawDataDir_) &&
    ( ! boost::filesystem::remove(runRawDataDir_) ) )
  {
    std::ostringstream oss;
    oss << "Failed to remove directory " << runRawDataDir_.string();
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }

  writeJSON();
}


bool rubuilder::bu::DiskWriter::process(toolbox::task::WorkLoop*)
{
  ::usleep(1000);
  
  processActive_ = true;
  
  try
  {
    while (
      doProcessing_ && (
        handleEvents() ||
        handleEoLS()
      )
    ) {};
  }
  catch(xcept::Exception &e)
  {
    processActive_ = false;
    stateMachine_->processFSMEvent( utils::Fail(e) );
  }
  
  processActive_ = false;
  
  return doProcessing_;
}


bool rubuilder::bu::DiskWriter::handleEvents()
{
  EventPtr event;
  if ( ! eventFIFO_.deq(event) ) return false;
  
  if ( event->runNumber() != runNumber_ )
  {
    std::ostringstream oss;
    oss << "Received an event with run number " << event->runNumber();
    oss << " while expecting events from run " << runNumber_;
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  const LumiHandlerPtr lumiHandler = getLumiHandler( event->lumiSection() );
  const FileHandlerPtr fileHandler = lumiHandler->getFileHandler(stateMachine_);
  
  if ( fileHandler->getAllocatedEventCount() == 1 )
    ++diskWriterMonitoring_.nbFiles;
  
  FileHandlerAndEventPtr fileHandlerAndEvent(
    new FileHandlerAndEvent(fileHandler, event)
  );
  while ( ! fileHandlerAndEventFIFO_.enq(fileHandlerAndEvent) ) { ::usleep(1000); }

  return true;
}


rubuilder::bu::LumiHandlerPtr rubuilder::bu::DiskWriter::getLumiHandler(const uint32_t lumiSection)
{
  boost::mutex::scoped_lock handlerSL(lumiHandlersMutex_);
  
  LumiHandlers::iterator pos = lumiHandlers_.lower_bound(lumiSection);

  if ( pos == lumiHandlers_.end() || (lumiHandlers_.key_comp()(lumiSection,pos->first)) )
  {
    // New lumi section
    const LumiHandlerPtr lumiHandler(new LumiHandler(
        buInstance_, runRawDataDir_, runMetaDataDir_, lumiSection, maxEventsPerFile_, numberOfWriters_));
    pos = lumiHandlers_.insert(pos, LumiHandlers::value_type(lumiSection, lumiHandler));
    
    boost::mutex::scoped_lock monitorSL(diskWriterMonitoringMutex_);
    ++diskWriterMonitoring_.nbLumiSections;
    diskWriterMonitoring_.currentLumiSection = lumiSection;
  }

  return pos->second;
}


bool rubuilder::bu::DiskWriter::handleEoLS()
{
  uint32_t lumiSection;
  if ( ! eolsFIFO_.deq(lumiSection) ) return false;
  
  boost::mutex::scoped_lock handlerSL(lumiHandlersMutex_);
  boost::mutex::scoped_lock monitorSL(diskWriterMonitoringMutex_);
  
  LumiHandlers::iterator pos = lumiHandlers_.find(lumiSection);
  
  if ( lumiSection > diskWriterMonitoring_.lastEoLS )
    diskWriterMonitoring_.lastEoLS = lumiSection;

  if ( pos == lumiHandlers_.end() )
  {
    // No events have been written for this lumi section
    // Use a dummy FileHandler to create an empty file
    LumiHandler emptyLumi(buInstance_, runRawDataDir_, runMetaDataDir_, lumiSection, 0, 0);
    emptyLumi.close();
    ++diskWriterMonitoring_.nbLumiSections;
  }
  else
  {
    pos->second->close();
    lumiHandlers_.erase(pos);
  }    
  
  return true;
}


bool rubuilder::bu::DiskWriter::writing(toolbox::task::WorkLoop*)
{  
  ::usleep(1000);

  writingActive_ = true;
  
  try
  {
    FileHandlerAndEventPtr fileHandlerAndEvent;
    bool gotEvent(false);
    do
    {
      {
        boost::mutex::scoped_lock sl(fileHandlerAndEventFIFOmutex_);
        gotEvent = fileHandlerAndEventFIFO_.deq(fileHandlerAndEvent);
      }
      if (gotEvent)
      {
        try
        {
          fileHandlerAndEvent->event->parseAndCheckData();
        }
        catch(exception::SuperFragment &e)
        {
          boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);
          ++diskWriterMonitoring_.nbEventsCorrupted;
          if ( tolerateCorruptedEvents_ )
          {
            LOG4CPLUS_ERROR(app_->getApplicationLogger(),
              xcept::stdformat_exception_history(e));
            app_->notifyQualified("error",e);
          }
          else
          {
            throw(e);
          }
        }

        fileHandlerAndEvent->event->writeToDisk(fileHandlerAndEvent->fileHandler);
        eventTable_->discardEvent( fileHandlerAndEvent->event->buResourceId() );

        boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);
        ++diskWriterMonitoring_.nbEventsWritten;
        const uint32_t eventNumber = fileHandlerAndEvent->event->evbId().eventNumber();
        if ( eventNumber > diskWriterMonitoring_.lastEventNumberWritten )
          diskWriterMonitoring_.lastEventNumberWritten = eventNumber;
      }
    }
    while (gotEvent);
  }
  catch(xcept::Exception &e)
  {
    writingActive_ = false;
    stateMachine_->processFSMEvent( utils::Fail(e) );
  }
  
  writingActive_ = false;
  
  return doProcessing_;
}


bool rubuilder::bu::DiskWriter::resourceMonitoring(toolbox::task::WorkLoop*)
{
  bool allOkay(true);

  allOkay &= checkDiskSize(rawDataDiskUsage_);
  allOkay &= checkDiskSize(metaDataDiskUsage_);

  eventTable_->requestEvents(allOkay);
  
  ::sleep(5);
  
  return doProcessing_;
}


bool rubuilder::bu::DiskWriter::checkDiskSize(DiskUsagePtr diskUsage)
{
  return ( diskUsage->update() && ! diskUsage->tooHigh() );
}


void rubuilder::bu::DiskWriter::writeJSON()
{
  const boost::filesystem::path jsonDefFile = runMetaDataDir_ / "EoR.jsd";
  defineJSON(jsonDefFile);

  std::ostringstream fileNameStream;
  fileNameStream
    << "EoR_" << std::setfill('0') << std::setw(8) << runNumber_ << ".jsn";
  const boost::filesystem::path jsonFile = runMetaDataDir_ / fileNameStream.str();
  
  if ( boost::filesystem::exists(jsonFile) )
  {
    std::ostringstream oss;
    oss << "The JSON file " << jsonFile.string() << " already exists.";
    XCEPT_RAISE(exception::DiskWriting, oss.str());
  }
  
  boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);
  
  std::ofstream json(jsonFile.string().c_str());
  json << "{"                                                                           << std::endl;
  json << "   \"Data\" : [ \""     << diskWriterMonitoring_.nbEventsWritten << "\", \""
                                   << diskWriterMonitoring_.nbFiles         << "\", \""
                                   << diskWriterMonitoring_.nbLumiSections  << "\" ],"  << std::endl;
  json << "   \"Definition\" : \"" << jsonDefFile.string()  << "\","                    << std::endl;
  json << "   \"Source\" : \"BU-"  << buInstance_   << "\""                             << std::endl;
  json << "}"                                                                           << std::endl;
  json.close();
}


void rubuilder::bu::DiskWriter::defineJSON(const boost::filesystem::path& jsonDefFile) const
{
  std::ofstream json(jsonDefFile.string().c_str());
  json << "{"                                                 << std::endl;
  json << "   \"legend\" : ["                                 << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NEvents\","                  << std::endl;
  json << "         \"operation\" : \"sum\""                  << std::endl;
  json << "      },"                                          << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NFiles\","                   << std::endl;
  json << "         \"operation\" : \"sum\""                  << std::endl;
  json << "      },"                                          << std::endl;
  json << "      {"                                           << std::endl;
  json << "         \"name\" : \"NLumis\","                   << std::endl;
  json << "         \"operation\" : \"sum\""                  << std::endl;
  json << "      }"                                           << std::endl;
  json << "   ],"                                             << std::endl;
  json << "   \"file\" : \"" << jsonDefFile.string() << "\""  << std::endl;
  json << "}"                                                 << std::endl;
  json.close();
}


void rubuilder::bu::DiskWriter::appendConfigurationItems(utils::InfoSpaceItems& params)
{
  writeEventsToDisk_ = false;
  numberOfWriters_ = 8;
  rawDataDir_ = "/tmp/raw";
  metaDataDir_ = "/tmp/meta";
  rawDataHighWaterMark_ = 0.7;
  rawDataLowWaterMark_ = 0.5;
  metaDataHighWaterMark_ = 0.9;
  metaDataLowWaterMark_ = 0.5;
  maxEventsPerFile_ = 2000;
  eolsFIFOCapacity_ = 1028;
  tolerateCorruptedEvents_ = false;
  
  diskWriterParams_.add("writeEventsToDisk", &writeEventsToDisk_);
  diskWriterParams_.add("numberOfWriters", &numberOfWriters_);
  diskWriterParams_.add("rawDataDir", &rawDataDir_);
  diskWriterParams_.add("metaDataDir", &metaDataDir_);
  diskWriterParams_.add("rawDataHighWaterMark", &rawDataHighWaterMark_);
  diskWriterParams_.add("rawDataLowWaterMark", &rawDataLowWaterMark_);
  diskWriterParams_.add("metaDataHighWaterMark", &metaDataHighWaterMark_);
  diskWriterParams_.add("metaDataLowWaterMark", &metaDataLowWaterMark_);
  diskWriterParams_.add("maxEventsPerFile", &maxEventsPerFile_);
  diskWriterParams_.add("eolsFIFOCapacity", &eolsFIFOCapacity_);
  diskWriterParams_.add("tolerateCorruptedEvents", &tolerateCorruptedEvents_);
  
  params.add(diskWriterParams_);
}


void rubuilder::bu::DiskWriter::appendMonitoringItems(utils::InfoSpaceItems& items)
{
  nbEvtsWritten_ = 0;
  nbFilesWritten_ = 0;
  nbEvtsCorrupted_ = 0;
  
  items.add("nbEvtsWritten", &nbEvtsWritten_);
  items.add("nbFilesWritten", &nbFilesWritten_);
  items.add("nbEvtsCorrupted", &nbEvtsCorrupted_);
}


void rubuilder::bu::DiskWriter::updateMonitoringItems()
{
  boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);
  
  nbEvtsWritten_ = diskWriterMonitoring_.nbEventsWritten;
  nbFilesWritten_ = diskWriterMonitoring_.nbFiles;
  nbEvtsCorrupted_ = diskWriterMonitoring_.nbEventsCorrupted;
}


void rubuilder::bu::DiskWriter::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);
  
  diskWriterMonitoring_.nbFiles = 0;
  diskWriterMonitoring_.nbEventsWritten = 0;
  diskWriterMonitoring_.nbLumiSections = 0;
  diskWriterMonitoring_.lastEventNumberWritten = 0;
  diskWriterMonitoring_.currentLumiSection = 0;
  diskWriterMonitoring_.lastEoLS = 0;
  diskWriterMonitoring_.nbEventsCorrupted = 0;
}


void rubuilder::bu::DiskWriter::configure(const uint32_t maxEvtsUnderConstruction)
{
  clear();

  eventFIFO_.resize(maxEvtsUnderConstruction);
  eolsFIFO_.resize(eolsFIFOCapacity_);
  fileHandlerAndEventFIFO_.resize(maxEvtsUnderConstruction);

  if ( writeEventsToDisk_ )
  {
    std::ostringstream buDir;
    buDir << "BU-" << std::setfill('0') << std::setw(3) << buInstance_;
    
    buRawDataDir_ = rawDataDir_.value_;
    buRawDataDir_ /= buDir.str();
    if ( ! boost::filesystem::exists(buRawDataDir_) &&
      ( ! boost::filesystem::create_directories(buRawDataDir_) ) )
    {
      std::ostringstream oss;
      oss << "Failed to create directory " << buRawDataDir_.string();
      XCEPT_RAISE(exception::DiskWriting, oss.str());
    }
    rawDataDiskUsage_.reset( new DiskUsage(buRawDataDir_, rawDataHighWaterMark_, rawDataLowWaterMark_) );
    
    buMetaDataDir_ = metaDataDir_.value_;
    buMetaDataDir_ /= buDir.str();
    if ( ! boost::filesystem::exists(buMetaDataDir_) &&
      ( ! boost::filesystem::create_directories(buMetaDataDir_) ) )
    {
      std::ostringstream oss;
      oss << "Failed to create directory " << buMetaDataDir_.string();
      XCEPT_RAISE(exception::DiskWriting, oss.str());
    }
    metaDataDiskUsage_.reset( new DiskUsage(buMetaDataDir_, metaDataHighWaterMark_, metaDataLowWaterMark_) );
    
    createWritingWorkLoops();
  }
}


void rubuilder::bu::DiskWriter::createWritingWorkLoops()
{
  const std::string identifier = utils::getIdentifier(app_->getApplicationDescriptor());
  
  try
  {
    // Leave any previous created workloops alone. Only add new ones if needed.
    for (uint16_t i=writingWorkLoops_.size(); i < numberOfWriters_; ++i)
    {
      std::ostringstream workLoopName;
      workLoopName << identifier << "DiskWriter_" << i;
      toolbox::task::WorkLoop* wl = toolbox::task::getWorkLoopFactory()->getWorkLoop( workLoopName.str(), "waiting" );
      
      if ( ! wl->isActive() ) wl->activate();
      writingWorkLoops_.push_back(wl);
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloops.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


void rubuilder::bu::DiskWriter::clear()
{
  EventPtr event;
  while ( eventFIFO_.deq(event) ) { event.reset(); }
  
  uint32_t lumiSection;
  while ( eolsFIFO_.deq(lumiSection) ) {}

  FileHandlerAndEventPtr fileHandlerAndEvent;
  while ( fileHandlerAndEventFIFO_.deq(fileHandlerAndEvent) ) { fileHandlerAndEvent.reset(); }
}


void rubuilder::bu::DiskWriter::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>DiskWriter</p>"                                     << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  {
    boost::mutex::scoped_lock sl(diskWriterMonitoringMutex_);
    
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>last evt number written</td>"                      << std::endl;
    *out << "<td>" << diskWriterMonitoring_.lastEventNumberWritten  << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td># files written</td>"                              << std::endl;
    *out << "<td>" << diskWriterMonitoring_.nbFiles << "</td>"      << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td># events written</td>"                             << std::endl;
    *out << "<td>" << diskWriterMonitoring_.nbEventsWritten << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td># corrupted events</td>"                           << std::endl;
    *out << "<td>" << diskWriterMonitoring_.nbEventsCorrupted << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td># lumi sections</td>"                              << std::endl;
    *out << "<td>" << diskWriterMonitoring_.nbLumiSections << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>current lumi section</td>"                         << std::endl;
    *out << "<td>" << diskWriterMonitoring_.currentLumiSection << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>last EoLS signal</td>"                             << std::endl;
    *out << "<td>" << diskWriterMonitoring_.lastEoLS << "</td>"     << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  if ( rawDataDiskUsage_.get() )
  {
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>Raw-data disk size (GB)</td>"                      << std::endl;
    *out << "<td>" << rawDataDiskUsage_->diskSizeGB() << "</td>"    << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>Raw-data disk usage</td>"                          << std::endl;
    *out << "<td>" << rawDataDiskUsage_->relDiskUsage() << "</td>"  << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  if ( metaDataDiskUsage_.get() )
  {
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>Meta-data disk size (GB)</td>"                     << std::endl;
    *out << "<td>" << metaDataDiskUsage_->diskSizeGB() << "</td>"   << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>Meta-data disk usage</td>"                         << std::endl;
    *out << "<td>" << metaDataDiskUsage_->relDiskUsage() << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  eventFIFO_.printHtml(out, app_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  fileHandlerAndEventFIFO_.printHtml(out, app_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  eolsFIFO_.printHtml(out, app_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  diskWriterParams_.printHtml("Configuration", out);

  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
