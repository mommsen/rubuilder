#ifndef _rubuilder_utils_OneToOneQueueCollection_h_
#define _rubuilder_utils_OneToOneQueueCollection_h_

#include <stdexcept>
#include <stdint.h>
#include <vector>

#ifdef RUBUILDER_BOOST
#include <boost/thread/mutex.hpp>
#else
#include <boost/thread/shared_mutex.hpp>
#endif

#include "rubuilder/utils/OneToOneQueue.h"
#include "xgi/Output.h"


namespace rubuilder { namespace utils
{

  /**
   * \ingroup xdaqApps
   * \brief A collection of OneToOneQueues
   */

  template <class T>
  class OneToOneQueueCollection
  {
  public:

    OneToOneQueueCollection(const std::string& name);
    OneToOneQueueCollection(const std::string& name, const uint32_t size);

    /**
     * Enqueue the element in the queue with the given index.
     * Returns false if the element cannot be enqueued
     */
    bool enq(const uint32_t index, const T&);

    /**
     * Dequeue an element from the queue with the given index.
     * Return false if no element can be dequeued.
     */
    bool deq(const uint32_t index, T&);

    /**
     * Dequeue an element from the next queue which has
     * an element. The queues are treated in a round-robin scheme.
     * Return false if no element can be dequeued.
     */
    bool deq(T&);

    /**
     * Dequeue an element from the queue which has the most
     * elements queued.
     * Return false if no element can be dequeued.
     */
    bool deqFromFullest(T&);

    /**
     * Return the size of the queues
     */
    uint32_t size() const;

    /**
     * Returns true if all queues are empty.
     */
    bool empty() const;

    /**
     * Clear all queues and removes the queues from the collection
     */
    void clear();

    /**
     * Resizes the queues.
     * Throws an exception if any queue is not empty.
     */
    void resize(const uint32_t size);

    /**
     * Print summary icon as HTML.
     */
    void printHtml(xgi::Output*, const std::string& urn);

    /**
     * Print horizontally all elements as HTML.
     */
    void printQueuesSummaryHtml(xgi::Output*, const std::string& urn);

    /**
     * Print horizontally upto nbElementsToPrint as HTML.
     */
    void printQueuesSummaryHtml(xgi::Output*, const std::string& urn, const uint32_t nbElementsToPrint);
 
    /**
     * Print vertically all elements as HTML.
     */
    void printVerticalHtml(xgi::Output*);

    /**
     * Print vertically upto nbElementsToPrint as HTML.
     */
    void printVerticalHtml(xgi::Output*, const uint32_t nbElementsToPrint);
 

  private:

    const std::string name_;
    uint32_t size_;
    #ifdef RUBUILDER_BOOST
    boost::mutex mutex_;
    #else
    boost::shared_mutex mutex_;
    #endif

    typedef std::map< uint32_t,OneToOneQueue<T> > Collection;
    Collection collection_;
    typename Collection::iterator nextQueue_;
  };

  
  //------------------------------------------------------------------
  // Implementation follows
  //------------------------------------------------------------------

  template <class T>
  OneToOneQueueCollection<T>::OneToOneQueueCollection(const std::string& name) :
  name_(name),
  size_(1)
  {}
  
  
  template <class T>
  OneToOneQueueCollection<T>::OneToOneQueueCollection(const std::string& name, const uint32_t size) :
  name_(name),
  size_(size)
  {}


  template <class T>
  inline uint32_t OneToOneQueueCollection<T>::size() const
  {
    return size_;  
  }


