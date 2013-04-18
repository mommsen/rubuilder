#ifndef _rubuilder_bu_FuRqstForResource_h_
#define _rubuilder_bu_FuRqstForResource_h_

#include <stdint.h>
#include <ostream>

#include "i2o/i2oDdmLib.h"


namespace rubuilder { namespace bu { // namespace rubuilder::bu

struct FuRqstForResource
{
  I2O_TID fuTid;
  uint32_t fuTransactionId;
  
  FuRqstForResource() :
  fuTid(0),
  fuTransactionId(0)
  {}
};
    
} } // namespace rubuilder::bu

inline std::ostream& operator<<
(
  std::ostream& s,
  rubuilder::bu::FuRqstForResource& rqst
)
{
  s << "Request from FU tid " << rqst.fuTid
    << " with fuTransactionId " << rqst.fuTransactionId;
  return s;
}



#endif // _rubuilder_bu_FuRqstForResource_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
