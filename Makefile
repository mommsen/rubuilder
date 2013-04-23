BUILD_HOME:=$(shell pwd)/../..

include $(XDAQ_ROOT)/config/mfAutoconf.rules
include $(XDAQ_ROOT)/config/mfDefs.$(XDAQ_OS)

Project=daq
Package=rubuilder
PackageName=rubuilder
Description="This package contains the software of the CMS RUBuilder."
Summary="CMS RUBuilder software"
Link="http://cms-ru-builder.web.cern.ch/cms-ru-builder"
PACKAGE_VER_MAJOR=5
PACKAGE_VER_MINOR=4
PACKAGE_VER_PATCH=0

Packages= \
	rubuilder/utils \
	rubuilder/bu \
	rubuilder/evm \
	rubuilder/ru \
	rubuilder/rui \
	rubuilder/ta \
	rubuilder/fu \
	rubuilder/tester \
	rubuilder

include $(XDAQ_ROOT)/config/Makefile.rules
include $(XDAQ_ROOT)/config/mfRPM.rules

.PHONY: _rpmall
_rpmall: spec_update makerpm removelocalinstall
	@echo "Building rubuilder rpm: done"

.PHONY: removelocalinstall
removelocalinstall :
	-rm -rf lib
	-rm -rf include

