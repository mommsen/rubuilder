#include "rubuilder/ru/EVMproxy.h"
#include "rubuilder/utils/Constants.h"


rubuilder::ru::EVMproxy::EVMproxy
(
  xdaq::Application* app
) :
app_(app),
logger_(app->getApplicationLogger()),
evbIdFIFO_("evbIdFIFO")
{
  resetMonitoringCounters();
}


void rubuilder::ru::EVMproxy::I2Ocallback(toolbox::mem::Reference* bufRef)
{
  I2O_MESSAGE_FRAME* stdMsg =
    (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
  msg::EvBidsMsg* msg =
    (msg::EvBidsMsg*)stdMsg;
  
  updateReadoutCounters(msg);
  handleReadoutMsg(msg);
  
  bufRef->release();
}


bool rubuilder::ru::EVMproxy::getTrigEvBid(rubuilder::utils::EvBid& evbId)
{
  return ( evbIdFIFO_.deq(evbId) );
}


void rubuilder::ru::EVMproxy::updateReadoutCounters(const rubuilder::msg::EvBidsMsg* msg)
{
  boost::mutex::scoped_lock sl(evmMonitoringMutex_);

  const uint32_t nbElements = msg->nbElements;
  evmMonitoring_.lastEventNumberFromEVM = msg->elements[nbElements-1].eventNumber();
  evmMonitoring_.payload += nbElements * sizeof(utils::EvBid);
  evmMonitoring_.logicalCount += nbElements;
  ++evmMonitoring_.i2oCount;
}


void rubuilder::ru::EVMproxy::handleReadoutMsg(const rubuilder::msg::EvBidsMsg* msg)
{
  for (uint32_t i=0; i<msg->nbElements; ++i)
  {
    while ( ! evbIdFIFO_.enq(msg->elements[i]) ) ::usleep(1000);
  }
}


void rubuilder::ru::EVMproxy::appendConfigurationItems(utils::InfoSpaceItems& params)
{
  evbIdFIFOCapacity_ = utils::DEFAULT_NB_EVENTS;
  
  evmParams_.clear();
  evmParams_.add("evbIdFIFOCapacity", &evbIdFIFOCapacity_);

  params.add(evmParams_);
}


void rubuilder::ru::EVMproxy::appendMonitoringItems(utils::InfoSpaceItems& items)
{
  lastEventNumberFromEVM_ = 0;
  i2oRUReadoutCount_ = 0;

  items.add("lastEventNumberFromEVM", &lastEventNumberFromEVM_);
  items.add("i2oRUReadoutCount", &i2oRUReadoutCount_);
}


void rubuilder::ru::EVMproxy::updateMonitoringItems()
{
  boost::mutex::scoped_lock sl(evmMonitoringMutex_);

  lastEventNumberFromEVM_ = evmMonitoring_.lastEventNumberFromEVM;
  i2oRUReadoutCount_ = evmMonitoring_.logicalCount;
}


void rubuilder::ru::EVMproxy::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(evmMonitoringMutex_);

  evmMonitoring_.payload = 0;
  evmMonitoring_.logicalCount = 0;
  evmMonitoring_.i2oCount = 0;
  evmMonitoring_.lastEventNumberFromEVM = 0;
}


void rubuilder::ru::EVMproxy::configure()
{
  clear();

  evbIdFIFO_.resize(evbIdFIFOCapacity_);
}


void rubuilder::ru::EVMproxy::clear()
{
  utils::EvBid evbId;
  while ( evbIdFIFO_.deq(evbId) ) {};
}


void rubuilder::ru::EVMproxy::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>EVMproxy</p>"                                        << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;

  {
    boost::mutex::scoped_lock sl(evmMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>last evt number from EVM</td>"                     << std::endl;
    *out << "<td>" << evmMonitoring_.lastEventNumberFromEVM << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">EVM input</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (kB)</td>"                                 << std::endl;
    *out << "<td>" << evmMonitoring_.payload / 0x400 << "</td>"     << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << evmMonitoring_.logicalCount << "</td>"        << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << evmMonitoring_.i2oCount << "</td>"            << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  evbIdFIFO_.printHtml(out, app_->getApplicationDescriptor()->getURN());
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
