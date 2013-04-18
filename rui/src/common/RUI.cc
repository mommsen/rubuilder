#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "rubuilder/rui/RUI.h"
#include "rubuilder/rui/StateMachine.h"
#include "rubuilder/utils/CreateStrings.h"
#include "rubuilder/utils/Exception.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"

#include <algorithm>


rubuilder::rui::RUI::RUI
(
  xdaq::Application* app
) :
app_(app),
tid_(0),
doProcessing_(false),
fragmentFIFO_("fragmentFIFO"),
superFragmentGenerator_(app->getApplicationDescriptor()->getURN())
{
  resetMonitoringCounters();
  startWorkLoops();
}


void rubuilder::rui::RUI::appendConfigurationItems(utils::InfoSpaceItems& params)
{
  destinationClass_ = "rubuilder::ru::Application";
  destinationInstance_ = app_->getApplicationDescriptor()->getInstance();
  usePlayback_ = false;
  playbackDataFile_ = "";  
  dummyBlockSize_ = 4096;
  dummyFedPayloadSize_ = 2048;
  dummyFedPayloadStdDev_ = 0;
  fragmentFIFOCapacity_ = 32;
  maxFragmentsInMemory_ = 8192;
  
  // The default has been chosen for simple tests that do not wish to set
  // FED source ids in the configuration file.  The default is 1 FED per
  // super-fragment, where each FED has a source id equal to the instance
  // number of the generating RUI.
  fedSourceIds_.clear();
  fedSourceIds_.push_back(app_->getApplicationDescriptor()->getInstance());

  ruiParams_.clear();
  ruiParams_.add("destinationClass", &destinationClass_);
  ruiParams_.add("destinationInstance", &destinationInstance_);
  ruiParams_.add("usePlayback", &usePlayback_);
  ruiParams_.add("playbackDataFile", &playbackDataFile_);
  ruiParams_.add("dummyBlockSize", &dummyBlockSize_);
  ruiParams_.add("dummyFedPayloadSize", &dummyFedPayloadSize_);
  ruiParams_.add("dummyFedPayloadStdDev", &dummyFedPayloadStdDev_);
  ruiParams_.add("fedSourceIds", &fedSourceIds_);
  ruiParams_.add("fragmentFIFOCapacity", &fragmentFIFOCapacity_);
  ruiParams_.add("maxFragmentsInMemory", &maxFragmentsInMemory_);

  params.add(ruiParams_);
}


void rubuilder::rui::RUI::appendMonitoringItems(utils::InfoSpaceItems& items)
{
  i2oEVMRUDataReadyCount_ = 0;
  
  items.add("i2oEVMRUDataReadyCount", &i2oEVMRUDataReadyCount_);
}


void rubuilder::rui::RUI::updateMonitoringItems()
{
  boost::mutex::scoped_lock sl(performanceMonitorMutex_);
  
  utils::PerformanceMonitor intervalEnd;
  getPerformance(intervalEnd);
  delta_ = intervalEnd - intervalStart_;
  intervalStart_ = intervalEnd;

  i2oEVMRUDataReadyCount_ = intervalEnd.N;
}


void rubuilder::rui::RUI::getPerformance(utils::PerformanceMonitor& performanceMonitor)
{
  boost::mutex::scoped_lock sl(dataMonitoringMutex_);

  performanceMonitor.N = dataMonitoring_.logicalCount;
  performanceMonitor.sumOfSizes = dataMonitoring_.payload;
  performanceMonitor.sumOfSquares = dataMonitoring_.payloadSquared;
}


void rubuilder::rui::RUI::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(dataMonitoringMutex_);
  
  dataMonitoring_.lastEventNumberToRU = 0;
  dataMonitoring_.i2oCount = 0;
  dataMonitoring_.payload = 0;
  dataMonitoring_.payloadSquared = 0;
  dataMonitoring_.logicalCount = 0;
}


