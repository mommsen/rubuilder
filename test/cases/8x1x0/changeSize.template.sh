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
		./stopAllXDAQs
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
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT rubuilder::ru::Application  0 Halt

#Enable EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Halt

#Enable BUs
sendSimpleCmdToApp FU0_SOAP_HOST_NAME  FU0_SOAP_PORT rubuilder::bu::Application  0 Halt

#Set parameter
sleep 2
setParam BU0_SOAP_HOST_NAME  BU0_SOAP_PORT rubuilder::ru::Application  0 dummyFedPayloadSize unsignedInt $1

dummyFedPayloadSizeRU0=`getParam BU0_SOAP_HOST_NAME  BU0_SOAP_PORT rubuilder::ru::Application  0 dummyFedPayloadSize xsd:unsignedInt`

echo "RU0 dummyFedPayloadSize: $dummyFedPayloadSizeRU0"


# Configure all applications
#EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Configure
#RUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME  BU0_SOAP_PORT rubuilder::ru::Application  0 Configure
#BUs
sendSimpleCmdToApp FU0_SOAP_HOST_NAME  FU0_SOAP_PORT rubuilder::bu::Application  0 Configure

sleep 10

#Enable RUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT rubuilder::ru::Application  0 Enable

#Enable EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT rubuilder::evm::Application 0 Enable

#Enable BUs
sendSimpleCmdToApp FU0_SOAP_HOST_NAME FU0_SOAP_PORT rubuilder::bu::Application  0 Enable
