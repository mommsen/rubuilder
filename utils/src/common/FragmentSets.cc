#include "rubuilder/utils/FragmentSets.h"
#include "rubuilder/utils/Exception.h"

#include "xercesc/framework/URLInputSource.hpp"
#include "xercesc/framework/Wrapper4InputSource.hpp"
#include "xercesc/util/XMLURL.hpp"
#include "xcept/Exception.h"
#include "xoap/DOMParserFactory.h"

#include <sstream>


DOMDocument *rubuilder::utils::FragmentSets::getDOMDocument
(
  const std::string &url
)
{
  XMLURL          *source = 0;
  xoap::DOMParser *parser = xoap::getDOMParserFactory()->get("configure");
  DOMDocument     *doc    = 0;
  
  try
  {
    source = new XMLURL(url.c_str());
  }
  catch (...)
  {
    XCEPT_RAISE(exception::DOM,
      "Failed to create XMLURL(" + url + ")");
  }
  
  try
  {
    doc = parser->parse(*source);
    delete source;
    return doc;
  }
  catch (xcept::Exception& e)
  {
    delete source;
    XCEPT_RETHROW(exception::DOM,
      "Cannot parse XML loaded from " + url, e);
  }
  
  return doc;
}


DOMNode *rubuilder::utils::FragmentSets::getAttribute
(
  DOMNode           *node,
  const std::string &attributeName
)
{
  DOMNamedNodeMap *attributes = node->getAttributes();
  DOMNode         *attribute  = 0;
  
  
  for(unsigned int i=0; i<attributes->getLength(); i++)
  {
    attribute = attributes->item(i);
    
    if(xoap::XMLCh2String(attribute->getNodeName()) == attributeName)
    {
      return attribute;
    }
  }
  
  XCEPT_RAISE(exception::DOM,
    "Failed to find attribute named: " + attributeName);
}


void rubuilder::utils::FragmentSets::addItem
(
  const unsigned int  fragmentSetId,
  DOMNode             *node
)
{
  DOMNode      *attribute = 0;
  std::string  s          = "";
  int          i          = 0;
  
  
  try
  {
    attribute = getAttribute(node, "value");
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::DOM,
      "Failed to get value attribute", e);
  }

  s = xoap::XMLCh2String(attribute->getNodeValue());
  i = atoi(s.c_str());
  
  fragmentSets_[fragmentSetId].insert(i);
}


/**
 * Adds the range represented by the specified DOM node to the fragment set
 * with the specified id.
 */
void rubuilder::utils::FragmentSets::addRange
(
  const unsigned int fragmentSetId,
  DOMNode            *node
)
{
  DOMNode      *attribute = 0;
  std::string  s          = "";
  int          min        = 0;
  int          max        = 0;
  int          i          = 0;
  
  
  try
  {
    attribute = getAttribute(node, "minvalue");
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::DOM,
      "Failed to get minValue attribute", e);
  }
  
  s   = xoap::XMLCh2String(attribute->getNodeValue());
  min = atoi(s.c_str());
  
  try
  {
    attribute = getAttribute(node, "maxvalue");
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::DOM,
      "Failed to get maxValue attribute", e);
  }
  
  s   = xoap::XMLCh2String(attribute->getNodeValue());
  max = atoi(s.c_str());
  
  for(i=min; i <= max; i++)
  {
    fragmentSets_[fragmentSetId].insert(i);
  }
}


void rubuilder::utils::FragmentSets::addFragmentSet
(
  DOMNode *node
)
{
  DOMNode                        *attribute      = 0;
  std::string                    s               = "";
  unsigned int                   fragmentSetId   = 0;
  std::string                    fragmentSetName = "";
  std::set<int, std::less<int> > emptyset;
  DOMNodeList                    *childNodes     = 0;
  DOMNode                        *childNode      = 0;
  std::string                    childName       = "";
  
  // Get the id of the fragment set
  try
  {
    attribute = getAttribute(node, "id");
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::DOM,
      "Failed to get id attribute of fragment set", e);
  }
  s  = xoap::XMLCh2String(attribute->getNodeValue());
  fragmentSetId = strtol(s.c_str(), 0, 10);
  
  // Get the name of the fragment set
  try
  {
    attribute = getAttribute(node, "name");
  }
  catch(xcept::Exception &e)
  {
    std::stringstream oss;
    
    oss << "Failed to get name attribute of fragment set with";
    oss << " id: " << fragmentSetId;
    
    XCEPT_RETHROW(exception::DOM, oss.str(),
      e);
  }
  fragmentSetName = xoap::XMLCh2String(attribute->getNodeValue());
  
  // Add fragment set name to map
  fragmentSetNames_[fragmentSetId] = fragmentSetName;
  
  // Empty the fragment set
  fragmentSets_[fragmentSetId] = emptyset;
  
  childNodes = node->getChildNodes();
  
  // Loop over items and ranges
  for(unsigned int i=0; i<childNodes->getLength(); i++)
  {
    childNode = childNodes->item(i);
    childName = xoap::XMLCh2String(childNode->getNodeName());
    
    if(childName == "item")
    {
      try
      {
        addItem(fragmentSetId, childNode);
      }
      catch(xcept::Exception &e)
      {
        std::stringstream oss;
        
        oss << "Failed to add item to fragment set with";
        oss << " id: "   << fragmentSetId;
        oss << " name: " << fragmentSetName;
        
        XCEPT_RETHROW(exception::DOM,
          oss.str(), e);
      }
    }
    else if(childName == "range")
    {
      try
      {
        addRange(fragmentSetId, childNode);
      }
      catch(xcept::Exception &e)
      {
        std::stringstream oss;
        
        oss << "Failed to add range to fragment set with";
        oss << " id: "   << fragmentSetId;
        oss << " name: " << fragmentSetName;
        
        XCEPT_RETHROW(exception::DOM,
          oss.str(), e);
      }
    }
  }
}


