#include "i2o/utils/AddressMap.h"
#include "rubuilder/evm/EoLSHandler.h"
#include "rubuilder/evm/SMproxy.h"
#include "rubuilder/utils/Exception.h"
#include "toolbox/mem/MemoryPoolFactory.h"


rubuilder::evm::SMproxy::SMproxy
(
  xdaq::Application* app,
  boost::shared_ptr<EoLSHandler> eolsHandler,
  toolbox::mem::Pool* fastCtrlMsgPool
) :
app_(app),
fastCtrlMsgPool_(fastCtrlMsgPool),
logger_(app->getApplicationLogger())
{
  resetMonitoringCounters();
  eolsHandler->registerSMproxy(this);
}


void rubuilder::evm::SMproxy::appendConfigurationItems(utils::InfoSpaceItems& params)
{
  autoDiscoverSM_ = true;
  
  smParams_.clear();
  smParams_.add("autoDiscoverSM", &autoDiscoverSM_);
  smParams_.add("smInstances", &smInstances_);

  params.add(smParams_);
}


void rubuilder::evm::SMproxy::appendMonitoringItems(utils::InfoSpaceItems& items)
{
}


void rubuilder::evm::SMproxy::updateMonitoringItems()
{
}


void rubuilder::evm::SMproxy::resetMonitoringCounters()
{
  boost::mutex::scoped_lock sl(EoLSMonitoringMutex_);
  EoLSMonitoring_.payload = 0;
  EoLSMonitoring_.msgCount = 0;
  EoLSMonitoring_.i2oCount = 0;
}


void rubuilder::evm::SMproxy::printHtml(xgi::Output *out)
{
  *out << "<div>"                                                 << std::endl;
  *out << "<p>SMproxy</p>"                                        << std::endl;
  *out << "<table>"                                               << std::endl;
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\">Monitoring</th>"                     << std::endl;
  *out << "</tr>"                                                 << std::endl;

  *out << "<tr>"                                                  << std::endl;
  *out << "<td colspan=\"2\" style=\"text-align:center\">EoLS msg</td>" << std::endl;
  *out << "</tr>"                                                 << std::endl;

  {
    boost::mutex::scoped_lock sl(EoLSMonitoringMutex_);
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>payload (bytes)</td>"                              << std::endl;
    *out << "<td>" << EoLSMonitoring_.payload << "</td>"            << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>msg count</td>"                                    << std::endl;
    *out << "<td>" << EoLSMonitoring_.msgCount << "</td>"           << std::endl;
    *out << "</tr>"                                                 << std::endl;
    *out << "<tr>"                                                  << std::endl;
    *out << "<td>I2O count</td>"                                    << std::endl;
    *out << "<td>" << EoLSMonitoring_.i2oCount << "</td>"           << std::endl;
    *out << "</tr>"                                                 << std::endl;
  }

  smParams_.printHtml("Configuration", out);

  *out << "</table>"                                              << std::endl;
  *out << "</div>"                                                << std::endl;
}


void rubuilder::evm::SMproxy::getApplicationDescriptors()
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
  
  // If autoDiscoverSM_ is true, the exported parameter "smInstances" is overwritten
  if( autoDiscoverSM_ )
  {
    try
    {
      discoverParticipatingSMs();
    }
    catch(xcept::Exception &e)
    {
      XCEPT_RETHROW(exception::Configuration,
        "Failed to discover SMs for SM descriptor list", e);
    }
  }
  else
  {
    try
    {
      fillParticipatingSMsUsingSMInstances();
    }
    catch(xcept::Exception &e)
    {
      XCEPT_RETHROW(exception::Configuration,
        "Failed to fill SM descriptor list using \"smInstances\"", e);
    }
  }
}


