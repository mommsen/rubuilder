#include "rubuilder/ru/EVMproxy.h"
#include "rubuilder/ru/BUproxy.h"
#include "rubuilder/ru/RU.h"
#include "rubuilder/ru/RUinput.h"
#include "rubuilder/ru/StateMachine.h"
#include "rubuilder/utils/CreateStrings.h"
#include "rubuilder/utils/Exception.h"
#include "rubuilder/utils/I2OMessages.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"

#include <algorithm>


rubuilder::ru::RU::RU
(
  xdaq::Application* app,
  SuperFragmentTablePtr superFragmentTable,
  boost::shared_ptr<BUproxy> buProxy,
  boost::shared_ptr<EVMproxy> evmProxy,
  boost::shared_ptr<RUinput> ruInput
) :
app_(app),
superFragmentTable_(superFragmentTable),
buProxy_(buProxy),
evmProxy_(evmProxy),
ruInput_(ruInput),
doProcessing_(false),
processActive_(false),
timerId_(timerManager_.getTimer())
{
  resetMonitoringCounters();
  startProcessingWorkLoop();
}


void rubuilder::ru::RU::appendConfigurationItems(utils::InfoSpaceItems& params)
{
  runNumber_ = 0;
  maxPairAgeMSec_ = 0; // Zero means forever

  params.add("runNumber", &runNumber_);
  params.add("maxPairAgeMSec", &maxPairAgeMSec_);

  // For historical reasons, maxPairAgeMSec can be negative
  if (maxPairAgeMSec_ < 0) maxPairAgeMSec_ = 0;
}


void rubuilder::ru::RU::appendMonitoringItems(utils::InfoSpaceItems& items)
{
  monitoringRunNumber_        = 0;
  nbSuperFragmentsInRU_       = 0;

  deltaT_                     = 0;
  deltaN_                     = 0;
  deltaSumOfSquares_          = 0;
  deltaSumOfSizes_            = 0;
  
  items.add("runNumber", &monitoringRunNumber_);
  items.add("nbSuperFragmentsInRU", &nbSuperFragmentsInRU_);

  items.add("deltaT", &deltaT_);
  items.add("deltaN", &deltaN_);
  items.add("deltaSumOfSquares", &deltaSumOfSquares_);
  items.add("deltaSumOfSizes", &deltaSumOfSizes_);
}


void rubuilder::ru::RU::updateMonitoringItems()
{
  monitoringRunNumber_ = runNumber_;

  nbSuperFragmentsInRU_.value_ = std::max(static_cast<uint64_t>(0),
    ruInput_->fragmentsCount() - buProxy_->i2oBUCacheCount());

  boost::mutex::scoped_lock sl(performanceMonitorMutex_);

  utils::PerformanceMonitor intervalEnd;
  getPerformance(intervalEnd);

  delta_ = intervalEnd - intervalStart_;
  
  deltaT_.value_ = delta_.seconds;
  deltaN_.value_ = delta_.N;
  deltaSumOfSizes_.value_ = delta_.sumOfSizes;
  deltaSumOfSquares_ = delta_.sumOfSquares;

  intervalStart_ = intervalEnd;
}


void rubuilder::ru::RU::getPerformance(utils::PerformanceMonitor& performanceMonitor)
{
  boost::mutex::scoped_lock sl(superFragmentMonitoringMutex_);

  performanceMonitor.N = superFragmentMonitoring_.count;
  performanceMonitor.sumOfSizes = superFragmentMonitoring_.payload;
  performanceMonitor.sumOfSquares = superFragmentMonitoring_.payloadSquared;
}


void rubuilder::ru::RU::resetMonitoringCounters()
{
  {
    boost::mutex::scoped_lock sl(performanceMonitorMutex_);
    intervalStart_ = utils::PerformanceMonitor();
    delta_ = utils::PerformanceMonitor();
  }
  {
    boost::mutex::scoped_lock sl(superFragmentMonitoringMutex_);
    superFragmentMonitoring_.count = 0;
    superFragmentMonitoring_.payload = 0;
    superFragmentMonitoring_.payloadSquared = 0;
  }
}


void rubuilder::ru::RU::configure()
{
  timerManager_.initTimer(timerId_, maxPairAgeMSec_);
}


void rubuilder::ru::RU::clear()
{
  superFragmentTable_->clear();
}


