#include "rubuilder/utils/EvBidFactory.h"


rubuilder::utils::EvBidFactory::EvBidFactory() :
previousEventNumber_(0),
resyncCount_(0)
{}


void rubuilder::utils::EvBidFactory::reset()
{
  previousEventNumber_ = 0;
  resyncCount_ = 0;
}


rubuilder::utils::EvBid rubuilder::utils::EvBidFactory::getEvBid(const uint32_t eventNumber)
{
  if ( eventNumber <= previousEventNumber_ ) ++resyncCount_;
  
  previousEventNumber_ = eventNumber;
  
  return EvBid(resyncCount_,eventNumber);
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
