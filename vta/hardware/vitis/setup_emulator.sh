source /tools/reconfig/xilinx/Vitis/Vitis/2019.2/settings64.sh
source /opt/xilinx/xrt/setup.sh
emconfigutil --platform xilinx_u280_xdma_201920_1
# export XCL_EMULATION_MODE=sw_emu
# export XCL_EMULATION_MODE=hw_emu
# modifiy makefile target also
#   TARGET := hw_emu or sw_emu
