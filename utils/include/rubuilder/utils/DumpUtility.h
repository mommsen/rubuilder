#ifndef _rubuilder_utils_DumpUtility_h_
#define _rubuilder_utils_DumpUtility_h_

#include "toolbox/mem/Reference.h"

#include <iostream>


namespace rubuilder { namespace utils { // namespace rubuilder::utils

class DumpUtility
{
public:
  
  static void dump
  (
    std::ostream&,
    toolbox::mem::Reference*
  );
  
  
private:
  
  static void dumpBlock
  (
    std::ostream&,
    toolbox::mem::Reference*,
    const uint32_t bufferCnt
  );
  
  static void dumpBlockData
  (
    std::ostream&,
    const unsigned char* data,
    uint32_t len
  );
  
}; // class DumpUtility
    
} } // namespace rubuilder::utils

#endif


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
