#ifndef _rubuilder_utils_FragmentSets_h_
#define _rubuilder_utils_FragmentSets_h_

#include "xoap/DOMParser.h"
#include "xoap/domutils.h"

#include <map>
#include <set>


namespace rubuilder { namespace utils { // namespace rubuilder::utils

/**
 * A map of fragment sets.
 */
class FragmentSets
{
private:

/**
 * Map of fragment sets indexed by fragment set id.
 */
std::map
<
    unsigned int,
    std::set<int, std::less<int> >,
    std::less<unsigned int >
> fragmentSets_;

/**
 * Map of fragment set names indexed by framnet set id.
 */
std::map
<
    unsigned int,
    std::string,
    std::less<unsigned int >
> fragmentSetNames_;

/**
 * Parses the document with the specified url and returns the resulting
 * DOM document.
 */
DOMDocument *getDOMDocument
(
    const std::string &url
);

/**
 * Returns the attribute of the specified node with the specified name.
 */
DOMNode *getAttribute
(
    DOMNode           *node,
    const std::string &attributeName
);

/**
 * Adds the item represented by the specified DOM node to the fragment set
 * with the specified id.
 */
void addItem
(
    const unsigned int fragmentSetId,
    DOMNode            *node
);

/**
 * Adds the range represented by the specified DOM node to the fragment set
 * with the specified id.
 */
void addRange
(
    const unsigned int fragmentSetId,
    DOMNode            *node
);

/**
 * Adds the fragment set represented by the specified DOM node to the map
 * of fragment sets.
 */
void addFragmentSet
(
    DOMNode *node
);

/**
 * Returns the size of the fragment set with the specified id.
 */
int getSize
(
    const unsigned int fragmentSetId
);


public:

/**
 * Initialises the map of fragment sets using the document with the
 * specified url.
 */
void init
(
    const std::string &url
);

/**
 * Returns the fragment set with the specified id.
 */
const std::set<int, std::less<int> > &getFragmentSet
(
    const unsigned int  fragmentSetId
);

/**
 * Returns the name of the fragment set with the specified id.
 */
std::string getName
(
    const unsigned int fragmentSetId
);

/**
 * Returns the map of fragment sets as a string.
 */
std::string toString();

}; // class FragmentSets

} } // namespace rubuilder::utils

#endif
