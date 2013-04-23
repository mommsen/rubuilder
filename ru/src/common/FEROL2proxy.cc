#include <algorithm>
#include <byteswap.h>
#include <iterator>
#include <string.h>

#include "interface/shared/fed_header.h"
#include "interface/shared/ferol_header.h"
#include "interface/shared/i2ogevb2g.h"
#include "rubuilder/ru/InputHandler.h"
#include "rubuilder/utils/Constants.h"
#include "rubuilder/utils/Exception.h"
#include "toolbox/net/URN.h"
#include "toolbox/mem/HeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"

rubuilder::ru::FEROL2proxy::FEROL2proxy(xdaq::Application* app) :
InputHandler(app),
dropInputData_(false)
{
  try
  {
    toolbox::net::URN urn("toolbox-mem-pool", "udapl");
    superFragmentPool_ = toolbox::mem::getMemoryPoolFactory()->findPool(urn);
  }
  catch (toolbox::mem::exception::MemoryPoolNotFound)
  {
    toolbox::net::URN poolURN("toolbox-mem-pool",app->getApplicationDescriptor()->getURN());
    try
    {
      superFragmentPool_ = toolbox::mem::getMemoryPoolFactory()->
        createPool(poolURN, new toolbox::mem::HeapAllocator());
    }
    catch (toolbox::mem::exception::DuplicateMemoryPool)
    {
      // Pool already exists from a previous construction of this class
      // Note that destroying the pool in the destructor is not working
      // because the previous instance is destroyed after the new one
      // is constructed.
      superFragmentPool_ = toolbox::mem::getMemoryPoolFactory()->
        findPool(poolURN);
    }
  }
  catch (toolbox::mem::exception::Exception e)
  {
    XCEPT_RETHROW(exception::OutOfMemory,
      "Failed to create memory pool for super-fragments.", e);
  }
}


void rubuilder::ru::FEROL2proxy::I2Ocallback(toolbox::mem::Reference* bufRef)
{
  char* i2oPayloadPtr = (char*)bufRef->getDataLocation() +
    sizeof(I2O_DATA_READY_MESSAGE_FRAME);
  const uint64_t h0 = bswap_64(*((uint64_t*)(i2oPayloadPtr)));
  const uint64_t h1 = bswap_64(*((uint64_t*)(i2oPayloadPtr + 8)));	                       

  //const uint16_t signature = (h0 & 0xFFFF000000000000) >> 48;
  //const uint16_t header = (h0 & 0x00000000F0000000) >> 28;
  //const uint64_t len = (h0 & 0x00000000000003ff) << 3;
  //const uint64_t fedId = (h1 & 0x0FFF000000000000) >> 48;
  //const uint64_t eventNumber = (h1 & 0x0000000000FFFFFF);
  const uint64_t fedId = FEROL_FEDID_EXTRACT(h1);
  const uint64_t eventNumber = FEROL_EVENTNB_EXTRACT(h1);

  // assert( signature == 0x475A );

  I2O_DATA_READY_MESSAGE_FRAME* frame =
    (I2O_DATA_READY_MESSAGE_FRAME*)bufRef->getDataLocation();
  // assert( frame->totalLength == len+16 );
  // assert( bufRef->getDataSize() == frame->totalLength + sizeof(I2O_DATA_READY_MESSAGE_FRAME) );

  {
    boost::mutex::scoped_lock sl(inputMonitoringMutex_);
    
    inputMonitoring_.lastEventNumber = eventNumber;
    inputMonitoring_.payload += frame->totalLength;
    ++inputMonitoring_.i2oCount;
    if ( FEROL_LASTPACKET_EXTRACT(h0) )
      ++inputMonitoring_.logicalCount;
  }
  FedFragmentFIFOs::iterator pos = fedFragmentFIFOs_.find(fedId);
  if ( pos == fedFragmentFIFOs_.end() )
  {
    std::stringstream msg;
    
    msg << "The received FED id " << fedId;
    msg << " for event " << eventNumber;
    msg << " is not in the excepted list ";
    std::copy(fedList_.begin(), fedList_.end(),
      std::ostream_iterator<uint16_t>(msg,","));
    
    XCEPT_RAISE(exception::Configuration, msg.str());
  }
  
  if ( dropInputData_ )
    bufRef->release();
  else
    while ( ! pos->second.enq(bufRef) ) { ::usleep(1000); }
}


bool rubuilder::ru::FEROL2proxy::getData
(
  const utils::EvBid& evbId,
  toolbox::mem::Reference*& superFragment
)
{
  toolbox::mem::Reference* bufRef = getNextFragment(evbId);
  superFragment = copyDataIntoDataBlock(bufRef, evbId);
  bufRef->release();

  // superFragment->release();
  // return false;  

  return true;
}


