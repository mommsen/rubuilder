#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "rubuilder/evm/BUproxy.h"
#include "rubuilder/evm/EoLSHandler.h"
#include "rubuilder/utils/Exception.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "xcept/Exception.h"
#include "xcept/tools.h"


rubuilder::evm::BUproxy::BUproxy
(
  xdaq::Application* app,
  boost::shared_ptr<EoLSHandler> eolsHandler,
  toolbox::mem::Pool* fastCtrlMsgPool
) :
app_(app),
lumiSectionTable_(eolsHandler),
fastCtrlMsgPool_(fastCtrlMsgPool),
logger_(app->getApplicationLogger()),
tid_(0),
eventFIFO_("eventFIFO"),
requestFIFOs_("requestFIFOs"),
releasedEvbIdFIFO_("releasedEvbIdFIFO"),
eolsFIFOs_("eolsFIFOs")
{
  resetMonitoringCounters();
  configure();
  eolsHandler->registerBUproxy(this);
}


void rubuilder::evm::BUproxy::I2Ocallback(toolbox::mem::Reference* bufRef)
{
  I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
  msg::EvtIdRqstsAndOrReleasesMsg* msg =
    (msg::EvtIdRqstsAndOrReleasesMsg*)stdMsg;

  updateAllocateClearCounters(msg->nbElements);

  pushRequestOntoFIFOs(msg, stdMsg->InitiatorAddress);

  bufRef->release();
}


void rubuilder::evm::BUproxy::updateAllocateClearCounters(const uint32_t& nbElements)
{
  boost::mutex::scoped_lock sl(allocateClearCountersMutex_);

  allocateClearCounters_.payload += nbElements * sizeof(msg::EvtIdRqstAndOrRelease);
  allocateClearCounters_.logicalCount += nbElements;
  ++allocateClearCounters_.i2oCount;
}


void rubuilder::evm::BUproxy::pushRequestOntoFIFOs
(
  const msg::EvtIdRqstsAndOrReleasesMsg* msg,
  const I2O_TID& buTid
)
{
  RqstFifoElement rqstForEvtId;
  rqstForEvtId.buTid    = buTid;
  rqstForEvtId.buIndex  = msg->srcIndex;

  ReleasedEvtIdFifoElement releasedEvtId;
  releasedEvtId.buIndex = msg->srcIndex;
  
  for (uint32_t i=0; i<msg->nbElements; ++i)
  {
    msg::EvtIdRqstAndOrRelease rqstAndOrRelease = msg->elements[i];
    rqstForEvtId.resourceId   = rqstAndOrRelease.resourceId;
    
    releasedEvtId.evbId       = rqstAndOrRelease.evbId;
    releasedEvtId.resourceId  = rqstAndOrRelease.resourceId;
    
    if ( rqstAndOrRelease.requestType & msg::EvtIdRqstAndOrRelease::REQUEST )
      requestEvent(rqstForEvtId);
    
    if ( rqstAndOrRelease.requestType & msg::EvtIdRqstAndOrRelease::RELEASE )
      releaseEvent(releasedEvtId);
  }
}


void rubuilder::evm::BUproxy::requestEvent
(
  const RqstFifoElement& rqstForEvtId
)
{
  toolbox::mem::Reference* bufRef = 0;
  if ( eolsFIFOs_.deq(rqstForEvtId.buIndex, bufRef) )
  {
    sendEoLStoBU(rqstForEvtId, bufRef);
  }
  else
  {
    while ( ! requestFIFOs_.enq(rqstForEvtId.buIndex, rqstForEvtId) ) ::usleep(1000);
  }
}


void rubuilder::evm::BUproxy::releaseEvent
(
  const ReleasedEvtIdFifoElement& releasedEvtId
)
{
  while ( ! releasedEvbIdFIFO_.enq(releasedEvtId) ) ::usleep(1000);
}


bool rubuilder::evm::BUproxy::processNextReleasedEvent()
{
  ReleasedEvtIdFifoElement releasedEvtId;
  if ( ! releasedEvbIdFIFO_.deq(releasedEvtId) ) return false;

  EvBidMap::iterator pos = evbIdMap_.find(releasedEvtId.evbId);
  if ( pos == evbIdMap_.end() )
  {
    std::stringstream errorMsg;
    errorMsg << "Received an event release message with unknown evb id: "
      << releasedEvtId;
    XCEPT_RAISE(exception::EventOrder,errorMsg.str());
  }
  lumiSectionTable_.decrementEventsInRuBuilder(pos->second);
  evbIdMap_.erase(pos);

  return true;
}


