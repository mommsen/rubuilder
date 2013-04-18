#ifndef _rubuilder_fu_Application_h_
#define _rubuilder_fu_Application_h_

#include "rubuilder/fu/FedSourceIdSet.h"
#include "rubuilder/fu/SynchronizedString.h"
#include "rubuilder/fu/exception/Exception.h"
#include "rubuilder/utils/WebUtils.h"
#include "i2o/i2oDdmLib.h"
#include "i2o/utils/AddressMap.h"
#include "toolbox/BSem.h"
#include "toolbox/fsm/FiniteStateMachine.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "xdaq/ApplicationGroup.h"
#include "xdaq/WebApplication.h"
#include "xdaq2rc/SOAPParameterExtractor.hh"
#include "xdata/Boolean.h"
#include "xdata/Double.h"
#include "xdata/InfoSpace.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"

#include <stdint.h>


namespace rubuilder { namespace fu { // namespace rubuilder::fu

/**
 * Example Filter Unit (FU) to be copied and modified by end-users.
 */
class Application :
public xdaq::WebApplication
{
public:

    /**
     * Define factory method for the instantion of FU applications.
     */
    XDAQ_INSTANTIATOR();

    /**
     * Constructor.
     */
    Application(xdaq::ApplicationStub *s)
    throw (xdaq::exception::Exception);


private:

    /**
     * Pointer to the descriptor of the RUBuilderTester application.
     *
     * It is normal for this pointer to be 0 if the RUBuilderTester application      * cannot be found.
     */
    xdaq::ApplicationDescriptor *rubuilderTesterDescriptor_;

    /**
     * I2o exception handler.
     */
    toolbox::exception::HandlerSignature *i2oExceptionHandler_;

    /**
     * The logger of this application.
     */
    Logger logger_;

    /**
     * The name of the info space that contains exported parameters used for
     * monitoring.
     */
    std::string monitoringInfoSpaceName_;

    /**
     * Info space that contains exported parameters used for monitoring.
     */
    xdata::InfoSpace *monitoringInfoSpace_;

    /**
     * The finite state machine of the application.
     */
    toolbox::fsm::FiniteStateMachine fsm_;

    /**
     * Helper class to extract FSM command and parameters from SOAP command
     */
    xdaq2rc::SOAPParameterExtractor soapParameterExtractor_;

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
    unsigned int instance_;

    /**
     * The application's URN.
     */
    std::string urn_;

    /**
     * The I2O tid of the application.
     */
    I2O_TID tid_;

    /**
     * Binary semaphore used to protect the internal data structures of the
     * application from multithreaded access.
     */
    toolbox::BSem applicationBSem_;

    /**
     * Used to display on the application web-pages the reason why the finite
     * state machine moved to the failed state if it did.
     */
    SynchronizedString reasonForFailed_;

    /**
     * Used to display on the application web-pages the description of the
     * event data fault detected if there is one.
     *
     * This parameter is reset to an empty string when the application is
     * configured.
     */
    SynchronizedString faultDescription_;

    /**
     * Name of the memory pool for creating FU to BU I2O control messages.
     */
    std::string i2oPoolName_;

    /**
     * Memory pool used for creating FU to BU I2O control messages.
     */
    toolbox::mem::Pool *i2oPool_;

    /**
     * The application descriptor of the BU from which the FU will request
     * events.
     */
    xdaq::ApplicationDescriptor *buDescriptor_;

    /**
     * The I2O tid of the BU from which the FU will request events.
     */
    I2O_TID buTid_;

    /**
     * The application's standard configuration parameters.
     */
    std::vector< std::pair<std::string, xdata::Serializable *> >
        stdConfigParams_;

    /**
     * The application's debug (private to developer) configuration parameters.
     */
    std::vector< std::pair<std::string, xdata::Serializable *> >
        dbgConfigParams_;

    /**
     * The application's monitor counters.
     */
    std::vector< std::pair<std::string, xdata::UnsignedInteger32 *> >
        monitorCounters_;

    /**
     * The application's standard monitoring parameters.
     */
    std::vector< std::pair<std::string, xdata::Serializable *> >
        stdMonitorParams_;

    /**
     * The application's debug (private to developer) monitoring parameters.
     */
    std::vector< std::pair<std::string, xdata::Serializable *> >
        dbgMonitorParams_;

    /**
     * The set of FED source ids of the current event.
     */
    FedSourceIdSet fedSourceIds_;

    /**
     * A history of the FED source ids which have been passed to the FU since
     * it was configured.
     */
    FedSourceIdSet fedSourceIdHistory_;

    /////////////////////////////////////////////////////////////
    // Beginning of exported parameters used for configuration //
    /////////////////////////////////////////////////////////////

    /**
     * Exported read/write parameter - The class name of the  BU that the FU
     * will request events from.
     */
    xdata::String buClass_;

    /**
     * Exported read/write parameter - The instance number of BU that the FU
     * will request events from.
     */
    xdata::UnsignedInteger32 buInstNb_;

    /**
     * Exported read/write parameter - Number of requests the FU should keep
     * outstanding between itself and the BU servicing its requests.
     */
    xdata::UnsignedInteger32 nbOutstandingRqsts_;

    /**
     * Exported read-only parameter specifying whether or not the FU should
     * sleep between events.
     *
     * This is used to simulate the time taken by a real FU to process an
     * event.
     */
    xdata::Boolean sleepBetweenEvents_;

    /**
     * Exported read-only parameter specifying the time in micro seconds the
     * FU should sleep when simulating the time taken by a real FU to process
     * an event.
     */
    xdata::UnsignedInteger32 sleepIntervalUSec_;

    /**
     * Exported read/write parameter specifying the number of events before the
     * FU will call exit() - the default value of 0 means that the FU will
     * never call exit().
     *
     * This exported parameter is for the sole purpose of testing a BU's
     * ability to tolerate a FU crashing.  A FU is configured to call exit when
     * it is to emulate a crash.
     */
    xdata::UnsignedInteger32 nbEventsBeforeExit_;

    /////////////////////////////////////////////////////////////
    // End of exported parameters used for configuration       //
    /////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////
    // Beginning of exported parameters used for monitoring //
    //////////////////////////////////////////////////////////

    /**
     * Exported read-only parameter specifying the current state of the
     * application.
     */
    xdata::String stateName_;

    /**
     * Exported read-only parameter - Total number of event the FU has
     * processed since it was last
     * configured.
     */
    xdata::UnsignedInteger32 nbEventsProcessed_;

    /**
     * Exported read-only parameter specifying where or not a fault has been
     * detected in the event data.
     *
     * This parameter is reset to "false" when the FU is configured.
     */
    xdata::Boolean faultDetected_;

    //////////////////////////////////////////////////////////
    // End of exported parameters used for monitoring       //
    //////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////
    // Beginning of exported parameters showing message payloads and counts //
    //////////////////////////////////////////////////////////////////////////

    /**
     * Exported read-only parameter specifying the payload transfered
     * by I2O_BU_ALLOCATE messages.
     */
    xdata::UnsignedInteger32 I2O_BU_ALLOCATE_Payload_;

    /**
     * Exported read-only parameter specifying the logical message
     * count for I2O_BU_ALLOCATE messages.
     */
    xdata::UnsignedInteger32 I2O_BU_ALLOCATE_LogicalCount_;

    /**
     * Exported read-only parameter specifying the i2o message frame
     * count for I2O_BU_ALLOCATE messages.
     */
    xdata::UnsignedInteger32 I2O_BU_ALLOCATE_I2oCount_;

    /**
     * Exported read-only parameter specifying the payload transfered
     * by I2O_BU_DISCARD messages.
     */
    xdata::UnsignedInteger32 I2O_BU_DISCARD_Payload_;

    /**
     * Exported read-only parameter specifying the logical message
     * count for I2O_BU_DISCARD messages.
     */
    xdata::UnsignedInteger32 I2O_BU_DISCARD_LogicalCount_;

    /**
     * Exported read-only parameter specifying the i2o message frame
     * count for I2O_BU_DISCARD messages.
     */
    xdata::UnsignedInteger32 I2O_BU_DISCARD_I2oCount_;

    /**
     * Exported read-only parameter specifying the payload transfered
     * by I2O_FU_TAKE messages.
     */
    xdata::UnsignedInteger32 I2O_FU_TAKE_Payload_;

    /**
     * Exported read-only parameter specifying the logical message
     * count for I2O_FU_TAKE messages.
     */
    xdata::UnsignedInteger32 I2O_FU_TAKE_LogicalCount_;

    /**
     * Exported read-only parameter specifying the i2o message frame
     * count for I2O_FU_TAKE messages.
     */
    xdata::UnsignedInteger32 I2O_FU_TAKE_I2oCount_;

    //////////////////////////////////////////////////////////////////////////
    // End of exported parameters showing message payloads and counts       //
    //////////////////////////////////////////////////////////////////////////


    /////////////////////////////////////////////////////////
    // Beginning of exported parameters used for debugging //
    /////////////////////////////////////////////////////////

    /**
     * For debugging only.
     *
     * Exported read-only parameter specifying the value of the
     * nbSuperFragmentsInEvent field of the last event data block to be
     * processed by the FU.
     */
    xdata::UnsignedInteger32 nbSuperFragmentsInEvent_;

    /////////////////////////////////////////////////////////
    // End of exported parameters used for debugging       //
    /////////////////////////////////////////////////////////


    /**
     * Head of super-fragment under-construction.
     */
    toolbox::mem::Reference *superFragmentHead_;

    /**
     * Tail of super-fragment under-construction.
     */
    toolbox::mem::Reference *superFragmentTail_;

    /**
     * Current block number of the super-fragment under construction.
     */
    unsigned int blockNb_;

    /**
     * Returns the name to be given to the logger of this application.
     */
    std::string generateLoggerName();

    /**
     * Returns a pointer to the descriptor of the RUBuilderTester application,
     * or 0 if the application cannot be found, which will be the case when
     * testing is not being performed.
     */
    xdaq::ApplicationDescriptor *getRUBuilderTester();

    /**
     * Returns the name of the info space that contains exported parameters
     * for monitoring.
     */
    std::string generateMonitoringInfoSpaceName
    (
        const std::string  appClass,
        const unsigned int appInstance
    );

    /**
     * Initialises and returns the application's standard configuration
     * parameters.
     */
    std::vector< std::pair<std::string, xdata::Serializable*> >
        initAndGetStdConfigParams();

    /**
     * Initialises and returns the application's debug (private to developer)
     * configuration parameters.
     */
    std::vector< std::pair<std::string, xdata::Serializable*> >
        initAndGetDbgConfigParams();

    /**
     * Resets the specified counters to zero.
     */
    void resetCounters
    (
        std::vector
        <
            std::pair<std::string, xdata::UnsignedInteger32*>
        > &counters
    );

    /**
     * Adds the specified counters vector to specified general parameters
     * vector.
     */
    void addCountersToParams
    (
        std::vector
        <
            std::pair<std::string, xdata::UnsignedInteger32*>
        > &counters,
        std::vector
        <
            std::pair<std::string, xdata::Serializable*>
        > &params
    );

    /**
     * Returns the application's monitor counters.
     */
    std::vector< std::pair<std::string, xdata::UnsignedInteger32*> >
    getMonitorCounters();

    /**
     * Initialises and returns the application's standard monitoring
     * parameters.
     */
    std::vector< std::pair<std::string, xdata::Serializable*> >
    initAndGetStdMonitorParams();

    /**
     * Initialises and returns the application's debug (private to developer)
     * monitoring parameters.
     */
    std::vector< std::pair<std::string, xdata::Serializable*> >
    initAndGetDbgMonitorParams();

    /**
     * Puts the specified parameters into the specified info space.
     */
    void putParamsIntoInfoSpace
    (
        std::vector< std::pair<std::string, xdata::Serializable*> > &params,
        xdata::InfoSpace                                            *s
    )
    throw (rubuilder::fu::exception::Exception);

    /**
     * Defines the finite state machine of the application.
     */
    void defineFsm()
    throw (rubuilder::fu::exception::Exception);

    /**
     * Binds the SOAP callbacks required to implement the finite state machine
     * of the application.
     */
    void bindFsmSoapCallbacks();

    /**
     * Web page used to visualise and control the finite state machine of the
     * application.
     */
    void fsmWebPage(xgi::Input *in, xgi::Output *out)
    throw (xgi::exception::Exception);

    /**
     * Processes the form sent from the finite state machine web page.
     */
    void processFsmForm(xgi::Input *in)
    throw (xgi::exception::Exception);

    /**
     * Callback responsible for processing FSM events received via
     * SOAP messages.
     */
    xoap::MessageReference processSoapFsmEvent(xoap::MessageReference msg)
    throw (xoap::exception::Exception);

    /**
     * Binds the I2O callbacks of the application.
     */
    void bindI2oCallbacks();

    /**
     * Binds the XGI callbacks of the application.
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
     * Prints the default web page.
     */
    void printDefaultWebPage(xgi::Input *in, xgi::Output *out)
    throw (xgi::exception::Exception);

    /**
     * Displays the developer's web page used for debugging.
     */
    void debugWebPage
    (
        xgi::Input  *in,
        xgi::Output *out
    )
    throw (xgi::exception::Exception);

    /**
     * Prints the specified parameters as an HTML table with the specified name.
     */
    void printParamsTable
    (
        xgi::Input                                                  *in,
        xgi::Output                                                 *out,
        const std::string                                           name,
        std::vector< std::pair<std::string, xdata::Serializable*> > &params
    )
    throw (xgi::exception::Exception);

    /**
     * SOAP callback thats reset all the monitoring counters.
     */
    xoap::MessageReference resetMonitoringCountersMsg
    (
        xoap::MessageReference msg
    )
    throw (xoap::exception::Exception);

    /**
     * Callback implementing the action to be executed on the
     * Halted->Ready transition.
     */
    void configureAction(toolbox::Event::Reference e)
    throw (toolbox::fsm::exception::Exception);

    /**
     * Callback implementing the action to be executed on the
     * Ready->Enabled transition.
     */
    void enableAction(toolbox::Event::Reference e)
    throw (toolbox::fsm::exception::Exception);

    /**
     * Callback implementing the action to be executed on the
     * ANY STATE->Halted transition.
     */
    void haltAction(toolbox::Event::Reference e)
    throw (toolbox::fsm::exception::Exception);

    /**
     * Callback implementing the action to be executed on the
     * ANY STATE->Failed transition.
     */
    void failAction(toolbox::Event::Reference event)
    throw (toolbox::fsm::exception::Exception);

    /**
     * Callback invoked when the state machine of the application has changed.
     */
    void stateChanged(toolbox::fsm::FiniteStateMachine & fsm)
    throw (toolbox::fsm::exception::Exception);

    /**
     * Invoked when an event data block has been received from the BU.
     */
    void I2O_FU_TAKE_Callback(toolbox::mem::Reference *bufRef);

    /**
     * Invoked when an end-of-lumi-section message  has been received from the BU.
     */
    void I2O_EVM_LUMISECTION_Callback(toolbox::mem::Reference *bufRef);

    /**
     * Processes the specified data block.
     */
    void processDataBlock(toolbox::mem::Reference *bufRef)
    throw (rubuilder::fu::exception::Exception);

    /**
     * Checks the payload sent by the TA.
     */
    void checkTAPayload(toolbox::mem::Reference *bufRef)
    throw (rubuilder::fu::exception::Exception);

    /**
     * Checks the payload sent by a RUI.
     */
    void checkRUIPayload(toolbox::mem::Reference *bufRef)
    throw (rubuilder::fu::exception::Exception);

    /**
     * Appends the specified block to the end of the super-fragment under
     * construction.
     */
    void appendBlockToSuperFragment(toolbox::mem::Reference *bufRef);

    /**
     * Checks the current super-fragment under construction.
     *
     * Note that the super-fragment under constrcution must be complete when
     * this method is called.
     */
    void checkSuperFragment()
    throw (rubuilder::fu::exception::Exception);

    /**
     * Returns the size in bytes of the sum of all the FED data that contained
     * witin the specified super-fragment.
     */
    unsigned int getSumOfFedData(toolbox::mem::Reference *bufRef);

    /**
     * Fills the specifed buffer with all the FED data from the specified
     * super-fragment.
     */
    void fillBufferWithSuperFragment
    (
        unsigned char           *buf,
        unsigned int            len,
        toolbox::mem::Reference *bufRef
    )
    throw (rubuilder::fu::exception::Exception);

    /**
     * Checks to see if the FED data in the specified buffer can be traversed
     * from trailer to header.
     */
    void checkFedTraversal
    (
        unsigned char* buf,
        unsigned int len
    )
    throw (rubuilder::fu::exception::Exception);

    /**
     * Checks for duplicate FED source IDs.
     */
    void checkFedSourceIds
    (
        unsigned char* buf,
        unsigned int len
    )
    throw (rubuilder::fu::exception::Exception);

    /**
     * Releases the memory used by the super-fragment under construction.
     */
    void releaseSuperFragment();

    /**
     * Creates and then sends an I2O_BU_ALLOCATE_MESSAGE_FRAME to the BU.
     */
    void allocateNEvents(const int n)
    throw (rubuilder::fu::exception::Exception);

    /**
     * Returns a new I2O_BU_ALLOCATE_MESSAGE_FRAME representing a request for
     * the specified number of events.
     */
    toolbox::mem::Reference *createBuAllocateMsg
    (
        toolbox::mem::MemoryPoolFactory *poolFactory,
        toolbox::mem::Pool              *pool,
        const I2O_TID                   taTid,
        const I2O_TID                   buTid,
        const int                       nbEvents
    )
    throw (rubuilder::fu::exception::Exception);

    /**
     * Creates and then sends an I2O_BU_DISCARD_MESSAGE_FRAME to the BU.
     */
    void discardEvent(const U32 buResourceId)
    throw (rubuilder::fu::exception::Exception);

    /**
     * Returns a new I2O_BU_DISCARD_MESSAGE_FRAME representing a request to
     * discard the event with the specified BU resource id.
     */
    toolbox::mem::Reference *createBuDiscardMsg
    (
        toolbox::mem::MemoryPoolFactory *poolFactory,
        toolbox::mem::Pool              *pool,
        const I2O_TID                   taTid,
        const I2O_TID                   buTid,
        const U32                       buResourceId
    )
    throw (rubuilder::fu::exception::Exception);

    /**
     * Returns the name of the memory pool used for creating FU to BU I2O
     * control messages.
     */
    std::string createI2oPoolName(const unsigned int  fuInstance);

    /**
     * Returns a "HeapAllocator" memory pool with the specified name.
     */
    toolbox::mem::Pool *createHeapAllocatorMemoryPool
    (
        toolbox::mem::MemoryPoolFactory *poolFactory,
        const std::string               poolName
    )
    throw (rubuilder::fu::exception::Exception);

    /**
     * Returns the url of the specified application.
     */
    std::string getUrl(xdaq::ApplicationDescriptor *appDescriptor);

    /**
     * Invoked when there is an I2O exception.
     */
    bool onI2oException(xcept::Exception &exception, void *context);

    /**
     * Creates and returns the error message of an I2O exception by specifying
     * the source and destination involved.
     */
    std::string createI2oErrorMsg
    (
        xdaq::ApplicationDescriptor *source,
        xdaq::ApplicationDescriptor *destination
    );

}; // class Application

} } // namespace rubuilder::fu

#endif
