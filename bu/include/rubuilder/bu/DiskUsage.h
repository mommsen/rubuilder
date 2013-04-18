#ifndef _rubuilder_bu_DiskUsage_h_
#define _rubuilder_bu_DiskUsage_h_

#ifdef RUBUILDER_BOOST
#include "rubuilder/boost/filesystem/convenience.hpp"
#else
#include <boost/filesystem/convenience.hpp>
#endif
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <stdint.h>
#include <sys/statfs.h>


namespace rubuilder { namespace bu { // namespace rubuilder::bu

  /**
   * \ingroup xdaqApps
   * \brief Monitor disk usage
   */
 
  class DiskUsage
  {
  public:
    
    DiskUsage
    (
      const boost::filesystem::path& path,
      const double highWaterMark,
      const double lowWaterMark
    );
  
    ~DiskUsage();

    /**
     * Update the information about the disk
     * Returns false if the disk cannot be accesssed
     */
    bool update();

    /**
     * Returns true if the disk usage is exceeding the high-water mark.
     * Once the high-water mark has been exceeded, the lower-water mark
     * has be be reached before it returns false again.
     */
    bool tooHigh();
    
    /**
     * Return the disk size in GB
     */
    double diskSizeGB();
    
    /**
     * Return the relative usage of the disk in percent
     */
    double relDiskUsage();
    
    
  private:
    
    void doStatFs();

    const boost::filesystem::path path_;
    const double highWaterMark_; 
    const double lowWaterMark_; 

    boost::mutex mutex_;
    int retVal_;
    struct statfs64 statfs_;
    bool tooHigh_;
    
    //  : path(p), highWaterMark(highWaterMark), lowWaterMark(lowWaterMark) {};
  };
    
  typedef boost::shared_ptr<DiskUsage> DiskUsagePtr;
  
} } // namespace rubuilder::bu

#endif // _rubuilder_bu_DiskUsage_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
