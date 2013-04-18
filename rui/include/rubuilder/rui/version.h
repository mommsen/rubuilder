#ifndef _rubuilder_rui_version_h_
#define _rubuilder_rui_version_h_

#include "config/PackageInfo.h"

#define RUBUILDERRUI_VERSION_MAJOR 2
#define RUBUILDERRUI_VERSION_MINOR 0
#define RUBUILDERRUI_VERSION_PATCH 0
#undef RUBUILDERRUI_PREVIOUS_VERSIONS


#define RUBUILDERRUI_VERSION_CODE PACKAGE_VERSION_CODE(RUBUILDERRUI_VERSION_MAJOR,RUBUILDERRUI_VERSION_MINOR,RUBUILDERRUI_VERSION_PATCH)
#ifndef RUBUILDERRUI_PREVIOUS_VERSIONS
#define RUBUILDERRUI_FULL_VERSION_LIST  PACKAGE_VERSION_STRING(RUBUILDERRUI_VERSION_MAJOR,RUBUILDERRUI_VERSION_MINOR,RUBUILDERRUI_VERSION_PATCH)
#else 
#define RUBUILDERRUI_FULL_VERSION_LIST  RUBUILDERRUI_PREVIOUS_VERSIONS "," PACKAGE_VERSION_STRING(RUBUILDERRUI_VERSION_MAJOR,RUBUILDERRUI_VERSION_MINOR,RUBUILDERRUI_VERSION_PATCH)
#endif 

namespace rubuilderrui
{
    const std::string package = "rubuilderrui";
    const std::string versions = RUBUILDERRUI_FULL_VERSION_LIST;
    const std::string version = PACKAGE_VERSION_STRING(RUBUILDERRUI_VERSION_MAJOR,RUBUILDERRUI_VERSION_MINOR,RUBUILDERRUI_VERSION_PATCH);
    const std::string description = "Example Readout Unit Interface (RUI)";
    const std::string summary = "Example Readout Unit Interface (RUI)";
    const std::string authors = "Steven Murray, Remi Mommsen";
    const std::string link = "http://cms-ru-builder.web.cern.ch/cms-ru-builder";

    config::PackageInfo getPackageInfo();

    void checkPackageDependencies()
    throw (config::PackageInfo::VersionException);

    std::set<std::string, std::less<std::string> > getPackageDependencies();
}

#endif
