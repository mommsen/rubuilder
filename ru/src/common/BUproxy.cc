#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "rubuilder/ru/BUproxy.h"
#include "rubuilder/utils/Exception.h"
#include "xcept/tools.h"

#include <string.h>


rubuilder::ru::BUproxy::BUproxy
(
  xdaq::Application* app,
  SuperFragmentTablePtr superFragmentTable
) :
app_(app),
logger_(app->getApplicationLogger()),
superFragmentTable_(superFragmentTable),
tid_(0),
instance_(0)
{
  resetMonitoringCounters();
}


void rubuilder::ru::BUproxy::I2Ocallback(toolbox::mem::Reference* bufRef)
{
  I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
  msg::RqstForFragsMsg* msg =
    (msg::RqstForFragsMsg*)stdMsg;
  
  updateRequestCounters(msg);
  handleRequest(msg);
  
  bufRef->release();
}


void rubuilder::ru::BUproxy::updateRequestCounters(const rubuilder::msg::RqstForFragsMsg* msg)
{
  boost::mutex::scoped_lock sl(requestMonitoringMutex_);
  
  const uint32_t nbElements = msg->nbElements;
  requestMonitoring_.payload += nbElements * sizeof(msg::RqstForFrag);
  requestMonitoring_.logicalCount += nbElements;
  ++requestMonitoring_.i2oCount;
  requestMonitoring_.logicalCountPerBU[msg->srcIndex] += nbElements;
}


void rubuilder::ru::BUproxy::handleRequest(const rubuilder::msg::RqstForFragsMsg* msg)
{
  SuperFragmentTable::Request request;
  request.buTid = ((I2O_MESSAGE_FRAME*)msg)->InitiatorAddress;
  request.buIndex = msg->srcIndex;

  for (uint32_t i=0; i<msg->nbElements; ++i)
  {
    // these are different for each request
    request.evbId = msg->elements[i].evbId;
    request.buResourceId = msg->elements[i].buResourceId;
    
    superFragmentTable_->addRequest(request);
  }
}


void rubuilder::ru::BUproxy::sendData
(
  const SuperFragmentTable::Request& request,
  toolbox::mem::Reference* head
)
{
  toolbox::mem::Reference* bufRef = head;
  uint32_t payload = 0;
  uint32_t i2oCount = 0;

  // Prepare each event data block for the BU
  while (bufRef)
  {
    I2O_MESSAGE_FRAME* stdMsg = (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
    I2O_PRIVATE_MESSAGE_FRAME* pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
    I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block = (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;

    stdMsg->InitiatorAddress = tid_;
    stdMsg->TargetAddress    = request.buTid;
    pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
    pvtMsg->XFunctionCode    = I2O_BU_CACHE;
    block->superFragmentNb   = instance_ + 1;
    block->buResourceId      = request.buResourceId;

    payload += (stdMsg->MessageSize << 2) -
      sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
    ++i2oCount;

    bufRef = bufRef->getNextReference();
  }

  {
     boost::mutex::scoped_lock sl(dataMonitoringMutex_);

     dataMonitoring_.lastEventNumberToBUs = request.evbId.eventNumber();
     dataMonitoring_.i2oCount += i2oCount;
     dataMonitoring_.payload += payload;
     ++dataMonitoring_.logicalCount;
     dataMonitoring_.payloadPerBU[request.buIndex] += payload;
  }
  
  xdaq::ApplicationDescriptor *bu = 0;
  try
  {
    bu = i2o::utils::getAddressMap()->getApplicationDescriptor(request.buTid);
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    
    oss << "Failed to get application descriptor for BU with tid ";
    oss << request.buTid;
    
    XCEPT_RAISE(exception::Configuration, oss.str());
  }
  
  try
  {
    app_->getApplicationContext()->
      postFrame(
        head,
        app_->getApplicationDescriptor(),
        bu//,
        //i2oExceptionHandler_,
        //bu
      );
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    
    oss << "Failed to send super fragment to BU";
    oss << bu->getInstance();
    
    XCEPT_RETHROW(exception::I2O, oss.str(), e);
  }
}


void rubuilder::ru::BUproxy::configure()
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

  try
  {
    instance_ = app_->getApplicationDescriptor()->getInstance();
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      "Failed to get instance for this application.", e);
  }

  getBuInstances();
}


void rubuilder::ru::BUproxy::getBuInstances()
{
  boost::mutex::scoped_lock sl(buInstancesMutex_);

  std::set<xdaq::ApplicationDescriptor*> buDescriptors;

  try
  {
    buDescriptors =
      app_->getApplicationContext()->
      getDefaultZone()->
      getApplicationDescriptors("rubuilder::bu::Application");
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      "Failed to get BU application descriptors", e);
  }

  if ( buDescriptors.empty() )
  {
    XCEPT_RAISE(exception::Configuration,
      "There must be at least one BU descriptor");
  }

  buInstances_.clear();
  
  for (std::set<xdaq::ApplicationDescriptor*>::const_iterator
         it=buDescriptors.begin(), itEnd =buDescriptors.end();
       it != itEnd; ++it)
  {
    buInstances_.insert((*it)->getInstance());
  }
}


void rubuilder::ru::BUproxy::appendConfigurationItems(utils::InfoSpaceItems& params)
{
}


