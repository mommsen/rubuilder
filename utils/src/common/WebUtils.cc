#include "rubuilder/utils/Constants.h"
#include "rubuilder/utils/TimerManager.h"
#include "rubuilder/utils/WebUtils.h"
#include "xcept/Exception.h"

#include <utility>

void rubuilder::utils::printWebPageIcon
(
    xgi::Output       *out,
    const std::string imgSrc,
    const std::string label,
    const std::string href
)
{
    *out << "<a href=\"" << href << "\">";
    *out << "<img style=\"border-style:none\"";
    *out << " src=\"" << imgSrc << "\"";
    *out << " alt=\"" << label << "\"";
    *out << " width=\"64\"";
    *out << " height=\"64\"/>";
    *out << "</a><br/>" << std::endl;
    *out << "<a href=\"" << href << "\">" << label << "</a>";
    *out << std::endl;
}


void rubuilder::utils::printWebPageTitleBar
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
    const std::string  testerIconSrc,
    const std::string  rubuilderTesterUrl
)
{
    *out << "<table border=\"0\" width=\"100%\">"                 << std::endl;
    *out << "<tr>"                                                << std::endl;
    *out << "  <td width=\"64\">"                                 << std::endl;
    *out << "    <img"                                            << std::endl;
    *out << "     align=\"middle\""                               << std::endl;
    *out << "     src=\"" << pageIconSrc << "\""                  << std::endl;
    *out << "     alt=\"" << pageIconAlt << "\""                  << std::endl;
    *out << "     width=\"64\""                                   << std::endl;
    *out << "     height=\"64\""                                  << std::endl;
    *out << "     border=\"\">"                                   << std::endl;
    *out << "  </td>"                                             << std::endl;
    *out << "  <td align=\"left\">"                               << std::endl;
    *out << "    <table width=\"100%\" border=\"0\""              << std::endl;
    *out << "      <tr>"                                          << std::endl;
    *out << "        <td>"                                        << std::endl;
    *out << "          <b>" << appClass << appInstance << "</b>"  << std::endl;
    *out << "        </td>"                                       << std::endl;
    *out << "        <td align=\"right\">"                        << std::endl;
    *out << "          <b>" << appState << "</b>"                 << std::endl;
    *out << "        </td>"                                       << std::endl;
    *out << "      </tr>"                                         << std::endl;
    *out << "      <tr>"                                          << std::endl;
    *out << "        <td>"                                        << std::endl;
    *out << "          Version " << appVersion << "<br>"          << std::endl;
    *out << "          <font size=\"-3\">(" << cvsId << ")</font>"<< std::endl;
    *out << "        </td>"                                       << std::endl;
    *out << "        <td align=\"right\">"                        << std::endl;
    *out << "          <a href=\"/" << appUrn << "/ParameterQuery\">XML</a>" << std::endl;
    *out << "        </td>"                                       << std::endl;
    *out << "      </tr>"                                         << std::endl;
    *out << "      <tr>"                                          << std::endl;
    *out << "        <td colspan=\"2\">"                          << std::endl;
    *out << "          Page last updated: ";
    *out << utils::getCurrentTimeUTC() << " UTC"                  << std::endl;
    *out << "        </td>"                                       << std::endl;
    *out << "      </tr>"                                         << std::endl;
    *out << "    </table>"                                        << std::endl;
    *out << "  </td>"                                             << std::endl;
    *out << "  <td class=\"app_links\" align=\"center\" width=\"70\">";
    *out << std::endl;
    printWebPageIcon(out, utils::HYPERDAQ_ICON, "HyperDAQ",
        "/urn:xdaq-application:service=hyperdaq");
    *out << "  </td>"                                             << std::endl;

    if(rubuilderTesterUrl != "")
    {
        *out << "  <td width=\"32\">"                             << std::endl;
        *out << "  </td>"                                         << std::endl;
        *out << "  <td class=\"app_links\" align=\"center\" width=\"64\">"
            << std::endl;
        printWebPageIcon(out, testerIconSrc, "Tester", rubuilderTesterUrl);
        *out << "  </td>"                                         << std::endl;
    }

    *out << "  <td width=\"32\">"                                 << std::endl;
    *out << "  </td>"                                             << std::endl;
    *out << "  <td class=\"app_links\" align=\"center\" width=\"64\">";
    *out << std::endl;
    printWebPageIcon(out, fsmIconSrc, "FSM", "/" + appUrn + "/fsm");
    *out << "  </td>"                                             << std::endl;

    *out << "  <td width=\"32\">"                                 << std::endl;
    *out << "  </td>"                                             << std::endl;
    *out << "  <td class=\"app_links\" align=\"center\" width=\"64\">";
    *out << std::endl;
    printWebPageIcon(out, dbgIconSrc, "Debug", "/" + appUrn + "/debug");
    *out << "  </td>"                                             << std::endl;

    *out << "  <td width=\"32\">"                                 << std::endl;
    *out << "  </td>"                                             << std::endl;
    *out << "  <td class=\"app_links\" align=\"center\" width=\"64\">";
    *out << std::endl;
    printWebPageIcon(out, appIconSrc, "Main", "/" + appUrn);
    *out << "  </td>"                                             << std::endl;

    *out << "</tr>"                                               << std::endl;
    *out << "</table>"                                            << std::endl;

    *out << "<hr>"                                                << std::endl;
}