void rubuilder::rui::RUI::configure()
{
  clear();

  fragmentFIFO_.resize(fragmentFIFOCapacity_);

  superFragmentGenerator_.configure(
    fedSourceIds_, usePlayback_, playbackDataFile_,
    dummyBlockSize_, dummyFedPayloadSize_, dummyFedPayloadStdDev_, maxFragmentsInMemory_);

  getApplicationDescriptors();
}


void rubuilder::rui::RUI::getApplicationDescriptors()
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
  
  try
  {
    ru_.descriptor =
      app_->getApplicationContext()->
      getDefaultZone()->
      getApplicationDescriptor(destinationClass_,
        destinationInstance_);
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    
    oss << "Failed to get application descriptor of RU";
    oss << destinationClass_.toString() << destinationInstance_.toString();
    
    XCEPT_RETHROW(exception::Configuration, oss.str(), e);
  }
  
  try
  {
    ru_.tid = i2o::utils::getAddressMap()->getTid(ru_.descriptor);
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    
    oss << "Failed to get I2O TID of RU";
    oss << destinationClass_.toString() << destinationInstance_.toString();
    XCEPT_RETHROW(exception::Configuration, oss.str(), e);
  }
}


void rubuilder::rui::RUI::clear()
{
  toolbox::mem::Reference* bufRef;
  while ( fragmentFIFO_.deq(bufRef) ) { bufRef->release(); }
}


void rubuilder::rui::RUI::startProcessing()
{
  doProcessing_ = true;
  superFragmentGenerator_.reset();
  generatingWL_->submit(generatingAction_);
  sendingWL_->submit(sendingAction_);
}


void rubuilder::rui::RUI::stopProcessing()
{
  doProcessing_ = false;
  while (generatingActive_) ::usleep(1000);
  while (sendingActive_) ::usleep(1000);
}


