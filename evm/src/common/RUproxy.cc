#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "rubuilder/evm/RUproxy.h"
#include "rubuilder/utils/Constants.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "xcept/tools.h"
#include "xdaq/ApplicationDescriptor.h"

#include <string.h>


rubuilder::evm::RUproxy::RUproxy
(
  xdaq::Application* app,
  toolbox::mem::Pool* fastCtrlMsgPool
) :
RUbroadcaster(app,fastCtrlMsgPool),
ruReadoutBufRef_(0),
timerId_(timerManager_.getTimer())
{
  resetMonitoringCounters();
}


void rubuilder::evm::RUproxy::appendConfigurationItems(utils::InfoSpaceItems& params)
{
  I2O_RU_READOUT_Packing_ = 8;
  msgAgeLimitDtMSec_      = utils::DEFAULT_MESSAGE_AGE_LIMIT_MSEC;

  ruParams_.clear();
  ruParams_.add("I2O_RU_READOUT_Packing", &I2O_RU_READOUT_Packing_);
  ruParams_.add("msgAgeLimitDtMSec", &msgAgeLimitDtMSec_);

  initRuInstances(ruParams_);

  params.add(ruParams_);

  configure();
}


void rubuilder::evm::RUproxy::appendMonitoringItems(utils::InfoSpaceItems& items)
{
  lastEventNumberToRUs_ = 0;
  i2oRUReadoutCount_ = 0;

  items.add("lastEventNumberToRUs", &lastEventNumberToRUs_);
  items.add("i2oRUReadoutCount", &i2oRUReadoutCount_);
}


void rubuilder::evm::RUproxy::updateMonitoringItems()
{
  boost::mutex::scoped_lock sl(ruMonitoringMutex_);

  lastEventNumberToRUs_ = ruMonitoring_.lastEventNumberToRUs;
  i2oRUReadoutCount_ = ruMonitoring_.msgCount;
}


void rubuilder::evm::RUproxy::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(ruMonitoringMutex_);

  ruMonitoring_.lastEventNumberToRUs = 0;
  ruMonitoring_.payload = 0;
  ruMonitoring_.msgCount = 0;
  ruMonitoring_.i2oCount = 0;
}

void rubuilder::evm::RUproxy::configure()
{
  ruReadoutBufSize_ = sizeof(msg::EvBidsMsg) -
    sizeof(utils::EvBid) +
    I2O_RU_READOUT_Packing_ * sizeof(utils::EvBid);
  
  timerManager_.initTimer(timerId_, msgAgeLimitDtMSec_);
}


void rubuilder::evm::RUproxy::clear()
{
  boost::mutex::scoped_lock sl(ruReadoutMutex_);

  if (ruReadoutBufRef_ != 0)
  {
    ruReadoutBufRef_->release();
    ruReadoutBufRef_ = 0;
  }  
}


