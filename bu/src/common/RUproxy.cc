#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "rubuilder/bu/RUproxy.h"
#include "rubuilder/utils/Constants.h"
#include "rubuilder/utils/EvBid.h"
#include "rubuilder/utils/I2OMessages.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "xcept/tools.h"
#include "xdaq/ApplicationDescriptor.h"

#include <string.h>


rubuilder::bu::RUproxy::RUproxy
(
  xdaq::Application* app,
  toolbox::mem::Pool* fastCtrlMsgPool
) :
RUbroadcaster(app,fastCtrlMsgPool),
index_(0),
blockFIFO_("blockFIFO"),
rqstForFragsBufRef_(0),
timerId_(timerManager_.getTimer())
{
  resetMonitoringCounters();
}


void rubuilder::bu::RUproxy::I2Ocallback(toolbox::mem::Reference* bufRef)
{
  // Break the chain (if there is one) into separate blocks and push those
  // blocks onto the back of the blockFIFO
  while (bufRef != 0)
  {
    toolbox::mem::Reference* nextBufRef = bufRef->getNextReference();
    bufRef->setNextReference(0);
    
    updateBlockCounters(bufRef);
    
    while ( ! blockFIFO_.enq(bufRef) ) ::usleep(1000);
    
    bufRef = nextBufRef;
  }
}


bool rubuilder::bu::RUproxy::getDataBlock(toolbox::mem::Reference*& bufRef)
{
  return ( blockFIFO_.deq(bufRef) );
}


void rubuilder::bu::RUproxy::updateBlockCounters(toolbox::mem::Reference* bufRef)
{
  boost::mutex::scoped_lock sl(blockMonitoringMutex_);

  const I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
  const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;
  const size_t payload =
    (stdMsg->MessageSize << 2) - sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
  const uint32_t ruInstance = block->superFragmentNb - 1;

  blockMonitoring_.lastEventNumberFromRUs = block->eventNumber;
  blockMonitoring_.payload += payload;
  blockMonitoring_.payloadPerRU[ruInstance] += payload;
  ++blockMonitoring_.i2oCount;
  if ( block->blockNb == (block->nbBlocksInSuperFragment-1) )
  {
    ++blockMonitoring_.logicalCount;
    ++blockMonitoring_.logicalCountPerRU[ruInstance];
  }
}


void rubuilder::bu::RUproxy::requestDataForTrigger(toolbox::mem::Reference* bufRef)
{
  boost::mutex::scoped_lock sl(rqstForFragsMutex_);

  // Create a request message if one does not exist
  if (rqstForFragsBufRef_ == 0) createRqstForFrags();
  
  const uint32_t nbElementsPacked = packRqstForFragsMsg(bufRef);
  
  if (nbElementsPacked == I2O_RU_SEND_Packing_)
  {
    sendRqstForFragsToAllRUs();
  }
}


void rubuilder::bu::RUproxy::createRqstForFrags()
{
  rqstForFragsBufRef_ =
    toolbox::mem::getMemoryPoolFactory()->
    getFrame(fastCtrlMsgPool_, rqstForFragsBufSize_);
  
  I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)rqstForFragsBufRef_->getDataLocation();
  stdMsg->InitiatorAddress = tid_;
  stdMsg->Function         = I2O_PRIVATE_MESSAGE;
  stdMsg->VersionOffset    = 0;
  stdMsg->MsgFlags         = 0;
  
  I2O_PRIVATE_MESSAGE_FRAME* pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
  pvtMsg->XFunctionCode    = I2O_RU_SEND;
  pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
  
  msg::RqstForFragsMsg* rqstForFragsMsg = (msg::RqstForFragsMsg*)stdMsg;
  rqstForFragsMsg->srcIndex   = index_;
  rqstForFragsMsg->nbElements = 0;
  
  // Start the aging timer
  timerManager_.restartTimer(timerId_);
}


