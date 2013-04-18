#ifndef _rubuilder_fu_version_h_
#define _rubuilder_fu_version_h_

#include "config/PackageInfo.h"

#define RUBUILDERFU_VERSION_MAJOR 2
#define RUBUILDERFU_VERSION_MINOR 0
#define RUBUILDERFU_VERSION_PATCH 0
#undef RUBUILDERFU_PREVIOUS_VERSIONS


#define RUBUILDERFU_VERSION_CODE PACKAGE_VERSION_CODE(RUBUILDERFU_VERSION_MAJOR,RUBUILDERFU_VERSION_MINOR,RUBUILDERFU_VERSION_PATCH)
#ifndef RUBUILDERFU_PREVIOUS_VERSIONS
#define RUBUILDERFU_FULL_VERSION_LIST  PACKAGE_VERSION_STRING(RUBUILDERFU_VERSION_MAJOR,RUBUILDERFU_VERSION_MINOR,RUBUILDERFU_VERSION_PATCH)
#else 
#define RUBUILDERFU_FULL_VERSION_LIST  RUBUILDERFU_PREVIOUS_VERSIONS "," PACKAGE_VERSION_STRING(RUBUILDERFU_VERSION_MAJOR,RUBUILDERFU_VERSION_MINOR,RUBUILDERFU_VERSION_PATCH)
#endif 

namespace rubuilderfu
{
    const std::string package = "rubuilderfu";
    const std::string versions = RUBUILDERFU_FULL_VERSION_LIST;
    const std::string version = PACKAGE_VERSION_STRING(RUBUILDERFU_VERSION_MAJOR,RUBUILDERFU_VERSION_MINOR,RUBUILDERFU_VERSION_PATCH);
    const std::string description = "Example Filter Unit (FU)";
    const std::string summary = "Example Filter Unit (FU)";
    const std::string authors = "Steven Murray, Remi Mommsen";
    const std::string link = "http://cms-ru-builder.web.cern.ch/cms-ru-builder";

    config::PackageInfo getPackageInfo();

    void checkPackageDependencies()
    throw (config::PackageInfo::VersionException);

    std::set<std::string, std::less<std::string> > getPackageDependencies();
}

#endif
