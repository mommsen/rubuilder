#include "i2o/Method.h"
#include "i2o/utils/AddressMap.h"
#include "interface/shared/i2oXFunctionCodes.h"
#include "rubuilder/utils/Exception.h"
#include "rubuilder/utils/Constants.h"
#include "rubuilder/utils/FragmentSets.h"
#include "rubuilder/utils/RUbroadcaster.h"
#include "rubuilder/utils/UnsignedInteger32Less.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "xcept/tools.h"
#include "xdaq/ApplicationDescriptor.h"

#include <string.h>


rubuilder::utils::RUbroadcaster::RUbroadcaster
(
  xdaq::Application* app,
  toolbox::mem::Pool* fastCtrlMsgPool
) :
app_(app),
fastCtrlMsgPool_(fastCtrlMsgPool),
logger_(app->getApplicationLogger()),
tid_(0),
ruCount_(0)
{}


void rubuilder::utils::RUbroadcaster::initRuInstances(InfoSpaceItems& params)
{
  useFragmentSet_         = false;
  fragmentSetsUrl_        = "";
  fragmentSetId_          = 0;

  getRuInstances();

  params.add("ruInstances", &ruInstances_);
  params.add("useFragmentSet", &useFragmentSet_);
  params.add("fragmentSetsUrl", &fragmentSetsUrl_);
  params.add("fragmentSetId", &fragmentSetId_);
}


void rubuilder::utils::RUbroadcaster::getRuInstances()
{
  boost::mutex::scoped_lock sl(ruInstancesMutex_);

  std::set<xdaq::ApplicationDescriptor*> ruDescriptors;
  
  try
  {
    ruDescriptors =
      app_->getApplicationContext()->
      getDefaultZone()->
      getApplicationDescriptors("rubuilder::ru::Application");
  }
  catch(xcept::Exception &e)
  {
    LOG4CPLUS_WARN(logger_,
      "There are no RU application descriptors : "
      << xcept::stdformat_exception_history(e));
    
    XCEPT_DECLARE_NESTED(exception::Configuration,
      sentinelException,
      "There are no RU application descriptors", e);
    app_->notifyQualified("warn",sentinelException);
    
    ruDescriptors.clear();
  }

  ruInstances_.clear();
  
  for (std::set<xdaq::ApplicationDescriptor*>::const_iterator
         it=ruDescriptors.begin(), itEnd =ruDescriptors.end();
       it != itEnd; ++it)
  {
    ruInstances_.push_back((*it)->getInstance());
  }

  std::sort(ruInstances_.begin(), ruInstances_.end(),
    UnsignedInteger32Less());
}


void rubuilder::utils::RUbroadcaster::getApplicationDescriptors()
{
  try
  {
    tid_ = i2o::utils::getAddressMap()->
      getTid(app_->getApplicationDescriptor());
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      "Failed to get I2O TID for this application.", e);
  }
  
  // The use of a fragment set overrides the exported parameter "ruInstances"
  if ( useFragmentSet_ )
  {
    try
    {
      fillParticipatingRUsUsingFragmentSet();
    }
    catch(xcept::Exception &e)
    {
      XCEPT_RETHROW(exception::Configuration,
        "Failed to fill RU descriptor list using a fragment set", e);
    }
  }
  else
  {
    try
    {
      fillParticipatingRUsUsingRuInstances();
    }
    catch(xcept::Exception &e)
    {
      XCEPT_RETHROW(exception::Configuration,
        "Failed to fill RU descriptor list using \"ruInstances\"", e);
    }
  }
}