uint32_t rubuilder::bu::RUproxy::packRqstForFragsMsg
(
  toolbox::mem::Reference* bufRef
)
{
  const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
  
  msg::RqstForFrag rqstForFrag;
  rqstForFrag.evbId        = utils::EvBid(block->resyncCount,block->eventNumber);
  rqstForFrag.buResourceId = block->buResourceId;
  rqstForFrag.padding      = 0;
  
  msg::RqstForFragsMsg* msg =
    (msg::RqstForFragsMsg*)rqstForFragsBufRef_->getDataLocation();

  // Insert and update number of requests
  msg->elements[msg->nbElements] = rqstForFrag;
  msg->nbElements++;

  // Calculate the size of the message with its new element
  const size_t msgSize = sizeof(msg::RqstForFragsMsg) -
    sizeof(msg::RqstForFrag) +
    sizeof(msg::RqstForFrag) * msg->nbElements;

  // Store the message size in both the I2O header and the reference
  msg->PvtMessageFrame.StdMessageFrame.MessageSize = msgSize >> 2;
  rqstForFragsBufRef_->setDataSize(msgSize);

  // Return the number of elements in the message
  return msg->nbElements;
}


void rubuilder::bu::RUproxy::sendRqstForFragsToAllRUs()
{
  sendToAllRUs(rqstForFragsBufRef_, rqstForFragsBufSize_);
  
  updateRequestCounters();

  // Free the requests message under construction
  // (its copies were sent not it)
  rqstForFragsBufRef_->release();
  rqstForFragsBufRef_ = 0;
}


void rubuilder::bu::RUproxy::updateRequestCounters()
{
  boost::mutex::scoped_lock sl(requestMonitoringMutex_);
  
  const msg::RqstForFragsMsg* msg =
    (msg::RqstForFragsMsg*)rqstForFragsBufRef_->getDataLocation();
  const uint32_t nbElements = msg->nbElements;

  requestMonitoring_.payload += nbElements * sizeof(msg::RqstForFrag);
  requestMonitoring_.logicalCount += nbElements;
  requestMonitoring_.i2oCount += participatingRUs_.size();
}


bool rubuilder::bu::RUproxy::sendOldRequests()
{
  boost::mutex::scoped_lock sl(rqstForFragsMutex_);

  if ( 
    rqstForFragsBufRef_ &&
    timerManager_.isFired(timerId_)
  )
  {
    sendRqstForFragsToAllRUs();
    return true;
  }
  else
  {
    return false;
  }
}


void rubuilder::bu::RUproxy::appendConfigurationItems(utils::InfoSpaceItems& params)
{
  I2O_RU_SEND_Packing_ = 8;
  blockFIFOCapacity_ = 16384;
  
  ruParams_.clear();
  ruParams_.add("I2O_RU_SEND_Packing", &I2O_RU_SEND_Packing_);
  ruParams_.add("blockFIFOCapacity", &blockFIFOCapacity_);
  
  initRuInstances(ruParams_);
  
  params.add(ruParams_);
}


void rubuilder::bu::RUproxy::appendMonitoringItems(utils::InfoSpaceItems& items)
{
  lastEventNumberFromRUs_ = 0;
  i2oBUCacheCount_ = 0;
  i2oRUSendCount_ = 0;

  items.add("lastEventNumberFromRUs", &lastEventNumberFromRUs_);
  items.add("i2oBUCacheCount", &i2oBUCacheCount_);
  items.add("i2oRUSendCount", &i2oRUSendCount_);
}


void rubuilder::bu::RUproxy::updateMonitoringItems()
{
  {
    boost::mutex::scoped_lock sl(requestMonitoringMutex_);
    i2oRUSendCount_ = requestMonitoring_.logicalCount;
  }
  {
    boost::mutex::scoped_lock sl(blockMonitoringMutex_);
    lastEventNumberFromRUs_ = blockMonitoring_.lastEventNumberFromRUs;
    i2oBUCacheCount_ = blockMonitoring_.logicalCount;
  }
}


