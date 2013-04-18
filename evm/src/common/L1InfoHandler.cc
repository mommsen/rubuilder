#include "interface/shared/frl_header.h"
#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"
#include "rubuilder/evm/EoLSHandler.h"
#include "rubuilder/evm/L1InfoHandler.h"
#include "rubuilder/utils/CreateStrings.h"
#include "rubuilder/utils/Exception.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"
#include "xdata/InfoSpaceFactory.h"
#include "xdata/TableIterator.h"

#ifdef EVM_L1_DEBUG
#include "fedbuilder/DataChecker.hh"
#endif

#include <sstream>


rubuilder::evm::L1InfoHandler::L1InfoHandler
(
  xdaq::Application* app,
  boost::shared_ptr<EoLSHandler> eolsHandler,
  toolbox::mem::Pool* fastCtrlMsgPool
) :
app_(app),
eolsHandler_(eolsHandler),
fastCtrlMsgPool_(fastCtrlMsgPool),
logger_(app->getApplicationLogger()),
rcmsNotifier_(logger_, app->getApplicationDescriptor(), app->getApplicationContext()),
idle_(true),
l1InfoFIFO_("l1InfoFIFO"),
lumiSectionInfoFIFO_("lumiSectionInfoFIFO"),
lastSeenLumiSection_(0),
currentLumiSectionInfo_(0)
{
  resetMonitoringCounters();
  createL1ScalersInfoSpace();
  initializeL1ScalersTable();
  initializeNotificationReceiver();

  startL1InfoWorkLoop();
  startL1ScalersWorkLoop();
}


void rubuilder::evm::L1InfoHandler::createL1ScalersInfoSpace()
{
  toolbox::net::URN l1ScalersURN =
    app_->createQualifiedInfoSpace("HLTS_L1Scalers");
  l1ScalersInfoSpace_ = xdata::getInfoSpaceFactory()->get(l1ScalersURN.toString());

  l1ScalersParams_ = initAndGetL1ScalersParams();
  l1ScalersParams_.putIntoInfoSpace(l1ScalersInfoSpace_, (xdata::ActionListener*)app_);
}


rubuilder::utils::InfoSpaceItems rubuilder::evm::L1InfoHandler::initAndGetL1ScalersParams()
{
  utils::InfoSpaceItems params;
  
  instance_ = app_->getApplicationDescriptor()->getInstance();

  params.add("instance", &instance_);
  params.add("runnumber", &runnumber_);
  params.add("lsnumber", &lsnumber_);
  params.add("eventcount", &eventcount_);
  params.add("physicstriggers", &physicstriggers_);
  params.add("calibrationtriggers", &calibrationtriggers_);
  params.add("randomtriggers", &randomtriggers_);
  params.add("skippedlumisections", &skippedlumisections_);
  params.add("l1technical", &l1technical_);
  params.add("l1decision_0_63", &l1decision_0_63_);
  params.add("l1decision_64_127", &l1decision_64_127_);
  
  params.appendItemNames(l1ScalersParamNames_);
  
  return params;
}


void rubuilder::evm::L1InfoHandler::initializeL1ScalersTable()
{
  l1ScalersTable_.reserve(1);
  l1ScalersTable_.addColumn("instance",            "unsigned int 32");
  l1ScalersTable_.addColumn("runnumber",           "unsigned int 32");
  l1ScalersTable_.addColumn("lsnumber",            "unsigned int 32");
  l1ScalersTable_.addColumn("eventcount",          "unsigned int 32");
  l1ScalersTable_.addColumn("physicstriggers",     "unsigned int 32");
  l1ScalersTable_.addColumn("calibrationtriggers", "unsigned int 32");
  l1ScalersTable_.addColumn("randomtriggers",      "unsigned int 32");
  l1ScalersTable_.addColumn("skippedlumisections", "unsigned int 32");
  // Java flash list deserializer does not support vectors
  for (uint16_t bit = 0; bit < utils::TRIGGER_BITS_COUNT; ++bit)
  {
    std::ostringstream bitName;
    bitName << bit;
    l1ScalersTable_.addColumn("l1technical_"+bitName.str(), "unsigned int 32");
  }
  for (uint16_t bit = 0; bit < 2*utils::TRIGGER_BITS_COUNT; ++bit)
  {
    std::ostringstream bitName;
    bitName << bit;
    l1ScalersTable_.addColumn("l1decision_" +bitName.str(), "unsigned int 32");
  }
  xdata::Table::iterator it = l1ScalersTable_.append();
  it->setField("instance", instance_);
}


