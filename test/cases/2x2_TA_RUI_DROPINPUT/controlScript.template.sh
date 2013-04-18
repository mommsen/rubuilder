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

# Configure and enable ptatcp
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT pt::atcp::PeerTransportATCP 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT pt::atcp::PeerTransportATCP 1 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME  RU1_SOAP_PORT pt::atcp::PeerTransportATCP 2 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT pt::atcp::PeerTransportATCP 3 Configure
sendSimpleCmdToApp BU1_SOAP_HOST_NAME  BU1_SOAP_PORT pt::atcp::PeerTransportATCP 4 Configure
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT pt::atcp::PeerTransportATCP 0 Enable
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT pt::atcp::PeerTransportATCP 1 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME  RU1_SOAP_PORT pt::atcp::PeerTransportATCP 2 Enable
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT pt::atcp::PeerTransportATCP 3 Enable
sendSimpleCmdToApp BU1_SOAP_HOST_NAME  BU1_SOAP_PORT pt::atcp::PeerTransportATCP 4 Enable

# Configure all applications
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::ta::Application  0 Configure
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::rui::Application 0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::rui::Application 1 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application  0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::ru::Application  1 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application  0 Configure
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT rubuilder::bu::Application  1 Configure

#Enable RUs
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application  0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::ru::Application  1 Enable

#Enable EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Enable

#Enable BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application  0 Enable
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT rubuilder::bu::Application  1 Enable

#Start generation of dummy super-fragments
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::rui::Application 0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::rui::Application 1 Enable

#Start servicing trigger credits
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::ta::Application  0 Enable

echo "Building for 5 seconds"
sleep 5

nbEvtsBuilt=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
echo "BU0 nbEvtsBuiltBU0=$nbEvtsBuilt"
if test $nbEvtsBuilt -ne 0
then
  echo "Test failed"
  exit 1
fi

nbEvtsBuilt=`getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT rubuilder::bu::Application 1 nbEvtsBuilt xsd:unsignedInt`
echo "BU1 nbEvtsBuiltBU0=$nbEvtsBuilt"
if test $nbEvtsBuilt -ne 0
then
  echo "Test failed"
  exit 1
fi

taTriggerCount=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::ta::Application 0 I2O_EVM_TRIGGER_LogicalCount xsd:unsignedInt`
echo "TA triggers=$taTriggerCount"
if test $taTriggerCount -lt 1000
then
  echo "Test failed"
  exit 1
fi

evmTriggerCount=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 nbTriggers xsd:unsignedInt`
echo "EVM triggers=$evmTriggerCount"
if test $evmTriggerCount -ne 0
then
  echo "Test failed"
  exit 1
fi

ruiEvts=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::rui::Application 0 i2oEVMRUDataReadyCount xsd:unsignedLong`
echo "RUI0 nbEvts=$ruiEvts"
if test $ruiEvts -lt 1000
then
  echo "Test failed"
  exit 1
fi

ruEvts=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application 0 nbSuperFragmentsReady xsd:unsignedInt`
echo "RU0 nbEvts=$ruEvts"
if test $ruEvts -ne 0
then
  echo "Test failed"
  exit 1
fi

ruiEvts=`getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::rui::Application 1 i2oEVMRUDataReadyCount xsd:unsignedLong`
echo "RUI1 nbEvts=$ruiEvts"
if test $ruiEvts -lt 1000
then
  echo "Test failed"
  exit 1
fi

ruEvts=`getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::ru::Application 1 nbSuperFragmentsReady xsd:unsignedInt`
echo "RU1 nbEvts=$ruEvts"
if test $ruEvts -ne 0
then
  echo "Test failed"
  exit 1
fi


echo "Test succeeded"
exit 0
