#ifndef _rubuilder_evm_TriggerFIFO_h_
#define _rubuilder_evm_TriggerFIFO_h_

#include "rubuilder/utils/OneToOneQueue.h"
#include "toolbox/mem/Reference.h"

namespace rubuilder { namespace evm { // namespace rubuilder::evm

  typedef utils::OneToOneQueue<toolbox::mem::Reference*> TriggerFIFO;

} } // namespace rubuilder::evm

#endif // _rubuilder_evm_TriggerFIFO_h_
