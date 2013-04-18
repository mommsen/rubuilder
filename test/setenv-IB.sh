# XDAQ
#export XDAQ_ROOT=/opt/xdaq
#export XDAQ_ROOT=/usr/local/src/xdaq/trunk
export XDAQ_ROOT=$HOME/daq/xdaq_root

export XDAQ_PLATFORM=`uname -m`
if test ".$XDAQ_PLATFORM" != ".x86_64"; then
    export XDAQ_PLATFORM=x86
fi
checkos=`$XDAQ_ROOT/config/checkos.sh`
export XDAQ_PLATFORM=$XDAQ_PLATFORM"_"$checkos

#export XDAQ_RUBUILDER=${XDAQ_ROOT}
export XDAQ_RUBUILDER=${HOME}/daq/trunk/${XDAQ_PLATFORM}
export XDAQ_DOCUMENT_ROOT=${XDAQ_ROOT}/htdocs
export LD_LIBRARY_PATH=${XDAQ_RUBUILDER}/lib:${XDAQ_ROOT}/lib:${LD_LIBRARY_PATH}
export PATH=${PATH}:${XDAQ_RUBUILDER}/bin:${XDAQ_ROOT}/bin

# RU builder tester
export RUB_TESTER_HOME=${HOME}/daq/trunk/daq/rubuilder/test
export TESTS_SYMBOL_MAP=${RUB_TESTER_HOME}/daq2valSymbolMap-IB.txt
export TEST_TYPE=IB

export PATH=${PATH}:${RUB_TESTER_HOME}/scripts

ulimit -l unlimited