void rubuilder::evm::SMproxy::discoverParticipatingSMs()
{
  std::set< xdaq::ApplicationDescriptor* > sms;
  
  try
  {
    sms =
      app_->getApplicationContext()->
      getDefaultZone()->
      getApplicationDescriptors("StorageManager");
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::Configuration,
      "Failed to get SM application descriptors", e);
  }
  
  // Clear list of participating SMs
  participatingSMs_.clear();
  smInstances_.clear();
  smInstances_.reserve(sms.size());
  
  for (std::set< xdaq::ApplicationDescriptor* >::iterator it = sms.begin(),
         itEnd = sms.end(); it != itEnd; ++it)
  {
    utils::ApplicationDescriptorAndTid sm;
    sm.descriptor = *it;
    try
    {
      sm.tid = i2o::utils::getAddressMap()->getTid(*it);
    }
    catch(xcept::Exception &e)
    {
      std::stringstream oss;
      
      oss << "Failed to get I2O TID for SM ";
      oss << sm.descriptor;
      
      XCEPT_RETHROW(exception::I2O, oss.str(), e);
    }
    
    participatingSMs_.insert(sm);
    smInstances_.push_back((*it)->getInstance());
  }
}


void rubuilder::evm::SMproxy::fillParticipatingSMsUsingSMInstances()
{
  // Clear list of participating SMs
  participatingSMs_.clear();
  
  // Fill list of participating SMs
  for(xdata::Vector<xdata::UnsignedInteger32>::iterator it = smInstances_.begin(),
        itEnd = smInstances_.end(); it != itEnd; ++it)
  {
    utils::ApplicationDescriptorAndTid sm;
    
    try
    {
      sm.descriptor =
        app_->getApplicationContext()->
        getDefaultZone()->
        getApplicationDescriptor("StorageManager", *it);
    }
    catch(xcept::Exception &e)
    {
      std::stringstream oss;
      
      oss << "Failed to get application descriptor for SM";
      oss << *it;
      
      XCEPT_RETHROW(exception::Configuration, oss.str(), e);
    }
    
    try
    {
      sm.tid = i2o::utils::getAddressMap()->getTid(sm.descriptor);
    }
    catch(xcept::Exception &e)
    {
      std::stringstream oss;
      
      oss << "Failed to get I2O TID for SM";
      oss << *it;
      
      XCEPT_RETHROW(exception::I2O, oss.str(), e);
    }
    
    if ( ! participatingSMs_.insert(sm).second )
    {
      std::stringstream oss;
      
      oss << "Particpating SM instance is a duplicate.";
      oss << " Instance:" << it->toString();
      
      XCEPT_RAISE(exception::Configuration, oss.str());
    }
  }
}


void rubuilder::evm::SMproxy::sendEoLSmsg(const I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME* msg)
{
  if ( participatingSMs_.empty() ) return;

  /////////////////////////////////////////////////////////////////////////
  // Make a copy of the end-of-lumisection message under construction    //
  // for each SM and send each copy to its corresponding SM              //
  /////////////////////////////////////////////////////////////////////////
  
  for (ParticipatingSMs::const_iterator it=participatingSMs_.begin(),
         itEnd=participatingSMs_.end();
       it != itEnd; ++it)
  {
    // Create an empty message
    toolbox::mem::Reference* copyBufRef =
      toolbox::mem::getMemoryPoolFactory()->
      getFrame(fastCtrlMsgPool_, sizeof(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME));
    char* copyFrame  = (char*)(copyBufRef->getDataLocation());
        
    // Copy the message under construction into
    // the newly created empty message
    memcpy(
      copyFrame,
      msg,
      sizeof(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME)
    );
        
    // Set the size of the copy
    copyBufRef->setDataSize(sizeof(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME));
        
    // Set the I2O TID target address
    ((I2O_MESSAGE_FRAME*)copyFrame)->TargetAddress = it->tid;
        
    // Send the pairs message to the SM
    try
    {
      app_->getApplicationContext()->
        postFrame(
          copyBufRef,
          app_->getApplicationDescriptor(),
          it->descriptor//,
          //i2oExceptionHandler_,
          //it->descriptor
        );
    }
    catch(xcept::Exception &e)
    {
      std::stringstream oss;
      
      oss << "Failed to send end-of-lumisection message to SM";
      oss << it->descriptor->getInstance();
      oss << ". ";
      
      XCEPT_RETHROW(exception::I2O, oss.str(), e);
    }
  }

  size_t msgCount =  participatingSMs_.size();

  boost::mutex::scoped_lock sl(EoLSMonitoringMutex_);
  EoLSMonitoring_.payload += msgCount * sizeof(I2O_EVM_END_OF_LUMISECTION_MESSAGE_FRAME);
  EoLSMonitoring_.msgCount++;
  EoLSMonitoring_.i2oCount += msgCount;
}



/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
