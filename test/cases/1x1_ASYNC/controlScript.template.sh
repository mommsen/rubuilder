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

# Start building events
curl http://EVM0_SOAP_HOST_NAME:EVM0_SOAP_PORT/urn:xdaq-application:class=rubuilder::tester::Application,instance=0/control?command=start &> /dev/null

echo "Building for 2 seconds"
sleep 2

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

state=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 stateName xsd:string`
echo "BU0 state=$state"
if test $state != "Enabled"
then
  echo "Test failed"
  exit 1
fi

nbExceptions=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::tester::Application 0 nbExceptions xsd:unsignedInt`
echo "RUBuilderTester0 nbExceptions=$nbExceptions"
if test $nbExceptions -ne 0
then
  echo "Test failed"
  exit 1
fi

stateChangeNotifications=`curl -s http://EVM0_SOAP_HOST_NAME:EVM0_SOAP_PORT/urn:xdaq-application:class=rubuilder::tester::Application,instance=0/stateChangeNotifications`
echo "==== RUBuilderTester0 state change notifications ===="
echo "$stateChangeNotifications"
echo "====================================================="

nbStateChangeNotificationsBEFORE=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::tester::Application 0 nbStateChangeNotifications xsd:unsignedInt`
echo "RUBuilderTester0 nbStateChangeNotificationsBEFORE=$nbStateChangeNotificationsBEFORE"

echo "Forcing EVM0 to fail"
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application  0 Fail &> /dev/null

echo "Forcing BU0 to fail"
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application  0 Fail &> /dev/null

echo "Forcing RU0 to fail"
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application  0 Fail &> /dev/null

state=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 stateName xsd:string`
echo "EVM0 state=$state"
if test $state != "Failed"
then
  echo "Test failed"
  exit 1
fi

state=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application 0 stateName xsd:string`
echo "RU0 state=$state"
if test $state != "Failed"
then
  echo "Test failed"
  exit 1
fi

state=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 stateName xsd:string`
echo "BU0 state=$state"
if test $state != "Failed"
then
  echo "Test failed"
  exit 1
fi

exceptions=`curl -s http://EVM0_SOAP_HOST_NAME:EVM0_SOAP_PORT/urn:xdaq-application:class=rubuilder::tester::Application,instance=0/exceptions`
echo "==== RUBuilderTester0 exceptions ===="
echo "$exceptions"
echo "====================================="

nbExceptions=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::tester::Application 0 nbExceptions xsd:unsignedInt`
echo "RUBuilderTester0 nbExceptions=$nbExceptions"
if test $nbExceptions -ne 0
then
  echo "Test failed"
  exit 1
fi

stateChangeNotifications=`curl -s http://EVM0_SOAP_HOST_NAME:EVM0_SOAP_PORT/urn:xdaq-application:class=rubuilder::tester::Application,instance=0/stateChangeNotifications`
echo "==== RUBuilderTester0 state change notifications ===="
echo "$stateChangeNotifications"
echo "====================================================="

nbStateChangeNotificationsAFTER=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::tester::Application 0 nbStateChangeNotifications xsd:unsignedInt`
echo "RUBuilderTester0 nbStateChangeNotificationsAFTER=$nbStateChangeNotificationsAFTER"
let "nbStateChangeNotificationsEXPECTED = nbStateChangeNotificationsBEFORE + 3"
echo "RUBuilderTester0 nbStateChangeNotificationsEXPECTED=$nbStateChangeNotificationsEXPECTED"
if test $nbStateChangeNotificationsAFTER -ne $nbStateChangeNotificationsEXPECTED
then
  echo "Test failed"
  exit 1
fi

echo "Test succeeded"
exit 0