int rubuilder::utils::FragmentSets::getSize
(
  const unsigned int fragmentSetId
)
{
  std::map
    <
      unsigned int ,
      std::set<int, std::less<int> >,
      std::less<unsigned int >
      >::const_iterator itor = fragmentSets_.find(fragmentSetId);
  
  if(itor == fragmentSets_.end())
  {
    std::stringstream oss;
    
    oss << "Cannot find fragment set with id: " << fragmentSetId;
    
    XCEPT_RAISE(exception::FragmentNotFound, oss.str());
  }
  
  return (*itor).second.size();
}


void rubuilder::utils::FragmentSets::init
(
  const std::string &url
)
{
  DOMDocument  *doc   = 0;
  DOMNodeList  *nodes = 0;
  DOMNode      *node  = 0;
  unsigned int i      = 0;
  
  
  try
  {
    doc = getDOMDocument(url);
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::DOM,
      "Could not create DOM document from " + url, e);
  }
  
  nodes = doc->getElementsByTagNameNS(xoap::XStr("*"),
    xoap::XStr("FragmentSetDef"));
  
  // For each fragment set definition
  for(i=0; i<nodes->getLength(); i++)
  {
    node = nodes->item(i);
    addFragmentSet(node);
  }
  
  doc->release();
}


const std::set<int, std::less<int> >
&rubuilder::utils::FragmentSets::getFragmentSet
(
  const unsigned int  fragmentSetId
)
{
  std::map
    <
      unsigned int ,
      std::set<int, std::less<int> >,
      std::less<unsigned int >
      >::const_iterator itor = fragmentSets_.find(fragmentSetId);

if(itor == fragmentSets_.end())
{
  std::stringstream oss;
  
  oss << "Cannot find fragment set with id: " << fragmentSetId;
  
  XCEPT_RAISE(exception::FragmentNotFound, oss.str());
}

return (*itor).second;
}


std::string rubuilder::utils::FragmentSets::getName
(
  const unsigned int fragmentSetId
)
{
  std::map
    <
      unsigned int ,
      std::string,
      std::less<unsigned int >
      >::const_iterator itor = fragmentSetNames_.find(fragmentSetId);
  
  if(itor == fragmentSetNames_.end())
  {
    std::stringstream oss;
    
    oss << "Cannot find fragment set with id: " << fragmentSetId;
    
    XCEPT_RAISE(exception::FragmentNotFound, oss.str());
  }
  
  return (*itor).second;
}


std::string rubuilder::utils::FragmentSets::toString()
{
  std::stringstream oss;
  
  std::map
    <
      unsigned int ,
      std::set<int, std::less<int> >,
      std::less<unsigned int >
      >::const_iterator mitor;

for(mitor=fragmentSets_.begin(); mitor!=fragmentSets_.end(); mitor++)
{
  int fragmentSetId = mitor->first;
  std::set<int, std::less<int> > fset =
    this->getFragmentSet(fragmentSetId);
  
  oss << "Fragment set";
  oss << " id: "   << fragmentSetId;
  oss << " name: " << getName(fragmentSetId);
  oss << " size:"  << getSize(fragmentSetId);
  oss << "\n";
  
  std::set<int, std::less<int> >::const_iterator sitor;
  
  for(sitor = fset.begin(); sitor != fset.end(); sitor++ )
  {
    oss << *sitor << " ";
  }
  
  oss << "\n";
}

return oss.str();
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
