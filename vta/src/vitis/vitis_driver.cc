/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*!
 * \file vitis_driver.cc
 * \brief VTA driver for Vitis.
 */
#include <vta/driver.h>
#include <vta/hw_spec.h>
#include <tvm/runtime/registry.h>
#include <type_traits>
#include <map>
#include <unordered_map>
#include <cstring>
#include <sstream>
#include <vta/xcl2.h>

/*
void VitisProgramBin(cl::Context& context, cl::Device& device, const char *binaryFile) {
  cl_int err;
  auto file_buf = xcl::read_binary_file(binaryFile);
  cl::Program::Binaries bins{{file_buf.data(), file_buf.size()}};
  OCL_CHECK(err, cl::Program program(context, {device}, bins, NULL, &err));
  if(err != CL_SUCCESS) {
    printf("Failed to program devcie with xclbin %s\n", binaryFile);
  }
}*/


class VTADevice {
  private:
    cl::Context context;
    cl::Device device;
    cl::Kernel vta;
    cl::CommandQueue cmd_queue;
    cl_int err; // OCL error checking flag

    // test
    cl::Buffer buffer_in1, buffer_in2, buffer_output;
    int *source_in1, *source_in2, *source_hw_results, *source_sw_results;
    int run_index;
  public:
    VTADevice() {
      char name[2048];

      std::vector<cl::Device> devices = xcl::get_xil_devices();
      device = devices[0]; // currently fixed with the first device
      clGetDeviceInfo(device(), CL_DEVICE_NAME, sizeof(name), name, NULL);

      printf("\t [VITIS_DEVICE_INFO] # of devices : %d \n", devices.size());
      printf("\t [VITIS_DEVICE_INFO] device name : %s \n", name);

      std::string binaryFile = "/nethome/jheo33/github/Vitis_Accel_Examples/hello_world/build_dir.sw_emu.xilinx_u280_xdma_201920_1/vadd.xclbin";
      std::vector<unsigned char> fileBuf = xcl::read_binary_file(binaryFile);
      printf("\t [VITIS BITSTREAM INFO] file buf : %d\n", fileBuf.size());

      cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};

      OCL_CHECK(err, context = cl::Context({device}, NULL, NULL, NULL, &err));
      OCL_CHECK(err, cmd_queue = cl::CommandQueue(context, {device},
                CL_QUEUE_PROFILING_ENABLE, &err));
      OCL_CHECK(err, cl::Program program(context, {device}, bins, NULL, &err));
      if(err != CL_SUCCESS) {
        printf("Fail to program device\n");
      }
      else {
        OCL_CHECK(err, vta = cl::Kernel(program, "vta", &err));
      }

      // test
      //  - data must be ready before the run function like VTAMemAlloc
      size_t vector_size_bytes = sizeof(int) * 4096;
      run_index = 0;

      // test data generation; this part would be done by TVM runtime stacks
      //  - TVM runtim invokes VTA runtimes to put some data into the memory
      //    allocated by VTAMemAlloc
      posix_memalign((void**)&source_in1, 4096, 4096 * sizeof(int));
      posix_memalign((void**)&source_in2, 4096, 4096 * sizeof(int));
      posix_memalign((void**)&source_hw_results, 4096, 4096 * sizeof(int));
      posix_memalign((void**)&source_sw_results, 4096, 4096 * sizeof(int));

      for (int i = 0; i < 4096; i++) {
        source_in1[i] = i;
        source_in2[i] = i;
        source_sw_results[i] = source_in1[i] + source_in2[i];
        source_hw_results[i] = 0;
      }



