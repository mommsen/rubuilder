#include "interface/evb/i2oEVBMsgs.h"
#include "rubuilder/evm/TRGproxy.h"
#include "rubuilder/evm/TRGproxyHandlers.h"
#include "rubuilder/utils/Constants.h"
#include "rubuilder/utils/DumpUtility.h"
#include "rubuilder/utils/Exception.h"

#include <sstream>


rubuilder::evm::TRGproxy::TRGproxy
(
  xdaq::Application* app,
  toolbox::mem::Pool* fastCtrlMsgPool
) :
app_(app),
fastCtrlMsgPool_(fastCtrlMsgPool),
logger_(app->getApplicationLogger()),
triggerFIFO_("triggerFIFO"),
handler_(new TRGproxyHandlers::GTPhandler(triggerFIFO_))
{
  resetMonitoringCounters();
}


void rubuilder::evm::TRGproxy::I2Ocallback(toolbox::mem::Reference* bufRef)
{
  if ( generateDummyTriggers_.value_ )
    XCEPT_RAISE(exception::Configuration,
      "Received trigger data while generating dummy triggers");

  pushOntoTriggerFIFO(bufRef);
}


void rubuilder::evm::TRGproxy::dumpTriggerToLogger(toolbox::mem::Reference* bufRef) const
{
  if ( ! dumpTriggersToLogger_.value_ ) return;

  std::stringstream oss;
  utils::DumpUtility::dump(oss, bufRef);
  LOG4CPLUS_INFO(logger_, oss.str());
}


void rubuilder::evm::TRGproxy::updateTriggerCounters(toolbox::mem::Reference* bufRef)
{
  boost::mutex::scoped_lock sl(triggerMonitoringMutex_);

  I2O_MESSAGE_FRAME *stdMsg =
    (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
  I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;
  
  size_t payloadSize = (stdMsg->MessageSize << 2) -
    sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);

  triggerMonitoring_.payload += payloadSize;
  triggerMonitoring_.payloadSquared +=
    payloadSize * payloadSize;
  ++triggerMonitoring_.nbTriggers;
  ++triggerMonitoring_.i2oCount;
  triggerMonitoring_.lastEventNumberFromTrigger =
    block->eventNumber;
}
 

void rubuilder::evm::TRGproxy::pushOntoTriggerFIFO(toolbox::mem::Reference* bufRef)
{
  if ( dropInputData_ )
  {
    bufRef->release();
  }
  else
  {
    while ( ! triggerFIFO_.enq(bufRef) ) ::usleep(1000);
  }
}


bool rubuilder::evm::TRGproxy::getNextTrigger(toolbox::mem::Reference*& bufRef)
{
  if ( handler_->getNextTrigger(bufRef) )
  {
    dumpTriggerToLogger(bufRef);
    updateTriggerCounters(bufRef);
    return true;
  }
  return false;
}


void rubuilder::evm::TRGproxy::sendOldTriggerMessage()
{
  handler_->sendOldTriggerMessage();
}


void rubuilder::evm::TRGproxy::stopRequestingTriggers()
{
  handler_->stopRequestingTriggers();
}


void rubuilder::evm::TRGproxy::appendConfigurationItems(utils::InfoSpaceItems& params)
{
  triggerSource_         = "GTP";
  triggerFIFOCapacity_   = utils::DEFAULT_NB_EVENTS;
  
  taClass_               = "rubuilder::ta::Application";
  taInstance_            = 0;
  I2O_TA_CREDIT_Packing_ = 8;

  dumpTriggersToLogger_  = false;
  dropInputData_         = false;
  generateDummyTriggers_ = false;
  dummyBlockSize_        = 4096;
  dummyFedPayloadSize_   = 2048;
  dummyFedPayloadStdDev_ = 0;
  dummyTriggerSourceId_  = utils::GTP_FED_ID;
  orbitsPerLS_           = 0x00040000; // 2^18 orbits
  usePlayback_           = false;
  playbackDataFile_      = "";  

  trgParams_.clear();
  trgParams_.add("triggerSource", &triggerSource_, utils::InfoSpaceItems::change);
  trgParams_.add("triggerFIFOCapacity", &triggerFIFOCapacity_);

  trgParams_.add("taClass", &taClass_);
  trgParams_.add("taInstance", &taInstance_);
  trgParams_.add("I2O_TA_CREDIT_Packing", &I2O_TA_CREDIT_Packing_);

  trgParams_.add("dumpTriggersToLogger", &dumpTriggersToLogger_);
  trgParams_.add("dropInputData", &dropInputData_);
  trgParams_.add("generateDummyTriggers", &generateDummyTriggers_);
  trgParams_.add("dummyBlockSize", &dummyBlockSize_);
  trgParams_.add("dummyFedPayloadSize", &dummyFedPayloadSize_);
  trgParams_.add("dummyFedPayloadStdDev", &dummyFedPayloadStdDev_);
  trgParams_.add("fedPayloadSize", &dummyFedPayloadSize_); // old naming, keep for backward compatibility
  trgParams_.add("dummyTriggerSourceId", &dummyTriggerSourceId_);
  trgParams_.add("orbitsPerLS", &orbitsPerLS_);
  trgParams_.add("usePlayback", &usePlayback_);
  trgParams_.add("playbackDataFile", &playbackDataFile_);

  params.add(trgParams_);

  configure();
}


