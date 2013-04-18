#!/bin/sh

# Launch executive processes
sendCmdToLauncher EVM0_SOAP_HOST_NAME EVM0_LAUNCHER_PORT STARTXDAQEVM0_SOAP_PORT
sendCmdToLauncher RU0_SOAP_HOST_NAME RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
sendCmdToLauncher RU1_SOAP_HOST_NAME RU1_LAUNCHER_PORT STARTXDAQRU1_SOAP_PORT
sendCmdToLauncher BU0_SOAP_HOST_NAME BU0_LAUNCHER_PORT STARTXDAQBU0_SOAP_PORT
sendCmdToLauncher BU1_SOAP_HOST_NAME BU1_LAUNCHER_PORT STARTXDAQBU1_SOAP_PORT

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
if ! webPingXDAQ BU1_SOAP_HOST_NAME BU1_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi

# Configure all executives
sendCmdToExecutive EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU0_SOAP_HOST_NAME  RU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU1_SOAP_HOST_NAME  RU1_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU0_SOAP_HOST_NAME  BU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU1_SOAP_HOST_NAME  BU1_SOAP_PORT configure.cmd.xml

# Start building events
curl http://EVM0_SOAP_HOST_NAME:EVM0_SOAP_PORT/urn:xdaq-application:lid=12/control?command=start &> /dev/null

echo "Building for 2 seconds"
sleep 2

#state=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application 0 stateName xsd:string`
#echo "RU0 state=$state"

nbEvtsBuiltBU0=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
nbEvtsBuiltBU1=`getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT rubuilder::bu::Application 1 nbEvtsBuilt xsd:unsignedInt`

echo "BU0 nbEvtsBuilt: $nbEvtsBuiltBU0"
echo "BU1 nbEvtsBuilt: $nbEvtsBuiltBU1"

if test $nbEvtsBuiltBU0 -lt 1000
then
  echo "Test failed"
  exit 1
fi

if test $nbEvtsBuiltBU1 -lt 1000
then
  echo "Test failed"
  exit 1
fi

# Start purging
curl http://EVM0_SOAP_HOST_NAME:EVM0_SOAP_PORT/urn:xdaq-application:lid=12/control?command=stop &> /dev/null

echo "Purging for 2 seconds"
sleep 2

state=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 stateName xsd:string`
echo "EVM state=$state"
if test $state != "Halted"
then
  echo "Test failed"
  exit 1
fi

state=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application 0 stateName xsd:string`
echo "RU0 state=$state"
if test $state != "Halted"
then
  echo "Test failed"
  exit 1
fi

state=`getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::ru::Application 1 stateName xsd:string`
echo "RU1 state=$state"
if test $state != "Halted"
then
  echo "Test failed"
  exit 1
fi

state=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 stateName xsd:string`
echo "BU0 state=$state"
if test $state != "Halted"
then
  echo "Test failed"
  exit 1
fi

state=`getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT rubuilder::bu::Application 1 stateName xsd:string`
echo "BU1 state=$state"
if test $state != "Halted"
then
  echo "Test failed"
  exit 1
fi

nbEvtsBuiltAtHaltBU0=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
nbEvtsBuiltAtHaltBU1=`getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT rubuilder::bu::Application 1 nbEvtsBuilt xsd:unsignedInt`

echo "BU0 nbEvtsBuiltAtHalt: $nbEvtsBuiltAtHaltBU0"
echo "BU1 nbEvtsBuiltAtHalt: $nbEvtsBuiltAtHaltBU1"

sleep 1

nbEvtsBuiltBU0=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
nbEvtsBuiltBU1=`getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT rubuilder::bu::Application 1 nbEvtsBuilt xsd:unsignedInt`

echo "BU0 nbEvtsBuilt: $nbEvtsBuiltBU0"
echo "BU1 nbEvtsBuilt: $nbEvtsBuiltBU1"

if test $nbEvtsBuiltBU0 -ne $nbEvtsBuiltAtHaltBU0
then
  echo "Test failed"
  exit 1
fi

if test $nbEvtsBuiltBU1 -ne $nbEvtsBuiltAtHaltBU1
then
  echo "Test failed"
  exit 1
fi

# Start building events again
curl http://EVM0_SOAP_HOST_NAME:EVM0_SOAP_PORT/urn:xdaq-application:lid=12/control?command=start &> /dev/null

echo "Building again for 2 seconds"
sleep 2

#state=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application 0 stateName xsd:string`
#echo "RU0 state=$state"

nbEvtsBuiltBU0=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
nbEvtsBuiltBU1=`getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT rubuilder::bu::Application 1 nbEvtsBuilt xsd:unsignedInt`

echo "BU0 nbEvtsBuilt: $nbEvtsBuiltBU0"
echo "BU1 nbEvtsBuilt: $nbEvtsBuiltBU1"

if test $nbEvtsBuiltBU0 -lt 1000
then
  echo "Test failed"
  exit 1
fi

if test $nbEvtsBuiltBU1 -lt 1000
then
  echo "Test failed"
  exit 1
fi

echo "Test succeeded"
exit 0