void rubuilder::evm::L1InfoHandler::initializeNotificationReceiver()
{
  xdata::InfoSpace *ispace = app_->getApplicationInfoSpace();
  
  ispace->fireItemAvailable("rcmsNotificationReceiver",
    rcmsNotifier_.getRcmsNotificationReceiverParameter());
  ispace->fireItemAvailable("foundNotificationReceiver",
    rcmsNotifier_.getFoundRcmsNotificationReceiverParameter());
  rcmsNotifier_.findRcmsNotificationReceiver();
  rcmsNotifier_.subscribeToChangesInRcmsNotificationReceiver(ispace);
}


uint32_t rubuilder::evm::L1InfoHandler::extractL1Info
(
  toolbox::mem::Reference* trigMsg,
  const uint32_t runNumber
)
{
  if ( ! enableL1Info_ ) return 0;
  
  toolbox::mem::Reference* bufRef = getNewL1Information();
  utils::L1Information* l1Info = (utils::L1Information*)bufRef->getDataLocation();
  if ( fillL1Info(trigMsg, l1Info) )
  {
    l1Info->runNumber = runNumber;
    verifyLumiSection(l1Info);
    while ( ! l1InfoFIFO_.enq(bufRef) ) ::usleep(1000);
    return l1Info->lsNumber;
  }
  else
  {
    #ifdef EVM_L1_DEBUG
    
    static int counter = 0;
    
    if ( ++counter < 100 )
    {
      std::stringstream oss;
      oss << "EVM could not extract L1 information (info ";
      if ( ! l1Info->isValid ) oss << "not ";
      oss << "valid): ";
      oss << l1Info->reason;
      
      fedbuilder::DataChecker dc(app);
      dc.dumpSuperFragment(
        trigMsg,
        true,
        oss.str());
    }

    #endif

    lastL1decodeError_ = l1Info->reason;
    bufRef->release();
    return 0;
  }
}


void rubuilder::evm::L1InfoHandler::enqCurrentLumiSectionInfo()
{
  const std::string errorMsg =
    "Failed to enqueue the current lumi-section information";

  try
  {
    boost::mutex::scoped_lock sl(currentLumiSectionInfoMutex_);
    if (currentLumiSectionInfo_)
    {
      while ( ! lumiSectionInfoFIFO_.enq(currentLumiSectionInfo_) ) ::usleep(1000);
      currentLumiSectionInfo_ = 0;
    }
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::L1Trigger,
      errorMsg, e);
  }
  catch(std::exception &e)
  {
    XCEPT_RAISE(exception::L1Trigger,
      errorMsg + e.what());
  }
  catch(...)
  {
    XCEPT_RAISE(exception::L1Trigger,
      errorMsg + "unknown exception");
  }
}


void rubuilder::evm::L1InfoHandler::appendConfigurationItems(utils::InfoSpaceItems& params)
{
  enableL1Info_                = true;
  sendL1FlashList_             = true;
  l1InfoFIFOCapacity_          = 5000;
  lumiSectionInfoFIFOCapacity_ = 100;

  params.add("enableL1Info", &enableL1Info_);
  params.add("sendL1FlashList", &sendL1FlashList_);
  params.add("l1InfoFIFOCapacity", &l1InfoFIFOCapacity_);
  params.add("lumiSectionInfoFIFOCapacity", &lumiSectionInfoFIFOCapacity_);

  configure();
}


void rubuilder::evm::L1InfoHandler::appendMonitoringItems(utils::InfoSpaceItems& items)
{
  lastL1InfoForLS_ = 0;

  items.add("lastL1InfoForLS", &lastL1InfoForLS_);
}