      // corresponding to VTAMemCopyFromHost
      OCL_CHECK(err,
                buffer_in1 = cl::Buffer(context,
                                        CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                        vector_size_bytes,
                                        source_in1,
                                        &err));
      OCL_CHECK(err,
                buffer_in2 = cl::Buffer(context,
                                        CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                        vector_size_bytes,
                                        source_in2,
                                        &err));
      OCL_CHECK(err,
                buffer_output = cl::Buffer(context,
                                       CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                                       vector_size_bytes,
                                       source_hw_results,
                                       &err));
			OCL_CHECK(err, err = vta.setArg(0, buffer_in1));
			OCL_CHECK(err, err = vta.setArg(1, buffer_in2));
			OCL_CHECK(err, err = vta.setArg(2, buffer_output));
			OCL_CHECK(err, err = vta.setArg(3, 4096));
			OCL_CHECK(err, err = cmd_queue.enqueueMigrateMemObjects({buffer_in1,
								buffer_in2}, 0));

    }

    int Run(vta_phy_addr_t insn_phy_addr,
            uint32_t insn_count,
            uint32_t wait_cycles) {
      cl::Event event;
      printf("\t[VITIS VTA RUN] run index : %d\n", run_index++);
      OCL_CHECK(err, err = cmd_queue.enqueueTask(vta));

      // VTAMemCopyToHost
      OCL_CHECK(err,
                err = cmd_queue.enqueueMigrateMemObjects({buffer_output},
                                                 CL_MIGRATE_MEM_OBJECT_HOST));
      cmd_queue.finish();

      bool match = true;
      for (int i = 0; i < 4096; i++) {
        if (source_hw_results[i] != source_sw_results[i]) {
          printf("\t\t[VITIS VTA RUN] Error: Result mismatch at %d\n", i);
          //printf("\t\t[VITIS VTA RUN] sw/hw : %d/%d\n", source_sw_results[i], source_hw_results[i]);
          match = false;
          break;
        }
      }
      printf("\t\t result : %s\n", (match ? "PASSED" : "FAILED"));

      // Data must be alread loaded to host memory
      // over here, just send them & let the kernel function work
      // until it's done
      //VTAGenericInsn* insn = static_cast<VTAGenericInsn*>(
      //    dram_->GetAddr(insn_phy_addr));
      //finish_counter_ = 0;
      //for (uint32_t i = 0; i < insn_count; ++i) {
      //  this->Run(insn + i);
      //}
      //
      //OCL_CHECK(err, cl::Buffer instruction_buff (context,
      //        CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
      //        insn_count * sizeof(VTAGenericInsn),
      //        reinterpret_cast<char*>(insn_phy_addr),
      //        &err));
      //OCL_CHECK(err, vta = cl::Kernel(program, "vta", &err));
      //OCL_CHECK(err, err = vta.setArg(0, instruction_buff));
      //OCL_CHECK(err, err = cmd_queue.enqueueTask(vta, NULL &event))
      //OCL_CHECK(err, err = q.enqueueMigrateMemObjects([instruction_buff]), CL_MIGRATE_MEM_OBJECT_HOST);
      //q.finish();
      return 0;
    }

  //private:
  //  void Run(const VTAGenericInsn* insn) {
  //    printf("execution...\n");
  //  }
};


/* VTA Driver Functions
 *   vta/include/vta/driver.h
 */
void* VTAMemAlloc(size_t size, int cached) {
  return malloc(size);
}

void VTAMemFree(void* buf) {
  free(buf);
}

vta_phy_addr_t VTAMemGetPhyAddr(void* buf) {
}

void VTAMemCopyFromHost(void* dst, const void* src, size_t size) {
  memcpy(dst, src, size);
}

void VTAMemCopyToHost(void* dst, const void* src, size_t size) {
  memcpy(dst, src, size);
}

void VTAFlushCache(void* vir_addr, vta_phy_addr_t phy_addr, int size) {
}

void VTAInvalidateCache(void* vir_addr, vta_phy_addr_t phy_addr, int size) {
}

VTADeviceHandle VTADeviceAlloc() {
  printf("VTADeviceAlloc()\n");
  return new VTADevice();
}

void VTADeviceFree(VTADeviceHandle handle) {
  delete static_cast<VTADevice*>(handle);
}

int VTADeviceRun(VTADeviceHandle handle,
                 vta_phy_addr_t insn_phy_addr,
                 uint32_t insn_count,
                 uint32_t wait_cycles) {
  return static_cast<VTADevice*>(handle)->Run(
      insn_phy_addr, insn_count, wait_cycles);
}

