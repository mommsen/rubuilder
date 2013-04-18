BUILD_HOME:=$(shell pwd)/../../..

include $(XDAQ_ROOT)/config/mfAutoconf.rules
include $(XDAQ_ROOT)/config/mfDefs.$(XDAQ_OS)

Project=daq
Package=rubuilder/messagesizes

Sources=
Executables=messagesizes.cc

IncludeDirs = \
	$(XDAQ_ROOT)/$(XDAQ_PLATFORM)/include \
	$(XDAQ_ROOT)/$(XDAQ_PLATFORM)/include/$(XDAQ_OS) \
	$(INTERFACE_EVB_INCLUDE_PREFIX)

UserCFlags =
#UserCCFlags = -g -Wall -Werror -pedantic-errors -Wno-long-long
UserCCFlags = -g -Wall -pedantic-errors -Wno-long-long
UserDynamicLinkFlags =
UserStaticLinkFlags =
UserExecutableLinkFlags =

# These libraries can be platform specific and
# potentially need conditional processing
#

Libraries = 

#
# Compile the source files and create a shared library
#
DynamicLibrary=

StaticLibrary=

ifdef Executable
Libraries=toolbox xoap xerces-c
Executables= $(Executable).cc
endif

include $(XDAQ_ROOT)/config/Makefile.rules
