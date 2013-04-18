#include <stdint.h>

#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "rubuilder/evm/BUproxy.h"
#include "rubuilder/evm/EoLSHandler.h"
#include "rubuilder/evm/SMproxy.h"
#include "rubuilder/utils/CreateStrings.h"
#include "rubuilder/utils/Exception.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "xcept/tools.h"


rubuilder::evm::EoLSHandler::EoLSHandler
(
  xdaq::Application* app,
  toolbox::mem::Pool* fastCtrlMsgPool
) :
app_(app),
buProxy_(0),
smProxy_(0),
fastCtrlMsgPool_(fastCtrlMsgPool),
logger_(app->getApplicationLogger()),
tid_(0),
idle_(true),
completedLumiSectionFIFO_("completedLumiSectionFIFO")
{
  resetMonitoringCounters();
  startWorkLoop();
}


void rubuilder::evm::EoLSHandler::send(const LumiSectionPair& ls)
{
  // Multiple threads are completing lumi sections.
  // Thus, protect the queue to have only one access at the time.
  boost::mutex::scoped_lock sl(completedLumiSectionFIFOMutex_);
  while ( ! completedLumiSectionFIFO_.enq(ls) ) ::usleep(1000);
}


void rubuilder::evm::EoLSHandler::appendConfigurationItems(utils::InfoSpaceItems& params)
{
  completedLumiSectionFIFOCapacity_ = 5000;

  params.add("completedLumiSectionFIFOCapacity", &completedLumiSectionFIFOCapacity_);

  configure();
}


void rubuilder::evm::EoLSHandler::appendMonitoringItems(utils::InfoSpaceItems& items)
{
  lastCompletedLS_ = 0;

  items.add("lastCompletedLS", &lastCompletedLS_);
}


void rubuilder::evm::EoLSHandler::updateMonitoringItems()
{
  lastCompletedLS_ = localLastCompletedLS_;
}


void rubuilder::evm::EoLSHandler::resetMonitoringCounters()
{
  localLastCompletedLS_ = 0;
}


void rubuilder::evm::EoLSHandler::configure()
{
  clear();
  completedLumiSectionFIFO_.resize(completedLumiSectionFIFOCapacity_);
}


void rubuilder::evm::EoLSHandler::clear()
{
  // completedLumiSectionFIFO is drained by indepent thread.
  // Thus, just wait until it is emptied.
  while( !completedLumiSectionFIFO_.empty() ) ::usleep(1000);
}


void rubuilder::evm::EoLSHandler::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>EoLSHandler</p>"                                    << std::endl;

  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<td>last completed LS</td>"                            << std::endl;
  *out << "<td>" << lastCompletedLS_ << "</td>"                   << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  completedLumiSectionFIFO_.printHtml(out, app_->getApplicationDescriptor()->getURN());
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "</table>"                                              << std::endl;

  *out << "</div>"                                                << std::endl;
}


void rubuilder::evm::EoLSHandler::getApplicationDescriptors()
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
}


void rubuilder::evm::EoLSHandler::startWorkLoop()
{
  try
  {
    const std::string identifier = utils::getIdentifier(app_->getApplicationDescriptor());
    
    EoLSSignalSenderWL_ = toolbox::task::getWorkLoopFactory()->
      getWorkLoop( identifier + "EoLSSignalSenderWorkLoop", "waiting" );
    
    if ( ! EoLSSignalSenderWL_->isActive() )
    {
      toolbox::task::ActionSignature* EoLSSignalSenderAction =
        toolbox::task::bind(this, &rubuilder::evm::EoLSHandler::EoLSSignalSender,
          identifier + "EoLSSignalSender");
      EoLSSignalSenderWL_->submit(EoLSSignalSenderAction);
      
      EoLSSignalSenderWL_->activate();
    }
  }
  catch (xcept::Exception& e)
  {
    std::string msg = "Failed to start workloop 'EoLSSignalSender'.";
    XCEPT_RETHROW(exception::L1Trigger, msg, e);
  }
}


bool rubuilder::evm::EoLSHandler::EoLSSignalSender(toolbox::task::WorkLoop*)
{
  std::string errorMsg = "Failed to send EoLS: ";
  idle_ = false;

  try
  {
    LumiSectionPair completedLumiSection;
    while ( completedLumiSectionFIFO_.deq(completedLumiSection) )
    {
      sendEoLSSignal(completedLumiSection);
      localLastCompletedLS_ = completedLumiSection.lumiSection;
    }
  }
  catch(xcept::Exception &e)
  {
    LOG4CPLUS_ERROR(logger_,
      errorMsg << xcept::stdformat_exception_history(e));
        
    XCEPT_DECLARE_NESTED(exception::L1Scalers,
      sentinelException, errorMsg, e);
    app_->notifyQualified("error",sentinelException);
  }
  catch(std::exception &e)
  {
    errorMsg += e.what();

    LOG4CPLUS_ERROR(logger_, errorMsg);

    XCEPT_DECLARE(exception::L1Scalers,
      sentinelException, errorMsg );
    app_->notifyQualified("error",sentinelException);
  }
  catch(...)
  {
    errorMsg += "Unknown exception";

    LOG4CPLUS_ERROR(logger_, errorMsg);

    XCEPT_DECLARE(exception::L1Scalers,
      sentinelException, errorMsg );
    app_->notifyQualified("error",sentinelException);
  }

  idle_ = true;

  ::sleep(1);
  return true; // reschedule this action
}


void rubuilder::evm::EoLSHandler::sendEoLSSignal(const LumiSectionPair& ls)
{
  toolbox::mem::Reference* bufRef = 0;

  try
  {
    bufRef = toolbox::mem::getMemoryPoolFactory()->
      getFrame(fastCtrlMsgPool_, sizeof(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME));
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::OutOfMemory,
      "Failed to get buffer for end-of-lumisection message", e);
  }
  catch(...)
  {
    XCEPT_RAISE(exception::OutOfMemory,
      "Failed to get buffer for end-of-lumisection message : Unknown exception");
  }

  try
  {
    bufRef->setDataSize(sizeof(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME));
        
    I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME* msg =
      (I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME*)bufRef->getDataLocation();
    I2O_PRIVATE_MESSAGE_FRAME* pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)msg;
    I2O_MESSAGE_FRAME* stdMsg = (I2O_MESSAGE_FRAME*)msg;
        
    pvtMsg->XFunctionCode    = I2O_EVM_LUMISECTION;
    pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
        
    stdMsg->MessageSize      = sizeof(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME) >> 2;
    stdMsg->InitiatorAddress = tid_;
    stdMsg->Function         = I2O_PRIVATE_MESSAGE;
    stdMsg->VersionOffset    = 0;
    stdMsg->MsgFlags         = 0;
    
    msg->runNumber           = ls.runNumber;
    msg->lumiSection         = ls.lumiSection;
    
    if (buProxy_) buProxy_->sendEoLSmsg(msg);
    if (smProxy_) smProxy_->sendEoLSmsg(msg);
        
    bufRef->release();
  }
  catch(xcept::Exception &e)
  {
    bufRef->release();
    XCEPT_RETHROW(exception::L1Trigger,
      "Failed to send end-of-lumisection message", e);
  }
  catch(...)
  {
    bufRef->release();
    XCEPT_RAISE(exception::L1Trigger,
      "Failed to send end-of-lumisection message : Unknown exception");
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
