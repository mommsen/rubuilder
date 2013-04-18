#!/bin/sh

# Launch executive processes
sendCmdToLauncher EVM0_SOAP_HOST_NAME EVM0_LAUNCHER_PORT STARTXDAQEVM0_SOAP_PORT
sendCmdToLauncher RU0_SOAP_HOST_NAME  RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
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
if ! webPingXDAQ BU0_SOAP_HOST_NAME BU0_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi

# Configure all executives
sendCmdToExecutive EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU0_SOAP_HOST_NAME  RU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU0_SOAP_HOST_NAME  BU0_SOAP_PORT configure.cmd.xml

# Configure and enable ptatcp
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT pt::atcp::PeerTransportATCP 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT pt::atcp::PeerTransportATCP 1 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT pt::atcp::PeerTransportATCP 2 Configure
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT pt::atcp::PeerTransportATCP 0 Enable
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT pt::atcp::PeerTransportATCP 1 Enable
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT pt::atcp::PeerTransportATCP 2 Enable

# Configure all applications
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT rubuilder::rui::Application 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT rubuilder::ru::Application  0 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT rubuilder::bu::Application  0 Configure

#Enable RUs
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application  0 Enable

#Enable EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Enable

#Enable BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application  0 Enable

#Start generation of dummy super-fragments
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::rui::Application 0 Enable

echo "Building for 2 seconds"
sleep 2

nbEvtsBuilt=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
echo "BU0 nbEvtsBuilt=$nbEvtsBuilt"
if test $nbEvtsBuilt -lt 100
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

state=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::rui::Application 0 stateName xsd:string`
echo "RUI0 state=$state"
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

nbExceptions=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::tester::Application 0 nbExceptions xsd:unsignedInt`
echo "RUBuilderTester0 nbExceptions=$nbExceptions"
#if test $nbExceptions -ne 0
#then
#  echo "Test failed"
#  exit 1
#fi

stateChangeNotifications=`curl -s http://EVM0_SOAP_HOST_NAME:EVM0_SOAP_PORT/urn:xdaq-application:class=rubuilder::tester::Application,instance=0/stateChangeNotifications`
echo "==== RUBuilderTester0 state change notifications ===="
echo "$stateChangeNotifications"
echo "====================================================="

nbStateChangeNotificationsBEFORE=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::tester::Application 0 nbStateChangeNotifications xsd:unsignedInt`
echo "RUBuilderTester0 nbStateChangeNotificationsBEFORE=$nbStateChangeNotificationsBEFORE"

echo "Stopping generation of super-fragments"
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::rui::Application 0 Halt

echo "Waiting 2 seconds for timeout to occur"
sleep 2

state=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application 0 stateName xsd:string`
echo "RU0 state=$state"
if test $state != "TimedOutBackPressuring"
then
  echo "Test failed"
  exit 1
fi

nbSuperFragmentsReady=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application 0 nbSuperFragmentsReady xsd:unsignedInt`
echo "RU0 nbSuperFragmentsReady=$nbSuperFragmentsReady"
if test $nbSuperFragmentsReady -ne 0
then
  echo "Test failed"
  exit 1
fi

nbExceptions=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::tester::Application 0 nbExceptions xsd:unsignedInt`
echo "RUBuilderTester0 nbExceptions=$nbExceptions"
#if test $nbExceptions -ne 1
#then
#  echo "Test failed"
#  exit 1
#fi

stateChangeNotifications=`curl -s http://EVM0_SOAP_HOST_NAME:EVM0_SOAP_PORT/urn:xdaq-application:class=rubuilder::tester::Application,instance=0/stateChangeNotifications`
echo "==== RUBuilderTester0 state change notifications ===="
echo "$stateChangeNotifications"
echo "====================================================="

nbStateChangeNotificationsAFTER=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::tester::Application 0 nbStateChangeNotifications xsd:unsignedInt`
echo "RUBuilderTester0 nbStateChangeNotificationsAFTER=$nbStateChangeNotificationsAFTER"
let "nbStateChangeNotificationsEXPECTED = nbStateChangeNotificationsBEFORE + 1"
echo "RUBuilderTester0 nbStateChangeNotificationsEXPECTED=$nbStateChangeNotificationsEXPECTED"

if test $nbStateChangeNotificationsAFTER -ne $nbStateChangeNotificationsEXPECTED
then
  echo "Test failed"
  exit 1
fi

# Halt all applications except rubuilder::rui::Application which is already halted
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Halt
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT rubuilder::ru::Application  0 Halt
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT rubuilder::bu::Application  0 Halt

# Configure all applications
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT rubuilder::rui::Application 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT rubuilder::ru::Application  0 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT rubuilder::bu::Application  0 Configure

#Enable RUs
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application  0 Enable

#Enable EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Enable

#Enable BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application  0 Enable

#Start generation of dummy super-fragments
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::rui::Application 0 Enable

echo "Building for 2 seconds"
sleep 2

nbEvtsBuilt=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
echo "BU0 nbEvtsBuilt=$nbEvtsBuilt"
if test $nbEvtsBuilt -lt 100
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

state=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::rui::Application 0 stateName xsd:string`
echo "RUI0 state=$state"
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

nbExceptions=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::tester::Application 0 nbExceptions xsd:unsignedInt`
echo "RUBuilderTester0 nbExceptions=$nbExceptions"
#if test $nbExceptions -ne 1
#then
#  echo "Test failed"
#  exit 1
#fi

echo "Test succeeded"
exit 0