  template <class T>
  bool OneToOneQueueCollection<T>::empty() const
  {
    #ifdef RUBUILDER_BOOST
    boost::mutex::scoped_lock sl(mutex_);
    #else
    boost::shared_lock<boost::shared_mutex> sharedLock(mutex_);
    #endif

    typename Collection::const_iterator it = collection_.begin();
    const typename Collection::const_iterator itEnd = collection_.end();
    while ( it != itEnd )
    {
      if ( ! it->second.empty() ) return false;
      ++it;
    }
    return true;
  }
  
  
  template <class T>
  void OneToOneQueueCollection<T>::resize(const uint32_t size)
  {
    #ifdef RUBUILDER_BOOST
    boost::mutex::scoped_lock sl(mutex_);
    #else
    boost::shared_lock<boost::shared_mutex> sharedLock(mutex_);
    #endif

    for (
      typename Collection::iterator it = collection_.begin(), itEnd = collection_.end();
      it != itEnd; ++it
    )
    {
      it->second.resize(size);
    }
    size_ = size;
  }
  
  
  template <class T>
  bool OneToOneQueueCollection<T>::enq(const uint32_t index, const T& element)
  {
    #ifdef RUBUILDER_BOOST
    boost::mutex::scoped_lock sl(mutex_);
    #else
    boost::upgrade_lock<boost::shared_mutex> upgradeLock(mutex_);
    #endif

    typename Collection::iterator pos = collection_.lower_bound(index);
    if ( pos == collection_.end() || collection_.key_comp()(index, pos->first) )
    {
      #ifndef RUBUILDER_BOOST
      boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(upgradeLock);
      #endif
      std::ostringstream queueName;
      queueName << name_ << "_" << index;
      pos = collection_.insert(pos, typename Collection::value_type(index,
          OneToOneQueue<T>(queueName.str(), size_)));
      nextQueue_ = pos;
    }
    return pos->second.enq(element);
  }
  
  
  template <class T>
  bool OneToOneQueueCollection<T>::deq(const uint32_t index, T& element)
  {
    #ifdef RUBUILDER_BOOST
    boost::mutex::scoped_lock sl(mutex_);
    #else
    boost::shared_lock<boost::shared_mutex> sharedLock(mutex_);
    #endif

    const typename Collection::iterator pos = collection_.find(index);
    if ( pos == collection_.end() ) return false;
    
    return pos->second.deq(element);
  }
  
  
  template <class T>
  bool OneToOneQueueCollection<T>::deq(T& element)
  {
    #ifdef RUBUILDER_BOOST
    boost::mutex::scoped_lock sl(mutex_);
    #else
    boost::shared_lock<boost::shared_mutex> sharedLock(mutex_);
    #endif

    if ( collection_.empty() ) return false;

    typename Collection::iterator it = nextQueue_;
    
    // Use a post-increment here: the current queue is used to dequeue,
    // but the iterator is incremented regardless if an element has
    // been dequeued or not.
    while ( ! (it++)->second.deq(element) )
    {
      if ( it == collection_.end() ) it = collection_.begin();
      if ( it == nextQueue_ ) return false; // Gone once through all queues
    }

    if ( it == collection_.end() )
      nextQueue_ = collection_.begin();
    else
      nextQueue_ = it;

    return true;
  }


  template <class T>
  bool OneToOneQueueCollection<T>::deqFromFullest(T& element)
  {
    #ifdef RUBUILDER_BOOST
    boost::mutex::scoped_lock sl(mutex_);
    #else
    boost::shared_lock<boost::shared_mutex> sharedLock(mutex_);
    #endif

    if ( collection_.empty() ) return false;
    
    size_t maxElements = 0;
    typename Collection::iterator fullestQueue = collection_.begin();
    
    for ( typename Collection::iterator it = collection_.begin(), itEnd = collection_.end();
          it != itEnd; ++it )
    {
      const size_t elements =  it->second.elements();
      if ( elements > maxElements )
      {
        maxElements = elements;
        fullestQueue = it;
      }
    }
    
    return fullestQueue->second.deq(element);
  }


  template <class T>
  void OneToOneQueueCollection<T>::printHtml(xgi::Output *out, const std::string& urn)
  {
    uint32_t minElements = size_;
    uint32_t maxElements = 0;
    double sum = 0;
    uint32_t queueCount = 0;
    
    {
      #ifdef RUBUILDER_BOOST
      boost::mutex::scoped_lock sl(mutex_);
      #else
      boost::shared_lock<boost::shared_mutex> sharedLock(mutex_);
      #endif
      
      for (
        typename Collection::iterator it = collection_.begin(), itEnd = collection_.end();
        it != itEnd; ++it
      )
      {
        const uint32_t elements = it->second.elements();
        if ( elements < minElements ) minElements = elements;
        if ( elements > maxElements ) maxElements = elements;
        sum += elements;
        ++queueCount;
      }
    }

    if ( queueCount == 0 ) minElements = 0;
    const double mean = queueCount>0 ? sum/queueCount : 0;
    const double lowestFillFraction = 100. * minElements / size_;
    const double meanFillFraction = 100. * mean / size_;
    const double maxFillFraction = 100. * maxElements / size_;
    
    *out << "<div class=\"queue\">" << std::endl;
    *out << "<table onclick=\"window.open('/" << urn << "/" << name_ << "','_blank')\" "
      << "onmouseover=\"this.style.cursor = 'pointer'; "
      << "document.getElementById('" << name_ << "').style.visibility = 'visible';\" "
      << "onmouseout=\"document.getElementById('" << name_ << "').style.visibility = 'hidden';\">" << std::endl;
    *out << "<colgroup>" << std::endl;
    *out << "<col width=\"" << static_cast<unsigned int>( lowestFillFraction + 0.5 ) << "%\"/>" << std::endl;
    *out << "<col width=\"" << static_cast<unsigned int>( (meanFillFraction-lowestFillFraction) + 0.5 ) << "%\"/>" << std::endl;
    *out << "<col width=\"" << static_cast<unsigned int>( (maxFillFraction-meanFillFraction) + 0.5 ) << "%\"/>" << std::endl;
    *out << "<col width=\"" << static_cast<unsigned int>( (100-maxFillFraction) + 0.5 ) << "%\"/>" << std::endl;
    *out << "</colgroup>" << std::endl;
    
    *out << "<tr>" << std::endl;
    *out << "<th colspan=\"4\">" << name_ << " (" << queueCount << " queues)</th>" << std::endl;
    *out << "</tr>" << std::endl;
    
    *out << "<tr>" << std::endl;
    *out << "<td style=\"height:1em;background:#97A5E0\"></td>" << std::endl;
    *out << "<td style=\"background:#8178fe\"></td>" << std::endl;
    *out << "<td style=\"background:#4671C7\"></td>" << std::endl;
    *out << "<td style=\"background:#feffd6\"></td>" << std::endl;
    *out << "</tr>" << std::endl;
    
    *out << "<tr>" << std::endl;
    *out << "<td colspan=\"4\" style=\"text-align:center\">[" <<
      minElements << "," << static_cast<int>( mean + 0.5 ) << "," << maxElements <<
      "] / " << size_ << "</td>" << std::endl;
    *out << "</tr>" << std::endl;
    
    *out << "</table>" << std::endl;
    
    *out << "<div id=\"" << name_ << "\" class=\"queuefloat\">" << std::endl;
    printQueuesSummaryHtml(out, urn, 10);
    *out << "</div>" << std::endl;
    
    *out << "</div>" << std::endl;
  }


