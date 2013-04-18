#include "rubuilder/tester/version.h"

#include "config/version.h"
#include "pt/version.h"
#include "rubuilder/utils/version.h"
#include "xcept/version.h"
#include "xdaq/version.h"
#include "xdata/version.h"
#include "xoap/version.h"


GETPACKAGEINFO(rubuildertester)


void rubuildertester::checkPackageDependencies()
throw (config::PackageInfo::VersionException)
{
	CHECKDEPENDENCY(config);  
	CHECKDEPENDENCY(pt);  
	CHECKDEPENDENCY(rubuilderutils);  
	CHECKDEPENDENCY(xcept);  
	CHECKDEPENDENCY(xdaq);  
	CHECKDEPENDENCY(xdata);  
	CHECKDEPENDENCY(xoap);  
}


std::set<std::string, std::less<std::string> >
rubuildertester::getPackageDependencies()
{
	std::set<std::string, std::less<std::string> > dependencies;

	ADDDEPENDENCY(dependencies,config); 
	ADDDEPENDENCY(dependencies,pt); 
	ADDDEPENDENCY(dependencies,rubuilderutils); 
	ADDDEPENDENCY(dependencies,xcept);
	ADDDEPENDENCY(dependencies,xdaq);
	ADDDEPENDENCY(dependencies,xdata);
	ADDDEPENDENCY(dependencies,xoap);

	return dependencies;
}
