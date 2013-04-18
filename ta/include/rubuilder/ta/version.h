#ifndef _rubuilder_ta_version_h_
#define _rubuilder_ta_version_h_

#include "config/PackageInfo.h"

#define RUBUILDERTA_VERSION_MAJOR 2
#define RUBUILDERTA_VERSION_MINOR 0
#define RUBUILDERTA_VERSION_PATCH 0
#undef RUBUILDERTA_PREVIOUS_VERSIONS


#define RUBUILDERTA_VERSION_CODE PACKAGE_VERSION_CODE(RUBUILDERTA_VERSION_MAJOR,RUBUILDERTA_VERSION_MINOR,RUBUILDERTA_VERSION_PATCH)
#ifndef RUBUILDERTA_PREVIOUS_VERSIONS
#define RUBUILDERTA_FULL_VERSION_LIST  PACKAGE_VERSION_STRING(RUBUILDERTA_VERSION_MAJOR,RUBUILDERTA_VERSION_MINOR,RUBUILDERTA_VERSION_PATCH)
#else 
#define RUBUILDERTA_FULL_VERSION_LIST  RUBUILDERTA_PREVIOUS_VERSIONS "," PACKAGE_VERSION_STRING(RUBUILDERTA_VERSION_MAJOR,RUBUILDERTA_VERSION_MINOR,RUBUILDERTA_VERSION_PATCH)
#endif 

namespace rubuilderta
{
    const std::string package = "rubuilderta";
    const std::string versions = RUBUILDERTA_FULL_VERSION_LIST;
    const std::string version = PACKAGE_VERSION_STRING(RUBUILDERTA_VERSION_MAJOR,RUBUILDERTA_VERSION_MINOR,RUBUILDERTA_VERSION_PATCH);
    const std::string description = "Example trigger adapter (TA)";
    const std::string summary = "Example trigger adapter (TA)";
    const std::string authors = "Steven Murray, Remi Mommsen";
    const std::string link = "http://cms-ru-builder.web.cern.ch/cms-ru-builder";

    config::PackageInfo getPackageInfo();

    void checkPackageDependencies()
    throw (config::PackageInfo::VersionException);

    std::set<std::string, std::less<std::string> > getPackageDependencies();
}

#endif
