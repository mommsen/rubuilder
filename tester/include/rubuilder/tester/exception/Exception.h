#ifndef _rubuilder_tester_exception_Exception_h_
#define _rubuilder_tester_exception_Exception_h_


#include "xcept/Exception.h"


namespace rubuilder { namespace tester { namespace exception { // namespace rubuilder::tester::exception

/**
 * Exception raised by the tester package.
 */
class Exception: public xcept::Exception
{
public:

    Exception
    (
        std::string name,
        std::string message,
        std::string module,
        int         line,
        std::string function
    ) : xcept::Exception(name, message, module, line, function)
    {
    }

    Exception
    (
        std::string      name,
        std::string      message,
        std::string      module,
        int              line,
        std::string      function,
        xcept::Exception &e
    ) : xcept::Exception(name, message, module, line, function, e)
    {
    }
};

} } } // namespace rubuilder::tester::exception

#endif
