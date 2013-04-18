#include "rubuilder/utils/version.h"

#include "config/version.h"
#include "interface/shared/version.h"
#include "pt/version.h"
#include "xcept/version.h"
#include "xdaq/version.h"
#include "xdata/version.h"
#include "xoap/version.h"


GETPACKAGEINFO(rubuilderutils)


void rubuilderutils::checkPackageDependencies()
throw (config::PackageInfo::VersionException)
{
	CHECKDEPENDENCY(config);  
	CHECKDEPENDENCY(interfaceshared);
	CHECKDEPENDENCY(xcept);  
	CHECKDEPENDENCY(xdaq);  
	CHECKDEPENDENCY(xdata);  
	CHECKDEPENDENCY(xoap);  
}


std::set<std::string, std::less<std::string> > rubuilderutils::getPackageDependencies()
{
	std::set<std::string, std::less<std::string> > dependencies;

	ADDDEPENDENCY(dependencies,config); 
	ADDDEPENDENCY(dependencies,interfaceshared); 
	ADDDEPENDENCY(dependencies,xcept);
	ADDDEPENDENCY(dependencies,xdaq);
	ADDDEPENDENCY(dependencies,xdata);
	ADDDEPENDENCY(dependencies,xoap);

	return dependencies;
}
