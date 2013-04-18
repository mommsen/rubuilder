#!/bin/sh

# Launch executive processes
sendCmdToLauncher EVM0_SOAP_HOST_NAME EVM0_LAUNCHER_PORT STARTXDAQEVM0_SOAP_PORT
sendCmdToLauncher RU0_SOAP_HOST_NAME RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
sendCmdToLauncher BU0_SOAP_HOST_NAME BU0_LAUNCHER_PORT STARTXDAQBU0_SOAP_PORT
sendCmdToLauncher FU0_SOAP_HOST_NAME FU0_LAUNCHER_PORT STARTXDAQFU0_SOAP_PORT

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
sendCmdToExecutive EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU0_SOAP_HOST_NAME  RU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU0_SOAP_HOST_NAME  BU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FU0_SOAP_HOST_NAME  FU0_SOAP_PORT configure.cmd.xml

# Configure and enable ptatcp
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT pt::atcp::PeerTransportATCP 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT pt::atcp::PeerTransportATCP 1 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT pt::atcp::PeerTransportATCP 2 Configure
sendSimpleCmdToApp FU0_SOAP_HOST_NAME  FU0_SOAP_PORT pt::atcp::PeerTransportATCP 3 Configure
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT pt::atcp::PeerTransportATCP 0 Enable
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT pt::atcp::PeerTransportATCP 1 Enable
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT pt::atcp::PeerTransportATCP 2 Enable
sendSimpleCmdToApp FU0_SOAP_HOST_NAME  FU0_SOAP_PORT pt::atcp::PeerTransportATCP 3 Enable

# Configure all applications
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::ta::Application  0 Configure
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::rui::Application 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application  0 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application  0 Configure
sendSimpleCmdToApp FU0_SOAP_HOST_NAME FU0_SOAP_PORT rubuilder::fu::Application  0 Configure

#Enable RUs
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application  0 Enable

#Enable EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Enable

#Enable BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application  0 Enable

#Start generation of dummy super-fragments
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::rui::Application 0 Enable

#Start servicing trigger credits
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::ta::Application  0 Enable

#Start filtering events
sendSimpleCmdToApp FU0_SOAP_HOST_NAME FU0_SOAP_PORT rubuilder::fu::Application  0 Enable

echo "Building for 2 seconds"
sleep 2

nbEvtsBuilt=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
echo "BU0 nbEvtsBuilt=$nbEvtsBuilt"
if test $nbEvtsBuilt -lt 1000
then
  echo "Test failed"
  exit 1
fi

state=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::ta::Application 0 stateName xsd:string`
echo "TA0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

state=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::rui::Application 0 stateName xsd:string`
echo "RUI0 state=$state"
if test $state != "Enabled"
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

state=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 stateName xsd:string`
echo "BU0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

echo "Emulating user looking at EVM0 main page"
curl http://EVM0_SOAP_HOST_NAME:EVM0_SOAP_PORT/urn:xdaq-application:class=EVM,instance=0 &> /dev/null

# Checking that EVM0 executive is listening
if ! webPingXDAQ EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT 1
then
  echo "Test failed - EVM0 executive is not listening"
  exit 1
fi

# Checking EVM0 is still enabled
state=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 stateName xsd:string`
echo "EVM0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

echo "Emulating user looking at EVM0 debug page"
curl http://EVM0_SOAP_HOST_NAME:EVM0_SOAP_PORT/urn:xdaq-application:class=EVM,instance=0/debug &> /dev/null

# Checking that EVM0 executive is listening
if ! webPingXDAQ EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT 1
then
  echo "Test failed - EVM0 executive is not listening"
  exit 1
fi

# Checking EVM0 is still enabled
state=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 stateName xsd:string`
echo "EVM0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

echo "Emulating user looking at BU0 main page"
curl http://BU0_SOAP_HOST_NAME:BU0_SOAP_PORT/urn:xdaq-application:class=BU,instance=0 &> /dev/null

# Checking that BU0 executive is listening
if ! webPingXDAQ BU0_SOAP_HOST_NAME BU0_SOAP_PORT 1
then
  echo "Test failed - BU0 executive is not listening"
  exit 1
fi

# Checking BU0 is still enabled
state=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 stateName xsd:string`
echo "BU0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

echo "Emulating user looking at BU0 debug page"
curl http://BU0_SOAP_HOST_NAME:BU0_SOAP_PORT/urn:xdaq-application:class=BU,instance=0/debug &> /dev/null

# Checking that BU0 executive is listening
if ! webPingXDAQ BU0_SOAP_HOST_NAME BU0_SOAP_PORT 1
then
  echo "Test failed - BU0 executive is not listening"
  exit 1
