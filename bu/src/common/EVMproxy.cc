#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "rubuilder/bu/EVMproxy.h"
#include "rubuilder/utils/Constants.h"
#include "rubuilder/utils/DumpUtility.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "xcept/tools.h"
#include "xdaq/ApplicationDescriptor.h"

#include <string.h>


rubuilder::bu::EVMproxy::EVMproxy
(
  xdaq::Application* app,
  toolbox::mem::Pool* fastCtrlMsgPool
) :
app_(app),
fastCtrlMsgPool_(fastCtrlMsgPool),
logger_(app->getApplicationLogger()),
tid_(0),
index_(0),
triggerFIFO_("triggerFIFO"),
evtIdRqstsAndOrReleasesBufRef_(0),
timerId_(timerManager_.getTimer())
{
  resetMonitoringCounters();
}


void rubuilder::bu::EVMproxy::I2Ocallback(toolbox::mem::Reference* bufRef)
{
  updateTriggerCounters(bufRef);
  dumpTriggersToLogger(bufRef);

  while ( ! triggerFIFO_.enq(bufRef) ) ::usleep(1000);
}


bool rubuilder::bu::EVMproxy::getTriggerBlock(toolbox::mem::Reference*& bufRef)
{
  return ( triggerFIFO_.deq(bufRef) );
}


void rubuilder::bu::EVMproxy::updateTriggerCounters(toolbox::mem::Reference* bufRef)
{
  boost::mutex::scoped_lock sl(triggerMonitoringMutex_);

  const I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
  const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;
  const size_t payload =
    (stdMsg->MessageSize << 2) - sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);

  triggerMonitoring_.lastEventNumberFromEVM = block->eventNumber;
  triggerMonitoring_.payload += payload;
  ++triggerMonitoring_.i2oCount;
  if ( block->blockNb == (block->nbBlocksInSuperFragment-1) )
  {
    ++triggerMonitoring_.logicalCount;
  }
}


void rubuilder::bu::EVMproxy::dumpTriggersToLogger(toolbox::mem::Reference* bufRef)
{
  if (dumpTriggersToLogger_)
  {
    std::ostringstream oss;
    utils::DumpUtility::dump(oss, bufRef);
    LOG4CPLUS_INFO(logger_, oss.str());
  }
}


void rubuilder::bu::EVMproxy::sendEvtIdRqstAndOrRelease
(
  const msg::EvtIdRqstAndOrRelease& evtIdRqstAndOrRelease
)
{
  boost::mutex::scoped_lock sl(evtIdRqstsAndOrReleasesMutex_);

  // If there is no message under construction, then create one
  if (evtIdRqstsAndOrReleasesBufRef_ == 0)
    createEvtIdRqstsAndOrReleaseMsg();

  const uint32_t nbElementsPacked =
    packEvtIdRqstsAndOrReleasesMsg(evtIdRqstAndOrRelease);

  if (nbElementsPacked == I2O_EVM_ALLOCATE_CLEAR_Packing_)
  {
    sendEvtIdRqstAndOrReleaseToEVM();
  }
}


void rubuilder::bu::EVMproxy::createEvtIdRqstsAndOrReleaseMsg()
{
  evtIdRqstsAndOrReleasesBufRef_ =
    toolbox::mem::getMemoryPoolFactory()->
    getFrame(fastCtrlMsgPool_, evtIdRqstsAndOrReleasesBufSize_);

  I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)evtIdRqstsAndOrReleasesBufRef_->getDataLocation();
  stdMsg->InitiatorAddress = tid_;
  stdMsg->TargetAddress    = evm_.tid;
  stdMsg->Function         = I2O_PRIVATE_MESSAGE;
  stdMsg->VersionOffset    = 0;
  stdMsg->MsgFlags         = 0;

  I2O_PRIVATE_MESSAGE_FRAME* pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
  pvtMsg->XFunctionCode    = I2O_EVM_ALLOCATE_CLEAR;
  pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;

  msg::EvtIdRqstsAndOrReleasesMsg* evtIdRqstsAndOrReleasesMsg =
    (msg::EvtIdRqstsAndOrReleasesMsg*)stdMsg;
  evtIdRqstsAndOrReleasesMsg->srcIndex   = index_;
  evtIdRqstsAndOrReleasesMsg->nbElements = 0;
  
  // Start the aging timer
  timerManager_.restartTimer(timerId_);
}


