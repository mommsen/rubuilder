#ifndef _rubuilder_ru_SuperFragment_h_
#define _rubuilder_ru_SuperFragment_h_

//#define FEDLIST_BITSET

#include <boost/shared_ptr.hpp>

#include <stdint.h>
#ifdef FEDLIST_BITSET
#include <bitset>
#else
#include <vector>
#endif

#include "interface/shared/fed_header.h"
#include "rubuilder/utils/EvBid.h"
#include "toolbox/mem/Reference.h"


namespace rubuilder {
  namespace ru {
    
    /**
     * \ingroup xdaqApps
     * \brief Represent a super fragment
     */
    
    class SuperFragment
    {
    public:

      #ifdef FEDLIST_BITSET
      typedef std::bitset<FED_SOID_WIDTH> FEDlist;
      #else
      typedef std::vector<uint16_t> FEDlist;
      #endif
      
      SuperFragment();
      SuperFragment(const utils::EvBid&, const FEDlist&);
      
      ~SuperFragment();
      
      /**
       * Append the toolbox::mem::Reference to the fragment.
       * Return false if the FED id is not expected.
       */
      bool append(uint16_t fedId, toolbox::mem::Reference*);

      /**
       * Return the head of the toolbox::mem::Reference chain
       */
      toolbox::mem::Reference* head() const
      { return head_; }

      /**
       * Return the size of the super fragment
       */
      size_t getSize() const
      { return size_; }

      /**
       * Return the event-builder id of the super fragment
       */
      utils::EvBid getEvBid() const
      { return evbId_; }

      /**
       * Return true if there's a valid super fragment
       */
      bool isValid() const
      { return ( size_ > 0 && head_ ); }

      /**
       * Return true if the super fragment is complete
       */
      bool isComplete() const
      #ifdef FEDLIST_BITSET
      { return remainingFEDs_.none(); }
      #else
      { return remainingFEDs_.empty(); }
      #endif
      
      
    private:
      
      #ifndef FEDLIST_BITSET
      bool checkFedId(const uint16_t fedId);
      #endif
      
      utils::EvBid evbId_;
      FEDlist remainingFEDs_;
      size_t size_;
      toolbox::mem::Reference* head_;
      toolbox::mem::Reference* tail_;
      
    }; // SuperFragment
    
    typedef boost::shared_ptr<SuperFragment> SuperFragmentPtr;
    
  } } // namespace rubuilder::ru


#endif // _rubuilder_ru_SuperFragment_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
