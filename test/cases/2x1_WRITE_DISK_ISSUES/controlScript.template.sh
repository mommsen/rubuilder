#!/bin/sh

testDir=/tmp/rubuilder_test
rm -rf $testDir
parentTestDir=`dirname $testDir`
usedDiskSpace=`df $parentTestDir | awk '/^\// {printf("%0.3f", 1 - ($4/$2) )}'`
lowWaterMark=`echo "$usedDiskSpace+0.0007" | bc`
highWaterMark=`echo "$usedDiskSpace+0.0008" | bc`

# Launch executive processes
sendCmdToLauncher EVM0_SOAP_HOST_NAME EVM0_LAUNCHER_PORT STARTXDAQEVM0_SOAP_PORT
sendCmdToLauncher RU0_SOAP_HOST_NAME RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
sendCmdToLauncher RU1_SOAP_HOST_NAME RU1_LAUNCHER_PORT STARTXDAQRU1_SOAP_PORT
sendCmdToLauncher BU0_SOAP_HOST_NAME BU0_LAUNCHER_PORT STARTXDAQBU0_SOAP_PORT

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

# Configure all executives
sendCmdToExecutive EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU0_SOAP_HOST_NAME  RU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU1_SOAP_HOST_NAME  RU1_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU0_SOAP_HOST_NAME  BU0_SOAP_PORT configure.cmd.xml

setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 rawDataDir string $testDir
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 metaDataDir string $testDir
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 rawDataLowWaterMark double $lowWaterMark
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 rawDataHighWaterMark double $highWaterMark

# Configure and enable ptatcp
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT pt::atcp::PeerTransportATCP 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT pt::atcp::PeerTransportATCP 1 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME  RU1_SOAP_PORT pt::atcp::PeerTransportATCP 2 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT pt::atcp::PeerTransportATCP 5 Configure
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT pt::atcp::PeerTransportATCP 0 Enable
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT pt::atcp::PeerTransportATCP 1 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME  RU1_SOAP_PORT pt::atcp::PeerTransportATCP 2 Enable
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT pt::atcp::PeerTransportATCP 5 Enable

# Configure all applications
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::ta::Application  0 Configure
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application  0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::ru::Application  1 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application  0 Configure

runNumber=`date "+%s"`

#Enable RUs
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application  0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::ru::Application  1 Enable

#Enable EVM
setParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 runNumber unsignedInt $runNumber
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Enable

#Enable BUs
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 runNumber unsignedInt $runNumber
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application  0 Enable

#Start servicing trigger credits
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::ta::Application  0 Enable

echo "Building..."
while [ `df $parentTestDir | awk -v hwm=$highWaterMark '/^\// {printf("%0.0f", (1-$4/$2 - hwm)*100000)}'` -lt 0 ]; do
    sleep 5
done

sleep 15

nbEvtsBuiltBefore=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
echo "BU0 nbEvtsBuiltBefore=$nbEvtsBuiltBefore"
if test $nbEvtsBuiltBefore -lt 1000
then
  echo "Test failed"
  exit 1
fi

sleep 2

nbEvtsBuiltAfter=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
echo "BU0 nbEvtsBuiltAfter=$nbEvtsBuiltAfter"
if test $nbEvtsBuiltAfter -gt $nbEvtsBuiltBefore
then
  echo "Test failed"
  exit 1
fi

echo "Freeing disk space..."
rm -f $testDir/BU-000/run$runNumber/*raw

sleep 15

nbEvtsBuiltAfterDelete=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 nbEvtsBuilt xsd:unsignedInt`
echo "BU0 nbEvtsBuiltAfterDelete=$nbEvtsBuiltAfterDelete"
if test $nbEvtsBuiltAfterDelete -le $nbEvtsBuiltAfter
then
  echo "Test failed"
  exit 1
fi

rm -f $testDir/BU-000/run$runNumber/*raw

echo "Failing the BU..."
rm -f $testDir/BU-000/run$runNumber/open/*001.raw
sleep 1
rm -f $testDir/BU-000/run$runNumber/open/*001.raw

sleep 10

state=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application 0 stateName xsd:string`
echo "BU0 state=$state"
if test $state != "Failed"
then
    echo "Test failed"
    exit 1
fi

ls -l $testDir/BU-000/run$runNumber

if test -x $testDir/BU-000/run$runNumber/open
then
    echo "Test failed: $testDir/BU-000/run$runNumber/open exists"
    exit 1
fi

if test ! -s $testDir/BU-000/run$runNumber/EoR_$runNumber.jsn
then
    echo "Test failed: $testDir/BU-000/run$runNumber/EoR_$runNumber.jsn does not exist"
    exit 1
fi

rm -rf $testDir

echo "Test succeeded"
exit 0

