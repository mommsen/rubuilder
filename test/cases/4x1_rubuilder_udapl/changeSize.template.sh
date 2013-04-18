#!/bin/sh
source ../setenv.sh
#Stop EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Stop
mainDirectory=`pwd`
state=""
while [ "$state" != "Ready" ]; do
        sleep 1
	state=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 stateName xsd:string`
        echo "EVM state is $state."
        if [[ "$state" = "Failed" || "$state" = "FlushFailed" ]]; then
                ./stopXDAQs
		cd $2
                #Control script
		./controlScript.sh
		cd $mainDirectory
                # Stop EVM
		sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Stop
		sleep 30
		state=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 stateName xsd:string`
        fi
done

#Halt RUs
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT rubuilder::ru::Application  0 Halt
sendSimpleCmdToApp RU1_SOAP_HOST_NAME  RU1_SOAP_PORT rubuilder::ru::Application  1 Halt
sendSimpleCmdToApp RU2_SOAP_HOST_NAME  RU2_SOAP_PORT rubuilder::ru::Application  2 Halt
sendSimpleCmdToApp RU3_SOAP_HOST_NAME  RU3_SOAP_PORT rubuilder::ru::Application  3 Halt

#Enable EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Halt

#Enable BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT rubuilder::bu::Application  0 Halt

#Set parameter
sleep 2
setParam RU0_SOAP_HOST_NAME  RU0_SOAP_PORT rubuilder::ru::Application  0 dummyFedPayloadSize unsignedInt $1
setParam RU1_SOAP_HOST_NAME  RU1_SOAP_PORT rubuilder::ru::Application  1 dummyFedPayloadSize unsignedInt $1
setParam RU2_SOAP_HOST_NAME  RU2_SOAP_PORT rubuilder::ru::Application  2 dummyFedPayloadSize unsignedInt $1
setParam RU3_SOAP_HOST_NAME  RU3_SOAP_PORT rubuilder::ru::Application  3 dummyFedPayloadSize unsignedInt $1

dummyFedPayloadSizeRU0=`getParam RU0_SOAP_HOST_NAME  RU0_SOAP_PORT rubuilder::ru::Application  0 dummyFedPayloadSize xsd:unsignedInt`
dummyFedPayloadSizeRU1=`getParam RU1_SOAP_HOST_NAME  RU1_SOAP_PORT rubuilder::ru::Application  1 dummyFedPayloadSize xsd:unsignedInt`
dummyFedPayloadSizeRU2=`getParam RU2_SOAP_HOST_NAME  RU2_SOAP_PORT rubuilder::ru::Application  2 dummyFedPayloadSize xsd:unsignedInt`
dummyFedPayloadSizeRU3=`getParam RU3_SOAP_HOST_NAME  RU3_SOAP_PORT rubuilder::ru::Application  3 dummyFedPayloadSize xsd:unsignedInt`

echo "RU0 dummyFedPayloadSize: $dummyFedPayloadSizeRU0"
echo "RU1 dummyFedPayloadSize: $dummyFedPayloadSizeRU1"
echo "RU2 dummyFedPayloadSize: $dummyFedPayloadSizeRU2"
echo "RU3 dummyFedPayloadSize: $dummyFedPayloadSizeRU3"


# Configure all applications
#EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Configure
#RUs
sendSimpleCmdToApp RU0_SOAP_HOST_NAME  RU0_SOAP_PORT rubuilder::ru::Application  0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME  RU1_SOAP_PORT rubuilder::ru::Application  1 Configure
sendSimpleCmdToApp RU2_SOAP_HOST_NAME  RU2_SOAP_PORT rubuilder::ru::Application  2 Configure
sendSimpleCmdToApp RU3_SOAP_HOST_NAME  RU3_SOAP_PORT rubuilder::ru::Application  3 Configure
#BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT rubuilder::bu::Application  0 Configure

sleep 10

#Enable RUs
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT rubuilder::ru::Application  0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT rubuilder::ru::Application  1 Enable
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT rubuilder::ru::Application  2 Enable
sendSimpleCmdToApp RU3_SOAP_HOST_NAME RU3_SOAP_PORT rubuilder::ru::Application  3 Enable

#Enable EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Enable

#Enable BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::bu::Application  0 Enable
