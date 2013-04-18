#ifndef _rubuilder_tester_version_h_
#define _rubuilder_tester_version_h_

#include "config/PackageInfo.h"

#define RUBUILDERTESTER_VERSION_MAJOR 1
#define RUBUILDERTESTER_VERSION_MINOR 13
#define RUBUILDERTESTER_VERSION_PATCH 3
#define RUBUILDERTESTER_PREVIOUS_VERSIONS "1.11.0,1.12.0,1.12.1,1.13.0,1.13.1,1.13.2"


#define RUBUILDERTESTER_VERSION_CODE PACKAGE_VERSION_CODE(RUBUILDERTESTER_VERSION_MAJOR,RUBUILDERTESTER_VERSION_MINOR,RUBUILDERTESTER_VERSION_PATCH)
#ifndef RUBUILDERTESTER_PREVIOUS_VERSIONS
#define RUBUILDERTESTER_FULL_VERSION_LIST  PACKAGE_VERSION_STRING(RUBUILDERTESTER_VERSION_MAJOR,RUBUILDERTESTER_VERSION_MINOR,RUBUILDERTESTER_VERSION_PATCH)
#else 
#define RUBUILDERTESTER_FULL_VERSION_LIST  RUBUILDERTESTER_PREVIOUS_VERSIONS "," PACKAGE_VERSION_STRING(RUBUILDERTESTER_VERSION_MAJOR,RUBUILDERTESTER_VERSION_MINOR,RUBUILDERTESTER_VERSION_PATCH)
#endif 

namespace rubuildertester
{
    const std::string package = "rubuildertester";
    const std::string versions = RUBUILDERTESTER_FULL_VERSION_LIST;
    const std::string description = "Tests the RU builder applications";
    const std::string summary = "Tests the RU builder applications";
    const std::string authors = "Steven Murray, Remi Mommsen";
    const std::string link = "http://cms-ru-builder.web.cern.ch/cms-ru-builder";

    config::PackageInfo getPackageInfo();

    void checkPackageDependencies()
    throw (config::PackageInfo::VersionException);

    std::set<std::string, std::less<std::string> > getPackageDependencies();
}

#endif
