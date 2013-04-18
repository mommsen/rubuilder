#ifndef _rubuilder_ru_version_h_
#define _rubuilder_ru_version_h_

#include "config/PackageInfo.h"

#define RUBUILDERRU_VERSION_MAJOR 4
#define RUBUILDERRU_VERSION_MINOR 0
#define RUBUILDERRU_VERSION_PATCH 0
#undef RUBUILDERRU_PREVIOUS_VERSIONS


#define RUBUILDERRU_VERSION_CODE PACKAGE_VERSION_CODE(RUBUILDERRU_VERSION_MAJOR,RUBUILDERRU_VERSION_MINOR,RUBUILDERRU_VERSION_PATCH)
#ifndef RUBUILDERRU_PREVIOUS_VERSIONS
#define RUBUILDERRU_FULL_VERSION_LIST  PACKAGE_VERSION_STRING(RUBUILDERRU_VERSION_MAJOR,RUBUILDERRU_VERSION_MINOR,RUBUILDERRU_VERSION_PATCH)
#else 
#define RUBUILDERRU_FULL_VERSION_LIST  RUBUILDERRU_PREVIOUS_VERSIONS "," PACKAGE_VERSION_STRING(RUBUILDERRU_VERSION_MAJOR,RUBUILDERRU_VERSION_MINOR,RUBUILDERRU_VERSION_PATCH)
#endif 

namespace rubuilderru
{
    const std::string package = "rubuilderru";
    const std::string versions = RUBUILDERRU_FULL_VERSION_LIST;
    const std::string version = PACKAGE_VERSION_STRING(RUBUILDERRU_VERSION_MAJOR,RUBUILDERRU_VERSION_MINOR,RUBUILDERRU_VERSION_PATCH);
    const std::string description = "Readout unit (RU)";
    const std::string summary = "Readout unit (RU)";
    const std::string authors = "Steven Murray, Remi Mommsen, Luciano Orsini, Johannes Gutleber";
    const std::string link = "http://cms-ru-builder.web.cern.ch/cms-ru-builder";

    config::PackageInfo getPackageInfo();

    void checkPackageDependencies()
    throw (config::PackageInfo::VersionException);

    std::set<std::string, std::less<std::string> > getPackageDependencies();
}

#endif