void rubuilder::evm::BUproxy::addEvent(const EventFifoElement& event)
{
  EoLSHandler::LumiSectionPair ls;
  ls.runNumber = event.runNumber;
  ls.lumiSection = event.lumiSection;
  lumiSectionTable_.incrementEventsInRuBuilder(ls);
  if ( ! evbIdMap_.insert(EvBidMap::value_type(event.evbId,ls)).second )
  {
    std::stringstream errorMsg;
    errorMsg << "Cannot add an event with an already existing evb id: "
      << event;
    XCEPT_RAISE(exception::EventOrder,errorMsg.str());
  }
  
  if ( ! eventFIFO_.enq(event) )
  {
    XCEPT_RAISE(exception::FIFO,
      "Failed to push event onto the back of event FIFO");
  }
}


bool rubuilder::evm::BUproxy::serviceBuRqst()
{
  if ( eventFIFO_.empty() ) return false;

  RqstFifoElement rqst;
  {
    boost::mutex::scoped_lock sl(requestFIFOsMutex_);
    if ( assignRoundRobin_ )
    {
      if ( ! requestFIFOs_.deq(rqst) ) return false;
    }
    else
    {
      if ( ! requestFIFOs_.deqFromFullest(rqst) ) return false;
    }
  }
    
  EventFifoElement event;
  assert( eventFIFO_.deq(event) ); // There must be an element as we checked that eventFIFO is not empty.
  
  I2O_MESSAGE_FRAME *stdMsg =
    (I2O_MESSAGE_FRAME*)event.trigBufRef->getDataLocation();
  I2O_PRIVATE_MESSAGE_FRAME *pvtMsg =
    (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
  I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME *block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;
  
  updateConfirmCounters(stdMsg);
  
  //////////////////////////////////////////////////////
  // Modify the trigger data message ready for the BU //
  //////////////////////////////////////////////////////
  
  stdMsg->InitiatorAddress = tid_;
  stdMsg->TargetAddress    = rqst.buTid;
  
  pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
  pvtMsg->XFunctionCode    = I2O_BU_CONFIRM;
  
  block->resyncCount       = event.evbId.resyncCount();
  block->lumiSection       = event.lumiSection;
  block->runNumber         = event.runNumber;
  block->buResourceId      = rqst.resourceId;
  
  xdaq::ApplicationDescriptor *bu = 0;
  try
  {
    bu = i2o::utils::getAddressMap()->getApplicationDescriptor(rqst.buTid);
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    
    oss << "Failed to get application descriptor for BU with tid ";
    oss << rqst.buTid;
    
    XCEPT_RAISE(exception::Configuration, oss.str());
  }
  
  try
  {
    app_->getApplicationContext()->
      postFrame(
        event.trigBufRef,
        app_->getApplicationDescriptor(),
        bu//,
        //i2oExceptionHandler_,
        //bu
      );
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    
    oss << "Failed to send event block to BU";
    oss << bu->getInstance();
    
    XCEPT_RETHROW(exception::I2O, oss.str(), e);
  }

  return true;
}


void rubuilder::evm::BUproxy::updateConfirmCounters(const I2O_MESSAGE_FRAME* stdMsg)
{
  I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME *block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;

  boost::mutex::scoped_lock sl(confirmCountersMutex_);

  confirmCounters_.payload += (stdMsg->MessageSize << 2) -
    sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
  ++confirmCounters_.logicalCount;
  ++confirmCounters_.i2oCount;
  confirmCounters_.lastEventNumberToBUs = block->eventNumber;
}


void rubuilder::evm::BUproxy::appendConfigurationItems(utils::InfoSpaceItems& params)
{
  eventFIFOCapacity_ = utils::DEFAULT_NB_EVENTS;
  requestFIFOCapacity_ = utils::DEFAULT_NB_EVENTS;
  releasedEvbIdFIFOCapacity_ = utils::DEFAULT_NB_EVENTS;
  eolsFIFOCapacity_ = 128;
  assignRoundRobin_ = false;

  buParams_.clear(); 
  buParams_.add("eventFIFOCapacity", &eventFIFOCapacity_);
  buParams_.add("requestFIFOCapacity", &requestFIFOCapacity_);
  buParams_.add("releasedEvbIdFIFOCapacity", &releasedEvbIdFIFOCapacity_);
  buParams_.add("eolsFIFOCapacity", &eolsFIFOCapacity_);
  buParams_.add("assignRoundRobin", &assignRoundRobin_);
  
  params.add(buParams_);
}


void rubuilder::evm::BUproxy::appendMonitoringItems(utils::InfoSpaceItems& items)
{
  lastEventNumberToBUs_ = 0;
  i2oEVMAllocClearCount_ = 0;
  i2oBUConfirmLogicalCount_ = 0;

  items.add("lastEventNumberToBUs", &lastEventNumberToBUs_);
  items.add("i2oEVMAllocClearCount", &i2oEVMAllocClearCount_);
  items.add("i2oBUConfirmLogicalCount", &i2oBUConfirmLogicalCount_);
}


void rubuilder::evm::BUproxy::updateMonitoringItems()
{
  {
    boost::mutex::scoped_lock sl(confirmCountersMutex_);

    lastEventNumberToBUs_ = confirmCounters_.lastEventNumberToBUs;
    i2oBUConfirmLogicalCount_ = confirmCounters_.logicalCount;
  }
  
  {
    boost::mutex::scoped_lock sl(allocateClearCountersMutex_);

    i2oEVMAllocClearCount_ = allocateClearCounters_.logicalCount;
  }
}


void rubuilder::evm::BUproxy::resetMonitoringCounters()
{
  {
    boost::mutex::scoped_lock sl(confirmCountersMutex_);
    confirmCounters_.payload = 0;
    confirmCounters_.logicalCount = 0;
    confirmCounters_.i2oCount = 0;
    confirmCounters_.lastEventNumberToBUs = 0;
  }

  {
    boost::mutex::scoped_lock sl(allocateClearCountersMutex_);
    allocateClearCounters_.payload = 0;
    allocateClearCounters_.logicalCount = 0;
    allocateClearCounters_.i2oCount = 0;
  }

  {
    boost::mutex::scoped_lock sl(EoLSMonitoringMutex_);
    EoLSMonitoring_.payload = 0;
    EoLSMonitoring_.msgCount = 0;
    EoLSMonitoring_.i2oCount = 0;
  }
}


void rubuilder::evm::BUproxy::configure()
{
  clear();

  eventFIFO_.resize(eventFIFOCapacity_);
  requestFIFOs_.resize(requestFIFOCapacity_);
  releasedEvbIdFIFO_.resize(releasedEvbIdFIFOCapacity_);
  eolsFIFOs_.resize(eolsFIFOCapacity_);
}


void rubuilder::evm::BUproxy::clear()
{
  EventFifoElement event;
  while( eventFIFO_.deq(event) ) { event.trigBufRef->release(); }
  
  RqstFifoElement rqst;
  while( requestFIFOs_.deq(rqst) ) {};
  
  ReleasedEvtIdFifoElement element;
  while( releasedEvbIdFIFO_.deq(element) ) {};

  toolbox::mem::Reference* bufRef;
  while( eolsFIFOs_.deq(bufRef) ) { bufRef->release(); }

  evbIdMap_.clear();
  lumiSectionTable_.clear();
}


void rubuilder::evm::BUproxy::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>BUproxy</p>"                                        << std::endl;

  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<td>last evt number to BUs</td>"                       << std::endl;
  *out << "<td>" << confirmCounters_.lastEventNumberToBUs << "</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\" style=\"text-align:center\">BU confirms</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;

  {
    boost::mutex::scoped_lock sl(confirmCountersMutex_);
    *out << "<tr>"                                                << std::endl;
    *out << "<td>payload (MB)</td>"                               << std::endl;
    *out << "<td>" << confirmCounters_.payload / 0x100000<< "</td>" << std::endl;
    *out << "</tr>"                                               << std::endl;
    *out << "<tr>"                                                << std::endl;
    *out << "<td>msg count</td>"                                  << std::endl;
    *out << "<td>" << confirmCounters_.logicalCount << "</td>"    << std::endl;
    *out << "</tr>"                                               << std::endl;
    *out << "<tr>"                                                << std::endl;
    *out << "<td>I2O count</td>"                                  << std::endl;
    *out << "<td>" << confirmCounters_.i2oCount << "</td>"        << std::endl;
    *out << "</tr>"                                               << std::endl;
  }

  {
    boost::mutex::scoped_lock sl(allocateClearCountersMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">BU allocate/clears</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (kB)</td>"                                << std::endl;
    *out << "<td>" << allocateClearCounters_.payload / 0x400 << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>msg count</td>"                                    << std::endl;
    *out << "<td>" << allocateClearCounters_.logicalCount << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << allocateClearCounters_.i2oCount << "</td>"    << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }

  {
    boost::mutex::scoped_lock sl(EoLSMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">EoLS msg</td>"      << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (bytes)</td>"                              << std::endl;
    *out << "<td>" << EoLSMonitoring_.payload << "</td>"            << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>msg count</td>"                                    << std::endl;
    *out << "<td>" << EoLSMonitoring_.msgCount << "</td>"           << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << EoLSMonitoring_.i2oCount << "</td>"           << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }

  const std::string urn = app_->getApplicationDescriptor()->getURN();
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  eventFIFO_.printHtml(out, urn);
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  requestFIFOs_.printHtml(out, urn);
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  releasedEvbIdFIFO_.printHtml(out, urn);
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  eolsFIFOs_.printHtml(out, urn);
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  buParams_.printHtml("Configuration", out);
  
  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


void rubuilder::evm::BUproxy::getApplicationDescriptors()
{
  try
  {
    tid_ = i2o::utils::getAddressMap()->
      getTid(app_->getApplicationDescriptor());
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      "Failed to get I2O TID for this application.", e);
  }


  const std::string errorMsg =
    "Failed to get descriptors of participating BUs: ";

  try
  {
    buDescriptors_ =
      app_->getApplicationContext()->
      getDefaultZone()->
      getApplicationDescriptors("rubuilder::bu::Application");
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      errorMsg, e);
  }
  catch(std::exception &e)
  {
    XCEPT_RAISE(exception::Configuration,
      errorMsg + e.what());
  }
  catch(...)
  {
    XCEPT_RAISE(exception::Configuration,
      errorMsg + "unknown exception");
  }
}


void rubuilder::evm::BUproxy::sendEoLSmsg(const I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME* msg)
{
  ///////////////////////////////////////////////////////////////////
  // Make a copy of the end-of-lumisection message for each BU and //
  // either send it if there's a request or enqueue it to be sent  //
  // when the next request arrives                                 //
  ///////////////////////////////////////////////////////////////////
  
  for (BUdescriptors::const_iterator it=buDescriptors_.begin(), itEnd=buDescriptors_.end();
       it != itEnd; ++it)
  {
    // Create an empty message
    toolbox::mem::Reference* copyBufRef =
      toolbox::mem::getMemoryPoolFactory()->
      getFrame(fastCtrlMsgPool_,
        sizeof(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME));
    char* copyFrame  = (char*)(copyBufRef->getDataLocation());
    
    // Copy the message under construction into
    // the newly created empty message
    memcpy(
      copyFrame,
      msg,
      sizeof(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME)
    );
    
    // Set the size of the copy
    copyBufRef->setDataSize(sizeof(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME));
    
    const uint32_t buIndex = (*it)->getInstance();
    RqstFifoElement rqstForEvtId;
    boost::mutex::scoped_lock sl(requestFIFOsMutex_);
    if ( requestFIFOs_.deq(buIndex, rqstForEvtId) )
    {
      // Send it to the BU
      sl.unlock();
      sendEoLStoBU(rqstForEvtId, copyBufRef);
    }
    else
    {
      // Put it into the queue
      sl.unlock();
      if ( ! eolsFIFOs_.enq(buIndex, copyBufRef) )
      {
        std::ostringstream oss;
        oss << "EoLS queue for BU " << buIndex
          << " is full.";
        
        XCEPT_RAISE(exception::L1Scalers, oss.str() );
      }
    }
  }
}


void rubuilder::evm::BUproxy::sendEoLStoBU
(
  const RqstFifoElement& rqstForEvtId,
  toolbox::mem::Reference* bufRef
)
{
  const std::string errorMsg =
    "Failed to send end-of-lumi-section signal to BU";
  
  try
  {
    I2O_MESSAGE_FRAME* stdMsg = (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
    stdMsg->TargetAddress = rqstForEvtId.buTid;
    
    I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME* msg =
      (I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME*)stdMsg;
    msg->buResourceId = rqstForEvtId.resourceId;
    
    xdaq::ApplicationDescriptor *bu =
      i2o::utils::getAddressMap()->getApplicationDescriptor(rqstForEvtId.buTid);
    
    // Send the message to the BU
    app_->getApplicationContext()->
      postFrame(
        bufRef,
        app_->getApplicationDescriptor(),
        bu//,
        //i2oExceptionHandler_,
        //bu
      );
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::I2O,
      errorMsg, e);
  }
  catch(std::exception &e)
  {
    XCEPT_RAISE(exception::I2O,
      errorMsg + e.what());
  }
  catch(...)
  {
    XCEPT_RAISE(exception::I2O,
      errorMsg + "unknown exception");
  }
  
  {
    boost::mutex::scoped_lock sl(EoLSMonitoringMutex_);
    EoLSMonitoring_.payload += sizeof(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME);
    ++EoLSMonitoring_.msgCount;
    ++EoLSMonitoring_.i2oCount;
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
