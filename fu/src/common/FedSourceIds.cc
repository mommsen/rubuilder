#include "rubuilder/fu/FedSourceIdSet.h"


rubuilder::fu::FedSourceIdSet::FedSourceIdSet() :
bSem_(toolbox::BSem::FULL)
{
}


bool rubuilder::fu::FedSourceIdSet::insert(const uint32_t id)
{
    bSem_.take();

    std::pair<std::set<uint32_t, std::less<uint32_t> >::iterator, bool> status
        = ids_.insert(id);
    bool firstInsertion = status.second;

    bSem_.give();

    return firstInsertion;
}


void rubuilder::fu::FedSourceIdSet::clear()
{
    bSem_.take();

    ids_.clear();

    bSem_.give();
}


void rubuilder::fu::FedSourceIdSet::printHtml(xgi::Output *out)
throw (xgi::exception::Exception)
{
    std::set<uint32_t, std::less<uint32_t> >::iterator itor;


    *out << "<table frame=\"void\" rules=\"cols\" class=\"params\">";
    *out << std::endl;

    *out << "<tr>"                      << std::endl;
    *out << "  <th>FED Source IDs</th>" << std::endl;

    bSem_.take();

    for
    (
        itor = ids_.begin();
        itor != ids_.end();
        itor++
    )
    {
        *out << "  <td>" << *itor << "</td>" << std::endl;
    }

    bSem_.give();

    *out << "</tr>" << std::endl;
    *out << "</table>" << std::endl;
}