void rubuilder::rui::RUI::startWorkLoops()
{
  try
  {
    const std::string identifier = utils::getIdentifier(app_->getApplicationDescriptor());
    
    generatingWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( identifier + "Generating", "waiting" );
    
    if ( ! generatingWL_->isActive() )
    {
      generatingAction_ =
        toolbox::task::bind(this, &rubuilder::rui::RUI::generating,
          identifier + "generating");
      
      generatingWL_->activate();
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'Generating'.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }

  try
  {
    const std::string identifier = utils::getIdentifier(app_->getApplicationDescriptor());
    
    sendingWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( identifier + "Sending", "waiting" );
    
    if ( ! sendingWL_->isActive() )
    {
      sendingAction_ =
        toolbox::task::bind(this, &rubuilder::rui::RUI::sending,
          identifier + "sending");
      
      sendingWL_->activate();
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'Sending'.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool rubuilder::rui::RUI::generating(toolbox::task::WorkLoop *wl)
{
  generatingActive_ = true;
  
  toolbox::mem::Reference* bufRef = 0;
  if ( superFragmentGenerator_.getData(bufRef) )
  {
    while (bufRef)
    {
      // Break any chained references
      toolbox::mem::Reference* nextRef = bufRef->getNextReference();
      bufRef->setNextReference(0);
      
      while ( doProcessing_ && !fragmentFIFO_.enq(bufRef) ) { ::usleep(1000); }
      
      bufRef = nextRef;
    }
  }
  
  generatingActive_ = false;

  return doProcessing_;
}


bool rubuilder::rui::RUI::sending(toolbox::task::WorkLoop *wl)
{
  sendingActive_ = true;
  
  toolbox::mem::Reference* bufRef = 0;
  if ( fragmentFIFO_.deq(bufRef) )
  {
    sendData(bufRef);
  }
 
  sendingActive_ = false;

  return doProcessing_;
}



void rubuilder::rui::RUI::sendData(toolbox::mem::Reference* bufRef)
{
  I2O_MESSAGE_FRAME* stdMsg = (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
  I2O_PRIVATE_MESSAGE_FRAME* pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)stdMsg;
  I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block = (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;
  
  stdMsg->InitiatorAddress = tid_;
  stdMsg->TargetAddress    = ru_.tid;
  pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
  pvtMsg->XFunctionCode    = I2O_EVMRU_DATA_READY;
  
  {
    boost::mutex::scoped_lock sl(dataMonitoringMutex_);
    
    const uint32_t payload = (stdMsg->MessageSize << 2) -
      sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);

    dataMonitoring_.lastEventNumberToRU = block->eventNumber;
    ++dataMonitoring_.i2oCount;
    dataMonitoring_.payload += payload;
    dataMonitoring_.payloadSquared += payload*payload;
    if ( block->blockNb == (block->nbBlocksInSuperFragment - 1) )
      ++dataMonitoring_.logicalCount;
  }
 
  app_->getApplicationContext()->
    postFrame(
      bufRef,
      app_->getApplicationDescriptor(),
      ru_.descriptor//,
      //i2oExceptionHandler_,
      //bu
    );
}


void rubuilder::rui::RUI::printHtml(xgi::Output *out, const uint32_t monitoringSleepSec)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>RUI</p>"                                            << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;

  {
    boost::mutex::scoped_lock sl(dataMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>last evt number to RU</td>"                        << std::endl;
    *out << "<td>" << dataMonitoring_.lastEventNumberToRU << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">Sent to RU</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (MB)</td>"                                 << std::endl;
    *out << "<td>" << dataMonitoring_.payload / 0x100000<< "</td>"  << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << dataMonitoring_.logicalCount << "</td>"       << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << dataMonitoring_.i2oCount << "</td>"           << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  
  *out << "<tr><td>&nbsp;</td><td>&nbsp;</td></tr>"                 << std::endl;
  
  {
    boost::mutex::scoped_lock sl(performanceMonitorMutex_);
    
    const std::_Ios_Fmtflags originalFlags=out->flags();
    const int originalPrecision=out->precision();
    out->precision(3);
    out->setf(std::ios::fixed);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>deltaT (s)</td>"                                   << std::endl;
    *out << "<td>" << delta_.seconds << "</td>"                     << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    out->precision(2);
    *out << "<td>throughput (MB/s)</td>"                            << std::endl;
    *out << "<td>" << 
      (delta_.seconds>0 ? delta_.sumOfSizes/(double)0x100000/delta_.seconds : 0)
      << "</td>"                                                    << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    out->setf(std::ios::scientific);
    *out << "<td>rate (events/s)</td>"                              << std::endl;
    *out << "<td>" << 
      (delta_.seconds>0 ? delta_.N/delta_.seconds : 0)
      << "</td>"                                                    << std::endl;
    *out << "</tr>"                                                 << std::endl;
    out->unsetf(std::ios::scientific);
    out->precision(1);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>fragment size (kB)</td>"                           << std::endl;
    *out << "<td>";
    if ( delta_.N>0 )
    {
      const double meanOfSquares =  static_cast<double>(delta_.sumOfSquares)/delta_.N;
      const double mean = static_cast<double>(delta_.sumOfSizes)/delta_.N;
      const double variance = meanOfSquares - (mean*mean);
      // Variance maybe negative due to lack of precision
      const double rms = variance > 0 ? std::sqrt(variance) : 0;
      *out << mean/0x400 << " +/- " << rms/0x400;
    }
    else
    {
      *out << "n/a";
    }
    *out << "</td>"                                                 << std::endl;
    *out << "</tr>"                                                 << std::endl;
    out->flags(originalFlags);
    out->precision(originalPrecision);
  }
 
  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  fragmentFIFO_.printHtml(out, app_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  ruiParams_.printHtml("Configuration", out);
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>monitoringSleepSec</td>"                           << std::endl;
  *out << "<td>" << monitoringSleepSec << "</td>"                 << std::endl;
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