void rubuilder::evm::L1InfoHandler::updateMonitoringItems()
{
  lastL1InfoForLS_ = localLastL1InfoForLS_;
}


void rubuilder::evm::L1InfoHandler::resetMonitoringCounters()
{
  localLastL1InfoForLS_ = 0;
}


void rubuilder::evm::L1InfoHandler::configure()
{
  clear();
  l1InfoFIFO_.resize(l1InfoFIFOCapacity_);
  lumiSectionInfoFIFO_.resize(lumiSectionInfoFIFOCapacity_);
}


void rubuilder::evm::L1InfoHandler::clear()
{
  // The FIFOs are drained by independent workloops.
  // Thus, just wait until they are empty.
  while ( ! empty() ) ::usleep(1000);

  lastSeenLumiSection_ = 0;
  lastL1decodeError_.clear();
  
  runnumber_ = 0;
  lsnumber_  = 0;
  eventcount_ = 0;
  physicstriggers_ = 0;
  calibrationtriggers_ = 0;
  randomtriggers_ = 0;
  skippedlumisections_ = 0;
  
  l1technical_.clear();
  l1decision_0_63_.clear();
  l1decision_64_127_.clear();
  
  l1technical_.setSize(utils::TRIGGER_BITS_COUNT);
  l1decision_0_63_.setSize(utils::TRIGGER_BITS_COUNT);
  l1decision_64_127_.setSize(utils::TRIGGER_BITS_COUNT);
  
  for (uint16_t i = 0; i < utils::TRIGGER_BITS_COUNT; ++i)
  {
    l1technical_[i]       = 0;
    l1decision_0_63_[i]   = 0;
    l1decision_64_127_[i] = 0;
  }
}


void rubuilder::evm::L1InfoHandler::printHtml(xgi::Output *out)
{
  const std::string urn = app_->getApplicationDescriptor()->getURN();
  
  *out << "<div>"                                                 << std::endl;
  *out << "<p>L1InfoHandler</p>"                                  << std::endl;

  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td><a href=\"/" << urn << "/lastL1Info\" "
    << "onclick=\"window.open(this.href); return false;\">"
    << "last L1 info</a> sent for LS</td>"                        << std::endl;
  *out << "<td>" << localLastL1InfoForLS_ << "</td>"              << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  l1InfoFIFO_.printHtml(out, urn);
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  lumiSectionInfoFIFO_.printHtml(out, urn);
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Configuration</th>"                  << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>enableL1Info</td>"                                 << std::endl;
  *out << "<td>" << enableL1Info_.toString() << "</td>"           << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>sendL1FlashList</td>"                              << std::endl;
  *out << "<td>" << sendL1FlashList_.toString() << "</td>"        << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "</table>"                                              << std::endl;

  *out << "</div>"                                                << std::endl;
}


