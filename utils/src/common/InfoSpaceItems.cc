#include "rubuilder/utils/Exception.h"
#include "rubuilder/utils/InfoSpaceItems.h"

#include <sstream>


void rubuilder::utils::InfoSpaceItems::add
(
  const std::string& name,
  xdata::Serializable* item,
  Listeners listener
)
{
  items_.push_back(std::make_pair(name,
      std::make_pair(item, listener)));
}


void rubuilder::utils::InfoSpaceItems::add(const InfoSpaceItems& other)
{
  const Items otherItems = other.getItems();
  items_.insert(items_.end(), otherItems.begin(), otherItems.end());
}


void rubuilder::utils::InfoSpaceItems::appendItemNames(std::list<std::string>& names) const
{
  for (Items::const_iterator it = items_.begin(), itEnd = items_.end();
       it != itEnd; ++it)
  {
    names.push_back(it->first);
  }
}


void rubuilder::utils::InfoSpaceItems::clear()
{
  // Should they also be removed from the infospace ?
  items_.clear();
}


void rubuilder::utils::InfoSpaceItems::putIntoInfoSpace(xdata::InfoSpace* s, xdata::ActionListener* l) const
{
  for (Items::const_iterator it = items_.begin(), itEnd = items_.end(); it != itEnd; ++it)
  {
    try
    {
      s->fireItemAvailable(it->first, it->second.first);
      switch (it->second.second) {
        case change :
          s->addItemChangedListener(it->first, l);
          break;
        case retrieve : 
          s->addItemRetrieveListener(it->first, l);
          break;
        case none :
          break;
      }
    }
    catch(xcept::Exception &e)
    {
      std::stringstream oss;

      oss << "Failed to put " << it->first;
      oss << " into infospace " << s->name();
      
      XCEPT_RETHROW(exception::InfoSpace, oss.str(), e);
    }
  }
}


void rubuilder::utils::InfoSpaceItems::printHtml(const std::string& title, xgi::Output* out) const
{
  *out << "<tr>"                                                  << std::endl;
  *out << "<th colspan=\"2\"><br/>" << title << "</th>"           << std::endl;
  *out << "</tr>"                                                 << std::endl;
  
  for (Items::const_iterator it = items_.begin(), itEnd = items_.end(); it != itEnd; ++it)
  {
    *out << "<tr>"                                                << std::endl;
    *out << "<td>"                                                << std::endl;
    *out << it->first                                             << std::endl;
    *out << "</td>"                                               << std::endl;
    *out << "<td>"                                                << std::endl;
    try
    {
      *out << it->second.first->toString()                        << std::endl;
    }
    catch (xdata::exception::Exception)
    {
      *out << "n/a"                                               << std::endl;
    }    
    *out << "</td>"                                               << std::endl;
    *out << "</tr>"                                               << std::endl;
  }
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
