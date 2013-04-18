#include "rubuilder/evm/version.h"

#include "config/version.h"
#include "pt/version.h"
#include "rubuilder/utils/version.h"
#include "xcept/version.h"
#include "xdaq/version.h"
#include "xdaq2rc/version.h"
#include "xdata/version.h"
#include "xoap/version.h"


GETPACKAGEINFO(rubuilderevm)


void rubuilderevm::checkPackageDependencies()
throw (config::PackageInfo::VersionException)
{
  CHECKDEPENDENCY(config);  
  CHECKDEPENDENCY(pt);  
  CHECKDEPENDENCY(rubuilderutils);  
  CHECKDEPENDENCY(xcept);  
  CHECKDEPENDENCY(xdaq);  
  CHECKDEPENDENCY(xdaq2rc);  
  CHECKDEPENDENCY(xdata);  
  CHECKDEPENDENCY(xoap);  
}


std::set<std::string, std::less<std::string> > rubuilderevm::getPackageDependencies()
{
  std::set<std::string, std::less<std::string> > dependencies;
  
  ADDDEPENDENCY(dependencies,config); 
  ADDDEPENDENCY(dependencies,pt); 
  ADDDEPENDENCY(dependencies,rubuilderutils); 
  ADDDEPENDENCY(dependencies,xcept);
  ADDDEPENDENCY(dependencies,xdaq);
  ADDDEPENDENCY(dependencies,xdaq2rc);
  ADDDEPENDENCY(dependencies,xdata);
  ADDDEPENDENCY(dependencies,xoap);
  
  return dependencies;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