bool rubuilder::evm::L1InfoHandler::fillL1Info
(
  toolbox::mem::Reference* trigMsg,
  utils::L1Information* l1Info
)
{
  toolbox::mem::Reference* current = trigMsg;
  std::list< toolbox::mem::Reference* > refList;
  
  while (current != 0)
  {
    refList.push_back( current );
    current = current->getNextReference();
  }
  
  std::list< toolbox::mem::Reference* >::reverse_iterator rit, ritEnd;
  rit = refList.rbegin();
  ritEnd = refList.rend();
  
  frlh_t* frlh = 0;            // holds the frl header of the current block
  fedt_t* trailer = 0;         // trailer of current fragment
  fedh_t* header = 0;          // header  of current fragment
  unsigned int blocklength = 0;
  unsigned int frlcount = 0;
  
  while ( rit != ritEnd )
  {
    ++frlcount;
    
    // this loop starts with the analysis of a new trailer.
    if ( blocklength == 0 )
    {
      // the next trailer is in a new block
      frlh = (frlh_t*)(
        (char*)(*rit)->getDataLocation()
        + sizeof( I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME )
      );
      blocklength = frlh->segsize & FRL_SEGSIZE_MASK;
      trailer = (fedt_t*)(((unsigned char*)(frlh+1)) + blocklength - 8);
      
      // the first frlblock must be the last one!
      if ( frlcount == 1 && (frlh->segsize & FRL_LAST_SEGM) != FRL_LAST_SEGM )
      {
        std::ostringstream msg;
        msg << "First frlblock is not the last one " <<
          (frlh->segsize & FRL_LAST_SEGM) << " != " <<
          FRL_LAST_SEGM;
        strncpy(l1Info->reason, msg.str().c_str(), sizeof(l1Info->reason));
        return false;
      }
    }
    
    // check trailer marker
    if ( FED_TCTRLID_EXTRACT( trailer->eventsize ) != FED_SLINK_END_MARKER )
    {
      std::ostringstream msg;
      msg << "Trailer marker wrong " <<
        FED_TCTRLID_EXTRACT( trailer->eventsize ) << " != " <<
        FED_SLINK_END_MARKER;
      strncpy(l1Info->reason, msg.str().c_str(), sizeof(l1Info->reason));
      return false;
    }
    
    // trailer length counts in 8 bytes!
    unsigned int fraglength = FED_EVSZ_EXTRACT( trailer->eventsize ) << 3;
    
    // now start the odyssee to the fragment header
    // advance to the block where we expect the header
    while ( fraglength > blocklength ) 
    {
      // advance to the next block
      fraglength -= blocklength;
      ++rit;
      if ( rit == ritEnd )
      {
        strncpy(l1Info->reason, "Premature end of block", sizeof(l1Info->reason));
        return false;
      }
      
      frlh = (frlh_t*)(
        (char*)(*rit)->getDataLocation()
        + sizeof( I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME )
      );
      blocklength = frlh->segsize & FRL_SEGSIZE_MASK;
    }
    
    // the header must be in the current block.
    unsigned char* fedptr = (((unsigned char*)(frlh + 1)) + blocklength - fraglength);
    header = (fedh_t*)fedptr;
    
    // blocklength now points to the end of the previous fragment or is 0
    blocklength -= fraglength;
    
    // check if here is really a header
    if ( FED_HCTRLID_EXTRACT(header->eventid) != FED_SLINK_START_MARKER )
    {
      std::ostringstream msg;
      msg << "Mismatch in FED header: " <<
        FED_HCTRLID_EXTRACT(header->eventid) << " != " <<
        FED_SLINK_START_MARKER;
      strncpy(l1Info->reason, msg.str().c_str(), sizeof(l1Info->reason));
      return false;
    }
    
    unsigned int fedId = FED_SOID_EXTRACT(header->sourceid);
    
    if (fedId == utils::GTP_FED_ID)
    {
      return extractInfoFromGTP(fedptr, l1Info);
    }
    else
    {
      if ( blocklength == 0 )
      {
        // the next trailer is in a new block
        ++rit;
      }
      else if ( blocklength > 0 )
      {
        // the next trailer is in the same block
        trailer = (fedt_t*)( header - 1 );
      }
      else
      {
        // should never get here
        strncpy(l1Info->reason, "Forbidden land", sizeof(l1Info->reason));
        return false;
      }
    }
  }
  
  // if we get here, the GTP_FED_ID was not found
  strncpy(l1Info->reason, "GTP_FED_ID was not found", sizeof(l1Info->reason));
  return false;
}