void rubuilder::bu::RUproxy::resetMonitoringCounters()
{
  {
    boost::mutex::scoped_lock sl(requestMonitoringMutex_);
    requestMonitoring_.payload = 0;
    requestMonitoring_.logicalCount = 0;
    requestMonitoring_.i2oCount = 0;
  }
  {
    boost::mutex::scoped_lock sl(blockMonitoringMutex_);
    blockMonitoring_.payload = 0;
    blockMonitoring_.logicalCount = 0;
    blockMonitoring_.i2oCount = 0;
    blockMonitoring_.lastEventNumberFromRUs = 0;
    blockMonitoring_.logicalCountPerRU.clear();
    blockMonitoring_.payloadPerRU.clear();
  }
}


void rubuilder::bu::RUproxy::configure(const int msgAgeLimitDtMSec)
{
  index_ = app_->getApplicationDescriptor()->getInstance();

  clear();

  blockFIFO_.resize(blockFIFOCapacity_);

  rqstForFragsBufSize_ = sizeof(msg::RqstForFragsMsg) -
    sizeof(msg::RqstForFrag) +
    I2O_RU_SEND_Packing_ * sizeof(msg::RqstForFrag);
  timerManager_.initTimer(timerId_, msgAgeLimitDtMSec);
}


void rubuilder::bu::RUproxy::clear()
{
  toolbox::mem::Reference* bufRef;
  while ( blockFIFO_.deq(bufRef) ) { bufRef->release(); }

  boost::mutex::scoped_lock sl(rqstForFragsMutex_);
  if (rqstForFragsBufRef_)
  {
    rqstForFragsBufRef_->release();
    rqstForFragsBufRef_ = 0;
  }
}


void rubuilder::bu::RUproxy::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>RUproxy</p>"                                        << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;
  {
    boost::mutex::scoped_lock sl(blockMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>last evt number from RUs</td>"                     << std::endl;
    *out << "<td>" << blockMonitoring_.lastEventNumberFromRUs << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  {
    boost::mutex::scoped_lock sl(requestMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">Requests for data</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (kB)</td>"                                 << std::endl;
    *out << "<td>" << requestMonitoring_.payload / 0x400 << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << requestMonitoring_.logicalCount << "</td>"    << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << requestMonitoring_.i2oCount << "</td>"        << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  {
    boost::mutex::scoped_lock sl(blockMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">Event data</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (MB)</td>"                                 << std::endl;
    *out << "<td>" << blockMonitoring_.payload / 0x100000 << "</td>"<< std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << blockMonitoring_.logicalCount << "</td>"      << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << blockMonitoring_.i2oCount << "</td>"          << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  blockFIFO_.printHtml(out, app_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  ruParams_.printHtml("Configuration", out);

  {
    boost::mutex::scoped_lock sl(blockMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">Statistics per RU</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\">"                                    << std::endl;
    *out << "<table style=\"border-collapse:collapse;padding:0px\">"<< std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>Instance</td>"                                     << std::endl;
    *out << "<td>Nb fragments</td>"                                 << std::endl;
    *out << "<td>Data payload (MB)</td>"                            << std::endl;
    *out << "</tr>"                                                 << std::endl;

    CountsPerRU::const_iterator it, itEnd;
    for (it=blockMonitoring_.logicalCountPerRU.begin(),
           itEnd = blockMonitoring_.logicalCountPerRU.end();
         it != itEnd; ++it)
    {
      *out << "<tr>"                                                << std::endl;
      *out << "<td>RU_" << it->first << "</td>"                     << std::endl;
      *out << "<td>" << it->second << "</td>"                       << std::endl;
      *out << "<td>" << blockMonitoring_.payloadPerRU[it->first] / 0x100000 << "</td>" << std::endl;
      *out << "</tr>"                                               << std::endl;
    }
    *out << "</table>"                                              << std::endl;

    *out << "</td>"                                                 << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  
  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
