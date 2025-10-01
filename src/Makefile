#
# OMNeT++/OMNEST Makefile for $(LIB_PREFIX)mecrt
#
# This file was generated with the command:
#  opp_makemake --make-so -f --deep -o mecrt -O out -KINET_PROJ=../../inet4.5 -KSIMU5G_PROJ=../../simu5g -DINET_IMPORT -DSIMU5G_IMPORT -I. -I../../inet4.5/src -L../../inet4.5/src -lINET_dbg -I../../simu5g/src -L../../simu5g/src -lsimu5g_dbg -I/home/chuanchao/gurobi1203/linux64/include -L/home/chuanchao/gurobi1203/linux64/lib -lgurobi_c++ -lgurobi120
#

# Name of target to be created (-o option)
TARGET_DIR = .
TARGET_NAME = $(LIB_PREFIX)mecrt$(D)
TARGET = $(TARGET_NAME)$(SHARED_LIB_SUFFIX)
TARGET_IMPLIB = $(TARGET_NAME)$(IMPLIB_SUFFIX)
TARGET_IMPDEF = $(TARGET_NAME)$(IMPDEF_SUFFIX)
TARGET_FILES = $(TARGET_DIR)/$(TARGET)

# C++ include paths (with -I)
INCLUDE_PATH = -I. -I../../inet4.5/src -I../../simu5g/src -I/home/chuanchao/gurobi1203/linux64/include

# Additional object and library files to link with
EXTRA_OBJS =

# Additional libraries (-L, -l options)
LIBS = $(LDFLAG_LIBPATH)../../inet4.5/src $(LDFLAG_LIBPATH)../../simu5g/src $(LDFLAG_LIBPATH)/home/chuanchao/gurobi1203/linux64/lib  -lINET_dbg -lsimu5g_dbg -lgurobi_c++ -lgurobi120

# Output directory
PROJECT_OUTPUT_DIR = ../out
PROJECTRELATIVE_PATH = src
O = $(PROJECT_OUTPUT_DIR)/$(CONFIGNAME)/$(PROJECTRELATIVE_PATH)

# Object files for local .cc, .msg and .sm files
OBJS = \
    $O/mecrt/apps/scheduler/Scheduler.o \
    $O/mecrt/apps/scheduler/SchemeBase.o \
    $O/mecrt/apps/scheduler/SchemeFastLR.o \
    $O/mecrt/apps/scheduler/SchemeFwdBase.o \
    $O/mecrt/apps/scheduler/SchemeFwdGameTheory.o \
    $O/mecrt/apps/scheduler/SchemeFwdGraphMatch.o \
    $O/mecrt/apps/scheduler/SchemeFwdQuickLR.o \
    $O/mecrt/apps/scheduler/SchemeGameTheory.o \
    $O/mecrt/apps/scheduler/SchemeIterative.o \
    $O/mecrt/apps/scheduler/SchemeSARound.o \
    $O/mecrt/apps/server/Server.o \
    $O/mecrt/apps/ue/UeApp.o \
    $O/mecrt/common/BandManager.o \
    $O/mecrt/common/Database.o \
    $O/mecrt/common/MecCommon.o \
    $O/mecrt/common/NetTopology.o \
    $O/mecrt/coreNetwork/MecGtpUser.o \
    $O/mecrt/coreNetwork/MecRtUser.o \
    $O/mecrt/feedback/MecDlFeedbackGenerator.o \
    $O/mecrt/mobility/MecMobility.o \
    $O/mecrt/mobility/MecStationaryMobility.o \
    $O/mecrt/nic/ip2nic/MecIP2Nic.o \
    $O/mecrt/nic/mac/GnbMac.o \
    $O/mecrt/nic/mac/SlientUeMac.o \
    $O/mecrt/nic/mac/UeMac.o \
    $O/mecrt/nic/mac/allocator/GnbAllocationModule.o \
    $O/mecrt/nic/mac/amc/MecNRAmc.o \
    $O/mecrt/nic/mac/amc/MecPilot.o \
    $O/mecrt/nic/mac/scheduler/GnbSchedulerDl.o \
    $O/mecrt/nic/mac/scheduler/GnbSchedulerUl.o \
    $O/mecrt/nic/mac/scheduler/RbManagerUl.o \
    $O/mecrt/nic/mac/scheme/FDSchemeDl.o \
    $O/mecrt/nic/mac/scheme/FDSchemeUl.o \
    $O/mecrt/nic/pdcp_rrc/MecPdcpRrcEnb.o \
    $O/mecrt/nic/pdcp_rrc/MecPdcpRrcUe.o \
    $O/mecrt/nic/phy/GnbPhy.o \
    $O/mecrt/nic/phy/UePhy.o \
    $O/mecrt/nic/rlc/um/MecRlcUm.o \
    $O/mecrt/packets/apps/Grant2Veh.o \
    $O/mecrt/routing/MecOspf.o \
    $O/mecrt/udp/MecUdp.o \
    $O/mecrt/packets/apps/Grant2Rsu_m.o \
    $O/mecrt/packets/apps/Grant2Veh_Base_m.o \
    $O/mecrt/packets/apps/RsuFeedback_m.o \
    $O/mecrt/packets/apps/ServiceStatus_m.o \
    $O/mecrt/packets/apps/VecPacket_m.o \
    $O/mecrt/packets/nic/VecDataInfo_m.o \
    $O/mecrt/packets/routing/OspfHello_m.o \
    $O/mecrt/packets/routing/OspfLsa_m.o

