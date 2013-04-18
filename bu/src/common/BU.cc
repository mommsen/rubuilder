#include "rubuilder/bu/BU.h"
#include "rubuilder/bu/EVMproxy.h"
#include "rubuilder/bu/RUproxy.h"
#include "rubuilder/bu/EventTable.h"
#include "rubuilder/bu/StateMachine.h"
#include "rubuilder/utils/CreateStrings.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"

#include <math.h>


rubuilder::bu::BU::BU
(
  xdaq::Application* app,
  boost::shared_ptr<EVMproxy> evmProxy,
  boost::shared_ptr<RUproxy> ruProxy,
  boost::shared_ptr<EventTable> eventTable
) :
app_(app),
evmProxy_(evmProxy),
ruProxy_(ruProxy),
eventTable_(eventTable),
runNumber_(0),
doProcessing_(false),
processActive_(false),
sendOldMessagesActionPending_(false)
{
  resetMonitoringCounters();
  startProcessingWorkLoop();
}


void rubuilder::bu::BU::appendConfigurationItems(utils::InfoSpaceItems& params)
{
  oldMessageSenderSleepUSec_ = 1000000;

  params.add("oldMessageSenderSleepUSec", &oldMessageSenderSleepUSec_);

  startOldMsgSenderSchedulerWorkLoop();
}


void rubuilder::bu::BU::appendMonitoringItems(utils::InfoSpaceItems& items)
{
  deltaT_                     = 0;
  deltaN_                     = 0;
  deltaSumOfSquares_          = 0;
  deltaSumOfSizes_            = 0;
  
  items.add("deltaT", &deltaT_);
  items.add("deltaN", &deltaN_);
  items.add("deltaSumOfSquares", &deltaSumOfSquares_);
  items.add("deltaSumOfSizes", &deltaSumOfSizes_);
}


void rubuilder::bu::BU::updateMonitoringItems()
{
  boost::mutex::scoped_lock sl(performanceMonitorMutex_);

  utils::PerformanceMonitor intervalEnd;
  eventTable_->getPerformance(intervalEnd);

  delta_ = intervalEnd - intervalStart_;
  
  deltaT_.value_ = delta_.seconds;
  deltaN_.value_ = delta_.N;
  deltaSumOfSizes_.value_ = delta_.sumOfSizes;
  deltaSumOfSquares_ = delta_.sumOfSquares;

  intervalStart_ = intervalEnd;
}


void rubuilder::bu::BU::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(performanceMonitorMutex_);
  intervalStart_ = utils::PerformanceMonitor();
  delta_ = utils::PerformanceMonitor();
}


void rubuilder::bu::BU::configure()
{
}


void rubuilder::bu::BU::clear()
{
}


void rubuilder::bu::BU::startProcessing(const uint32_t runNumber)
{
  runNumber_ = runNumber;
  doProcessing_ = true;
  processingWL_->submit(processingAction_);
}


void rubuilder::bu::BU::stopProcessing()
{
  doProcessing_ = false;
  while (processActive_) ::usleep(1000);
  while (sendOldMessagesActionPending_) ::usleep(1000);
}


