#ifndef _rubuilder_tester_Application_h_
#define _rubuilder_tester_Application_h_

#include "rubuilder/utils/ApplicationInstanceLess.h"
#include "rubuilder/tester/exception/Exception.h"
#include "rubuilder/utils/WebUtils.h"
#include "i2o/i2oDdmLib.h"
#include "i2o/utils/AddressMap.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "xdaq/ApplicationGroup.h"
#include "xdaq/WebApplication.h"
#include "xdata/Boolean.h"
#include "xdata/InfoSpace.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"

#include <list>
#include <map>
#include <vector>


namespace rubuilder { namespace tester { // namespace rubuilder::tester

/**
 * Tests the RU builder applications.
 */
class Application :
public xdaq::WebApplication
{
public:

    /**
     * Define factory method for the instantion of RUBuilderTester applications.
     */
    XDAQ_INSTANTIATOR();

    /**
     * Constructor.
     */
    Application(xdaq::ApplicationStub *s)
    throw (xdaq::exception::Exception);

    /**
     * Invoked when an exception has been received from the sentinel.
     */
    void onException(xcept::Exception &e);


private:

    /**
     * Maps the urls of applications to their descriptors.
     */
    std::map<std::string, xdaq::ApplicationDescriptor*> appUrlToDescriptor_;

    /**
     * Rolling window of received exceptions.
     */
    std::list<std::string> exceptionsWindow_;

    /**
     * Rolling window of received state change notifications.
     */
    std::list<std::string> stateChangeNotificationsWindow_;

    /**
     * The logger of this application.
     */
    Logger logger_;

    /**
     * Used to access the I2O address map without a function call.
     */
    i2o::utils::AddressMap *i2oAddressMap_;

    /**
     * Used to access the memory pool factory without a function call.
     */
    toolbox::mem::MemoryPoolFactory *poolFactory_;

    /**
     * Used to access the application's info space without a function call.
     */
    xdata::InfoSpace *appInfoSpace_;

    /**
     * Used to access the application's descriptor without a function call.
     */
    xdaq::ApplicationDescriptor *appDescriptor_;

    /**
     * Used to access the application's context without a function call.
     */
    xdaq::ApplicationContext *appContext_;

    /**
     * The XML class name of the application.
     */
    std::string xmlClass_;

    /**
     * The instance number of the application.
     */
    unsigned int  instance_;

    /**
     * The application's URN.
     */
    std::string urn_;

    /**
     * True if there are atcps and they are enabled.
     */
    bool atcpsAreEnabled_;

    /**
     * The application descriptors of all the PeerTransportATCP
     * applications.
     */
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    > atcpDescriptors_;
    

    /**
     * The application descriptors of all the BU applications.
     */
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    > buDescriptors_;

    /**
     * The application descriptors of all the EVM applications.
     */
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    > evmDescriptors_;

    /**
     * The application descriptors of all the FU applications.
     */
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    > fuDescriptors_;

    /**
     * The application descriptors of all the RU applications.
     */
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    > ruDescriptors_;

    /**
     * The application descriptors of all the RUI applications.
     */
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    > ruiDescriptors_;

    /**
     * The application descriptors of all the TA applications.
     */
    std::set
    <
        xdaq::ApplicationDescriptor*,
        rubuilder::utils::ApplicationInstanceLess
    > taDescriptors_;

    /**
     * True if the test of the RU builder applications has been started, else
     * false.
     */
    bool testStarted_;


    ////////////////////////////////////////////////////////
    // Beginning of exported parameters for configuration //
    ////////////////////////////////////////////////////////

    /**
     * Exported read/write parameter specifying the size of the exceptions
     * window.
     */
    xdata::UnsignedInteger32 exceptionsWindowSize_;

    /**
     * Exported read/write parameter specifying the size of the state
     * change notifications window.
     */
    xdata::UnsignedInteger32 stateChangeNotificationsWindowSize_;

    ////////////////////////////////////////////////////////
    // End of exported parameters for configuration       //
    ////////////////////////////////////////////////////////


    /////////////////////////////////////////////////////
    // Beginning of exported parameters for monitoring //
    /////////////////////////////////////////////////////

    /**
     * Exported read-only parameter specifying the current state of the
     * application.
     */
    xdata::String stateName_;

    /**
     * Exported read-only parameter specifying the total number of exceptions
     * received via the sentinel.
     */
    xdata::UnsignedInteger32 nbExceptions_;

    /**
     * Exported read-only parameter specifying the number of state change
     * notifcations that have been received.
     */
    xdata::UnsignedInteger32 nbStateChangeNotifications_;

    /////////////////////////////////////////////////////
    // End of exported parameters for monitoring       //
    /////////////////////////////////////////////////////


    /**
     * The application's standard configuration parameters.
     */
    std::vector< std::pair<std::string, xdata::Serializable *> >
        stdConfigParams_;

    /**
     * The application's standard monitoring parameters.
     */
    std::vector< std::pair<std::string, xdata::Serializable *> >
        stdMonitorParams_;

    /**
     * Initialises the map of application urls to application descriptors.
     */
    void initAppUrlToDescriptor();

    /**
     * Returns the name to be given to the logger of this application.
     */
    std::string generateLoggerName();

    /**
     * Initialises the specified vector of application descriptiors with the
     * descriptors of applications having the specified class name.
     *
     * The contents of the vector are sorted by application instance number.
     *
     * If there are no applications with the specified class name, then the
     * vector is cleared.
     */
    void initAppDescriptors
    (
        std::set
        <
            xdaq::ApplicationDescriptor*,
            rubuilder::utils::ApplicationInstanceLess
        > &descriptors,
        const std::string className
    );

    /**
     * Binds the XGI callback methods.
     */
    void bindXgiCallbacks();

    /**
     * Creates the CSS file for this application.
     */
    void css
    (
        xgi::Input  *in,
        xgi::Output *out
    )
    { rubuilder::utils::css(in,out); }

    /**
     * The default web page of the application.
     */
    void defaultWebPage(xgi::Input *in, xgi::Output *out)
    throw (xgi::exception::Exception);

    /**
     * Web page used to control the RU builder.
     */
    void controlWebPage(xgi::Input *in, xgi::Output *out)
    throw (xgi::exception::Exception);

    /**
     * Processes the form sent from the control web page.
     */
    void processControlForm(xgi::Input *in)
    throw (xgi::exception::Exception);

    /**
     * Web page displaying the exception and state change notification
     * messages received by the RUBuilderTester.
     */
    void messagesWebPage(xgi::Input *in, xgi::Output *out)
    throw (xgi::exception::Exception);

    /**
     * Plain text web page showing the exceptions received by the
     * RUBuilderTester.
     */
    void exceptionsWebPage(xgi::Input *in, xgi::Output *out)
    throw (xgi::exception::Exception);

    /**
     * Plain text web page showing the state change notifications received by
     * the RUBuilderTester.
     */
    void stateChangeNotificationsWebPage
    (
        xgi::Input *in,
        xgi::Output *out
    )
    throw (xgi::exception::Exception);

    /**
     * Emulates the RCMS state listener.
     */
    xoap::MessageReference sendNotificationReply(xoap::MessageReference msg)
    throw (xoap::exception::Exception);

    /**
     * Returns the value of sourceUrl embedded within the specified
     * SendNotificationReply SOAP message.
     */
    std::string extractSourceURLFromSendNotificationReply
    (
        xoap::MessageReference msg
    )
    throw (xoap::exception::Exception);

    /**
     * Returns the value of stateName embedded within the specified
     * SendNotificationReply SOAP message.
     */
    std::string extractStateNameFromSendNotificationReply
    (
        xoap::MessageReference msg
    )
    throw (xoap::exception::Exception);

    /**
     * Prints hyper-text links to the specified applications.
     */
    void printAppInstanceLinks
    (
        xgi::Output *out,
        std::set
        <
            xdaq::ApplicationDescriptor*,
            rubuilder::utils::ApplicationInstanceLess
        > &appDescriptors
    );

    /**
     * Prints a hyper-text link to the specified application.
     */
    void printAppLink
    (
        xgi::Output                 *out,
        xdaq::ApplicationDescriptor *appDescriptor
    );

    /**
     * Prints a set of HTML tables displaying EVM monitoring information.
     */
    void printEVMTables
    (
        xgi::Output *out,
        std::set
        <
            xdaq::ApplicationDescriptor*,
            rubuilder::utils::ApplicationInstanceLess
        > &evmDescriptors
    )
    throw (rubuilder::tester::exception::Exception);

    /**
     * Prints a HTML table displaying EVM monitoring information.
     */
    void printEVMTable
    (
        xgi::Output                 *out,
        xdaq::ApplicationDescriptor *evmDescriptor
    )
    throw (rubuilder::tester::exception::Exception);

    /**
     * Prints a set of HTML tables displaying RU monitoring information.
     */
    void printRUTables
    (
        xgi::Output *out,
        std::set
        <
            xdaq::ApplicationDescriptor*,
            rubuilder::utils::ApplicationInstanceLess
        > &ruDescriptors
    )
    throw (rubuilder::tester::exception::Exception);

    /**
     * Prints a HTML table displaying RU monitoring information.
     */
    void printRUTable
    (
        xgi::Output                 *out,
        xdaq::ApplicationDescriptor *ruDescriptor
    )
    throw (rubuilder::tester::exception::Exception);

    /**
     * Prints a set of HTML tables displaying BU monitoring information.
     */
    void printBUTables
    (
        xgi::Output *out,
        std::set
        <
            xdaq::ApplicationDescriptor*,
            rubuilder::utils::ApplicationInstanceLess
        > &buDescriptors
    )
    throw (rubuilder::tester::exception::Exception);

    /**
     * Prints a HTML table displaying BU monitoring information.
     */
    void printBUTable
    (
        xgi::Output                 *out,
        xdaq::ApplicationDescriptor *buDescriptor
    )
    throw (rubuilder::tester::exception::Exception);

    /**
     * Get and returns the values of statistics parameters exported by the
     * specified application.
     */
    std::vector< std::pair<std::string, std::string> > getStats
    (
        xdaq::ApplicationDescriptor *appDescriptor
    );

    /**
     * Returns the url of the specified application.
     */
    std::string getUrl(xdaq::ApplicationDescriptor *appDescriptor);

    /**
     * Initialises and returns the application's standard configuration
     * parameters.
     */
    std::vector< std::pair<std::string, xdata::Serializable*> >
    initAndGetStdConfigParams()
    throw (rubuilder::tester::exception::Exception);

    /**
     * Initialises and returns the application's standard monitoring
     * parameters.
     */
    std::vector< std::pair<std::string, xdata::Serializable*> >
    initAndGetStdMonitorParams()
    throw (rubuilder::tester::exception::Exception);

    /**
     * Puts the specified parameters into the specified info space.
     */
    void putParamsIntoInfoSpace
    (
        std::vector< std::pair<std::string, xdata::Serializable*> > &params,
        xdata::InfoSpace                                            *s
    )
    throw (rubuilder::tester::exception::Exception);

    /**
     * Starts the test.
     */
    void startTest()
    throw (rubuilder::tester::exception::Exception);

    /**
     * Checks that there is a minimum set of applications to make a RU builder.
     */
    void checkThereIsARuBuilder()
    throw (rubuilder::tester::exception::Exception);

    /**
     * Configures and enables all ptATCPs.
     */
    void configureAndEnableAtcps()
    throw (rubuilder::tester::exception::Exception);

    /**
     * Tell the EVM whether or not it should generate dummy triggers.
     */
    void setEVMGenerateDummyTriggers(const bool value)
    throw (rubuilder::tester::exception::Exception);

    /**
     * Tell the RUs whether or not they should generate dummy super-fragments.
     */
    void setRUsGenerateDummySuperFrags(const bool vale)
    throw (rubuilder::tester::exception::Exception);

    /**
     * Tells the BUs whether or not to drop events.
     */
    void setBUsDropEvents(const bool value)
    throw (rubuilder::tester::exception::Exception);

    /**
     * Starts the imaginary trigger, i.e. the TA.
     */
    void startTrigger()
    throw (rubuilder::tester::exception::Exception);

    /**
     * Starts just the RU builder, i.e. the BUs, EVM and RUs.
     */
    void startRuBuilder()
    throw (rubuilder::tester::exception::Exception);

    /**
     * Starts the imaginary FED builder, i.e. the RUIs.
     */
    void startFedBuilder()
    throw (rubuilder::tester::exception::Exception);

    /**
     * Starts the imaginary filter farm, i.e. the FUs.
     */
    void startFilterFarm()
    throw (rubuilder::tester::exception::Exception);

    /**
     * Stops the test.
     */
    void stopTest()
    throw (rubuilder::tester::exception::Exception);

    /**
     * Stops the imaginary FED builder, i.e. the RUIs.
     */
    void stopFedBuilder()
    throw (rubuilder::tester::exception::Exception);

    /**
     * Stops just the RU builder, i.e. BUs, EVM and RUs.
     */
    void stopRuBuilder()
    throw (rubuilder::tester::exception::Exception);

    /**
     * Stops the imaginary trigger, i.e. the TA.
     */
    void stopTrigger()
    throw (rubuilder::tester::exception::Exception);

    /**
     * Stops the imaginary filter farm, i.e. the FUs.
     */
    void stopFilterFarm()
    throw (rubuilder::tester::exception::Exception);

    /**
     * Sends the specified FSM event as a SOAP message to the specified
     * application.  An exception is raised if the application does not reply
     * successfully with a SOAP response.
     */
    void sendFSMEventToApp
    (
        const std::string            eventName,
        xdaq::ApplicationDescriptor* destAppDescriptor
    )
    throw (rubuilder::tester::exception::Exception);

    /**
     * Creates a simple SOAP message representing a command with no
     * parameters.
     */
    xoap::MessageReference createSimpleSOAPCmdMsg(const std::string cmdName)
    throw (rubuilder::tester::exception::Exception);

    /**
     * Gets and returns the value of the specified parameter from the specified
     * application.
     */
    std::string getScalarParam
    (
        xdaq::ApplicationDescriptor* destAppDescriptor,
        const std::string            paramName,
        const std::string            paramType
    )
    throw (rubuilder::tester::exception::Exception);

    /**
     * Sets the specified parameter of the specified application to the
     * specified value.
     */
    void setScalarParam
    (
        xdaq::ApplicationDescriptor* destAppDescriptor,
        const std::string            paramName,
        const std::string            paramType,
        const std::string            paramValue
    )
    throw (rubuilder::tester::exception::Exception);

    /**
     * Creates a ParameterGet SOAP message.
     */
    xoap::MessageReference createParameterGetSOAPMsg
    (
        const std::string appClass,
        const std::string paramName,
        const std::string paramType
    )
    throw (rubuilder::tester::exception::Exception);

    /**
     * Creates a ParameterSet SOAP message.
     */
    xoap::MessageReference createParameterSetSOAPMsg
    (
        const std::string appClass,
        const std::string paramName,
        const std::string paramType,
        const std::string paramValue
    )
    throw (rubuilder::tester::exception::Exception);

    /**
     * Returns the value of the specified parameter from the specified SOAP
     * message.
     */
    std::string extractScalarParameterValueFromSoapMsg
    (
        xoap::MessageReference msg,
        const std::string      paramName
    )
    throw (rubuilder::tester::exception::Exception);

    /**
     * Retruns the node with the specified local name from the specified list
     * of node.  An exception is thrown if the node is not found.
     */
    DOMNode *findNode
    (
        DOMNodeList       *nodeList,
        const std::string nodeLocalName
    )
    throw (rubuilder::tester::exception::Exception);

    /**
     * Prints the title bar found at the top of each web page.
     */
    void printWebPageTitleBar
    (
        xgi::Output       *out,
        const std::string pageIconSrc,
        const std::string pageIconAlt
    )
    throw (xgi::exception::Exception);

    /**
     * Prints one of the icons used to navigate this application's web site.
     */
    void printWebPageIcon
    (
        xgi::Output       *out,
        const std::string imgSrc,
        const std::string label,
        const std::string href
    )
    throw (xgi::exception::Exception);

}; // class Application

} } // namespace rubuilder::tester

#endif