uint32_t rubuilder::bu::EVMproxy::packEvtIdRqstsAndOrReleasesMsg
(
  const msg::EvtIdRqstAndOrRelease& evtIdRqstAndOrRelease
)
{
  msg::EvtIdRqstsAndOrReleasesMsg *msg =
    (msg::EvtIdRqstsAndOrReleasesMsg*)evtIdRqstsAndOrReleasesBufRef_->getDataLocation();
  
  // Insert and update number of elements
  msg->elements[msg->nbElements] = evtIdRqstAndOrRelease;
  msg->nbElements++;

  // Calculate the size of the message with its new element
  const size_t msgSize = sizeof(msg::EvtIdRqstsAndOrReleasesMsg) -
    sizeof(msg::EvtIdRqstAndOrRelease) +
    sizeof(msg::EvtIdRqstAndOrRelease) * msg->nbElements;
  
  // Store the message size in both the I2O header and the reference
  msg->PvtMessageFrame.StdMessageFrame.MessageSize = msgSize >> 2;
  evtIdRqstsAndOrReleasesBufRef_->setDataSize(msgSize);
  
  // Return the number of elements in the message
  return msg->nbElements;
}


void rubuilder::bu::EVMproxy::sendEvtIdRqstAndOrReleaseToEVM()
{
  updateRqstAndOrReleaseCounters();
  
  try
  {
    app_->getApplicationContext()->
      postFrame
      (
        evtIdRqstsAndOrReleasesBufRef_,
        app_->getApplicationDescriptor(),
        evm_.descriptor //,
        //i2oExceptionHandler_,
        //evm_.descriptor
      );
    evtIdRqstsAndOrReleasesBufRef_ = 0;
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::I2O,
      "Failed to send I2O_EVM_ALLOCATE_CLEAR", e);
  }
}


void rubuilder::bu::EVMproxy::updateRqstAndOrReleaseCounters()
{
  boost::mutex::scoped_lock sl(rqstAndOrReleaseMonitoringMutex_);
  
  msg::EvtIdRqstsAndOrReleasesMsg* msg =
    (msg::EvtIdRqstsAndOrReleasesMsg*)evtIdRqstsAndOrReleasesBufRef_->getDataLocation();
  const uint32_t nbElements = msg->nbElements;

  rqstAndOrReleaseMonitoring_.payload +=
    nbElements * sizeof(msg::EvtIdRqstAndOrRelease);
  rqstAndOrReleaseMonitoring_.logicalCount += nbElements;
  ++rqstAndOrReleaseMonitoring_.i2oCount;
}


bool rubuilder::bu::EVMproxy::sendOldRequests()
{
  boost::mutex::scoped_lock sl(evtIdRqstsAndOrReleasesMutex_);

  if ( 
    evtIdRqstsAndOrReleasesBufRef_ &&
    timerManager_.isFired(timerId_)
  )
  {
    sendEvtIdRqstAndOrReleaseToEVM();
    return true;
  }
  else
  {
    return false;
  }
}


void rubuilder::bu::EVMproxy::appendConfigurationItems(utils::InfoSpaceItems& params)
{
  nbEvtIdsInBuilder_ = utils::DEFAULT_NB_EVENTS;
  I2O_EVM_ALLOCATE_CLEAR_Packing_ = 8;
  evmInstance_ = -1 ; // Explicitly indicate parameter not set
  dumpTriggersToLogger_ = false;
  
  evmParams_.clear();
  evmParams_.add("nbEvtIdsInBuilder", &nbEvtIdsInBuilder_);
  evmParams_.add("I2O_EVM_ALLOCATE_CLEAR_Packing", &I2O_EVM_ALLOCATE_CLEAR_Packing_);
  evmParams_.add("evmInstance", &evmInstance_);
  evmParams_.add("dumpTriggersToLogger", &dumpTriggersToLogger_);

  params.add(evmParams_);
}


void rubuilder::bu::EVMproxy::appendMonitoringItems(utils::InfoSpaceItems& items)
{
  lastEventNumberFromEVM_ = 0;
  i2oBUConfirmCount_ = 0;
  i2oEVMAllocCount_ = 0;

  items.add("lastEventNumberFromEVM", &lastEventNumberFromEVM_);
  items.add("i2oBUConfirmCount", &i2oBUConfirmCount_);
  items.add("i2oEVMAllocCount", &i2oEVMAllocCount_);
}


void rubuilder::bu::EVMproxy::updateMonitoringItems()
{
  {
    boost::mutex::scoped_lock sl(triggerMonitoringMutex_);
    lastEventNumberFromEVM_ = triggerMonitoring_.lastEventNumberFromEVM;
    i2oBUConfirmCount_ = triggerMonitoring_.logicalCount;
  }
  {
    boost::mutex::scoped_lock sl(rqstAndOrReleaseMonitoringMutex_);
    i2oEVMAllocCount_ = rqstAndOrReleaseMonitoring_.logicalCount;
  }
}


