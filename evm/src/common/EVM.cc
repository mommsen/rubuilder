#include "rubuilder/evm/BUproxy.h"
#include "rubuilder/evm/EVM.h"
#include "rubuilder/evm/L1InfoHandler.h"
#include "rubuilder/evm/RUproxy.h"
#include "rubuilder/evm/StateMachine.h"
#include "rubuilder/evm/TRGproxy.h"
#include "rubuilder/utils/CreateStrings.h"
#include "rubuilder/utils/Exception.h"
#include "rubuilder/utils/I2OMessages.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"

#include <math.h>


rubuilder::evm::EVM::EVM
(
  xdaq::Application* app,
  boost::shared_ptr<TRGproxy> trgProxy,
  boost::shared_ptr<RUproxy> ruProxy,
  boost::shared_ptr<BUproxy> buProxy,
  boost::shared_ptr<L1InfoHandler> l1InfoHandler
) :
app_(app),
trgProxy_(trgProxy),
ruProxy_(ruProxy),
buProxy_(buProxy),
l1InfoHandler_(l1InfoHandler),
doProcessing_(false),
processActive_(false),
sendOldMessagesActionPending_(false)
{
  resetMonitoringCounters();
  startProcessingWorkLoop();
}


void rubuilder::evm::EVM::appendConfigurationItems(utils::InfoSpaceItems& params)
{
  runNumber_                 = 0;
  nbEvtIdsInBuilder_         = utils::DEFAULT_NB_EVENTS;
  oldMessageSenderSleepUSec_ = 1000000;

  params.add("runNumber", &runNumber_);
  params.add("nbEvtIdsInBuilder", &nbEvtIdsInBuilder_);
  params.add("oldMessageSenderSleepUSec", &oldMessageSenderSleepUSec_);

  startOldMsgSenderSchedulerWorkLoop();
}


void rubuilder::evm::EVM::appendMonitoringItems(utils::InfoSpaceItems& items)
{
  monitoringRunNumber_        = 0;
  nbTriggersInEVM_            = 0;
  nbEvtsBuilt_                = 0;
  nbEvtsInBuilder_            = 0;
  ruBuilderIsFlushed_         = true;
  
  deltaT_                     = 0;
  deltaN_                     = 0;
  deltaSumOfSquares_          = 0;
  deltaSumOfSizes_            = 0;
  
  items.add("runNumber", &monitoringRunNumber_);
  items.add("nbTriggersInEVM", &nbTriggersInEVM_);
  items.add("nbEvtsBuilt", &nbEvtsBuilt_);
  items.add("nbEvtsInBuilder", &nbEvtsInBuilder_);
  items.add("ruBuilderIsFlushed", &ruBuilderIsFlushed_);
  
  items.add("deltaT", &deltaT_);
  items.add("deltaN", &deltaN_);
  items.add("deltaSumOfSquares", &deltaSumOfSquares_);
  items.add("deltaSumOfSizes", &deltaSumOfSizes_);
}


void rubuilder::evm::EVM::updateMonitoringItems()
{
  monitoringRunNumber_ = runNumber_;

  boost::mutex::scoped_lock sl(performanceMonitorMutex_);

  utils::PerformanceMonitor intervalEnd;
  trgProxy_->getTriggerPerformance(intervalEnd);

  delta_ = intervalEnd - intervalStart_;
  
  deltaT_.value_ = delta_.seconds;
  deltaN_.value_ = delta_.N;
  deltaSumOfSizes_.value_ = delta_.sumOfSizes;
  deltaSumOfSquares_ = delta_.sumOfSquares;

  nbTriggersInEVM_.value_ = intervalEnd.N - buProxy_->getConfirmLogicalCount();

  std::ostringstream msg;
  if (nbEvtsInBuilder_.value_ > 0)
  {
    ruBuilderIsFlushed_ = false;
    msg << nbEvtsInBuilder_ << " events are being built";
  }
  else if ( ! ((delta_.seconds > 0) && (delta_.N == 0)) )
  {
    ruBuilderIsFlushed_ = false;
    msg << delta_.N << " events have been built in the last " << delta_.seconds << "s";
  }
  else if ( ! trgProxy_->empty() )
  {
    ruBuilderIsFlushed_ = false;
    msg << "the trigger is still sending events";
  }
  else
  {
    ruBuilderIsFlushed_ = true;
  }
  reasonForNotFlushed_ = msg.str();

  intervalStart_ = intervalEnd;
}


