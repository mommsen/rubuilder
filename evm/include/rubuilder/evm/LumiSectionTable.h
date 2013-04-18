#ifndef _rubuilder_evm_LumiSectionTable_h_
#define _rubuilder_evm_LumiSectionTable_h_

#include <boost/shared_ptr.hpp>
#include <ostream>
#include <stdint.h>
#include <string>
#include <map>
#include <vector>

#include "i2o/i2o.h"
#include "rubuilder/evm/EoLSHandler.h"
#include "rubuilder/utils/Constants.h"
#include "rubuilder/utils/EventUtils.h"
#include "toolbox/BSem.h"


namespace rubuilder { namespace evm { // namespace rubuilder::evm

  class LumiSectionTable
  {
  public:
  
    LumiSectionTable(boost::shared_ptr<EoLSHandler>);
    
    void incrementEventsInRuBuilder(const EoLSHandler::LumiSectionPair&);
    void decrementEventsInRuBuilder(const EoLSHandler::LumiSectionPair&);
    
    void clear();
    
  private:
  
    boost::shared_ptr<EoLSHandler> eolsHandler_;

    uint32_t currentLumiSectionNumber_;
    
    typedef std::map<EoLSHandler::LumiSectionPair, uint32_t> LumiSectionMap;
    LumiSectionMap lumiSectionMap_;

    void sendEoLSsignal();
    
  }; // LumiSectionTable

} } // namespace rubuilder::evm


#endif // _rubuilder_evm_LumiSectionTable_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
