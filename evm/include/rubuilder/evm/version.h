#ifndef _rubuilder_evm_version_h_
#define _rubuilder_evm_version_h_

#include "config/PackageInfo.h"

#define RUBUILDEREVM_VERSION_MAJOR 5
#define RUBUILDEREVM_VERSION_MINOR 0
#define RUBUILDEREVM_VERSION_PATCH 3
#define RUBUILDEREVM_PREVIOUS_VERSIONS "4.3.0,4.3.1,4.3.2"


#define RUBUILDEREVM_VERSION_CODE PACKAGE_VERSION_CODE(RUBUILDEREVM_VERSION_MAJOR,RUBUILDEREVM_VERSION_MINOR,RUBUILDEREVM_VERSION_PATCH)
#ifndef RUBUILDEREVM_PREVIOUS_VERSIONS
#define RUBUILDEREVM_FULL_VERSION_LIST  PACKAGE_VERSION_STRING(RUBUILDEREVM_VERSION_MAJOR,RUBUILDEREVM_VERSION_MINOR,RUBUILDEREVM_VERSION_PATCH)
#else 
#define RUBUILDEREVM_FULL_VERSION_LIST  RUBUILDEREVM_PREVIOUS_VERSIONS "," PACKAGE_VERSION_STRING(RUBUILDEREVM_VERSION_MAJOR,RUBUILDEREVM_VERSION_MINOR,RUBUILDEREVM_VERSION_PATCH)
#endif 

namespace rubuilderevm
{
  const std::string package = "rubuilderevm";
  const std::string versions = RUBUILDEREVM_FULL_VERSION_LIST;
  const std::string version = PACKAGE_VERSION_STRING(RUBUILDEREVM_VERSION_MAJOR,RUBUILDEREVM_VERSION_MINOR,RUBUILDEREVM_VERSION_PATCH);
  const std::string description = "Event manager (EVM)";
  const std::string summary = "Event manager (EVM)";
  const std::string authors = "Remi Mommsen, Steven Murray";
  const std::string link = "http://cms-ru-builder.web.cern.ch/cms-ru-builder";
  
  config::PackageInfo getPackageInfo();
  
  void checkPackageDependencies()
  throw (config::PackageInfo::VersionException);
  
  std::set<std::string, std::less<std::string> > getPackageDependencies();
}

#endif


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
