#include "rubuilder/utils/XoapUtils.h"
#include "rubuilder/utils/Exception.h"
#include "toolbox/Event.h"
#include "xcept/Exception.h"
#include "xcept/tools.h"
#include "xdaq/NamespaceURI.h"
#include "xoap/domutils.h"
#include "xoap/MessageFactory.h"
#include "xoap/SOAPBody.h"
#include "xoap/SOAPEnvelope.h"
#include "xoap/SOAPName.h"
#include "xoap/SOAPPart.h"

#include <iostream>
#include <string>


xoap::MessageReference rubuilder::utils::createFsmSoapResponseMsg
(
  const std::string event,
  const std::string state
)
{
  try
  {
    xoap::MessageReference message = xoap::createMessage();
    xoap::SOAPEnvelope envelope = message->getSOAPPart().getEnvelope();
    xoap::SOAPBody body = envelope.getBody();
    std::string responseString = event + "Response";
    xoap::SOAPName responseName =
      envelope.createName(responseString, "xdaq", XDAQ_NS_URI);
    xoap::SOAPBodyElement responseElement =
      body.addBodyElement(responseName);
    xoap::SOAPName stateName =
      envelope.createName("state", "xdaq", XDAQ_NS_URI);
    xoap::SOAPElement stateElement =
      responseElement.addChildElement(stateName);
    xoap::SOAPName attributeName =
      envelope.createName("stateName", "xdaq", XDAQ_NS_URI);
    
    
    stateElement.addAttribute(attributeName, state);
    
    return message;
  }
  catch(xcept::Exception &e)
  {
    XCEPT_RETHROW(exception::SOAP,
      "Failed to create FSM SOAP response message for event:" +
      event + " and result state:" + state,  e);
  }
}


void rubuilder::utils::printSoapMsgToStdOut
(
  xoap::MessageReference message
)
{
  DOMNode     *node = message->getEnvelope();
  std::string msgStr;
  
  
  xoap::dumpTree(node, msgStr);
  
  std::cout << "*************** MESSAGE START ****************\n";
  std::cout << msgStr << "\n";
  std::cout << "*************** MESSAGE FINISH ***************\n";
  std::cout << std::flush;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
