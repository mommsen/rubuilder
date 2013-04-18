#!/bin/sh

# Launch executive processes
sendCmdToLauncher EVM0_SOAP_HOST_NAME EVM0_LAUNCHER_PORT STARTXDAQEVM0_SOAP_PORT
sendCmdToLauncher RU0_SOAP_HOST_NAME RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
sendCmdToLauncher RU1_SOAP_HOST_NAME RU1_LAUNCHER_PORT STARTXDAQRU1_SOAP_PORT
sendCmdToLauncher BU0_SOAP_HOST_NAME BU0_LAUNCHER_PORT STARTXDAQBU0_SOAP_PORT

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

echo "Set rubuilder::ru::Application mask: RU0 is in and RU1 is out"
sendCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 setEvm_Ru0InRu1Out.cmd.xml
sendCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT rubuilder::bu::Application  0 setBu__Ru0InRu1Out.cmd.xml

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

#Enable BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application  0 Enable

echo "Building for 2 seconds"
sleep 2

nbEvtsBuiltBU0=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
echo "BU0 nbEvtsBuilt: $nbEvtsBuiltBU0"

if test $nbEvtsBuiltBU0 -lt 1000
then
  echo "Test failed"
  exit 1
fi

nbSuperFragsRU0Sent=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application 0 i2oBUCacheCount xsd:unsignedLong`
nbSuperFragsRU1Sent=`getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::ru::Application 1 i2oBUCacheCount xsd:unsignedLong`

echo "RU0 nbSuperFragsSent: $nbSuperFragsRU0Sent"
echo "RU1 nbSuperFragsSent: $nbSuperFragsRU1Sent"

if test $nbSuperFragsRU0Sent -lt 1000
then
  echo "Test failed"
  exit 1
fi

if test $nbSuperFragsRU1Sent -ne 0
then
  echo "Test failed"
  exit 1
fi

# Halt all applications
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Halt
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT rubuilder::ru::Application  0 Halt
sendSimpleCmdToApp RU1_SOAP_HOST_NAME  RU1_SOAP_PORT rubuilder::ru::Application  1 Halt
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT rubuilder::bu::Application  0 Halt

sleep 1

nbEvtsBuiltAtHaltBU0=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
echo "BU0 nbEvtsBuiltAtHalt: $nbEvtsBuiltAtHaltBU0"

echo "Purging for 1 second"
sleep 1

nbEvtsBuiltBU0=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
echo "BU0 nbEvtsBuilt: $nbEvtsBuiltBU0"

if test $nbEvtsBuiltBU0 -ne $nbEvtsBuiltAtHaltBU0
then
  echo "Test failed"
  exit 1
fi

echo "Set rubuilder::ru::Application mask: RU0 is out and RU1 is in"
sendCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 setEvm_Ru0OutRu1In.cmd.xml
sendCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT rubuilder::bu::Application  0 setBu__Ru0OutRu1In.cmd.xml

# Configure all applications
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT  rubuilder::ru::Application  0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT  rubuilder::ru::Application  1 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT  rubuilder::bu::Application  0 Configure

#Enable RUs
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application  0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::ru::Application  1 Enable

#Enable EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Enable

#Enable BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application  0 Enable

echo "Building for 2 seconds"
sleep 2

nbEvtsBuiltBU0=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
echo "BU0 nbEvtsBuilt: $nbEvtsBuiltBU0"

if test $nbEvtsBuiltBU0 -lt 1000
then
  echo "Test failed"
  exit 1
fi

nbSuperFragsRU0Sent=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application 0 i2oBUCacheCount xsd:unsignedLong`
nbSuperFragsRU1Sent=`getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::ru::Application 1 i2oBUCacheCount xsd:unsignedLong`

echo "RU0 nbSuperFragsSent: $nbSuperFragsRU0Sent"
echo "RU1 nbSuperFragsSent: $nbSuperFragsRU1Sent"

if test $nbSuperFragsRU0Sent -ne 0
then
  echo "Test failed"
  exit 1
fi

if test $nbSuperFragsRU1Sent -lt 1000
then
  echo "Test failed"
  exit 1
fi

echo "Test succeeded"
exit 0
