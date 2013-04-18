#!/bin/sh

# Launch executive process
sendCmdToLauncher EVM0_SOAP_HOST_NAME EVM0_LAUNCHER_PORT STARTXDAQEVM0_SOAP_PORT

if ! webPingXDAQ EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi

# Configure executive
sendCmdToExecutive EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT configure.cmd.xml

# Configure all applications
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Configure
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::ru::Application  0 Configure
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::bu::Application  0 Configure

#Enable RU
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::ru::Application  0 Enable

#Enable EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Enable

#Enable BU
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::bu::Application  0 Enable

echo "Building for 2 seconds"
sleep 2

nbEvtsBuilt=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
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

state=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::ru::Application 0 stateName xsd:string`
echo "RU0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

state=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::bu::Application 0 stateName xsd:string`
echo "BU0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

echo "Test succeeded"
exit 0