bool rubuilder::evm::L1InfoHandler::extractInfoFromGTP
(
  const unsigned char* fedptr,
  utils::L1Information* l1Info
)
{
  using namespace evtn;
  
  //set the evm board sense
  if (! set_evm_board_sense(fedptr) )
  {
    strncpy(l1Info->reason, "Cannot decode EVM board sense", sizeof(l1Info->reason));
    return false;
  }
  
  //check that we've got the TCS chip
  if (! has_evm_tcs(fedptr) )
  {
    strncpy(l1Info->reason, "No TCS chip found", sizeof(l1Info->reason));
    return false;
  }
  
  //check that we've got the FDL chip
  if (! has_evm_fdl(fedptr) )
  {
    strncpy(l1Info->reason, "No FDL chip found", sizeof(l1Info->reason));
    return false;
  }
  
  //check that we got the right bunch crossing
  if ( getfdlbxevt(fedptr) != 0 )
  {
    std::ostringstream msg;
    msg << "Wrong bunch crossing in event (expect 0): "
      << getfdlbxevt(fedptr);
    strncpy(l1Info->reason, msg.str().c_str(), sizeof(l1Info->reason));
    return false;
  }
  
  //extract lumi section number
  //use offline numbering scheme where LS starts with 1
  l1Info->lsNumber = getlbn(fedptr) + 1;
  l1Info->eventType = getevtyp(fedptr);
  
  if (l1Info->eventType == 1) // Physics or Technical Trigger
  {
    //extract level 1 trigger bits
    l1Info->l1Technical = getfdlttr(fedptr);
    l1Info->l1Decision_0_63 = getfdlta1(fedptr);
    l1Info->l1Decision_64_127 = getfdlta2(fedptr);
  }
  
  l1Info->isValid = true;
  
  return true;
}