void rubuilder::evm::RUproxy::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>RUproxy</p>"                                        << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;

  {
    boost::mutex::scoped_lock sl(ruMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>last evt number to RUs</td>"                       << std::endl;
    *out << "<td>" << ruMonitoring_.lastEventNumberToRUs << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">RU readout</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (kB)</td>"                                 << std::endl;
    *out << "<td>" << ruMonitoring_.payload / 0x400<< "</td>"       << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>msg count</td>"                                    << std::endl;
    *out << "<td>" << ruMonitoring_.msgCount << "</td>"             << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << ruMonitoring_.i2oCount << "</td>"             << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }

  ruParams_.printHtml("Configuration", out);

  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


void rubuilder::evm::RUproxy::addEvBid(const rubuilder::utils::EvBid& evbId)
{
  boost::mutex::scoped_lock sl(ruReadoutMutex_);
  
  // Pack the EvBid for the RUs into the EvBids message under construction
  uint32_t nbElementsPacked = packEvBidsMsg(evbId);
  
  // Send the EvBids message if it is full
  if ( nbElementsPacked == I2O_RU_READOUT_Packing_ )
  {
    sendEvBidsToAllRUs();
  }
}


bool rubuilder::evm::RUproxy::sendOldEvBids()
{
  boost::mutex::scoped_lock sl(ruReadoutMutex_);

  if ( 
    ruReadoutBufRef_ &&
    timerManager_.isFired(timerId_)
  )
  {
    sendEvBidsToAllRUs();
    return true;
  }
  else
  {
    return false;
  }
}


void rubuilder::evm::RUproxy::sendEvBidsToAllRUs()
{
  sendToAllRUs(ruReadoutBufRef_, ruReadoutBufSize_);

  updateCounters();

  ///////////////////////////////////////////////
  // Free the EvBid message under construction //
  // (its copies were sent not it)             //
  ///////////////////////////////////////////////
  
  ruReadoutBufRef_->release();
  ruReadoutBufRef_ = 0;
}


uint32_t rubuilder::evm::RUproxy::packEvBidsMsg(const rubuilder::utils::EvBid& element)
{
  // Create an EvBids message if one does not exist
  if ( ruReadoutBufRef_ == 0 ) createNewReadoutMsg();

  msg::EvBidsMsg* evbIdsMsg =
    (msg::EvBidsMsg*)ruReadoutBufRef_->getDataLocation();

  // Insert and update number of elements
  evbIdsMsg->elements[evbIdsMsg->nbElements] = element;
  ++(evbIdsMsg->nbElements);

  // Calculate the size of the message with its new element
  size_t msgSize = sizeof(msg::EvBidsMsg)
    - sizeof(utils::EvBid) +
    sizeof(utils::EvBid) * evbIdsMsg->nbElements;

  // Store the message size in both the I2O header and the reference
  evbIdsMsg->PvtMessageFrame.StdMessageFrame.MessageSize = msgSize >> 2;
  ruReadoutBufRef_->setDataSize(msgSize);

  // Return the number of elements in the message
  return evbIdsMsg->nbElements;
}
    

void rubuilder::evm::RUproxy::createNewReadoutMsg()
{
  ruReadoutBufRef_ = toolbox::mem::getMemoryPoolFactory()->
    getFrame(fastCtrlMsgPool_, ruReadoutBufSize_);
  msg::EvBidsMsg* evbIdsMsg =
    (msg::EvBidsMsg*)ruReadoutBufRef_->getDataLocation();
  I2O_MESSAGE_FRAME* stdMsg = (I2O_MESSAGE_FRAME*)evbIdsMsg;
  I2O_PRIVATE_MESSAGE_FRAME* pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
  
  ruReadoutBufRef_->setDataSize(ruReadoutBufSize_);
  
  stdMsg->MessageSize      = ruReadoutBufSize_ >> 2;
  stdMsg->InitiatorAddress = tid_;
  stdMsg->Function         = I2O_PRIVATE_MESSAGE;
  stdMsg->VersionOffset    = 0;
  stdMsg->MsgFlags         = 0;
  
  pvtMsg->XFunctionCode    = I2O_RU_READOUT;
  pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
  
  evbIdsMsg->nbElements = 0;
  
  // Start the aging timer
  timerManager_.restartTimer(timerId_);
}


void rubuilder::evm::RUproxy::updateCounters()
{
  boost::mutex::scoped_lock sl(ruMonitoringMutex_);

  msg::EvBidsMsg *evbIdsMsg =
    (msg::EvBidsMsg*)ruReadoutBufRef_->getDataLocation();
  ruMonitoring_.payload += evbIdsMsg->nbElements * sizeof(utils::EvBid);
  ruMonitoring_.msgCount += evbIdsMsg->nbElements;
  ruMonitoring_.i2oCount += participatingRUs_.size();
  ruMonitoring_.lastEventNumberToRUs =
    evbIdsMsg->elements[evbIdsMsg->nbElements - 1].eventNumber();
}



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