toolbox::mem::Reference* rubuilder::ru::FEROL2proxy::getNextFragment(const utils::EvBid& evbId)
{
  toolbox::mem::Reference* head = 0; 
  toolbox::mem::Reference* tail = 0;
  
  for (FedFragmentFIFOs::iterator it = fedFragmentFIFOs_.begin(), itEnd = fedFragmentFIFOs_.end();
       it != itEnd; ++it)
  {
    toolbox::mem::Reference* bufRef;
    while ( ! it->second.deq(bufRef) ) {}
    
    char* i2oPayloadPtr = (char*)bufRef->getDataLocation() +
      sizeof(I2O_DATA_READY_MESSAGE_FRAME);
    const uint64_t h1 = bswap_64(*((uint64_t*)(i2oPayloadPtr + 8)));	                       
    const uint64_t fedId = FEROL_FEDID_EXTRACT(h1);
    const uint64_t eventNumber = FEROL_EVENTNB_EXTRACT(h1);
    
    const utils::EvBid fedEvBid = evbIdFactories_[fedId].getEvBid(eventNumber);
    if ( fedEvBid != evbId )
    {
      std::stringstream oss;
      
      oss << "Mismatch detected: expected evb id "
        << evbId << ", but found evb id "
        << fedEvBid << " in FEROL header from FED " << fedId;
      
      XCEPT_RAISE(exception::MismatchDetected, oss.str());
    }
    
    if (head)
      tail->setNextReference(bufRef);
    else
      head = bufRef;
    
    do {
      tail = bufRef;
      bufRef = bufRef->getNextReference();
    } while (bufRef);
  }

  return head;
}


toolbox::mem::Reference* rubuilder::ru::FEROL2proxy::copyDataIntoDataBlock
(
  toolbox::mem::Reference* bufRef,
  const utils::EvBid& evbId
) const
{
  toolbox::mem::Reference* head =
    toolbox::mem::getMemoryPoolFactory()->getFrame(superFragmentPool_,blockSize_);
  head->setDataSize(blockSize_);
  char* payload = (char*)head->getDataLocation() + sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
  size_t payloadSize = blockSize_ - sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
  toolbox::mem::Reference* tail = head;
  uint32_t blockCount = 1;
  size_t dataSize = 0;

  while ( bufRef )
  {
    size_t currentFragmentSize =
      ((I2O_DATA_READY_MESSAGE_FRAME*)bufRef->getDataLocation())->totalLength;
    size_t copiedSize = 0;
    
    while ( currentFragmentSize > payloadSize )
    {
      // fill the remaining block
      memcpy(payload, (char*)bufRef->getDataLocation() + sizeof(I2O_DATA_READY_MESSAGE_FRAME) + copiedSize,
        payloadSize);
      copiedSize += payloadSize;
      currentFragmentSize -= payloadSize;
      
      // get a new block
      toolbox::mem::Reference* nextBlock =
        toolbox::mem::getMemoryPoolFactory()->getFrame(superFragmentPool_,blockSize_);
      nextBlock->setDataSize(blockSize_);
      payload = (char*)nextBlock->getDataLocation() + sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
      payloadSize = blockSize_ - sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
      tail->setNextReference(nextBlock);
      tail = nextBlock;
      ++blockCount;
      dataSize = 0;
    }

    // fill the remaining fragment into the block
    memcpy(payload, (char*)bufRef->getDataLocation() + sizeof(I2O_DATA_READY_MESSAGE_FRAME) + copiedSize,
      currentFragmentSize);
    payload += currentFragmentSize;
    payloadSize -= currentFragmentSize;
    dataSize += currentFragmentSize;
    
    bufRef = bufRef->getNextReference();
  }
  tail->setDataSize(dataSize + sizeof(I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME));
  
  fillBlockInfo(head, evbId, blockCount);

  return head;
}


void rubuilder::ru::FEROL2proxy::fillBlockInfo
(
  toolbox::mem::Reference* bufRef,
  const utils::EvBid& evbId,
  const uint32_t nbBlocks
) const
{
  uint32_t blockNb = 0;
  
  while (bufRef != 0)
  {
    I2O_MESSAGE_FRAME* stdMsg = (I2O_MESSAGE_FRAME*)bufRef->getDataLocation();
    stdMsg->MessageSize      = bufRef->getDataSize() >> 2;
    stdMsg->InitiatorAddress = 0;
    stdMsg->TargetAddress    = 0;
    stdMsg->Function         = I2O_PRIVATE_MESSAGE;
    stdMsg->VersionOffset    = 0;
    stdMsg->MsgFlags         = 0;  // Point-to-point
    
    I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME* block =
      (I2O_EVENT_DATA_BLOCK_MESSAGE_FRAME*)stdMsg;
    block->eventNumber = evbId.eventNumber();
    block->resyncCount = evbId.resyncCount();
    block->blockNb = blockNb;
    block->nbBlocksInSuperFragment = nbBlocks;
    
    bufRef = bufRef->getNextReference();
    ++blockNb;
  }
}


