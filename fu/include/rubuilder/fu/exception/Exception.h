#ifndef _rubuilder_fu_exception_Exception_h_
#define _rubuilder_fu_exception_Exception_h_


#include "xcept/Exception.h"


namespace rubuilder { namespace fu { namespace exception { // namespace rubuilder::fu::exception

/**
 * Exception raised by the fu package.
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

} } } // namespace rubuilder::fu::exception

#endif
