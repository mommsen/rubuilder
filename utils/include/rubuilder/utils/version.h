#ifndef _rubuilder_utils_version_h_
#define _rubuilder_utils_version_h_

#include "config/PackageInfo.h"

#define RUBUILDERUTILS_VERSION_MAJOR 4
#define RUBUILDERUTILS_VERSION_MINOR 0
#define RUBUILDERUTILS_VERSION_PATCH 0
#undef RUBUILDERUTILS_PREVIOUS_VERSIONS


#define RUBUILDERUTILS_VERSION_CODE PACKAGE_VERSION_CODE(RUBUILDERUTILS_VERSION_MAJOR,RUBUILDERUTILS_VERSION_MINOR,RUBUILDERUTILS_VERSION_PATCH)
#ifndef RUBUILDERUTILS_PREVIOUS_VERSIONS
#define RUBUILDERUTILS_FULL_VERSION_LIST  PACKAGE_VERSION_STRING(RUBUILDERUTILS_VERSION_MAJOR,RUBUILDERUTILS_VERSION_MINOR,RUBUILDERUTILS_VERSION_PATCH)
#else 
#define RUBUILDERUTILS_FULL_VERSION_LIST  RUBUILDERUTILS_PREVIOUS_VERSIONS "," PACKAGE_VERSION_STRING(RUBUILDERUTILS_VERSION_MAJOR,RUBUILDERUTILS_VERSION_MINOR,RUBUILDERUTILS_VERSION_PATCH)
#endif 

namespace rubuilderutils
{
    const std::string package = "rubuilderutils";
    const std::string versions = RUBUILDERUTILS_FULL_VERSION_LIST;
    const std::string description = "Utilities shared by the RU builder applications";
    const std::string summary = "RU builder utilities";
    const std::string authors = "Steven Murray, Remi Mommsen";
    const std::string link = "http://cms-ru-builder.web.cern.ch/cms-ru-builder";

    config::PackageInfo getPackageInfo();

    void checkPackageDependencies()
    throw (config::PackageInfo::VersionException);

    std::set<std::string, std::less<std::string> > getPackageDependencies();
}

#endif
