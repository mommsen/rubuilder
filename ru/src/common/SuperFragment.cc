#include "interface/shared/i2ogevb2g.h"
#include "rubuilder/ru/SuperFragment.h"


rubuilder::ru::SuperFragment::SuperFragment() :
size_(0),
head_(0),tail_(0)
{}

rubuilder::ru::SuperFragment::SuperFragment(const utils::EvBid& evbId, const FEDlist& fedList) :
evbId_(evbId),
remainingFEDs_(fedList),
size_(0),
head_(0),tail_(0)
{}


rubuilder::ru::SuperFragment::~SuperFragment()
{
  if (head_) head_->release();
}


bool rubuilder::ru::SuperFragment::append
(
  const uint16_t fedId,
  toolbox::mem::Reference* bufRef
)
{
  #ifdef FEDLIST_BITSET
  if ( ! remainingFEDs_.test(fedId) ) return false;
  remainingFEDs_.reset(fedId);
  #else
  if ( ! checkFedId(fedId) ) return false;
  #endif
  
  if (head_)
    tail_->setNextReference(bufRef);
  else
    head_ = bufRef;
  
  toolbox::mem::Reference* nextBufRef = bufRef;
  do {
    tail_ = nextBufRef;
    size_ += nextBufRef->getDataSize() - sizeof(I2O_DATA_READY_MESSAGE_FRAME);
    nextBufRef = nextBufRef->getNextReference();
  } while (nextBufRef);
  
  return true;
}


#ifndef FEDLIST_BITSET
inline bool rubuilder::ru::SuperFragment::checkFedId(const uint16_t fedId)
{
  FEDlist::iterator fedPos =
    std::find(remainingFEDs_.begin(),remainingFEDs_.end(),fedId);

  if ( fedPos == remainingFEDs_.end() ) return false;

  remainingFEDs_.erase(fedPos);

  return true;
}
#endif


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
