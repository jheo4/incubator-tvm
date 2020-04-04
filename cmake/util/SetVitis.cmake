# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

#######################################################
# Enhanced version of find CUDA.
#
# Usage:
#   set_vitis(${USE_VTA_FPGA})
#
# Provide variables:
#    CXX
#    VITIS_OPENCL_INCLUDE
#    VITIS_VIVADO_INCLUDE
#    OPENCL_LIB
#    opencl_LDFLAGS
#
macro(set_vitis use_vta_fpga)
  set(__use_vitis ${use_vta_fpga})
  if(EXISTS $ENV{XILINX_VITIS} AND EXISTS $ENV{XILINX_XRT} AND EXISTS $ENV{XILINX_VIVADO})
      # Currently the host architecture is fixed with x86 only
      # Xilinx Utils
      set(CXX "$ENV{XILINX_VIVADO}/tps/gcc-6.2.0/bin/g++")

      # Xilinx xcl
      set(VITIS_OPENCL_INCLUDE "$ENV{XILINX_XRT}/include")
      set(VITIS_VIVADO_INCLUDE "$ENV{XILINX_VIVADO}/include")

      set(OPENCL_LIB "${OPENCL_LIB} $ENV{XILINX_XRT}/lib")
      set(opencl_LDFLAGS "-L${OPENCL_LIB} -lOpenCL -lpthread")
      list(APPEND TVM_RUNTIME_LINKER_LIBS ${opencl_LDFLAGS} -lrt -lstdc++)

      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

      set(_set_vitis TRUE)
  else()
    message(STATUS "XILINX_VITIS and XILINX_XRT are not set")
  endif()

  if(_set_vitis)
    message(STATUS "CXX=" ${CXX})
    message(STATUS "VITIS_OPENCL_INCLUDE=" ${VITIS_OPENCL_INCLUDE})
    message(STATUS "VITIS_VIVADO_INCLUDE=" ${VITIS_VIVADO_INCLUDE})
    message(STATUS "OPENCL_LIB=" ${OPENCL_LIB})
    message(STATUS "opencl_LDFLAGS=" ${opencl_LDFLAGS})
  endif(_set_vitis)
endmacro(set_vitis)