void rubuilder::ru::RU::startProcessing()
{
  doProcessing_ = true;
  processingWL_->submit(processingAction_);
}


void rubuilder::ru::RU::stopProcessing()
{
  doProcessing_ = false;
  while (processActive_) ::usleep(1000);
}


void rubuilder::ru::RU::startProcessingWorkLoop()
{
  try
  {
    const std::string identifier = utils::getIdentifier(app_->getApplicationDescriptor());
    
    processingWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( identifier + "Processing", "waiting" );
    
    if ( ! processingWL_->isActive() )
    {
      processingAction_ =
        toolbox::task::bind(this, &rubuilder::ru::RU::process,
          identifier + "process");
      
      processingWL_->activate();
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'Processing'.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool rubuilder::ru::RU::process(toolbox::task::WorkLoop *wl)
{
  processActive_ = true;
  
  //fix affinity to core 15
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(15, &cpuset);
  const int status = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if ( status != 0  )
  {
    std::ostringstream oss;
    oss << "Failed to set affinity for workloop 'Processing': "
      << strerror(status);
    XCEPT_RAISE(exception::Configuration, oss.str());
  }

  while (doProcessing_)
  {
    try
    {
      // Wait for a trigger
      utils::EvBid evbId;
      while ( doProcessing_ && ! evmProxy_->getTrigEvBid(evbId) ) {}; //::usleep(1000);
      
      // Wait for the corresponding event fragment
      timerManager_.restartTimer(timerId_);
      toolbox::mem::Reference* bufRef = 0;
      while ( doProcessing_ && ! ruInput_->getData(evbId,bufRef) )
      {
        //::usleep(1000);
        if ( maxPairAgeMSec_ > 0 && timerManager_.isFired(timerId_) )
        {
          std::ostringstream msg;
          msg << "Waited for more than " << maxPairAgeMSec_ << 
            " ms for the event data block for trigger evbId " << evbId;
          XCEPT_RAISE(exception::TimedOut, msg.str());
        }
      }
      
      if (bufRef)
      {
        updateSuperFragmentCounters(bufRef);
        superFragmentTable_->addEvBidAndBlock(evbId, bufRef);
      }
    }
    catch(exception::MismatchDetected &e)
    {
      processActive_ = false;
      stateMachine_->processFSMEvent( MismatchDetected(e) );
    }
    catch(exception::TimedOut &e)
    {
      processActive_ = false;
      stateMachine_->processFSMEvent( TimedOut(e) );
    }
    catch(xcept::Exception &e)
    {
      processActive_ = false;
      stateMachine_->processFSMEvent( utils::Fail(e) );
    }
  }
  
  processActive_ = false;
  
  return doProcessing_;
}


void rubuilder::ru::RU::updateSuperFragmentCounters(toolbox::mem::Reference* head)
{
  uint32_t payload = 0;
  toolbox::mem::Reference* bufRef = head;

  while (bufRef)
  {
    const I2O_MESSAGE_FRAME* stdMsg =
      (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
    payload +=
      (stdMsg->MessageSize << 2) - sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
    bufRef = bufRef->getNextReference();
  }

  boost::mutex::scoped_lock sl(superFragmentMonitoringMutex_);
  
  ++superFragmentMonitoring_.count;
  superFragmentMonitoring_.payload += payload;
  superFragmentMonitoring_.payloadSquared += payload*payload;
}


void rubuilder::ru::RU::printHtml(xgi::Output *out, const uint32_t monitoringSleepSec)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>RU</p>"                                            << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>run number</td>"                                   << std::endl;
  *out << "<td>" << runNumber_ << "</td>"                         << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td># fragments in RU</td>"                            << std::endl;
  *out << "<td>" << nbSuperFragmentsInRU_ << "</td>"              << std::endl;
  *out << "</tr>"                                                 << std::endl;

  {
    boost::mutex::scoped_lock sl(superFragmentMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td># fragments built</td>"                            << std::endl;
    *out << "<td>" << superFragmentMonitoring_.count << "</td>"     << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }
  
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
  *out << "<th colspan=\"2\"><br/>Configuration</th>"             << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>monitoringSleepSec</td>"                           << std::endl;
  *out << "<td>" << monitoringSleepSec << "</td>"                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>maxPairAgeMSec</td>"                               << std::endl;
  *out << "<td>" << maxPairAgeMSec_ << "</td>"                    << std::endl;
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