void rubuilder::evm::TRGproxy::triggerSourceChanged()
{
  const std::string triggerSource = triggerSource_.toString();

  LOG4CPLUS_INFO(logger_, "Setting trigger input to " + triggerSource);

  if ( triggerSource == "GTP" )
  {
    handler_.reset( new TRGproxyHandlers::GTPhandler(triggerFIFO_) );
  }
  else if ( triggerSource == "GTPe" )
  {
    handler_.reset( new TRGproxyHandlers::GTPehandler(triggerFIFO_, app_->getApplicationDescriptor()->getURN()) );
  }
  else if ( triggerSource == "FEROL" )
  {
    handler_.reset( new TRGproxyHandlers::FEROLhandler(triggerFIFO_, app_->getApplicationDescriptor()->getURN()) );
  }
  else if ( triggerSource == "TA" )
  {
    handler_.reset( new TRGproxyHandlers::TAhandler(app_,fastCtrlMsgPool_,triggerFIFO_) );
  }
  else if ( triggerSource == "Local" )
  {
    handler_.reset( new TRGproxyHandlers::DummyTrigger(app_->getApplicationDescriptor()->getURN()) );
  }
  else
  {
    XCEPT_RAISE(exception::Configuration,
      "Unknown trigger input source " + triggerSource + " requested.");
  }

  if ( triggerSource != "Local" && generateDummyTriggers_ )
  {
    XCEPT_RAISE(exception::Configuration,
      "Requested dummy triggers but input source is '" + 
      triggerSource + "' instead of 'Local'");
  }

  configure();
  resetMonitoringCounters();
}


void rubuilder::evm::TRGproxy::appendMonitoringItems(utils::InfoSpaceItems& items)
{
  lastEventNumberFromTrigger_ = 0;
  i2oEVMTriggerCount_ = 0;

  items.add("lastEventNumberFromTrigger", &lastEventNumberFromTrigger_);
  items.add("i2oEVMTriggerCount", &i2oEVMTriggerCount_);
}


void rubuilder::evm::TRGproxy::updateMonitoringItems()
{
  boost::mutex::scoped_lock sl(triggerMonitoringMutex_);

  lastEventNumberFromTrigger_ = triggerMonitoring_.lastEventNumberFromTrigger;
  i2oEVMTriggerCount_ = triggerMonitoring_.nbTriggers;
}


void rubuilder::evm::TRGproxy::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(triggerMonitoringMutex_);

  triggerMonitoring_.nbTriggers = 0;
  triggerMonitoring_.lastEventNumberFromTrigger = 0;
  triggerMonitoring_.payload = 0;
  triggerMonitoring_.payloadSquared = 0;
  triggerMonitoring_.i2oCount = 0;

  handler_->reset();
}


void rubuilder::evm::TRGproxy::getTriggerPerformance(utils::PerformanceMonitor& performanceMonitor)
{
  boost::mutex::scoped_lock sl(triggerMonitoringMutex_);

  performanceMonitor.N = triggerMonitoring_.nbTriggers;
  performanceMonitor.sumOfSizes = triggerMonitoring_.payload;
  performanceMonitor.sumOfSquares = triggerMonitoring_.payloadSquared;
}


void rubuilder::evm::TRGproxy::configure()
{
  clear();
  triggerFIFO_.resize(triggerFIFOCapacity_);
  
  TRGproxyHandlers::Configuration conf;
  conf.dropInputData = dropInputData_.value_;
  conf.dummyBlockSize = dummyBlockSize_;
  conf.dummyFedPayloadSize = dummyFedPayloadSize_.value_;
  conf.dummyFedPayloadStdDev = dummyFedPayloadStdDev_.value_;
  conf.fedSourceIds.push_back(dummyTriggerSourceId_);
  conf.orbitsPerLS = orbitsPerLS_.value_;
  conf.I2O_TA_CREDIT_Packing = I2O_TA_CREDIT_Packing_.value_;
  conf.taClass = taClass_;
  conf.taInstance = taInstance_;
  conf.usePlayback = usePlayback_.value_;
  conf.playbackDataFile = playbackDataFile_.value_;
  handler_->configure(conf);
}


void rubuilder::evm::TRGproxy::clear()
{
  toolbox::mem::Reference* trigBufRef;
  while( triggerFIFO_.deq(trigBufRef) ) {trigBufRef->release();}
}


void rubuilder::evm::TRGproxy::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>TRGproxy - " << triggerSource_.toString() << "</p>" << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<colgroup>"                                            << std::endl;
  *out << "<col style=\"text-align:left\"/>"                      << std::endl;
  *out << "<col style=\"text-align:right\"/>"                     << std::endl;
  *out << "</colgroup>"                                           << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;

  {
    boost::mutex::scoped_lock sl(triggerMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>number of triggers</td>"                           << std::endl;
    *out << "<td>" << triggerMonitoring_.nbTriggers << "</td>"      << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>last evt number from TRG</td>"                     << std::endl;
    *out << "<td>" << triggerMonitoring_.lastEventNumberFromTrigger
      << "</td>"                                                    << std::endl;
    *out << "</tr>"                                                 << std::endl;
    
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">EVM trigger</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (MB)</td>"                                 << std::endl;
    *out << "<td>" << triggerMonitoring_.payload / 0x100000 << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>msg count</td>"                                    << std::endl;
    *out << "<td>" << triggerMonitoring_.nbTriggers << "</td>"      << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << triggerMonitoring_.i2oCount << "</td>"        << std::endl;
    *out << "</tr>"                                                 << std::endl;
    
    handler_->printHtml(out);
  }
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  triggerFIFO_.printHtml(out, app_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  trgParams_.printHtml("Configuration", out);

  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