void rubuilder::bu::BU::startProcessingWorkLoop()
{
  try
  {
    const std::string identifier = utils::getIdentifier(app_->getApplicationDescriptor());
    
    processingWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( identifier + "Processing", "waiting" );
    
    if ( ! processingWL_->isActive() )
    {
      processingAction_ =
        toolbox::task::bind(this, &rubuilder::bu::BU::process,
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


bool rubuilder::bu::BU::process(toolbox::task::WorkLoop *wl)
{
  ::usleep(1000);

  processActive_ = true;
  
  try
  {
    while ( doProcessing_ && doWork() ) {};
  }
  catch(xcept::Exception &e)
  {
    processActive_ = false;
    stateMachine_->processFSMEvent( utils::Fail(e) );
  }        
  
  processActive_ = false;
  
  return doProcessing_;
}


bool rubuilder::bu::BU::doWork()
{
  bool anotherRound = false;
  toolbox::mem::Reference* bufRef;
  
  // If there is an allocated event id
  if( evmProxy_->getTriggerBlock(bufRef) )
  {
    const uint32_t ruCount = ruProxy_->getRuCount();
    eventTable_->startConstruction(ruCount, bufRef);
    
    if ( ruCount > 0 )
      ruProxy_->requestDataForTrigger(bufRef);
    
    anotherRound = true;
  }
  
  // If there is an event data block
  if( ruProxy_->getDataBlock(bufRef) )
  {
    eventTable_->appendSuperFragment(bufRef);
    
    anotherRound = true;
  }
  
  return anotherRound;
}


void rubuilder::bu::BU::startOldMsgSenderSchedulerWorkLoop()
{
  try
  {
    const std::string identifier = utils::getIdentifier(app_->getApplicationDescriptor());
    
    oldMsgSenderSchedulerWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( identifier + "OldMsgSenderScheduler", "waiting" );
    
    if ( ! oldMsgSenderSchedulerWL_->isActive() )
    {
      toolbox::task::ActionSignature* oldMsgSenderSchedulerAction =
        toolbox::task::bind(this, &rubuilder::bu::BU::oldMsgSenderScheduler,
          identifier + "oldMsgSenderScheduler");

      oldMsgSenderSchedulerWL_->submit(oldMsgSenderSchedulerAction);

      oldMsgSenderSchedulerWL_->activate();

      sendOldMessagesAction_ = 
        toolbox::task::bind(this, &rubuilder::bu::BU::sendOldMessages,
          identifier + "sendOldMessage");
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'oldMsgSenderScheduler'.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool rubuilder::bu::BU::oldMsgSenderScheduler(toolbox::task::WorkLoop *wl)
{
  ::usleep(oldMessageSenderSleepUSec_.value_);

  boost::mutex::scoped_lock sl(sendOldMessagesActionPendingMutex_);
  
  if ( doProcessing_ && !sendOldMessagesActionPending_ )
  {
    // avoid submitting many sendOldMessagesActions when
    // processing workloop is busy with processing messages
    processingWL_->submit(sendOldMessagesAction_);
    sendOldMessagesActionPending_ = true;
  }

  return true;
}


bool rubuilder::bu::BU::sendOldMessages(toolbox::task::WorkLoop *wl)
{
  try
  {
    evmProxy_->sendOldRequests();
  }
  catch(xcept::Exception &e)
  {
    boost::mutex::scoped_lock sl(sendOldMessagesActionPendingMutex_);
    sendOldMessagesActionPending_ = false;
    
    XCEPT_DECLARE_NESTED(exception::I2O,
      sentinelException,
      "Failed to send an old set of I2O_EVM_ALLOCATE_CLEAR messages", e);
    stateMachine_->processFSMEvent( utils::Fail(sentinelException) );
  }        
  
  try
  {
    ruProxy_->sendOldRequests();
  }
  catch(xcept::Exception &e)
  {
    boost::mutex::scoped_lock sl(sendOldMessagesActionPendingMutex_);
    sendOldMessagesActionPending_ = false;
    
    XCEPT_DECLARE_NESTED(exception::I2O,
      sentinelException,
      "Failed to send old I2O_RU_SEND messages", e);
    stateMachine_->processFSMEvent( utils::Fail(sentinelException) );
  }

  boost::mutex::scoped_lock sl(sendOldMessagesActionPendingMutex_);
  sendOldMessagesActionPending_ = false;
  
  return false;
}


void rubuilder::bu::BU::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>BU</p>"                                             << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>run number</td>"                                   << std::endl;
  *out << "<td>" << runNumber_ << "</td>"                         << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  eventTable_->printMonitoringInformation(out);

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
    *out << "<td>event size (kB)</td>"                              << std::endl;
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
  
  eventTable_->printQueueInformation(out);
  
  eventTable_->printConfiguration(out);
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>maxEvtsUnderConstruction</td>"                     << std::endl;
  *out << "<td>" << stateMachine_->maxEvtsUnderConstruction() << "</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>msgAgeLimitDtMSec</td>"                            << std::endl;
  *out << "<td>" << stateMachine_->msgAgeLimitDtMSec() << "</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>oldMessageSenderSleepUSec</td>"                    << std::endl;
  *out << "<td>" << oldMessageSenderSleepUSec_ << "</td>"         << std::endl;
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