  template <class T>
  void OneToOneQueueCollection<T>::printQueuesSummaryHtml
  (
    xgi::Output *out,
    const std::string& urn
  )
  {
    printQueuesSummaryHtml( out, urn, size() );
  }
  
  
  template <class T>
  void OneToOneQueueCollection<T>::printQueuesSummaryHtml
  (
    xgi::Output  *out,
    const std::string& urn,
    const uint32_t nbQueuesToPrint
  )
  {
    #ifdef RUBUILDER_BOOST
    boost::mutex::scoped_lock sl(mutex_);
    #else
    boost::shared_lock<boost::shared_mutex> sharedLock(mutex_);
    #endif

    typename Collection::iterator it = nextQueue_;
    
    const uint32_t nbQueues = std::min(nbQueuesToPrint, static_cast<uint32_t>(collection_.size()));

    if ( nbQueues == 0 ) return;

    *out << "<table>" << std::endl;
    *out << "<colgroup>" << std::endl;
    *out << "<col width=\"15%\"/>" << std::endl;
    *out << "<col width=\"70%\"/>" << std::endl;
    *out << "<col width=\"15%\"/>" << std::endl;
    *out << "</colgroup>" << std::endl;

    for ( uint32_t i = 0; i < nbQueues; ++i )
    {
      const uint32_t cachedSize = it->second.size(); 
      const uint32_t cachedElements = it->second.elements();
      const double fillFraction = cachedSize > 0 ? 100. * cachedElements / cachedSize : 0;

      *out << "<tr>" << std::endl;
      *out << "<td style=\"text-align:left\">" << it->first << "</td>" << std::endl;

      *out << "<td>" << std::endl;
      *out << "<table>" << std::endl;
      *out << "<colgroup>" << std::endl;
      *out << "<col width=\"" << static_cast<unsigned int>( fillFraction + 0.5 ) << "%\"/>" << std::endl;
      *out << "<col width=\"" << static_cast<unsigned int>( (100-fillFraction) + 0.5 ) << "%\"/>" << std::endl;
      *out << "</colgroup>" << std::endl;
      *out << "<tr>" << std::endl;
      *out << "<td style=\"height:1em;background:#8178fe\"></td>" << std::endl;
      *out << "<td style=\"background:#feffd6\"></td>" << std::endl;
      *out << "</tr>" << std::endl;
      *out << "</table>" << std::endl;
      *out << "</td>" << std::endl;

      *out << "<td style=\"text-align:right\">" << cachedElements << "</td>" << std::endl;
      *out << "</tr>" << std::endl;
      
      if ( ++it == collection_.end() ) it = collection_.begin();
    }
   
    *out << "</table>" << std::endl;
  }

  
  template <class T>
  void OneToOneQueueCollection<T>::printVerticalHtml
  (
    xgi::Output *out
  )
  {
    printVerticalHtml( out, size() );
  }
  
  
  template <class T>
  void OneToOneQueueCollection<T>::printVerticalHtml
  (
    xgi::Output  *out,
    const uint32_t nbElementsToPrint
  )
  {
    #ifdef RUBUILDER_BOOST
    boost::mutex::scoped_lock sl(mutex_);
    #else
    boost::shared_lock<boost::shared_mutex> sharedLock(mutex_);
    #endif

    if ( collection_.empty() ) return;
    
    *out << "<table>" << std::endl;
    *out << "<tr>" << std::endl;
    
    for (
      typename Collection::iterator it = collection_.begin(), itEnd = collection_.end();
      it != itEnd; ++it
    )
    {
      *out << "<td>" << std::endl;
      it->second.printVerticalHtml(out, nbElementsToPrint);
      *out << "</td>" << std::endl;
    }
    
    *out << "</tr>" << std::endl;
    *out << "</table>" << std::endl;
  }
  
  
}} // namespace rubuilder::utils

#endif // _rubuilder_utils_OneToOneQueueCollection_h_

/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
