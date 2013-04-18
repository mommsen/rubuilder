#include "rubuilder/utils/I2OMessages.h"


std::ostream& operator<<
(
  std::ostream& s,
  const rubuilder::msg::EvtIdRqstAndOrRelease& rqstAndOrRelease
)
{
  s << "type=";
  if ( rqstAndOrRelease.requestType & rubuilder::msg::EvtIdRqstAndOrRelease::REQUEST )
    s << "REQUEST";
  
  if ( rqstAndOrRelease.requestType & rubuilder::msg::EvtIdRqstAndOrRelease::RELEASE )
    s << "RELEASE";
  s << " ";
  
  s << "resourceId="  << rqstAndOrRelease.resourceId << " ";
  s << rqstAndOrRelease.evbId;
  return s;
}


std::ostream& operator<<
(
  std::ostream& s,
  const rubuilder::msg::EvtIdRqstsAndOrReleasesMsg* msg
)
{
  s << "EvtIdRqstsAndOrReleasesMsg\n";
  
  s << "PvtMessageFrame.StdMessageFrame.VersionOffset=";
  s << msg->PvtMessageFrame.StdMessageFrame.VersionOffset << "\n";
  
  s << "PvtMessageFrame.StdMessageFrame.MsgFlags=";
  s << msg->PvtMessageFrame.StdMessageFrame.MsgFlags << "\n";
  
  s << "PvtMessageFrame.StdMessageFrame.MessageSize=";
  s << msg->PvtMessageFrame.StdMessageFrame.MessageSize << "\n";
  
  s << "PvtMessageFrame.StdMessageFrame.TargetAddress=";
  s << msg->PvtMessageFrame.StdMessageFrame.TargetAddress << "\n";
  
  s << "PvtMessageFrame.StdMessageFrame.InitiatorAddress=";
  s << msg->PvtMessageFrame.StdMessageFrame.InitiatorAddress << "\n";
  
  s << "PvtMessageFrame.StdMessageFrame.Function=";
  s << msg->PvtMessageFrame.StdMessageFrame.Function << "\n";
  
  s << "PvtMessageFrame.XFunctionCode=";
  s << msg->PvtMessageFrame.XFunctionCode << "\n";
  
  s << "PvtMessageFrame.OrganizationID=";
  s << msg->PvtMessageFrame.OrganizationID << "\n";
  
  s << "srcIndex="   << msg->srcIndex   << "\n";
  s << "nbElements=" << msg->nbElements << "\n";
  
  
  for (unsigned int i=0; i<msg->nbElements; ++i)
  {
    s << "elements[" << i << "]: " << msg->elements[i] << "\n";
  }
  
  return s;
}


std::ostream& operator<<
(
  std::ostream& s,
  const rubuilder::msg::EvBidsMsg* msg
)
{
  s << "EvBidsMsg\n";
  
  s << "PvtMessageFrame.StdMessageFrame.VersionOffset=";
  s << msg->PvtMessageFrame.StdMessageFrame.VersionOffset << "\n";
  
  s << "PvtMessageFrame.StdMessageFrame.MsgFlags=";
  s << msg->PvtMessageFrame.StdMessageFrame.MsgFlags << "\n";
  
  s << "PvtMessageFrame.StdMessageFrame.MessageSize=";
  s << msg->PvtMessageFrame.StdMessageFrame.MessageSize << "\n";
  
  s << "PvtMessageFrame.StdMessageFrame.TargetAddress=";
  s << msg->PvtMessageFrame.StdMessageFrame.TargetAddress << "\n";
  
  s << "PvtMessageFrame.StdMessageFrame.InitiatorAddress=";
  s << msg->PvtMessageFrame.StdMessageFrame.InitiatorAddress << "\n";
  
  s << "PvtMessageFrame.StdMessageFrame.Function=";
  s << msg->PvtMessageFrame.StdMessageFrame.Function << "\n";
  
  s << "PvtMessageFrame.XFunctionCode=";
  s << msg->PvtMessageFrame.XFunctionCode << "\n";
  
  s << "PvtMessageFrame.OrganizationID=";
  s << msg->PvtMessageFrame.OrganizationID << "\n";
  
  s << "nbElements=";
  s << msg->nbElements << "\n";
  
  for (unsigned int i=0; i<msg->nbElements; ++i)
  {
    s << "elements[" << i << "]: " << msg->elements[i] << "\n";
  }
  
  return s;
}


std::ostream& operator<<
(
  std::ostream& s,
  const rubuilder::msg::RqstForFrag &rqst
)
{
  s << rqst.evbId << " ";
  s << "buResourceId=" << rqst.buResourceId;

  return s;
}


std::ostream& operator<<
(
  std::ostream& s,
  const rubuilder::msg::RqstForFragsMsg *msg
)
{
  s << "RqstForFragsMsg\n";
  
  s << "PvtMessageFrame.StdMessageFrame.VersionOffset=";
  s << msg->PvtMessageFrame.StdMessageFrame.VersionOffset << "\n";
  
  s << "PvtMessageFrame.StdMessageFrame.MsgFlags=";
  s << msg->PvtMessageFrame.StdMessageFrame.MsgFlags << "\n";
  
  s << "PvtMessageFrame.StdMessageFrame.MessageSize=";
  s << msg->PvtMessageFrame.StdMessageFrame.MessageSize << "\n";
  
  s << "PvtMessageFrame.StdMessageFrame.TargetAddress=";
  s << msg->PvtMessageFrame.StdMessageFrame.TargetAddress << "\n";
  
  s << "PvtMessageFrame.StdMessageFrame.InitiatorAddress=";
  s << msg->PvtMessageFrame.StdMessageFrame.InitiatorAddress << "\n";
  
  s << "PvtMessageFrame.StdMessageFrame.Function=";
  s << msg->PvtMessageFrame.StdMessageFrame.Function << "\n";
  
  s << "PvtMessageFrame.XFunctionCode=";
  s << msg->PvtMessageFrame.XFunctionCode << "\n";
  
  s << "PvtMessageFrame.OrganizationID=";
  s << msg->PvtMessageFrame.OrganizationID << "\n";
  
  s << "srcIndex="   << msg->srcIndex   << "\n";
  s << "nbElements=" << msg->nbElements << "\n";
  
  
  for (unsigned int i=0; i<msg->nbElements; ++i)
  {
    s << "elements[" << i << "]: " << msg->elements[i] << "\n";
  }
  
  return s;
}


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
