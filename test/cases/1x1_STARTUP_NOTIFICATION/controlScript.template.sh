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

nbStateChangeNotifications=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::tester::Application 0 nbStateChangeNotifications xsd:unsignedInt`
echo "RUBuilderTester0 nbStateChangeNotifications=$nbStateChangeNotifications"
if test $nbStateChangeNotifications -ne 3
then
  echo "Test failed"
  exit 1
fi

echo "Test succeeded"
exit 0