void rubuilder::ru::FEROL2proxy::configure(const Configuration& conf)
{
  clear();

  blockSize_ = conf.dummyBlockSize;
  dropInputData_ = conf.dropInputData;

  fedFragmentFIFOs_.clear();
  fedList_.clear();
  fedList_.reserve(conf.fedSourceIds.size());
  xdata::Vector<xdata::UnsignedInteger32>::const_iterator it, itEnd;
  for (it = conf.fedSourceIds.begin(), itEnd = conf.fedSourceIds.end();
       it != itEnd; ++it)
  {
    const uint16_t fedId = it->value_;
    if (fedId > FED_SOID_WIDTH)
    {
      std::ostringstream oss;
      
      oss << "fedSourceId is too large.";
      oss << "Actual value: " << fedId;
      oss << " Maximum value: FED_SOID_WIDTH=" << FED_SOID_WIDTH;
      
      XCEPT_RAISE(exception::Configuration, oss.str());
    }
    std::ostringstream fifoName;
    fifoName << "FED_" << fedId;
    FragmentFIFO fifo(fifoName.str());
    fifo.resize(conf.blockFIFOCapacity);
    if ( ! fedFragmentFIFOs_.insert(FedFragmentFIFOs::value_type(fedId,fifo)).second )
    {
      std::ostringstream oss;
      oss << "Duplicated FED id specified: " << fedId;
      XCEPT_RAISE(exception::Configuration, oss.str());
    }
    fedList_.push_back(fedId);
  }
  
  evbIdFactories_.clear();
}


void rubuilder::ru::FEROL2proxy::clear()
{
  for (FedFragmentFIFOs::iterator it = fedFragmentFIFOs_.begin(), itEnd = fedFragmentFIFOs_.end();
        it != itEnd; ++it)
  {
    toolbox::mem::Reference* bufRef;
    while ( it->second.deq(bufRef) ) { bufRef->release(); }
  }
  
  for ( EvBidFactories::iterator it = evbIdFactories_.begin(), itEnd = evbIdFactories_.end();
        it != itEnd; ++it)
  {
    it->second.reset();
  }
}


void rubuilder::ru::FEROL2proxy::printHtml(xgi::Output *out)
{
  {
    boost::mutex::scoped_lock sl(inputMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>last evt number from FEROL</td>"                   << std::endl;
    *out << "<td>" << inputMonitoring_.lastEventNumber << "</td>"   << std::endl;
    *out << "</tr>"                                                 << std::endl;
    // *out << "<tr>"                                                  << std::endl;
    // *out << "<td>super fragments under construction</td>"           << std::endl;
    // *out << "<td>" << superFragmentMap_.size() << "</td>"           << std::endl;
    // *out << "</tr>"                                                 << std::endl;
    // *out << "<tr>"                                                  << std::endl;
    *out << "<td colspan=\"2\" style=\"text-align:center\">RU input</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (MB)</td>"                                 << std::endl;
    *out << "<td>" << inputMonitoring_.payload / 0x100000<< "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>logical count</td>"                                << std::endl;
    *out << "<td>" << inputMonitoring_.logicalCount << "</td>"      << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << inputMonitoring_.i2oCount << "</td>"          << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>FED size (Bytes)</td>"                             << std::endl;
    *out << "<td>" <<
      (inputMonitoring_.logicalCount>0 ? static_cast<double>(inputMonitoring_.payload) / inputMonitoring_.logicalCount : 0)
      << "</td>" << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }

  *out << "<tr>"                                                  << std::endl;
  *out << "<td style=\"text-align:center\" colspan=\"2\">"        << std::endl;
  for (FedFragmentFIFOs::iterator it = fedFragmentFIFOs_.begin(), itEnd = fedFragmentFIFOs_.end();
       it != itEnd; ++it)
  {
    it->second.printHtml(out, app_->getApplicationDescriptor()->getURN());
  }
  *out << "</td>"                                                 << std::endl;
  *out << "</tr>"                                                 << std::endl;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
