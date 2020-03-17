############## Command Wrappers ################
COPY = cp -rf
MKDIR = mkdir -p

############### Directory Vars #################
# hw / hw_emu / sw_emu
TARGET := hw
HOST_ARCH := x86
DEVICE = xilinx_u280_xdma_201920_1


PWD = $(shell readlink -f .)
XILINX_PROVIDED = $(PWD)/xilinx
FPGA_DIR = $(PWD)/fpga
HOST_DIR = $(PWD)/host

BUILD_DIR = $(PWD)/build
BUILD_TEMP = $(PWD)/temp_$(TARGET)
EMCONFIG_DIR = $(BUILD_TEMP)


############### Compile Setup #################
VPP := v++

include $(XILINX_PROVIDED)/utils.mk
include $(XILINX_PROVIDED)/xcl2/xcl2.mk
include $(XILINX_PROVIDED)/opencl/opencl.mk

CXXFLAGS += $(xcl2_CXXFLAGS)
CXXFLAGS += $(opencl_CXXFLAGS) -Wall -O0 -g -std=c++11
CXXFLAGS += -fmessage-length=0

LDFLAGS += $(xcl2_LDFALGS)
LDFLAGS += $(opencl_LDFLAGS)
LDFLAGS += -lrt -lstdc++

CLFLAGS += -t $(TARGET) --platform $(DEVICE) --save-temps
ifneq ($(TARGET), hw)
		CLFLAGS += -g
endif


############### Result Vars #################
HOST_NAME += test_host
HOST_SRC += $(xcl2_SRCS)
HOST_SRC += $(HOST_DIR)/$(HOST_NAME).cc


############### Compile ###############
check:
		$(ECHO) "$(HOST_SRC)"

.PHONY: all clean cleanall docs emconfig
all: check-devices host kernel emconfig

.PHONY: host
host: $(HOST_NAME)
$(HOST_NAME): check-xrt $(HOST_SRC) $(HOST_HDRS)
	$(CXX) $(CXXFLAGS) $(HOST_SRC) $(HOST_HDRS) -o '$@' $(LDFLAGS)


KERNEL_NAME = vta
KERNEL_SOURCE += $(FPGA_DIR)/$(KERNEL_NAME).cc
KERNEL_OBJECT += $(BUILD_TEMP)/$(KERNEL_NAME).xo
KERNEL_CONTAINER += $(BUILD_DIR)/$(KERNEL_NAME).xclbin

.PHONY: kernel
kernel: $(KERNEL_CONTAINER)
$(KERNEL_OBJECT): $(KERNEL_SOURCE)
	$(MKDIR) $(BUILD_TEMP)
	$(VPP) $(CLFLAGS) --jobs 4 --temp_dir $(BUILD_TEMP) -c -k $(KERNEL_NAME) \
		-I'$(<D)' -o'$@' '$<'
$(KERNEL_CONTAINER): $(KERNEL_OBJECT)
	$(MKDIR) $(BUILD_DIR)
	$(VPP) $(CLFLAGS) --jobs 4 --temp_dir $(BUILD_DIR) -l $(LDCFLAGS) -o'$@' $(+)

emconfig: $(EMCONFIG_DIR)/emconfig.json
	$(EMCONFIG_DIR)/emconfig.json:
		emconfigutil --platform $(DEVICE) --od $(EMCONFIG_DIR)

test: all
ifeq ($(TARGET),$(filter $(TARGET),sw_emu hw_emu))
	cp -rf $(BUILD_TEMP)/emconfig.json .
	XCL_EMULATION_MODE=$(TARGET) ./$(HOST_NAME) $(KERNEL_BITSTREAM)
	./$(EXECUTABLE) $(BUILD_DIR)/vadd.xclbin
	perf_analyze profile -i profile_summary.csv -f html
endif

clean:
	-$(RMDIR) $(HOST_NAME) $(XCLBIN)/{*sw_emu*,*hw_emu*}
	-$(RMDIR) profile_* TempConfig system_estimate.xtxt *.rpt *.csv
	-$(RMDIR) $(HOST_DIR)/*.ll *v++* .Xil emconfig.json dltmp* xmltmp* *.log \
		*.jou *.wcfg *.wdb

cleanall: clean
	-$(RMDIR) $(BUILD_DIR) $(BUILD_TEMP)
	-$(RMDIR) _x.* *xclbin.run_summary qemu-memory-_* emulation/ _vimage/ \
		pl* start_simulation.sh *.xclbin

