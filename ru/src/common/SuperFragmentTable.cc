#include <sstream>

#include "interface/evb/i2oEVBMsgs.h"
#include "rubuilder/ru/BUproxy.h"
#include "rubuilder/ru/SuperFragmentTable.h"
#include "rubuilder/utils/Exception.h"
#include "xcept/tools.h"


rubuilder::ru::SuperFragmentTable::SuperFragmentTable() :
nbSuperFragmentsReady_(0)
{}


void rubuilder::ru::SuperFragmentTable::registerBUproxy(boost::shared_ptr<BUproxy> buProxy)
{
  buProxy_ = buProxy;
}


void rubuilder::ru::SuperFragmentTable::addEvBidAndBlock
(
  const rubuilder::utils::EvBid& evbId,
  toolbox::mem::Reference* bufRef
)
{
  boost::mutex::scoped_lock sl(mutex_);
  
  // Cross check event number
  const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
  const utils::EvBid blockEvBid = utils::EvBid(block->resyncCount,block->eventNumber);
  if (evbId != blockEvBid)
  {
    std::stringstream oss;
    
    oss << "EvB Id mismatch between trigger and data block.";
    oss << " Trigger: "  << evbId;
    oss << " Super-fragment: " << blockEvBid;
    
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  // Check that the slot in the super-fragment table is free
  Data::iterator dataPos = data_.lower_bound(evbId);
  if ( dataPos != data_.end() && !(data_.key_comp()(evbId,dataPos->first)) )
  {
    std::stringstream oss;
    oss << "A super-fragment is already in the lookup table: " << evbId;
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  ++nbSuperFragmentsReady_;

  Requests::iterator requestPos = requests_.find(evbId);
  if ( requestPos != requests_.end() )
  {
    // There's already a request for this super fragment
    dataReady(requestPos->second, bufRef);
    requests_.erase(requestPos);
  }
  else
  {
    // No request yet
    data_.insert(dataPos, Data::value_type(evbId,bufRef));
  }
}


void rubuilder::ru::SuperFragmentTable::addRequest(const Request& request)
{
  boost::mutex::scoped_lock sl(mutex_);
  
  // Check that the slot in the request table is free
  Requests::iterator requestPos = requests_.lower_bound(request.evbId);
  if ( requestPos != requests_.end() && !(requests_.key_comp()(request.evbId,requestPos->first)) )
  {
    std::stringstream oss;
    oss << "A request is already in the lookup table: " << request.evbId;
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }

  Data::iterator dataPos = data_.find(request.evbId);
  if ( dataPos != data_.end() )
  {
    // The super fragment for this request is already available
    dataReady(request, dataPos->second);
    data_.erase(dataPos);
  }
  else
  {
    // The request cannot be fulfilled yet
    requests_.insert(requestPos, Requests::value_type(request.evbId,request));
  }
}


void rubuilder::ru::SuperFragmentTable::dataReady
(
  const Request& request,
  toolbox::mem::Reference* bufRef
)
{
  // Cross check event number
  const I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
    (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)bufRef->getDataLocation();
  const utils::EvBid blockEvBid = utils::EvBid(block->resyncCount,block->eventNumber);
  if (request.evbId != blockEvBid)
  {
    std::stringstream oss;
    
    oss << "Event number mismatch when servicing BU request.";
    oss << " Request: "        << request.evbId;
    oss << " Super-fragment: " << blockEvBid;
    
    XCEPT_RAISE(exception::EventOrder, oss.str());
  }
  
  buProxy_->sendData(request, bufRef);
  --nbSuperFragmentsReady_;
}


void rubuilder::ru::SuperFragmentTable::clear()
{
  boost::mutex::scoped_lock sl(mutex_);
  
  Data::const_iterator it, itEnd;
  for (it = data_.begin(), itEnd = data_.end();
       it != itEnd; ++it)
  {
    it->second->release();
  }
  data_.clear();
  requests_.clear();
  nbSuperFragmentsReady_ = 0;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