void rubuilder::evm::EVM::resetMonitoringCounters()
{
  nbEvtsBuilt_ = 0;
  nbEvtsInBuilder_ = 0;
  
  evbIdFactory_.reset();
  
  boost::mutex::scoped_lock sl(performanceMonitorMutex_);
  intervalStart_ = utils::PerformanceMonitor();
  delta_ = utils::PerformanceMonitor();
}


void rubuilder::evm::EVM::startProcessing()
{
  doProcessing_ = true;
  processingWL_->submit(processingAction_);
}


void rubuilder::evm::EVM::stopProcessing()
{
  doProcessing_ = false;
  while (processActive_) ::usleep(1000);
  while (sendOldMessagesActionPending_) ::usleep(1000);
}


void rubuilder::evm::EVM::startProcessingWorkLoop()
{
  try
  {
    const std::string identifier = utils::getIdentifier(app_->getApplicationDescriptor());
    
    processingWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( identifier + "Processing", "waiting" );
    
    if ( ! processingWL_->isActive() )
    {
      processingAction_ =
        toolbox::task::bind(this, &rubuilder::evm::EVM::process,
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


bool rubuilder::evm::EVM::process(toolbox::task::WorkLoop *wl)
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


bool rubuilder::evm::EVM::doWork()
{
  bool anotherRound = false;
  
  // If there is a released event id from a BU
  if ( buProxy_->processNextReleasedEvent() )
  {
    ++nbEvtsBuilt_.value_;
    --nbEvtsInBuilder_.value_;
    anotherRound = true;
  }
  
  // If there is a free event id and a trigger
  toolbox::mem::Reference* trigBufRef = 0;
  if( nbEvtsInBuilder_.value_ < nbEvtIdsInBuilder_ && trgProxy_->getNextTrigger(trigBufRef) )
  {
    I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* trigMsg =
      (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)trigBufRef->getDataLocation();
    
    if(trigMsg->nbBlocksInSuperFragment != 1)
    {
      std::stringstream oss;
      
      oss << "nbBlocksInSuperFragment field of event data block from";
      oss << " trigger is not 1.";
      oss << " Received: " << trigMsg->nbBlocksInSuperFragment;
      
      XCEPT_RAISE(exception::Configuration, oss.str());
    }
    
    if(trigMsg->blockNb != 0)
    {
      std::stringstream oss;
      
      oss << "blockNb field of event data block from trogger is not 0.";
      oss << " Received: " << trigMsg->blockNb;
      
      XCEPT_RAISE(exception::Configuration, oss.str());
    }
    
    // Mark the trigger data as being super-fragment 0
    trigMsg->superFragmentNb = 0;
    
    // Get the event-builder id
    utils::EvBid evbId = evbIdFactory_.getEvBid(trigMsg->eventNumber);
    
    ruProxy_->addEvBid(evbId);
    uint32_t lumiSection = l1InfoHandler_->extractL1Info(trigBufRef, runNumber_);
    
    // Create a trigger data message ready to satisfy
    // a BU request for an allocated EvB id
    BUproxy::EventFifoElement eventForABU;
    eventForABU.trigBufRef  = trigBufRef;
    eventForABU.evbId       = evbId;
    eventForABU.runNumber   = runNumber_;
    eventForABU.lumiSection = lumiSection;
    buProxy_->addEvent(eventForABU);

    ++nbEvtsInBuilder_.value_;
    
    anotherRound = true;
  }
  
  anotherRound |= buProxy_->serviceBuRqst();
  
  return anotherRound;
}


void rubuilder::evm::EVM::startOldMsgSenderSchedulerWorkLoop()
{
  try
  {
    const std::string identifier = utils::getIdentifier(app_->getApplicationDescriptor());
    
    oldMsgSenderSchedulerWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( identifier + "OldMsgSenderScheduler", "waiting" );
    
    if ( ! oldMsgSenderSchedulerWL_->isActive() )
    {
      toolbox::task::ActionSignature* oldMsgSenderSchedulerAction =
        toolbox::task::bind(this, &rubuilder::evm::EVM::oldMsgSenderScheduler,
          identifier + "oldMsgSenderScheduler");

      oldMsgSenderSchedulerWL_->submit(oldMsgSenderSchedulerAction);

      oldMsgSenderSchedulerWL_->activate();

      sendOldMessagesAction_ = 
        toolbox::task::bind(this, &rubuilder::evm::EVM::sendOldMessages,
          identifier + "sendOldMessage");
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'oldMsgSenderScheduler'.";
    XCEPT_RETHROW(exception::WorkLoop, msg, e);
  }
}


bool rubuilder::evm::EVM::oldMsgSenderScheduler(toolbox::task::WorkLoop *wl)
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


bool rubuilder::evm::EVM::sendOldMessages(toolbox::task::WorkLoop *wl)
{
  try
  {
    ruProxy_->sendOldEvBids();
  }
  catch(xcept::Exception &e)
  {
    XCEPT_DECLARE_NESTED(exception::I2O,
      sentinelException,
      "Failed to send an old set of I2O_RU_READOUT messages", e);
    stateMachine_->processFSMEvent( utils::Fail(sentinelException) );
  }        

  try
  {
    trgProxy_->sendOldTriggerMessage();
  }
  catch(xcept::Exception &e)
  {
    XCEPT_DECLARE_NESTED(exception::I2O,
      sentinelException,
      "Failed to send old trigger messages", e);
    stateMachine_->processFSMEvent( utils::Fail(sentinelException) );
  }

  boost::mutex::scoped_lock sl(sendOldMessagesActionPendingMutex_);
  sendOldMessagesActionPending_ = false;
  
  return false;
}


void rubuilder::evm::EVM::printHtml(xgi::Output *out, const uint32_t monitoringSleepSec)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>EVM</p>"                                            << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>run number</td>"                                   << std::endl;
  *out << "<td>" << runNumber_ << "</td>"                         << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td># triggers in EVM</td>"                            << std::endl;
  *out << "<td>" << nbTriggersInEVM_ << "</td>"                   << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td># evts built</td>"                                 << std::endl;
  *out << "<td>" << nbEvtsBuilt_ << "</td>"                       << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td># evts in builder</td>"                            << std::endl;
  *out << "<td>" << nbEvtsInBuilder_ << "</td>"                   << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\">"                                    << std::endl;
  if (ruBuilderIsFlushed_)
    *out << "ruBuilder is flushed";
  else
    *out << "ruBuilder is not flushed because<br/>" << reasonForNotFlushed_;
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  

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
    out->precision(6);
    *out << "<td>rate (events/s)</td>"                              << std::endl;
    *out << "<td>" << 
      (delta_.seconds>0 ? delta_.N/delta_.seconds : 0)
      << "</td>"                                                    << std::endl;
    *out << "</tr>"                                                 << std::endl;
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
    out->precision(originalPrecision);
    out->flags(originalFlags);
  }

  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\"><br/>Configuration</th>"             << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<td>nbEvtIdsInBuilder</td>"                            << std::endl;
  *out << "<td>" << nbEvtIdsInBuilder_ << "</td>"                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>monitoringSleepSec</td>"                           << std::endl;
  *out << "<td>" << monitoringSleepSec << "</td>"                 << std::endl;
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