void rubuilder::bu::EVMproxy::resetMonitoringCounters()
{
  {
    boost::mutex::scoped_lock sl(triggerMonitoringMutex_);
    triggerMonitoring_.payload = 0;
    triggerMonitoring_.logicalCount = 0;
    triggerMonitoring_.i2oCount = 0;
    triggerMonitoring_.lastEventNumberFromEVM = 0;
  }
  {
    boost::mutex::scoped_lock sl(rqstAndOrReleaseMonitoringMutex_);
    rqstAndOrReleaseMonitoring_.payload = 0;
    rqstAndOrReleaseMonitoring_.logicalCount = 0;
    rqstAndOrReleaseMonitoring_.i2oCount = 0;
  }
}



void rubuilder::bu::EVMproxy::configure(const int msgAgeLimitDtMSec)
{
  index_ = app_->getApplicationDescriptor()->getInstance();

  clear();

  triggerFIFO_.resize(nbEvtIdsInBuilder_);
  
  evtIdRqstsAndOrReleasesBufSize_ = sizeof(msg::EvtIdRqstsAndOrReleasesMsg) -
    sizeof(msg::EvtIdRqstAndOrRelease) +
    I2O_EVM_ALLOCATE_CLEAR_Packing_ * sizeof(msg::EvtIdRqstAndOrRelease);
  timerManager_.initTimer(timerId_, msgAgeLimitDtMSec);
}


void rubuilder::bu::EVMproxy::getApplicationDescriptors()
{
  try
  {
    tid_ = i2o::utils::getAddressMap()->
      getTid(app_->getApplicationDescriptor());
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      "Failed to get the I2O TID of this application", e);
  }
  
  // If the instance number of the EVM has not been given
  if (evmInstance_.value_ < 0)
  {
    // Try to find instance number by assuming the first EVM found is the
    // one to be used.
    
    std::set< xdaq::ApplicationDescriptor* > evms;
    
    try
    {
      evms =
        app_->getApplicationContext()->
        getDefaultZone()->
        getApplicationDescriptors("rubuilder::evm::Application");
    }
    catch(xcept::Exception &e)
    {
      XCEPT_RETHROW(exception::Configuration,
        "Failed to get EVM application descriptor", e);
    }
    
    if ( evms.empty() )
    {
      XCEPT_RAISE(exception::Configuration,
        "Failed to get EVM application descriptor");
    }
    
    evm_.descriptor = *(evms.begin());
  }
  else
  {
    try
    {
      evm_.descriptor =
        app_->getApplicationContext()->
        getDefaultZone()->
        getApplicationDescriptor("rubuilder::evm::Application",
          evmInstance_.value_);
    }
    catch(xcept::Exception &e)
    {
      std::ostringstream oss;
      
      oss << "Failed to get application descriptor of EVM";
      oss << evmInstance_.toString();
      
      XCEPT_RETHROW(exception::Configuration, oss.str(), e);
    }
  }
  
  try
  {
    evm_.tid = i2o::utils::getAddressMap()->getTid(evm_.descriptor);
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      "Failed to get the I2O TID of the EVM", e);
  }
}


void rubuilder::bu::EVMproxy::clear()
{
  toolbox::mem::Reference* bufRef;
  while ( triggerFIFO_.deq(bufRef) ) { bufRef->release(); }

  boost::mutex::scoped_lock sl(evtIdRqstsAndOrReleasesMutex_);
  if (evtIdRqstsAndOrReleasesBufRef_)
  {
    evtIdRqstsAndOrReleasesBufRef_->release();
    evtIdRqstsAndOrReleasesBufRef_ = 0;
  }
}


void rubuilder::bu::EVMproxy::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>EVMproxy</p>"                                       << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;

  {
    boost::mutex::scoped_lock sl(triggerMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>last evt number from EVM</td>"                     << std::endl;
    *out << "<td>" << triggerMonitoring_.lastEventNumberFromEVM << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>EVM instance</td>"                                 << std::endl;
    *out << "<td>";
    if ( evm_.descriptor )
      *out << evm_.descriptor->getInstance();
    else
      *out << "n/a";
    *out << "</td>"                                                 << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">Trigger data</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (MB)</td>"                                 << std::endl;
    *out << "<td>" << triggerMonitoring_.payload / 0x100000 << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << triggerMonitoring_.logicalCount << "</td>"    << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << triggerMonitoring_.i2oCount << "</td>"        << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  {
    boost::mutex::scoped_lock sl(rqstAndOrReleaseMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">Allocate/clear messages</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (kB)</td>"                                 << std::endl;
    *out << "<td>" << rqstAndOrReleaseMonitoring_.payload / 0x400 << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << rqstAndOrReleaseMonitoring_.logicalCount << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << rqstAndOrReleaseMonitoring_.i2oCount << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  triggerFIFO_.printHtml(out, app_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  evmParams_.printHtml("Configuration", out);
  
  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