void rubuilder::evm::L1InfoHandler::startL1InfoWorkLoop()
{
  try
  {
    const std::string identifier = utils::getIdentifier(app_->getApplicationDescriptor());
    
    l1InfoWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( identifier + "L1Info", "waiting" );
    
    if ( ! l1InfoWL_->isActive() )
    {
      toolbox::task::ActionSignature* processL1InfoAction = 
        toolbox::task::bind(this, &rubuilder::evm::L1InfoHandler::processL1Info,
          identifier + "processL1Info");
      l1InfoWL_->submit(processL1InfoAction);
      
      l1InfoWL_->activate();
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'L1Info'.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool rubuilder::evm::L1InfoHandler::processL1Info(toolbox::task::WorkLoop*)
{
  std::string errorMsg = "Failed to process L1 information: ";

  try
  {
    boost::mutex::scoped_lock sl(currentLumiSectionInfoMutex_);
    processAllAvailableL1Infos();
  }
  catch(xcept::Exception &e)
  {
    LOG4CPLUS_ERROR(logger_,
      errorMsg << xcept::stdformat_exception_history(e));
        
    XCEPT_DECLARE_NESTED(exception::L1Scalers,
      sentinelException, errorMsg, e);
    app_->notifyQualified("error",sentinelException);
  }
  catch(std::exception &e)
  {
    errorMsg += e.what();

    LOG4CPLUS_ERROR(logger_, errorMsg);

    XCEPT_DECLARE(exception::L1Scalers,
      sentinelException, errorMsg );
    app_->notifyQualified("error",sentinelException);
  }
  catch(...)
  {
    errorMsg += "Unknown exception";

    LOG4CPLUS_ERROR(logger_, errorMsg);

    XCEPT_DECLARE(exception::L1Scalers,
      sentinelException, errorMsg );
    app_->notifyQualified("error",sentinelException);
  }

  ::usleep(1000);
  return true; // reschedule this action
}


void rubuilder::evm::L1InfoHandler::processAllAvailableL1Infos()
{
  toolbox::mem::Reference* bufRef;
  while ( l1InfoFIFO_.deq(bufRef) )
  {
    addLumiSectionInfo((utils::L1Information*)bufRef->getDataLocation());
    bufRef->release();
  }
}


void rubuilder::evm::L1InfoHandler::verifyLumiSection(const utils::L1Information* l1Info)
{
  // LS have to increase monotonically

  if ( l1Info->lsNumber < lastSeenLumiSection_ )
  {
    std::ostringstream msg;
    msg << "Current LS " << l1Info->lsNumber << 
      " is smaller than previously seen LS " <<
      lastSeenLumiSection_ << ".";
    XCEPT_RAISE(exception::L1Trigger, msg.str());
  }
  lastSeenLumiSection_ = l1Info->lsNumber;
}


toolbox::mem::Reference* rubuilder::evm::L1InfoHandler::getNewLumiSectionInfo
(
  uint32_t runNumber,
  uint32_t lumiSection,
  uint32_t previousLumiSection
)
{
  toolbox::mem::Reference* bufRef;
  try
  {
    bufRef = toolbox::mem::getMemoryPoolFactory()->
      getFrame(fastCtrlMsgPool_, sizeof(LumiSectionInfo));
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::OutOfMemory,
      "Failed to get buffer for lumi-section info", e);
  }
  catch(...)
  {
    XCEPT_RAISE(exception::OutOfMemory,
      "Failed to get buffer for lumi-section info : Unknown exception");
  }
  
  bufRef->setDataSize(sizeof(LumiSectionInfo));

  LumiSectionInfo* lsInfo = (LumiSectionInfo*)bufRef->getDataLocation();
  lsInfo->runNumber = runNumber;
  lsInfo->lsNumber = lumiSection;
  lsInfo->skippedLumiSections = lumiSection - previousLumiSection - 1;
  lsInfo->eventCount = 0;
  lsInfo->physicsTriggers = 0;
  lsInfo->calibrationTriggers = 0;
  lsInfo->randomTriggers = 0;
  
  for (uint16_t i = 0; i < utils::TRIGGER_BITS_COUNT; ++i)
  {
    lsInfo->l1Technical[i]       = 0;
    lsInfo->l1Decision_0_63[i]   = 0;
    lsInfo->l1Decision_64_127[i] = 0;
  }

  addSkippedLumiSectionsToFIFO(lsInfo);

  return bufRef;
}


void rubuilder::evm::L1InfoHandler::addSkippedLumiSectionsToFIFO(const LumiSectionInfo* lsInfo)
{
  EoLSHandler::LumiSectionPair ls;
  ls.runNumber = lsInfo->runNumber;

  for (uint32_t lumiSection = lsInfo->lsNumber - lsInfo->skippedLumiSections;
       lumiSection < lsInfo->lsNumber; ++lumiSection)
  {
    ls.lumiSection = lumiSection;
    eolsHandler_->send(ls);
  }
}


toolbox::mem::Reference* rubuilder::evm::L1InfoHandler::getNewL1Information()
{
  toolbox::mem::Reference* bufRef;
  try
  {
    bufRef = toolbox::mem::getMemoryPoolFactory()->
      getFrame(fastCtrlMsgPool_, sizeof(utils::L1Information));
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::OutOfMemory,
      "Failed to get buffer for L1 info", e);
  }
  catch(...)
  {
    XCEPT_RAISE(exception::OutOfMemory,
      "Failed to get buffer for L1 info : Unknown exception");
  }
  
  bufRef->setDataSize(sizeof(utils::L1Information));
  
  utils::L1Information* l1Info = (utils::L1Information*)bufRef->getDataLocation();
  l1Info->reset();

  return bufRef;
}


void rubuilder::evm::L1InfoHandler::addLumiSectionInfo
(
  const utils::L1Information* l1Info
)
{
  // currentLumiSectionInfoMutex_ is taken by calling method processL1Info

  if ( currentLumiSectionInfo_ == 0 )
  {
    // First LS seen by EVM since reset
    currentLumiSectionInfo_ = getNewLumiSectionInfo(l1Info->runNumber, l1Info->lsNumber, 0);
  }
  else if ( l1Info->lsNumber > ((LumiSectionInfo*)currentLumiSectionInfo_->getDataLocation())->lsNumber )
  {
    // New LS started
    // Note: we derive here the last LS number from the currentLumiSectionInfo and do _not_ use
    // the class parameter lastSeenLumiSection_ as this thread might be out of sync compared to
    // the main thread receiving the trigger information.
    uint32_t lastLumiSectionNumber =
      ((LumiSectionInfo*)currentLumiSectionInfo_->getDataLocation())->lsNumber;
    while ( ! lumiSectionInfoFIFO_.enq(currentLumiSectionInfo_) ) ::usleep(1000);
    
    currentLumiSectionInfo_ =
      getNewLumiSectionInfo(l1Info->runNumber, l1Info->lsNumber, lastLumiSectionNumber);
  }

  LumiSectionInfo* lsInfo = (LumiSectionInfo*)currentLumiSectionInfo_->getDataLocation();
  addEventCounts(lsInfo, l1Info);
  addTriggerBits(lsInfo, l1Info);
}


void rubuilder::evm::L1InfoHandler::addEventCounts
(
  LumiSectionInfo* lsInfo,
  const utils::L1Information* l1Info
) const
{
  ++(lsInfo->eventCount);
  switch (l1Info->eventType)
  {
    case 1:
      ++(lsInfo->physicsTriggers);
      break;
    case 2:
      ++(lsInfo->calibrationTriggers);
      break;
    case 3:
      ++(lsInfo->randomTriggers);
      break;
  }
}


void rubuilder::evm::L1InfoHandler::addTriggerBits
(
  LumiSectionInfo* lsInfo,
  const utils::L1Information* l1Info
) const
{
  sumBits(l1Info->l1Technical,       lsInfo->l1Technical);
  sumBits(l1Info->l1Decision_0_63,   lsInfo->l1Decision_0_63);
  sumBits(l1Info->l1Decision_64_127, lsInfo->l1Decision_64_127);
}


void rubuilder::evm::L1InfoHandler::sumBits
(
  const uint64_t& triggerBits,
  L1TriggerBitsArray& sums
) const
{
  if (triggerBits == 0) return;
  
  uint64_t mask = 1;
  for (uint16_t i = 0; i < utils::TRIGGER_BITS_COUNT; ++i)
  {
    if (triggerBits & mask) ++(sums[i]);
    mask <<= 1;
  }
}


void rubuilder::evm::L1InfoHandler::startL1ScalersWorkLoop()
{
  try
  {
    const std::string identifier = utils::getIdentifier(app_->getApplicationDescriptor());
    
    l1ScalersWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( identifier + "L1Scalers", "waiting" );
    
    if ( ! l1ScalersWL_->isActive() )
    {
      toolbox::task::ActionSignature* sendL1ScalersAction =
        toolbox::task::bind(this, &rubuilder::evm::L1InfoHandler::sendL1Scalers,
          identifier + "sendL1Scalers");
      l1ScalersWL_->submit(sendL1ScalersAction);
      
      l1ScalersWL_->activate();
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'L1Scalers'.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool rubuilder::evm::L1InfoHandler::sendL1Scalers(toolbox::task::WorkLoop*)
{
  std::string errorMsg = "Failed to send L1Scalers information: ";
  idle_ = false;
  
  try
  {
    sendAllAvailableL1Scalers();
  }
  catch(xcept::Exception &e)
  {
    LOG4CPLUS_ERROR(logger_,
      errorMsg << xcept::stdformat_exception_history(e));
        
    XCEPT_DECLARE_NESTED(exception::L1Scalers,
      sentinelException, errorMsg, e);
    app_->notifyQualified("error",sentinelException);
  }
  catch(std::exception &e)
  {
    errorMsg += e.what();

    LOG4CPLUS_ERROR(logger_, errorMsg);

    XCEPT_DECLARE(exception::L1Scalers,
      sentinelException, errorMsg );
    app_->notifyQualified("error",sentinelException);
  }
  catch(...)
  {
    errorMsg += "Unknown exception";

    LOG4CPLUS_ERROR(logger_, errorMsg);

    XCEPT_DECLARE(exception::L1Scalers,
      sentinelException, errorMsg );
    app_->notifyQualified("error",sentinelException);
  }

  idle_ = true;

  ::sleep(1);
  return true; // reschedule this action
}


void rubuilder::evm::L1InfoHandler::sendAllAvailableL1Scalers()
{
  toolbox::mem::Reference* bufRef;
  while ( lumiSectionInfoFIFO_.deq(bufRef) )
  {
    updateL1Scalers((LumiSectionInfo*)bufRef->getDataLocation());
    bufRef->release();
  }
}


void rubuilder::evm::L1InfoHandler::updateL1Scalers(const LumiSectionInfo* lsInfo)
{
  try
  {
    l1ScalersInfoSpace_->lock();
        
    runnumber_ = lsInfo->runNumber;
    lsnumber_ = lsInfo->lsNumber;
    eventcount_ = lsInfo->eventCount;
    physicstriggers_ = lsInfo->physicsTriggers;
    calibrationtriggers_ = lsInfo->calibrationTriggers;
    randomtriggers_ = lsInfo->randomTriggers;
    skippedlumisections_ = lsInfo->skippedLumiSections;
        
    for (uint16_t i = 0; i < utils::TRIGGER_BITS_COUNT; ++i)
    {
      l1technical_[i]       = lsInfo->l1Technical[i];
      l1decision_0_63_[i]   = lsInfo->l1Decision_0_63[i];
      l1decision_64_127_[i] = lsInfo->l1Decision_64_127[i];
    }
    
    sendL1ScalersMsg();
    
    l1ScalersInfoSpace_->unlock();

    localLastL1InfoForLS_ = lsInfo->lsNumber;
  }
  catch(xcept::Exception &e)
  {
    l1ScalersInfoSpace_->unlock();
    
    XCEPT_RETHROW(exception::L1Scalers,
      "Failed to update L1 scaler info", e);
  }
  catch (...)
  {
    l1ScalersInfoSpace_->unlock();
    
    XCEPT_RAISE(exception::L1Scalers, 
      "Failed to update L1 scalers info: unknown exception");
  }
  
  if (sendL1FlashList_)
  {
    try
    {
      // The fireItemGroupChanged locks the infospace
      l1ScalersInfoSpace_->fireItemGroupChanged(l1ScalersParamNames_, app_);
    }
    catch (xdata::exception::Exception &e)
    {
      XCEPT_RETHROW(exception::L1Scalers,
        "Failed to fireItemGroupChanged for L1Scalers info space values", e);
    }
  }
}


void rubuilder::evm::L1InfoHandler::sendL1ScalersMsg()
{
  try
  {
    xdata::Table::iterator it = l1ScalersTable_.begin();
    it->setField("runnumber", runnumber_);
    it->setField("lsnumber", lsnumber_);
    it->setField("eventcount", eventcount_);
    it->setField("physicstriggers", physicstriggers_);
    it->setField("calibrationtriggers", calibrationtriggers_);
    it->setField("randomtriggers", randomtriggers_);
    it->setField("skippedlumisections", skippedlumisections_);
    for (uint16_t bit = 0; bit < utils::TRIGGER_BITS_COUNT; ++bit)
    {
      std::ostringstream bitName;
      bitName << bit;
      it->setField("l1technical_"+bitName.str(), l1technical_[bit]);
    }
    for (uint16_t bit = 0; bit < utils::TRIGGER_BITS_COUNT; ++bit)
    {
      std::ostringstream bitName;
      bitName << bit;
      it->setField("l1decision_" +bitName.str(),l1decision_0_63_[bit]);
    }
    for (uint16_t bit = 0; bit < utils::TRIGGER_BITS_COUNT; ++bit)
    {
      std::ostringstream bitName;
      bitName << (bit+utils::TRIGGER_BITS_COUNT);
      it->setField("l1decision_" +bitName.str(),l1decision_64_127_[bit]);
    }
    rcmsNotifier_.sendData(l1ScalersTable_, "urn:xdaq-flashlist:HLTS_L1Scalers");
  }
  catch (xdata::exception::Exception &e)
  {
    XCEPT_RETHROW(exception::L1Scalers,
      "Failed to send L1 scalers to L1Scalers", e);
  }
}


void rubuilder::evm::L1InfoHandler::printL1Scalers(xgi::Output* out)
{
  if ( ! enableL1Info_ ) return;
  
  *out << "<div class=\"queuedetail\">"                         << std::endl;
  *out << "<table border=\"1\">"                                << std::endl;
  try
  {
    l1ScalersParams_.printHtml("L1 trigger information", out);
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::XGI,
      "Failed to print level 1 trigger information table", e);
  }
  *out << "</table>"                                            << std::endl;
  if ( ! lastL1decodeError_.empty() )
  {
    *out << "<p>"                                               << std::endl;
    *out << "Last error from L1 trigger decoding: "             << std::endl;
    *out << "<pre>"                                             << std::endl;
    *out <<  lastL1decodeError_                                 << std::endl;
    *out << "</pre>"                                            << std::endl;
    *out << "</p>"                                              << std::endl;
  }
  *out << "</div>"                                              << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
