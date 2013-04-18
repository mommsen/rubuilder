#!/bin/sh

# Launch executive processes
sendCmdToLauncher EVM0_SOAP_HOST_NAME EVM0_LAUNCHER_PORT STARTXDAQEVM0_SOAP_PORT
sendCmdToLauncher RU0_SOAP_HOST_NAME  RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
sendCmdToLauncher RU1_SOAP_HOST_NAME  RU1_LAUNCHER_PORT STARTXDAQRU1_SOAP_PORT
sendCmdToLauncher BU0_SOAP_HOST_NAME  BU0_LAUNCHER_PORT STARTXDAQBU0_SOAP_PORT

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
if ! webPingXDAQ BU0_SOAP_HOST_NAME BU0_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi

# Configure all executives
sendCmdToExecutive EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU0_SOAP_HOST_NAME  RU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU1_SOAP_HOST_NAME  RU1_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU0_SOAP_HOST_NAME  BU0_SOAP_PORT configure.cmd.xml

# Configure and enable ptatcp
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT pt::atcp::PeerTransportATCP 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT pt::atcp::PeerTransportATCP 1 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME  RU1_SOAP_PORT pt::atcp::PeerTransportATCP 2 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT pt::atcp::PeerTransportATCP 3 Configure
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT pt::atcp::PeerTransportATCP 0 Enable
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT pt::atcp::PeerTransportATCP 1 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME  RU1_SOAP_PORT pt::atcp::PeerTransportATCP 2 Enable
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT pt::atcp::PeerTransportATCP 3 Enable

# Configure all applications
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT rubuilder::ru::Application  0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME  RU1_SOAP_PORT rubuilder::ru::Application  1 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT rubuilder::bu::Application  0 Configure

#Enable RUs
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application  0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::ru::Application  1 Enable

#Enable EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Enable

#Enable BU
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application  0 Enable

echo "Waiting 2 seconds"
sleep 2
echo "Finished waiting"

nbEvtsBuilt=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
echo "BU0 nbEvtsBuilt=$nbEvtsBuilt"
if test $nbEvtsBuilt -lt 1000
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

state=`getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::ru::Application 1 stateName xsd:string`
echo "RU1 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

state=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 stateName xsd:string`
echo "BU0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

echo "Test succeeded"
exit 0
