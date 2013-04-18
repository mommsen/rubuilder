#ifndef _rubuilder_utils_Exception_h_
#define _rubuilder_utils_Exception_h_

#include "xcept/Exception.h"

/**
 * Exception raised in case of a configuration problem
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, Configuration)

/**
 * Exception raised by FragmentSets on a DOM error.
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, DOM)

/**
 * Exception raised by the EVM when failing to provide dummy triggers
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, DummyTrigger)

/**
 * Exception raised when encountering an out-of-order event
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, EventOrder)

/**
 * Exception raised by queue templates
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, FIFO)

/**
 * Exception raised by FragmentSets if a fragment is not found.
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, FragmentNotFound)

/**
 * Exception raised when a state machine problem arises
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, FSM)

/**
 * Exception raised when an I2O problem occured
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, I2O)

/**
 * Exception raised when encountering an info-space problem
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, InfoSpace)

/**
 * Exception raised by the EVM when encountering a L1 scaler problem
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, L1Scalers)

/**
 * Exception raised by the EVM when encountering a L1 trigger problem
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, L1Trigger)

/**
 * Exception raised when a super-fragment mismatch occured
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, MismatchDetected)

/**
 * Exception raised when encountering a problem providing monitoring information
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, Monitoring)

/**
 * Exception raised when running out of memory
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, OutOfMemory)

/**
 * Exception raised when on response to a SOAP message
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, SOAP)

/**
 * Exception raised when a super-fragment problem occured
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, SuperFragment)

/**
 * Exception raised when a super-fragment timed-out
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, TimedOut)

/**
 * Exception raised by issues with xdaq::WorkLoop
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, WorkLoop)

/**
 * Exception raised when failing to write data do disk
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, DiskWriting)

/**
 * Exception raised when failing to create XGI documents
 */
XCEPT_DEFINE_EXCEPTION(rubuilder, XGI)


#endif
