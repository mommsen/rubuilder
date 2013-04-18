#include "rubuilder/evm/LumiSectionTable.h"
#include "rubuilder/utils/Exception.h"


rubuilder::evm::LumiSectionTable::LumiSectionTable(boost::shared_ptr<EoLSHandler> eolsHandler) :
eolsHandler_(eolsHandler)
{}


void rubuilder::evm::LumiSectionTable::incrementEventsInRuBuilder
(
  const EoLSHandler::LumiSectionPair& ls
)
{
  if ( ls.lumiSection == 0 ) return;
  
  currentLumiSectionNumber_ = ls.lumiSection;
  
  std::string errorMsg =
    "Failed to add lumi section to lumiSectionMap";
  
  try
  {
    ++lumiSectionMap_[ls];
  }
  catch (std::exception e)
  {
    errorMsg += ": ";
    errorMsg += e.what();
    XCEPT_RAISE(exception::L1Trigger, errorMsg);
  }
  catch (...)
  {
    XCEPT_RAISE(exception::L1Trigger, errorMsg);
  }
}


void rubuilder::evm::LumiSectionTable::decrementEventsInRuBuilder
(
  const EoLSHandler::LumiSectionPair& ls
)
{
  if ( ls.lumiSection == 0 ) return;
  
  std::stringstream errorMsg;
  errorMsg << 
    "Failed to update lumi section info in lumiSectionMap for lumi section " <<
    ls.lumiSection;
  
  try
  {
    LumiSectionMap::iterator pos = lumiSectionMap_.find(ls);
    
    if ( pos == lumiSectionMap_.end() )
    {
      std::stringstream oss;
      
      oss << "Tried to free an event id with unknown lumi section " << ls.lumiSection;
      XCEPT_RAISE(exception::EventOrder, oss.str());
    }
    
    if ( pos->second > 0 ) 
    {
      //Decrement the number of events being processed for this lumi section
      --(pos->second);
    }
    else
    {
      std::stringstream oss;
      
      oss << "Tried to free more events than allocated for lumi section " << ls.lumiSection;
      XCEPT_RAISE(exception::EventOrder, oss.str());
    }
    
    sendEoLSsignal();
  }
  catch (xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::L1Trigger, errorMsg.str(), e);
  }
  catch (std::exception e)
  {
    errorMsg << ": ";
    errorMsg << e.what();
    XCEPT_RAISE(exception::L1Trigger, errorMsg.str());
  }
  catch (...)
  {
    XCEPT_RAISE(exception::L1Trigger, errorMsg.str());
  }
}


void rubuilder::evm::LumiSectionTable::sendEoLSsignal()
{
  LumiSectionMap::iterator it = lumiSectionMap_.begin();
  while ( it != lumiSectionMap_.end() )
  {
    if ( it->second == 0 && it->first.lumiSection < currentLumiSectionNumber_ )
      // all events in this LS are processed and the trigger is sending a later LS,
      // i.e. no more events for this LS will arrive
    {
      eolsHandler_->send(it->first);
      lumiSectionMap_.erase(it++);
    }
    else
    {
      ++it;
    }
  }
}


void rubuilder::evm::LumiSectionTable::clear()
{
  lumiSectionMap_.clear();
  currentLumiSectionNumber_ = 0;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
