#ifndef _rubuilder_utils_I2OMessages_h_
#define _rubuilder_utils_I2OMessages_h_

#include <iostream>
#include <stdint.h>

#include "i2o/i2o.h"
#include "rubuilder/utils/EvBid.h"


namespace rubuilder { namespace msg { // namespace rubuilder::msg

  /**
   * A request for an event id and/or a released event id.
   */
  typedef struct
  {
    enum BitMasksForRequest {
      REQUEST = 0x1,
      RELEASE = 0x2
    };
    uint32_t requestType; // Bit mask identifying release and/or request.
    uint32_t resourceId;  // Id of the resource within the BU - always filled.
    utils::EvBid evbId;   // Event builder id of the event.

  } EvtIdRqstAndOrRelease;


  /**
   * BU to EVM message that contains zero or more requests for event ids
   * plus zero or more released event ids.
   */
  typedef struct
  {
    I2O_PRIVATE_MESSAGE_FRAME PvtMessageFrame; // I2O information.
    uint32_t srcIndex;                         // Index of the source application.
    uint32_t nbElements;                       // Number of messages.
    EvtIdRqstAndOrRelease elements[1];         // Request and/or release messages

  } EvtIdRqstsAndOrReleasesMsg;


  /**
   * EVM to RU meesage that contains one or more EvBids.
   */
  typedef struct
  {
    I2O_PRIVATE_MESSAGE_FRAME PvtMessageFrame; // I2O information.
    uint32_t nbElements;                       // Number of EvB ids.
    uint32_t padding;                          // Padding for 64-bit alignment.
    utils::EvBid elements[1];                  // The list of EvB ids.
    
  } EvBidsMsg;


  /**
   * Contains the event id and trigger event number of the event fragment
   * wanted and the id of the resource that will be used to assemble the whole
   * event.
   */
  typedef struct
  {
    utils::EvBid evbId;    // Event builder id of the event.
    uint32_t buResourceId; // Id of the BU resource used to assemble the event.
    uint32_t padding;      // Padding for 64-bit alignment.
    
  } RqstForFrag;


  /**
   * BU to RU message that contains one or more requests for event
   * fragments, where each request is represented by the event id of the event
   * fragment wanted and the id of the resource that will be used to assemble
   * the whole event.
   */
  typedef struct
  {
    I2O_PRIVATE_MESSAGE_FRAME PvtMessageFrame; // I2O information.
    uint32_t srcIndex;                         // Index of the source application.
    uint32_t nbElements;                       // Number of requests
    RqstForFrag elements[1];                   // Requests
    
  } RqstForFragsMsg;


} } // namespace rubuilder::msg

std::ostream& operator<<
(
  std::ostream&,
  const rubuilder::msg::EvtIdRqstAndOrRelease&
);

std::ostream& operator<<
(
  std::ostream&,
  const rubuilder::msg::EvtIdRqstsAndOrReleasesMsg*
);

std::ostream& operator<<
(
  std::ostream&,
  const rubuilder::msg::EvBidsMsg*
);

std::ostream& operator<<
(
  std::ostream&,
  const rubuilder::msg::RqstForFrag&
);

std::ostream& operator<<
(
  std::ostream&,
  const rubuilder::msg::RqstForFragsMsg*
);


#endif // _rubuilder_utils_I2OMessages_h_


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