void rubuilder::utils::printParamsTable
(
    xgi::Output                                                 *out,
    const std::string                                           name,
    std::vector< std::pair<std::string, xdata::Serializable*> > &params
)
{
    *out << "<table frame=\"void\" rules=\"rows\" class=\"params\">";
    *out << std::endl;

    *out << "  <tr>"                                              << std::endl;
    *out << "    <th colspan=2>"                                  << std::endl;
    *out << "      " << name                                      << std::endl;
    *out << "    </th>"                                           << std::endl;
    *out << "  </tr>"                                             << std::endl;


    std::vector
    <
        std::pair<std::string, xdata::Serializable*>
    >::const_iterator itor;

    for(itor=params.begin(); itor!= params.end(); itor++)
    {
        *out << "  <tr>"                                          << std::endl;

        // Name
        *out << "    <td>"                                        << std::endl;
        *out << "      " << itor->first                           << std::endl;
        *out << "    </td>"                                       << std::endl;

        // Value
        *out << "    <td>"                                        << std::endl;

        std::string str;

        try
        {
            str = itor->second->toString();
        }
        catch(xcept::Exception &e)
        {
            str = e.what();
        }
        *out << "      " << str << std::endl;

        *out << "    </td>"                                       << std::endl;

        *out << "  </tr>"                                         << std::endl;
    }

    *out << "</table>"                                            << std::endl;
}


void rubuilder::utils::css
(
    xgi::Input  *in,
    xgi::Output *out
)
{
    out->getHTTPResponseHeader().addHeader("Content-Type", "text/css");

    *out << "body"                                                << std::endl;
    *out << "{"                                                   << std::endl;
    *out << "background-color: white;"                            << std::endl;
    *out << "font-family: Arial;"                                 << std::endl;
    *out << "}"                                                   << std::endl;
    *out                                                          << std::endl;
    *out << ".app_links"                                          << std::endl;
    *out << "{"                                                   << std::endl;
    *out << "font-size: 14px;"                                    << std::endl;
    *out << "line-height: 14px;"                                  << std::endl;
    *out << "}"                                                   << std::endl;
    *out                                                          << std::endl;
    *out << "table.params th"                                     << std::endl;
    *out << "{"                                                   << std::endl;
    *out << "color: white;"                                       << std::endl;
    *out << "background-color: #66F;"                             << std::endl;
    *out << "}"                                                   << std::endl;
    *out                                                          << std::endl;
    *out << "table.fifo"                                          << std::endl;
    *out << "{"                                                   << std::endl;
    *out << "border-style: solid;"                                << std::endl;
    *out << "border-width: thin;"                                 << std::endl;
    *out << "}"                                                   << std::endl;
    *out                                                          << std::endl;
    *out << "table.fifo th"                                       << std::endl;
    *out << "{"                                                   << std::endl;
    *out << "color: white;"                                       << std::endl;
    *out << "background-color: #66F;"                             << std::endl;
    *out << "}"                                                   << std::endl;
}
