#ifndef _rubuilder_utils_WebUtils_h_
#define _rubuilder_utils_WebUtils_h_

#include "xgi/Input.h"
#include "xgi/Output.h"
#include "xdata/Serializable.h"

#include <string>
#include <vector>


namespace rubuilder { namespace utils { // namespace rubuilder::utils

/**
 * Prints a nagivation icon.
 */
void printWebPageIcon
(
    xgi::Output       *out,
    const std::string imgSrc,
    const std::string label,
    const std::string href
);

/**
 * Prints the standard web page title bar for a RU builder application.
 *
 * Note: If there is no RU builder tester application then set
 * rubuilderTesterUrl to empty string ("").
 */
void printWebPageTitleBar
(
    xgi::Output        *out,
    const std::string  pageIconSrc,
    const std::string  pageIconAlt,
    const std::string  appClass,
    const unsigned int appInstance,
    const std::string  appState,
    const std::string  appUrn,
    const std::string  appVersion,
    const std::string  cvsId,
    const std::string  appIconSrc,
    const std::string  dbgIconSrc,
    const std::string  fsmIconSrc,
    const std::string  testerIconSrc = "",
    const std::string  rubuilderTesterUrl = ""
);

/**
 * Prints the specified parameters as an HTML table with the specified name.
 */
void printParamsTable
(
    xgi::Output                                                 *out,
    const std::string                                           name,
    std::vector< std::pair<std::string, xdata::Serializable*> > &params
);


/**
 * Creates the CSS file.
 */
void css
(
    xgi::Input  *in,
    xgi::Output *out
);

} } // namespace rubuilder::utils

#endif
