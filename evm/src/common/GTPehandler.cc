#include "i2o/i2o.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/frl_header.h"
#include "interface/shared/GlobalEventNumber.h"
#include "rubuilder/evm/TRGproxyHandlers.h"
#include "rubuilder/utils/EvBid.h"
#include "rubuilder/utils/EventUtils.h"
#include "rubuilder/utils/Exception.h"


void rubuilder::evm::TRGproxyHandlers::GTPehandler::configure(const Configuration& conf)
{
  orbitsPerLS_ = conf.orbitsPerLS;
  superFragmentGenerator_.configure(
    conf.fedSourceIds, conf.usePlayback, conf.playbackDataFile,
    conf.dummyBlockSize, conf.dummyFedPayloadSize, conf.dummyFedPayloadStdDev);
}


bool rubuilder::evm::TRGproxyHandlers::GTPehandler::getNextTrigger(toolbox::mem::Reference*& bufRef)
{
  if ( ! triggerFIFO_.deq(bufRef) ) return false;

  utils::L1Information l1Info;

  I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* gtpeMsg =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
  
  const uint32_t eventNumber = gtpeMsg->eventNumber;
  
  const unsigned char *gtpeFedAddr = ((unsigned char*)gtpeMsg)
    + sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME)
    + sizeof(frlh_t);
  
  if ( ! evtn::gtpe_board_sense(gtpeFedAddr) )
  {
    XCEPT_RAISE(exception::L1Trigger,
      "Received trigger fragment without GPTe board id.");
  }
  l1Info.bunchCrossing = evtn::gtpe_getbx(gtpeFedAddr);
  l1Info.eventType = (l1Info.bunchCrossing % 3) + 1;
  l1Info.orbitNumber = evtn::gtpe_getorbit(gtpeFedAddr);
  l1Info.lsNumber = l1Info.orbitNumber / orbitsPerLS_;
  utils::setFakeTriggerBits(-1, l1Info);

  bufRef->release();
  
  utils::EvBid evbId = evbIdFactory_.getEvBid(eventNumber);
  const bool success = superFragmentGenerator_.getData(
    bufRef,
    evbId,
    l1Info
  );

  return success;
}


void rubuilder::evm::TRGproxyHandlers::GTPehandler::reset()
{
  superFragmentGenerator_.reset();
  evbIdFactory_.reset();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