fi

# Checking BU0 is still enabled
state=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 stateName xsd:string`
echo "BU0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

echo "Emulating user looking at RU0 main page"
curl http://RU0_SOAP_HOST_NAME:RU0_SOAP_PORT/urn:xdaq-application:class=RU,instance=0 &> /dev/null

# Checking that RU0 executive is listening
if ! webPingXDAQ RU0_SOAP_HOST_NAME RU0_SOAP_PORT 1
then
  echo "Test failed - RU0 executive is not listening"
  exit 1
fi

# Checking RU0 is still enabled
state=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application 0 stateName xsd:string`
echo "RU0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

echo "Emulating user looking at RU0 debug page"
curl http://RU0_SOAP_HOST_NAME:RU0_SOAP_PORT/urn:xdaq-application:class=RU,instance=0/debug &> /dev/null

# Checking that RU0 executive is listening
if ! webPingXDAQ RU0_SOAP_HOST_NAME RU0_SOAP_PORT 1
then
  echo "Test failed - RU0 executive is not listening"
  exit 1
fi

# Checking RU0 is still enabled
state=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application 0 stateName xsd:string`
echo "RU0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

echo "Emulating user looking at TA0 main page"
curl http://EVM0_SOAP_HOST_NAME:EVM0_SOAP_PORT/urn:xdaq-application:class=TA,instance=0 &> /dev/null

# Checking that EVM0 executive is listening
if ! webPingXDAQ EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT 1
then
  echo "Test failed - EVM0 executive is not listening"
  exit 1
fi

# Checking TA0 is still enabled
state=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::ta::Application 0 stateName xsd:string`
echo "TA0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

echo "Emulating user looking at TA0 debug page"
curl http://EVM0_SOAP_HOST_NAME:EVM0_SOAP_PORT/urn:xdaq-application:class=TA,instance=0/debug &> /dev/null

# Checking that EVM0 executive is listening
if ! webPingXDAQ EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT 1
then
  echo "Test failed - EVM0 executive is not listening"
  exit 1
fi

# Checking TA0 is still enabled
state=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::ta::Application 0 stateName xsd:string`
echo "TA0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

echo "Emulating user looking at RUI0 main page"
curl http://RU0_SOAP_HOST_NAME:RU0_SOAP_PORT/urn:xdaq-application:class=RUI,instance=0 &> /dev/null

# Checking that RU0 executive is listening
if ! webPingXDAQ RU0_SOAP_HOST_NAME RU0_SOAP_PORT 1
then
  echo "Test failed - RU0 executive is not listening"
  exit 1
fi

# Checking RUI0 is still enabled
state=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::rui::Application 0 stateName xsd:string`
echo "RUI0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

echo "Emulating user looking at RUI0 debug page"
curl http://RU0_SOAP_HOST_NAME:RU0_SOAP_PORT/urn:xdaq-application:class=RUI,instance=0/debug &> /dev/null

# Checking that RU0 executive is listening
if ! webPingXDAQ RU0_SOAP_HOST_NAME RU0_SOAP_PORT 1
then
  echo "Test failed - RU0 executive is not listening"
  exit 1
fi

# Checking RUI0 is still enabled
state=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::rui::Application 0 stateName xsd:string`
echo "RUI0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

echo "Emulating user looking at FU0 main page"
curl http://FU0_SOAP_HOST_NAME:FU0_SOAP_PORT/urn:xdaq-application:class=FU,instance=0 &> /dev/null

# Checking that FU0 executive is listening
if ! webPingXDAQ FU0_SOAP_HOST_NAME FU0_SOAP_PORT 1
then
  echo "Test failed - FU0 executive is not listening"
  exit 1
fi

# Checking FU0 is still enabled
state=`getParam FU0_SOAP_HOST_NAME FU0_SOAP_PORT rubuilder::fu::Application 0 stateName xsd:string`
echo "FU0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

echo "Emulating user looking at FU0 debug page"
curl http://FU0_SOAP_HOST_NAME:FU0_SOAP_PORT/urn:xdaq-application:class=FU,instance=0/debug &> /dev/null

# Checking that FU0 executive is listening
if ! webPingXDAQ FU0_SOAP_HOST_NAME FU0_SOAP_PORT 1
then
  echo "Test failed - FU0 executive is not listening"
  exit 1
fi

# Checking FU0 is still enabled
state=`getParam FU0_SOAP_HOST_NAME FU0_SOAP_PORT rubuilder::fu::Application 0 stateName xsd:string`
echo "FU0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

echo "Test succeeded"
exit 0