# Message files
MSGFILES = \
    mecrt/packets/apps/Grant2Rsu.msg \
    mecrt/packets/apps/Grant2Veh_Base.msg \
    mecrt/packets/apps/RsuFeedback.msg \
    mecrt/packets/apps/ServiceStatus.msg \
    mecrt/packets/apps/VecPacket.msg \
    mecrt/packets/nic/VecDataInfo.msg \
    mecrt/packets/routing/OspfHello.msg \
    mecrt/packets/routing/OspfLsa.msg

# SM files
SMFILES =

# Other makefile variables (-K)
INET_PROJ=../../inet4.5
SIMU5G_PROJ=../../simu5g

#------------------------------------------------------------------------------

# Pull in OMNeT++ configuration (Makefile.inc)

ifneq ("$(OMNETPP_CONFIGFILE)","")
CONFIGFILE = $(OMNETPP_CONFIGFILE)
else
CONFIGFILE = $(shell opp_configfilepath)
endif

ifeq ("$(wildcard $(CONFIGFILE))","")
$(error Config file '$(CONFIGFILE)' does not exist -- add the OMNeT++ bin directory to the path so that opp_configfilepath can be found, or set the OMNETPP_CONFIGFILE variable to point to Makefile.inc)
endif

include $(CONFIGFILE)

# Simulation kernel and user interface libraries
OMNETPP_LIBS = -loppenvir$D $(KERNEL_LIBS) $(SYS_LIBS)
ifneq ($(PLATFORM),win32)
LIBS += -Wl,-rpath,$(abspath ../../inet4.5/src) -Wl,-rpath,$(abspath ../../simu5g/src) -Wl,-rpath,$(abspath /home/chuanchao/gurobi1203/linux64/lib)
endif

COPTS = $(CFLAGS) $(IMPORT_DEFINES) -DINET_IMPORT -DSIMU5G_IMPORT $(INCLUDE_PATH) -I$(OMNETPP_INCL_DIR)
MSGCOPTS = $(INCLUDE_PATH)
SMCOPTS =

# we want to recompile everything if COPTS changes,
# so we store COPTS into $COPTS_FILE (if COPTS has changed since last build)
# and make the object files depend on it
COPTS_FILE = $O/.last-copts
ifneq ("$(COPTS)","$(shell cat $(COPTS_FILE) 2>/dev/null || echo '')")
  $(shell $(MKPATH) "$O")
  $(file >$(COPTS_FILE),$(COPTS))
endif

