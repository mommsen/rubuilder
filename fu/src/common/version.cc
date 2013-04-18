#include "rubuilder/fu/version.h"

#include "config/version.h"
#include "pt/version.h"
#include "xcept/version.h"
#include "xdaq/version.h"
#include "xdata/version.h"
#include "xoap/version.h"


GETPACKAGEINFO(rubuilderfu)


void rubuilderfu::checkPackageDependencies()
throw (config::PackageInfo::VersionException)
{
	CHECKDEPENDENCY(config);  
	CHECKDEPENDENCY(pt);  
	CHECKDEPENDENCY(xcept);  
	CHECKDEPENDENCY(xdaq);  
	CHECKDEPENDENCY(xdata);  
	CHECKDEPENDENCY(xoap);  
}


std::set<std::string, std::less<std::string> > rubuilderfu::getPackageDependencies()
{
	std::set<std::string, std::less<std::string> > dependencies;

	ADDDEPENDENCY(dependencies,config); 
	ADDDEPENDENCY(dependencies,pt); 
	ADDDEPENDENCY(dependencies,xcept);
	ADDDEPENDENCY(dependencies,xdaq);
	ADDDEPENDENCY(dependencies,xdata);
	ADDDEPENDENCY(dependencies,xoap);

	return dependencies;
}
