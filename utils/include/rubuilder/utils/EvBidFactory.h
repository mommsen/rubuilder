#ifndef _rubuilder_utils_EvBidFactory_h_
#define _rubuilder_utils_EvBidFactory_h_

#include <stdint.h>

#include "rubuilder/utils/EvBid.h"

namespace rubuilder { namespace utils { // namespace rubuilder::utils

  class EvBidFactory
  {
  public:
    
    EvBidFactory();
    
    /**
     * Return EvBid for given eventNumber
     */
    EvBid getEvBid(const uint32_t eventNumber);

    /**
     * Reset the counters;
     */
    void reset();
    
  private:
    
    uint32_t previousEventNumber_;
    uint32_t resyncCount_;
    
  };

} } // namespace rubuilder::utils

#endif // _rubuilder_utils_EvBidFactory_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