# On Windows, the target has additional file(s). An import lib and an optional debug symbol file is created too.
ifeq ($(PLATFORM),win32)
  TARGET_FILES+= $(TARGET_DIR)/$(TARGET_IMPLIB)
  LDFLAGS+= $(LDFLAG_IMPLIB)$O/$(TARGET_IMPLIB)
  ifeq ($(TOOLCHAIN_NAME),clang-msabi)
    ifeq ($(MODE),debug)
      TARGET_FILES+=$(TARGET_DIR)/$(TARGET_NAME).pdb
    endif
  else
    TARGET_FILES+= $(TARGET_DIR)/$(TARGET_IMPDEF)
    LDFLAGS+= $(LDFLAG_IMPDEF)$O/$(TARGET_IMPDEF)
  endif
endif

#------------------------------------------------------------------------------
# User-supplied makefile fragment(s)
#------------------------------------------------------------------------------

# Main target
all: $(TARGET_FILES)

$(TARGET_DIR)/% :: $O/%
	@mkdir -p $(TARGET_DIR)
	$(Q)$(LN) $< $@
ifeq ($(TOOLCHAIN_NAME),clang-msabi)
	-$(Q)-$(LN) $(<:%.dll=%.lib) $(@:%.dll=%.lib) 2>/dev/null

$O/$(TARGET_NAME).pdb: $O/$(TARGET)
endif

$O/$(TARGET) $O/$(TARGET_IMPDEF) $O/$(TARGET_IMPLIB) &: $(OBJS)  $(wildcard $(EXTRA_OBJS)) Makefile $(CONFIGFILE)
	@$(MKPATH) $O
	@echo Creating shared library: $@
	$(Q)$(SHLIB_LD) -o $O/$(TARGET) $(OBJS) $(EXTRA_OBJS) $(AS_NEEDED_OFF) $(WHOLE_ARCHIVE_ON) $(LIBS) $(WHOLE_ARCHIVE_OFF) $(OMNETPP_LIBS) $(LDFLAGS)
	$(Q)$(SHLIB_POSTPROCESS) $O/$(TARGET)
ifeq ($(PLATFORM),win32)
	$(Q)llvm-ar d $O/$(TARGET_IMPLIB) $(TARGET) # WORKAROUND: throw away the first file from the archive to make the LLD generated import lib valid
endif

.PHONY: all clean cleanall depend msgheaders smheaders

# disabling all implicit rules
.SUFFIXES :
.PRECIOUS : %_m.h %_m.cc

$O/%.o: %.cc $(COPTS_FILE) | msgheaders smheaders
	@$(MKPATH) $(dir $@)
	$(qecho) "$<"
	$(Q)$(CXX) -c $(CXXFLAGS) $(COPTS) -o $@ $<

%_m.cc %_m.h: %.msg
	$(qecho) MSGC: $<
	$(Q)$(MSGC) -s _m.cc -MD -MP -MF $O/$(basename $<)_m.h.d $(MSGCOPTS) $?

%_sm.cc %_sm.h: %.sm
	$(qecho) SMC: $<
	$(Q)$(SMC) -c++ -suffix cc $(SMCOPTS) $?

msgheaders: $(MSGFILES:.msg=_m.h)

smheaders: $(SMFILES:.sm=_sm.h)

clean:
	$(qecho) Cleaning $(TARGET)
	$(Q)-rm -rf $O
	$(Q)-rm -f $(TARGET_FILES)
	$(Q)-rm -f $(call opp_rwildcard, . , *_m.cc *_m.h *_sm.cc *_sm.h)

cleanall:
	$(Q)$(CLEANALL_COMMAND)
	$(Q)-rm -rf $(PROJECT_OUTPUT_DIR)

help:
	@echo "$$HELP_SYNOPSYS"
	@echo "$$HELP_TARGETS"
	@echo "$$HELP_VARIABLES"
	@echo "$$HELP_EXAMPLES"

# include all dependencies
-include $(OBJS:%=%.d) $(MSGFILES:%.msg=$O/%_m.h.d)
