#!/bin/sh

# Launch executive processes
sendCmdToLauncher EVM0_SOAP_HOST_NAME EVM0_LAUNCHER_PORT STARTXDAQEVM0_SOAP_PORT
sendCmdToLauncher RU0_SOAP_HOST_NAME RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
sendCmdToLauncher RU1_SOAP_HOST_NAME RU1_LAUNCHER_PORT STARTXDAQRU1_SOAP_PORT
sendCmdToLauncher FU0_SOAP_HOST_NAME FU0_LAUNCHER_PORT STARTXDAQFU0_SOAP_PORT
sendCmdToLauncher FU1_SOAP_HOST_NAME FU1_LAUNCHER_PORT STARTXDAQFU1_SOAP_PORT

# Check that executives are listening
if ! webPingXDAQ EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ RU0_SOAP_HOST_NAME RU0_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ RU1_SOAP_HOST_NAME RU1_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FU0_SOAP_HOST_NAME FU0_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FU1_SOAP_HOST_NAME FU1_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi

# Configure all executives
sendCmdToExecutive EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU0_SOAP_HOST_NAME  RU0_SOAP_PORT  configure.cmd.xml
sendCmdToExecutive RU1_SOAP_HOST_NAME  RU1_SOAP_PORT  configure.cmd.xml
sendCmdToExecutive FU0_SOAP_HOST_NAME  FU0_SOAP_PORT  configure.cmd.xml
sendCmdToExecutive FU1_SOAP_HOST_NAME  FU1_SOAP_PORT  configure.cmd.xml

# Configure and enable ptatcp
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT pt::atcp::PeerTransportATCP 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT pt::atcp::PeerTransportATCP 1 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME  RU1_SOAP_PORT pt::atcp::PeerTransportATCP 2 Configure
sendSimpleCmdToApp FU0_SOAP_HOST_NAME  FU0_SOAP_PORT pt::atcp::PeerTransportATCP 3 Configure
sendSimpleCmdToApp FU1_SOAP_HOST_NAME  FU1_SOAP_PORT pt::atcp::PeerTransportATCP 4 Configure
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT pt::atcp::PeerTransportATCP 0 Enable
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT pt::atcp::PeerTransportATCP 1 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME  RU1_SOAP_PORT pt::atcp::PeerTransportATCP 2 Enable
sendSimpleCmdToApp FU0_SOAP_HOST_NAME  FU0_SOAP_PORT pt::atcp::PeerTransportATCP 3 Enable
sendSimpleCmdToApp FU1_SOAP_HOST_NAME  FU1_SOAP_PORT pt::atcp::PeerTransportATCP 4 Enable

# Configure all applications
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::ta::Application  0 Configure
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::rui::Application 0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::rui::Application 1 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application  0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::ru::Application  1 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::bu::Application  0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::bu::Application  1 Configure
sendSimpleCmdToApp FU0_SOAP_HOST_NAME FU0_SOAP_PORT rubuilder::fu::Application  0 Configure
sendSimpleCmdToApp FU1_SOAP_HOST_NAME FU1_SOAP_PORT rubuilder::fu::Application  1 Configure

#Enable RUs
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application  0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::ru::Application  1 Enable

#Enable EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Enable

#Enable BUs
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::bu::Application  0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::bu::Application  1 Enable

#Start generation of dummy super-fragments
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::rui::Application 0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::rui::Application 1 Enable

#Start servicing trigger credits
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::ta::Application  0 Enable

#Start filtering events
sendSimpleCmdToApp FU0_SOAP_HOST_NAME FU0_SOAP_PORT rubuilder::fu::Application  0 Enable
sendSimpleCmdToApp FU1_SOAP_HOST_NAME FU1_SOAP_PORT rubuilder::fu::Application  1 Enable

echo "Waiting 2 seconds"
sleep 2
echo "Finished waiting"

nbEvtsBuiltBU0=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
nbEvtsBuiltBU1=`getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::bu::Application 1 nbEvtsBuilt xsd:unsignedInt`

echo "BU0 nbEvtsBuilt: $nbEvtsBuiltBU0"
echo "BU1 nbEvtsBuilt: $nbEvtsBuiltBU1"

if test $nbEvtsBuiltBU0 -gt 1000 -a $nbEvtsBuiltBU1 -gt 1000
then
  echo "Test succeeded"
  exit 0
else
  echo "Test failed"
  exit 1
fi
