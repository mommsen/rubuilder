#include <byteswap.h>

#include "i2o/i2o.h"
#include "interface/evb/i2oEVBMsgs.h"
#include "interface/shared/frl_header.h"
#include "interface/shared/GlobalEventNumber.h"
#include "interface/shared/i2ogevb2g.h"
#include "rubuilder/evm/TRGproxyHandlers.h"
#include "rubuilder/utils/EvBid.h"
#include "rubuilder/utils/EventUtils.h"
#include "rubuilder/utils/Exception.h"


void rubuilder::evm::TRGproxyHandlers::FEROLhandler::configure(const Configuration& conf)
{
  orbitsPerLS_ = conf.orbitsPerLS;
  superFragmentGenerator_.configure(
    conf.fedSourceIds, conf.usePlayback, conf.playbackDataFile,
    conf.dummyBlockSize, conf.dummyFedPayloadSize, conf.dummyFedPayloadStdDev);
}


bool rubuilder::evm::TRGproxyHandlers::FEROLhandler::getNextTrigger(toolbox::mem::Reference*& bufRef)
{
  if ( ! triggerFIFO_.deq(bufRef) ) return false;

  utils::L1Information l1Info;

  char* i2oPayloadPtr = (char*)bufRef->getDataLocation() +
    sizeof(I2O_DATA_READY_MESSAGE_FRAME);
  const uint64_t h0 = bswap_64(*((uint64_t*)(i2oPayloadPtr)));
  const uint64_t eventNumber = (h0 & 0x0000000000FFFFFF);
  
  const utils::EvBid evbId = evbIdFactory_.getEvBid(eventNumber);
  
  // const unsigned char *gtpeFedAddr = ((unsigned char*)gtpeMsg)
  //   + sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME)
  //   + sizeof(frlh_t);
  
  // if ( ! evtn::gtpe_board_sense(gtpeFedAddr) )
  // {
  //   XCEPT_RAISE(exception::L1Trigger,
  //     "Received trigger fragment without GPTe board id.");
  // }
  // l1Info.bunchCrossing = evtn::gtpe_getbx(gtpeFedAddr);
  // l1Info.eventType = (l1Info.bunchCrossing % 3) + 1;
  // l1Info.orbitNumber = evtn::gtpe_getorbit(gtpeFedAddr);
  // l1Info.lsNumber = l1Info.orbitNumber / orbitsPerLS_;
  // utils::setFakeTriggerBits(-1, l1Info);

  bufRef->release();
  
  const bool success = superFragmentGenerator_.getData(
    bufRef,
    evbId,
    l1Info
  );

  return success;
}


void rubuilder::evm::TRGproxyHandlers::FEROLhandler::reset()
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