void rubuilder::utils::RUbroadcaster::fillParticipatingRUsUsingFragmentSet()
{
  FragmentSets fragmentSets;

  if ( fragmentSetsUrl_.value_.empty() )
  {
    XCEPT_RAISE(exception::Configuration,
      "fragmentSetsUrl has no value");
  }
  
  try
  {
    fragmentSets.init(fragmentSetsUrl_.value_);
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      "Failed to initialise map of fragment sets", e);
  }
  
  std::set<int, std::less<int> > fragmentSet;
  
  try
  {
    fragmentSet = fragmentSets.getFragmentSet(fragmentSetId_.value_);
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    
    oss << "Failed to get fragment set with";
    oss << " id: " << fragmentSetId_.toString();
    
    XCEPT_RETHROW(exception::Configuration, oss.str(), e);
  }
  
  std::string fragmentSetName = "";
  
  try
  {
    fragmentSetName = fragmentSets.getName(fragmentSetId_.value_);
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    
    oss << "Failed to get name of fragment set with";
    oss << " id: " << fragmentSetId_.toString();
    
    XCEPT_RETHROW(exception::Configuration, oss.str(), e);
  }
  
  // Clear list of participating RUs
  participatingRUs_.clear();

  // Fill list of participating RUs
  for (std::set<int, std::less<int> >::iterator it = fragmentSet.begin(),
         itEnd = fragmentSet.end();
       it != itEnd; ++it)
  {
    fillRUInstance(*it);
  }
  ruCount_ = participatingRUs_.size();
}


void rubuilder::utils::RUbroadcaster::fillParticipatingRUsUsingRuInstances()
{  
  boost::mutex::scoped_lock sl(ruInstancesMutex_);

  // Clear list of participating RUs
  participatingRUs_.clear();

  // Fill list of participating RUs
  for (RUInstances::iterator it = ruInstances_.begin(),
         itEnd = ruInstances_.end();
       it != itEnd; ++it)
  {
    fillRUInstance(*it);
  }
  ruCount_ = participatingRUs_.size();
}


void rubuilder::utils::RUbroadcaster::fillRUInstance(xdata::UnsignedInteger32 instance)
{
  ApplicationDescriptorAndTid ru;
  
  try
  {
    ru.descriptor =
      app_->getApplicationContext()->
      getDefaultZone()->
      getApplicationDescriptor("rubuilder::ru::Application", instance);
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    
    oss << "Failed to get application descriptor for RU ";
    oss << instance.toString();
    
    XCEPT_RETHROW(exception::Configuration, oss.str(), e);
  }
  
  try
  {
    ru.tid = i2o::utils::getAddressMap()->getTid(ru.descriptor);
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    
    oss << "Failed to get I2O TID for RU ";
    oss << instance.toString();
    
    XCEPT_RETHROW(exception::I2O, oss.str(), e);
  }
  
  if ( ! participatingRUs_.insert(ru).second )
  {
    std::stringstream oss;
    
    oss << "Participating RU instance is a duplicate.";
    oss << " Instance:" << instance.toString();
    
    XCEPT_RAISE(exception::Configuration, oss.str());
  }
}

void rubuilder::utils::RUbroadcaster::sendToAllRUs
(
  toolbox::mem::Reference* bufRef,
  const size_t bufSize
)
{
  ////////////////////////////////////////////////////////////
  // Make a copy of the pairs message under construction    //
  // for each RU and send each copy to its corresponding RU //
  ////////////////////////////////////////////////////////////

  for ( RUDescriptorsAndTids::const_iterator it = participatingRUs_.begin(),
          itEnd = participatingRUs_.end();
        it != itEnd; ++it)
  {
    // Create an empty request message
    toolbox::mem::Reference* copyBufRef =
      toolbox::mem::getMemoryPoolFactory()->
      getFrame(fastCtrlMsgPool_, bufSize);
    char* copyFrame  = (char*)(copyBufRef->getDataLocation());

    // Copy the message under construction into
    // the newly created empty message
    memcpy(
      copyFrame,
      bufRef->getDataLocation(),
      bufRef->getDataSize()
    );

    // Set the size of the copy
    copyBufRef->setDataSize(bufRef->getDataSize());

    // Set the I2O TID target address
    ((I2O_MESSAGE_FRAME*)copyFrame)->TargetAddress = it->tid;

    // Send the pairs message to the RU
    try
    {
      app_->getApplicationContext()->
        postFrame(
          copyBufRef,
          app_->getApplicationDescriptor(),
          it->descriptor //,
          //i2oExceptionHandler_,
          //it->descriptor
        );
    }
    catch(xcept::Exception &e)
    {
      std::stringstream oss;
      
      oss << "Failed to send message to RU";
      oss << it->descriptor->getInstance();

      XCEPT_RETHROW(exception::I2O, oss.str(), e);
    }
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
