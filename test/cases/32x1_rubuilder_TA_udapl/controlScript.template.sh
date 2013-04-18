#!/bin/sh

rm -rf /ramdisk/BU-000

# Launch executive processes
sendCmdToLauncher EVM0_SOAP_HOST_NAME EVM0_LAUNCHER_PORT STARTXDAQEVM0_SOAP_PORT
sendCmdToLauncher RU0_SOAP_HOST_NAME  RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
sendCmdToLauncher BU0_SOAP_HOST_NAME  BU0_LAUNCHER_PORT STARTXDAQBU0_SOAP_PORT
sendCmdToLauncher RU1_SOAP_HOST_NAME  RU1_LAUNCHER_PORT STARTXDAQRU1_SOAP_PORT
sendCmdToLauncher RU2_SOAP_HOST_NAME  RU2_LAUNCHER_PORT STARTXDAQRU2_SOAP_PORT
sendCmdToLauncher RU3_SOAP_HOST_NAME  RU3_LAUNCHER_PORT STARTXDAQRU3_SOAP_PORT
sendCmdToLauncher RU4_SOAP_HOST_NAME  RU4_LAUNCHER_PORT STARTXDAQRU4_SOAP_PORT
sendCmdToLauncher RU5_SOAP_HOST_NAME  RU5_LAUNCHER_PORT STARTXDAQRU5_SOAP_PORT
sendCmdToLauncher RU6_SOAP_HOST_NAME  RU6_LAUNCHER_PORT STARTXDAQRU6_SOAP_PORT
sendCmdToLauncher RU7_SOAP_HOST_NAME  RU7_LAUNCHER_PORT STARTXDAQRU7_SOAP_PORT
sendCmdToLauncher RU8_SOAP_HOST_NAME  RU8_LAUNCHER_PORT STARTXDAQRU8_SOAP_PORT
sendCmdToLauncher RU9_SOAP_HOST_NAME  RU9_LAUNCHER_PORT STARTXDAQRU9_SOAP_PORT

# Check that executives are listening
if ! webPingXDAQ EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ BU0_SOAP_HOST_NAME BU0_SOAP_PORT 5
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
if ! webPingXDAQ RU2_SOAP_HOST_NAME RU2_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ RU3_SOAP_HOST_NAME RU3_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ RU4_SOAP_HOST_NAME RU4_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ RU5_SOAP_HOST_NAME RU5_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ RU6_SOAP_HOST_NAME RU6_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ RU7_SOAP_HOST_NAME RU7_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ RU8_SOAP_HOST_NAME RU8_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ RU9_SOAP_HOST_NAME RU9_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi


# Configure all executives
sendCmdToExecutive EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU0_SOAP_HOST_NAME  BU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU0_SOAP_HOST_NAME  RU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU1_SOAP_HOST_NAME  RU1_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU2_SOAP_HOST_NAME  RU2_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU3_SOAP_HOST_NAME  RU3_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU4_SOAP_HOST_NAME  RU4_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU5_SOAP_HOST_NAME  RU5_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU6_SOAP_HOST_NAME  RU6_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU7_SOAP_HOST_NAME  RU7_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU8_SOAP_HOST_NAME  RU8_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU9_SOAP_HOST_NAME  RU9_SOAP_PORT configure.cmd.xml

# Configure all applications
runNumber=`date "+%s"`
setParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 runNumber unsignedInt $runNumber
setParam BU0_SOAP_HOST_NAME  BU0_SOAP_PORT  rubuilder::bu::Application  0 runNumber unsignedInt $runNumber

sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::ta::Application 0 Configure
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT rubuilder::bu::Application  0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT rubuilder::ru::Application  0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME  RU1_SOAP_PORT rubuilder::ru::Application  1 Configure
sendSimpleCmdToApp RU2_SOAP_HOST_NAME  RU2_SOAP_PORT rubuilder::ru::Application  2 Configure
sendSimpleCmdToApp RU3_SOAP_HOST_NAME  RU3_SOAP_PORT rubuilder::ru::Application  3 Configure
sendSimpleCmdToApp RU4_SOAP_HOST_NAME  RU4_SOAP_PORT rubuilder::ru::Application  4 Configure
sendSimpleCmdToApp RU5_SOAP_HOST_NAME  RU5_SOAP_PORT rubuilder::ru::Application  5 Configure
sendSimpleCmdToApp RU6_SOAP_HOST_NAME  RU6_SOAP_PORT rubuilder::ru::Application  6 Configure
sendSimpleCmdToApp RU7_SOAP_HOST_NAME  RU7_SOAP_PORT rubuilder::ru::Application  7 Configure
sendSimpleCmdToApp RU8_SOAP_HOST_NAME  RU8_SOAP_PORT rubuilder::ru::Application  8 Configure
sendSimpleCmdToApp RU9_SOAP_HOST_NAME  RU9_SOAP_PORT rubuilder::ru::Application  9 Configure

#Enable RUs
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application  0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::ru::Application  1 Enable
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT rubuilder::ru::Application  2 Enable
sendSimpleCmdToApp RU3_SOAP_HOST_NAME RU3_SOAP_PORT rubuilder::ru::Application  3 Enable
sendSimpleCmdToApp RU4_SOAP_HOST_NAME RU4_SOAP_PORT rubuilder::ru::Application  4 Enable
sendSimpleCmdToApp RU5_SOAP_HOST_NAME RU5_SOAP_PORT rubuilder::ru::Application  5 Enable
sendSimpleCmdToApp RU6_SOAP_HOST_NAME RU6_SOAP_PORT rubuilder::ru::Application  6 Enable
sendSimpleCmdToApp RU7_SOAP_HOST_NAME RU7_SOAP_PORT rubuilder::ru::Application  7 Enable
sendSimpleCmdToApp RU8_SOAP_HOST_NAME RU8_SOAP_PORT rubuilder::ru::Application  8 Enable
sendSimpleCmdToApp RU9_SOAP_HOST_NAME RU9_SOAP_PORT rubuilder::ru::Application  9 Enable

#Enable EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::ta::Application 0 Enable
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

echo "Test succeeded"
exit 0
