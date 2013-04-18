#ifndef _rubuilder_fu_FedSourceIdSet_h
#define _rubuilder_fu_FedSourceIdSet_h

#include "toolbox/BSem.h"
#include "xgi/Output.h"
#include "xgi/exception/Exception.h"

#include <set>
#include <stdint.h>


namespace rubuilder { namespace fu { // namespace rubuilder::fu

/**
 * A thread-safe set of FED source IDs.
 */
class FedSourceIdSet
{
private:

    /**
     * Set of FED source IDs to be made thread-safe.
     */
    std::set<uint32_t, std::less<uint32_t> > ids_;

    /**
     * Binary semaphore used to protect the set of FED source IDs.
     */
    toolbox::BSem bSem_;


public:

    /**
     * Constructor.
     */
    FedSourceIdSet();

    /**
     * Inserts the specified FED source ID and returns true if this is the
     * first time the FED source ID has been inserted.
     */
    bool insert(const uint32_t id);

    /**
     * Clears the contents of the set.
     */
    void clear();

    /**
     * Prints the contents of the set in HTML format.
     */
    void printHtml(xgi::Output *out)
    throw (xgi::exception::Exception);
};

} } // namespace rubuilder::fu

#endif
