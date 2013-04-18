#ifndef _rubuilder_bu_SuperFragmentDescriptor_h_
#define _rubuilder_bu_SuperFragmentDescriptor_h_

#include <boost/shared_ptr.hpp>

#include "toolbox/mem/Reference.h"


namespace rubuilder { namespace bu { // namespace rubuilder::bu

/**
 * Super-Fragment descriptor.
 *
 * Linked list of the event data blocks appended so far.
 */
class SuperFragmentDescriptor
{
public:
  SuperFragmentDescriptor(toolbox::mem::Reference* bufRef) :
  head(bufRef), tail(bufRef) { assert(bufRef); }
  
  ~SuperFragmentDescriptor()
  {
    if (head) head->release();
  }

  void append(toolbox::mem::Reference* bufRef)
  {
    tail->setNextReference(bufRef);
    tail = bufRef;
  }

  toolbox::mem::Reference* get() const
  {
    return head;
  }

  toolbox::mem::Reference* duplicate() const
  {
    return head->duplicate();
  }

private:

  toolbox::mem::Reference* head;
  toolbox::mem::Reference* tail;

};

typedef boost::shared_ptr<SuperFragmentDescriptor> SuperFragmentDescriptorPtr;

} } // namespace rubuilder::bu

#endif // _rubuilder_bu_SuperFragmentDescriptor_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
