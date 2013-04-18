#ifndef _rubuilder_bu_version_h_
#define _rubuilder_bu_version_h_

#include "config/PackageInfo.h"

#define RUBUILDERBU_VERSION_MAJOR 3
#define RUBUILDERBU_VERSION_MINOR 0
#define RUBUILDERBU_VERSION_PATCH 0
#undef RUBUILDERBU_PREVIOUS_VERSIONS


#define RUBUILDERBU_VERSION_CODE PACKAGE_VERSION_CODE(RUBUILDERBU_VERSION_MAJOR,RUBUILDERBU_VERSION_MINOR,RUBUILDERBU_VERSION_PATCH)
#ifndef RUBUILDERBU_PREVIOUS_VERSIONS
#define RUBUILDERBU_FULL_VERSION_LIST  PACKAGE_VERSION_STRING(RUBUILDERBU_VERSION_MAJOR,RUBUILDERBU_VERSION_MINOR,RUBUILDERBU_VERSION_PATCH)
#else 
#define RUBUILDERBU_FULL_VERSION_LIST  RUBUILDERBU_PREVIOUS_VERSIONS "," PACKAGE_VERSION_STRING(RUBUILDERBU_VERSION_MAJOR,RUBUILDERBU_VERSION_MINOR,RUBUILDERBU_VERSION_PATCH)
#endif 

namespace rubuilderbu
{
    const std::string package = "rubuilderbu";
    const std::string versions = RUBUILDERBU_FULL_VERSION_LIST;
    const std::string version = PACKAGE_VERSION_STRING(RUBUILDERBU_VERSION_MAJOR,RUBUILDERBU_VERSION_MINOR,RUBUILDERBU_VERSION_PATCH);
    const std::string description = "Builder unit (BU)";
    const std::string summary = "Builder unit (BU)";
    const std::string authors = "Steven Murray, Remi Mommsen";
    const std::string link = "http://cms-ru-builder.web.cern.ch/cms-ru-builder";

    config::PackageInfo getPackageInfo();

    void checkPackageDependencies()
    throw (config::PackageInfo::VersionException);

    std::set<std::string, std::less<std::string> > getPackageDependencies();
}

#endif
