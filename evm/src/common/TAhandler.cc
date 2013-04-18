#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "rubuilder/evm/TRGproxyHandlers.h"
#include "rubuilder/utils/Constants.h"
#include "rubuilder/utils/Exception.h"
#include "toolbox/mem/MemoryPoolFactory.h"


rubuilder::evm::TRGproxyHandlers::TAhandler::TAhandler
(
  xdaq::Application* app,
  toolbox::mem::Pool* fastCtrlMsgPool,
  TriggerFIFO& triggerFIFO
) :
TriggerHandler(),
app_(app),
fastCtrlMsgPool_(fastCtrlMsgPool),
triggerFIFO_(triggerFIFO),
timerId_(timerManager_.getTimer())
{
  reset();
}


void rubuilder::evm::TRGproxyHandlers::TAhandler::configure(const Configuration& conf)
{
  I2O_TA_CREDIT_Packing_ = conf.I2O_TA_CREDIT_Packing;
  
  findTA(conf.taClass, conf.taInstance);
}


void rubuilder::evm::TRGproxyHandlers::TAhandler::findTA(const std::string& taClass, const uint32_t taInstance)
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
    ta_.descriptor = app_->getApplicationContext()->getDefaultZone()->
      getApplicationDescriptor(taClass, taInstance);
  }
  catch(xcept::Exception &e)
  {
    std::ostringstream msg;
    msg << "Requested TA adapter, but TA " << taClass
      << " : " << taInstance << " not found.";
    XCEPT_RETHROW(exception::Configuration, msg.str(), e);
  }

  try
  {
    ta_.tid = i2o::utils::getAddressMap()->getTid(ta_.descriptor);
  }
  catch(xcept::Exception &e)
  {
    std::ostringstream msg;
    msg << "Failed to get I2O TID for TA " << taClass
      << " : " << taInstance;
    XCEPT_RETHROW(exception::Configuration, msg.str(), e);
  }
}


bool rubuilder::evm::TRGproxyHandlers::TAhandler::getNextTrigger(toolbox::mem::Reference*& bufRef)
{
  if ( firstRequest_ ) {
    requestTriggers( triggerFIFO_.size() );
    firstRequest_ = false;
  }

  if ( triggerFIFO_.deq(bufRef) )
  {
    if ( doRequestTriggers_ ) requestTriggers(1);
    return true;
  }

  return false;
}


void rubuilder::evm::TRGproxyHandlers::TAhandler::stopRequestingTriggers()
{
  doRequestTriggers_ = false;
  if ( nbCreditsToBeSent_ > 0 ) sendTrigCredits(nbCreditsToBeSent_);
}


void rubuilder::evm::TRGproxyHandlers::TAhandler::requestTriggers(const uint32_t& nbTriggers)
{
  if ( nbCreditsToBeSent_ == 0 )
  {
    // Start age timer if first credit
    timerManager_.restartTimer(timerId_);
  }

  nbCreditsToBeSent_ += nbTriggers;

  while ( nbCreditsToBeSent_ > I2O_TA_CREDIT_Packing_)
  {
    sendTrigCredits(I2O_TA_CREDIT_Packing_);
  }
}


void rubuilder::evm::TRGproxyHandlers::TAhandler::sendOldTriggerMessage()
{
  // If there is a trigger credits message under construction
  // and it is too old
  if (
    (nbCreditsToBeSent_ > 0) &&
    timerManager_.isFired(timerId_)
  )
  {
    sendTrigCredits(nbCreditsToBeSent_);
  }
}


void rubuilder::evm::TRGproxyHandlers::TAhandler::printHtml(xgi::Output* out)
{
  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\" style=\"text-align:center\">TA credit</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>payload (kB)</td>"                                 << std::endl;
  *out << "<td>" << taMonitoring_.payload / 0x400 << "</td>"      << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>msg count</td>"                                    << std::endl;
  *out << "<td>" << taMonitoring_.triggersRequested << "</td>"    << std::endl;
  *out << "</tr>"                                                 << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<td>I2O count</td>"                                    << std::endl;
  *out << "<td>" << taMonitoring_.i2oCount << "</td>"             << std::endl;
  *out << "</tr>"                                                 << std::endl;
}


void rubuilder::evm::TRGproxyHandlers::TAhandler::reset()
{
  nbCreditsToBeSent_ = 0;
  doRequestTriggers_ = true;
  firstRequest_ = true;
  
  taMonitoring_.triggersRequested = 0;
  taMonitoring_.payload = 0;
  taMonitoring_.i2oCount = 0;

  timerManager_.initTimer(timerId_ , utils::DEFAULT_MESSAGE_AGE_LIMIT_MSEC);
}


void rubuilder::evm::TRGproxyHandlers::TAhandler::sendTrigCredits(const uint32_t& nbTriggers)
{
  toolbox::mem::Reference     *bufRef = 0;
  I2O_TA_CREDIT_MESSAGE_FRAME *msg    = 0;
  I2O_PRIVATE_MESSAGE_FRAME   *pvtMsg = 0;
  I2O_MESSAGE_FRAME           *stdMsg = 0;

  try
  {
    bufRef = toolbox::mem::getMemoryPoolFactory()->
      getFrame(fastCtrlMsgPool_, sizeof(I2O_TA_CREDIT_MESSAGE_FRAME));
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::OutOfMemory,
      "Failed to get buffer for trigger credits message", e);
  }
  catch(...)
  {
    XCEPT_RAISE(exception::OutOfMemory,
      "Failed to get buffer for trigger credits message : Unknown exception");
  }
  
  bufRef->setDataSize(sizeof(I2O_TA_CREDIT_MESSAGE_FRAME));
  
  msg    = (I2O_TA_CREDIT_MESSAGE_FRAME*)bufRef->getDataLocation();
  pvtMsg = (I2O_PRIVATE_MESSAGE_FRAME*)msg;
  stdMsg = (I2O_MESSAGE_FRAME*)msg;
  
  pvtMsg->XFunctionCode    = I2O_TA_CREDIT;
  pvtMsg->OrganizationID   = XDAQ_ORGANIZATION_ID;
  
  stdMsg->MessageSize      = sizeof(I2O_TA_CREDIT_MESSAGE_FRAME) >> 2;
  stdMsg->InitiatorAddress = tid_;
  stdMsg->TargetAddress    = ta_.tid;
  stdMsg->Function         = I2O_PRIVATE_MESSAGE;
  stdMsg->VersionOffset    = 0;
  stdMsg->MsgFlags         = 0;
  
  msg->nbCredits           = nbTriggers;
  
  try
  {
    app_->getApplicationContext()->
      postFrame(
        bufRef,
        app_->getApplicationDescriptor(),
        ta_.descriptor// ,
        // i2oExceptionHandler_,
        // ta_.descriptor
      );
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::I2O,
      "Failed to send trigger credits message", e);
  }
  catch(...)
  {
    XCEPT_RAISE(exception::I2O,
            "Failed to send trigger credits message : Unknown exception");
  }
  
  taMonitoring_.triggersRequested += nbTriggers;
  taMonitoring_.payload += sizeof(I2O_TA_CREDIT_MESSAGE_FRAME) -
    sizeof(I2O_PRIVATE_MESSAGE_FRAME);
  ++taMonitoring_.i2oCount;

  nbCreditsToBeSent_ -= nbTriggers;
}



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