void rubuilder::ru::BUproxy::appendMonitoringItems(utils::InfoSpaceItems& items)
{
  lastEventNumberToBUs_ = 0;
  nbSuperFragmentsReady_ = 0;
  i2oBUCacheCount_ = 0;
  i2oRUSendCountBU_.clear();
  i2oBUCachePayloadBU_.clear();

  items.add("lastEventNumberToBUs", &lastEventNumberToBUs_);
  items.add("nbSuperFragmentsReady", &nbSuperFragmentsReady_);
  items.add("i2oBUCacheCount", &i2oBUCacheCount_);
  items.add("i2oRUSendCountBU", &i2oRUSendCountBU_);
  items.add("i2oBUCachePayloadBU", &i2oBUCachePayloadBU_);
}


void rubuilder::ru::BUproxy::updateMonitoringItems()
{
  nbSuperFragmentsReady_ = superFragmentTable_->getNbSuperFragmentsReady();

  {
    boost::mutex::scoped_lock sl(requestMonitoringMutex_);
    
    i2oRUSendCountBU_.clear();
    i2oRUSendCountBU_.reserve(requestMonitoring_.logicalCountPerBU.size());
    CountsPerBU::const_iterator it, itEnd;
    for (it = requestMonitoring_.logicalCountPerBU.begin(),
           itEnd = requestMonitoring_.logicalCountPerBU.end();
         it != itEnd; ++it)
    {
      i2oRUSendCountBU_.push_back(it->second);
    }
  }
  {
    boost::mutex::scoped_lock sl(dataMonitoringMutex_);
    
    lastEventNumberToBUs_ = dataMonitoring_.lastEventNumberToBUs;
    i2oBUCacheCount_ = dataMonitoring_.logicalCount;
    
    i2oBUCachePayloadBU_.clear();
    i2oBUCachePayloadBU_.reserve(dataMonitoring_.payloadPerBU.size());
    CountsPerBU::const_iterator it, itEnd;
    for (it = dataMonitoring_.payloadPerBU.begin(),
           itEnd = dataMonitoring_.payloadPerBU.end();
         it != itEnd; ++it)
    {
      i2oBUCachePayloadBU_.push_back(it->second);
    }
  }
}


void rubuilder::ru::BUproxy::resetMonitoringCounters()
{
  boost::mutex::scoped_lock rsl(requestMonitoringMutex_);
  boost::mutex::scoped_lock dsl(dataMonitoringMutex_);

  requestMonitoring_.payload = 0;
  requestMonitoring_.logicalCount = 0;
  requestMonitoring_.i2oCount = 0;
  requestMonitoring_.logicalCountPerBU.clear();
  
  dataMonitoring_.lastEventNumberToBUs = 0;
  dataMonitoring_.payload = 0;
  dataMonitoring_.logicalCount = 0;
  dataMonitoring_.i2oCount = 0;
  dataMonitoring_.payloadPerBU.clear();

  boost::mutex::scoped_lock sl(buInstancesMutex_);
    
  BUInstances::const_iterator it, itEnd;
  for (it=buInstances_.begin(), itEnd = buInstances_.end();
       it != itEnd; ++it)
  {
    const uint32_t buInstance = *it;

    requestMonitoring_.logicalCountPerBU[buInstance] = 0;
    dataMonitoring_.payloadPerBU[buInstance] = 0;
  }
}


void rubuilder::ru::BUproxy::clear()
{
}


void rubuilder::ru::BUproxy::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>BUproxy</p>"                                        << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;

  boost::mutex::scoped_lock rsl(requestMonitoringMutex_);
  boost::mutex::scoped_lock dsl(dataMonitoringMutex_);

  *out << "<tr>"                                                  << std::endl;
  *out << "<td>last evt number to BUs</td>"                       << std::endl;
  *out << "<td>" << dataMonitoring_.lastEventNumberToBUs << "</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td># ready fragments</td>"                            << std::endl;
  *out << "<td>" << superFragmentTable_->getNbSuperFragmentsReady() << "</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\" style=\"text-align:center\">BU requests</td>" << std::endl;
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
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\" style=\"text-align:center\">BU events cache</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>payload (MB)</td>"                                 << std::endl;
  *out << "<td>" << dataMonitoring_.payload / 0x100000 << "</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>logical count</td>"                                << std::endl;
  *out << "<td>" << dataMonitoring_.logicalCount << "</td>"       << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>I2O count</td>"                                    << std::endl;
  *out << "<td>" << dataMonitoring_.i2oCount << "</td>"           << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\" style=\"text-align:center\">Statistics per BU</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  *out << "<table style=\"border-collapse:collapse;padding:0px\">"<< std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>Instance</td>"                                     << std::endl;
  *out << "<td>Nb requests</td>"                                  << std::endl;
  *out << "<td>Data payload (MB)</td>"                            << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  boost::mutex::scoped_lock sl(buInstancesMutex_);
    
  BUInstances::const_iterator it, itEnd;
  for (it=buInstances_.begin(), itEnd = buInstances_.end();
       it != itEnd; ++it)
  {
    const uint32_t buInstance = *it;

    *out << "<tr>"                                                << std::endl;
    *out << "<td>BU_" << buInstance << "</td>"                    << std::endl;
    *out << "<td>" << requestMonitoring_.logicalCountPerBU[buInstance] << "</td>" << std::endl;
    *out << "<td>" << dataMonitoring_.payloadPerBU[buInstance] / 0x100000 << "</td>" << std::endl;
    *out << "</tr>"                                               << std::endl;
  }
  *out << "</table>"                                              << std::endl;
  
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
