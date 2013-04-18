#!/bin/sh

# Launch executive processes
sendCmdToLauncher EVM0_SOAP_HOST_NAME EVM0_LAUNCHER_PORT STARTXDAQEVM0_SOAP_PORT
sendCmdToLauncher RU0_SOAP_HOST_NAME RU0_LAUNCHER_PORT  STARTXDAQRU0_SOAP_PORT
sendCmdToLauncher RU1_SOAP_HOST_NAME RU1_LAUNCHER_PORT  STARTXDAQRU1_SOAP_PORT
sendCmdToLauncher FU0_SOAP_HOST_NAME FU0_LAUNCHER_PORT  STARTXDAQFU0_SOAP_PORT
sendCmdToLauncher FU1_SOAP_HOST_NAME FU1_LAUNCHER_PORT  STARTXDAQFU1_SOAP_PORT

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
sendCmdToExecutive RU0_SOAP_HOST_NAME RU0_SOAP_PORT  configure.cmd.xml
sendCmdToExecutive RU1_SOAP_HOST_NAME RU1_SOAP_PORT  configure.cmd.xml
sendCmdToExecutive FU0_SOAP_HOST_NAME FU0_SOAP_PORT  configure.cmd.xml
sendCmdToExecutive FU1_SOAP_HOST_NAME FU1_SOAP_PORT  configure.cmd.xml

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
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT  rubuilder::ru::Application  0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT  rubuilder::bu::Application  0 Configure
sendSimpleCmdToApp FU0_SOAP_HOST_NAME FU0_SOAP_PORT  rubuilder::fu::Application  0 Configure
sendSimpleCmdToApp FU1_SOAP_HOST_NAME FU1_SOAP_PORT  rubuilder::fu::Application  1 Configure

#Enable RU
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application  0 Enable

#Enable EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Enable

#Enable BU
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::bu::Application  0 Enable

#Start filtering events
sendSimpleCmdToApp FU0_SOAP_HOST_NAME FU0_SOAP_PORT rubuilder::fu::Application  0 Enable
sendSimpleCmdToApp FU1_SOAP_HOST_NAME FU1_SOAP_PORT rubuilder::fu::Application  1 Enable

echo "Building for 10 seconds"
sleep 10

nbEvtsBuilt=`getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
echo "BU0 nbEvtsBuilt=$nbEvtsBuilt"
if test $nbEvtsBuilt -lt 1000
then
  echo "Test failed"
  exit 1
fi

state=`getParam FU0_SOAP_HOST_NAME FU0_SOAP_PORT rubuilder::fu::Application 0 stateName xsd:string`
echo "FU0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

state=`getParam FU1_SOAP_HOST_NAME FU1_SOAP_PORT rubuilder::fu::Application 1 stateName xsd:string`
echo "FU1 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

state=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 stateName xsd:string`
echo "EVM0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

state=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application 0 stateName xsd:string`
echo "RU0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

state=`getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::bu::Application 0 stateName xsd:string`
echo "BU0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

echo "Test succeeded"
exit 0
