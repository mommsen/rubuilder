#!/bin/sh

# Launch executive processes
sendCmdToLauncher RU0_SOAP_HOST_NAME  RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
sendCmdToLauncher EVM0_SOAP_HOST_NAME EVM0_LAUNCHER_PORT STARTXDAQEVM0_SOAP_PORT
sendCmdToLauncher BU0_SOAP_HOST_NAME  BU0_LAUNCHER_PORT STARTXDAQBU0_SOAP_PORT
sendCmdToLauncher FU0_SOAP_HOST_NAME  FU0_LAUNCHER_PORT STARTXDAQFU0_SOAP_PORT

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
if ! webPingXDAQ BU0_SOAP_HOST_NAME BU0_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FU0_SOAP_HOST_NAME FU0_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi


# Configure all executives
sendCmdToExecutive RU0_SOAP_HOST_NAME  RU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU0_SOAP_HOST_NAME  BU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FU0_SOAP_HOST_NAME  FU0_SOAP_PORT configure.cmd.xml

# Configure and enable ptatcp
#sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT pt::atcp::PeerTransportATCP 0 Configure
#sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT pt::atcp::PeerTransportATCP 1 Configure
#sendSimpleCmdToApp FU0_SOAP_HOST_NAME  FU0_SOAP_PORT pt::atcp::PeerTransportATCP 5 Configure
#sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT pt::atcp::PeerTransportATCP 0 Enable
#sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT pt::atcp::PeerTransportATCP 1 Enable
#sendSimpleCmdToApp FU0_SOAP_HOST_NAME  FU0_SOAP_PORT pt::atcp::PeerTransportATCP 5 Enable

# Configure and enable ptatcp
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT pt::utcp::Application 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT pt::utcp::Application 0 Enable


# Configure all applications
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT rubuilder::ru::Application  0 Configure
sendSimpleCmdToApp FU0_SOAP_HOST_NAME  FU0_SOAP_PORT rubuilder::bu::Application  0 Configure

#Set parameter
sleep 4
setParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT Client  0 currentSize unsignedLong 128

setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT Server  0 currentSize unsignedLong 128


#Enable RUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::ru::Application  0 Enable

#Enable EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Enable

#Enable BUs
sendSimpleCmdToApp FU0_SOAP_HOST_NAME FU0_SOAP_PORT rubuilder::bu::Application  0 Enable

#Enable BUs (FEROL)
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT Server 0 start

#Enable RUs (FEROL)
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT Client 0 start


echo "Building for 2 seconds"
sleep 8

nbEvtsBuiltBU0=`getParam FU0_SOAP_HOST_NAME FU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`

echo "BU0 nbEvtsBuilt: $nbEvtsBuiltBU0"

if test $nbEvtsBuiltBU0 -lt 1000
then
  echo "Test failed"
  exit 1
fi

nbCounterServerRU0=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT Client 0 counter  xsd:unsignedLong`

nbCounterServerBU0=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT Server 0 counter  xsd:unsignedLong`

echo "Client 0 nbEvtsBuilt: $nbCounterServerRU0"

echo "Server 0 nbEvtsBuilt: $nbCounterServerBU0"

if test $nbCounterServerRU0 -lt 100
then
  echo "Test failed"
  exit 1
fi

if test $nbCounterServerBU0 -lt 100
then
  echo "Test failed"
  exit 1
fi


echo "Test succeeded"
exit 0
