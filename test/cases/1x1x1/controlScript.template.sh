#!/bin/sh

# Launch executive processes
sendCmdToLauncher FEROL0_SOAP_HOST_NAME FEROL0_LAUNCHER_PORT STARTXDAQFEROL0_SOAP_PORT
sendCmdToLauncher EVM0_SOAP_HOST_NAME EVM0_LAUNCHER_PORT STARTXDAQEVM0_SOAP_PORT
sendCmdToLauncher RU0_SOAP_HOST_NAME RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
sendCmdToLauncher BU1_SOAP_HOST_NAME BU1_LAUNCHER_PORT STARTXDAQBU1_SOAP_PORT

# Check that executives are listening
if ! webPingXDAQ FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
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
if ! webPingXDAQ BU1_SOAP_HOST_NAME BU1_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi


# Configure all executives
sendCmdToExecutive FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU0_SOAP_HOST_NAME RU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU1_SOAP_HOST_NAME BU1_SOAP_PORT configure.cmd.xml

# Configure and enable ptutcp
sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT pt::utcp::Application 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::utcp::Application 1 Configure
sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT pt::utcp::Application 0 Enable
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::utcp::Application 1 Enable

# Configure all applications
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Configure
sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT evb::test::DummyFEROL 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application 0 Configure
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT rubuilder::bu::Application 0 Configure


#Enable RUs
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application 0 Enable

#Enable EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Enable

#Enable BUs
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT rubuilder::bu::Application 0 Enable

#Enable FEROLs
sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT evb::test::DummyFEROL 0 Enable


echo "Building for 2 seconds"
sleep 8

nbEvtsBuiltBU1=`getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`

echo "BU1 nbEvtsBuilt: $nbEvtsBuiltBU1"

if test $nbEvtsBuiltBU1 -lt 1000
then
  echo "Test failed"
  exit 1
fi

echo "Test succeeded"
exit 0
