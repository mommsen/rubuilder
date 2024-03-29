#!/bin/sh

# Launch executive processes
sendCmdToLauncher FEROL0_SOAP_HOST_NAME FEROL0_LAUNCHER_PORT STARTXDAQFEROL0_SOAP_PORT
sendCmdToLauncher FEROL1_SOAP_HOST_NAME FEROL1_LAUNCHER_PORT STARTXDAQFEROL1_SOAP_PORT
sendCmdToLauncher FEROL2_SOAP_HOST_NAME FEROL2_LAUNCHER_PORT STARTXDAQFEROL2_SOAP_PORT
sendCmdToLauncher FEROL3_SOAP_HOST_NAME FEROL3_LAUNCHER_PORT STARTXDAQFEROL3_SOAP_PORT
sendCmdToLauncher FEROL4_SOAP_HOST_NAME FEROL4_LAUNCHER_PORT STARTXDAQFEROL4_SOAP_PORT
sendCmdToLauncher FEROL5_SOAP_HOST_NAME FEROL5_LAUNCHER_PORT STARTXDAQFEROL5_SOAP_PORT
sendCmdToLauncher FEROL6_SOAP_HOST_NAME FEROL6_LAUNCHER_PORT STARTXDAQFEROL6_SOAP_PORT
sendCmdToLauncher FEROL7_SOAP_HOST_NAME FEROL7_LAUNCHER_PORT STARTXDAQFEROL7_SOAP_PORT
sendCmdToLauncher RU0_SOAP_HOST_NAME RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT

# Check that executives are listening
if ! webPingXDAQ FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ RU0_SOAP_HOST_NAME RU0_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi


# Configure all executives
sendCmdToExecutive FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU0_SOAP_HOST_NAME RU0_SOAP_PORT configure.cmd.xml

# Configure and enable ptutcp
sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT pt::utcp::Application 0 Configure
sendSimpleCmdToApp FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT pt::utcp::Application 1 Configure
sendSimpleCmdToApp FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT pt::utcp::Application 2 Configure
sendSimpleCmdToApp FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT pt::utcp::Application 3 Configure
sendSimpleCmdToApp FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT pt::utcp::Application 4 Configure
sendSimpleCmdToApp FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT pt::utcp::Application 5 Configure
sendSimpleCmdToApp FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT pt::utcp::Application 6 Configure
sendSimpleCmdToApp FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT pt::utcp::Application 7 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::utcp::Application 8 Configure
sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT pt::utcp::Application 0 Enable
sendSimpleCmdToApp FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT pt::utcp::Application 1 Enable
sendSimpleCmdToApp FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT pt::utcp::Application 2 Enable
sendSimpleCmdToApp FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT pt::utcp::Application 3 Enable
sendSimpleCmdToApp FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT pt::utcp::Application 4 Enable
sendSimpleCmdToApp FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT pt::utcp::Application 5 Enable
sendSimpleCmdToApp FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT pt::utcp::Application 6 Enable
sendSimpleCmdToApp FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT pt::utcp::Application 7 Enable
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::utcp::Application 8 Enable


#Set parameter
sleep 4
setParam FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT Client 0 currentSize unsignedLong 2048
setParam FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT Client 1 currentSize unsignedLong 2048
setParam FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT Client 2 currentSize unsignedLong 2048
setParam FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT Client 3 currentSize unsignedLong 2048
setParam FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT Client 4 currentSize unsignedLong 2048
setParam FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT Client 5 currentSize unsignedLong 2048
setParam FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT Client 6 currentSize unsignedLong 2048
setParam FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT Client 7 currentSize unsignedLong 2048
setParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT Server 0 currentSize unsignedLong 2048

#Enable FEROLs
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT Server 0 start

sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT Client 0 start
sendSimpleCmdToApp FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT Client 1 start
sendSimpleCmdToApp FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT Client 2 start
sendSimpleCmdToApp FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT Client 3 start
sendSimpleCmdToApp FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT Client 4 start
sendSimpleCmdToApp FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT Client 5 start
sendSimpleCmdToApp FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT Client 6 start
sendSimpleCmdToApp FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT Client 7 start

