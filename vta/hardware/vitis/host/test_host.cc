#include <xcl2.hpp>
#include <algorithm>
#include <vector>

#define DATA_SIZE 4096

using namespace std;

int main(int argc, char** argv)
{
  if(argc != 2) {
    cout << "Usage: " << argv[0] << " <XCLBIN file>" << endl;
    return EXIT_FAILURE;
  }

  string xclbin_fn = argv[1];
  size_t vector_size_bytes = sizeof(int) * DATA_SIZE;

  cout << xclbin_fn << endl;

  cl_int err;
  cl::Context context;
  cl::Kernel kernel_vadd;
  cl::CommandQueue cmd_queue;

  auto devices = xcl::get_xil_devices();
  auto device = devices[0];

  auto xclbin_file = xcl::read_binary_file(xclbin_fn);
  cl::Program::Binaries xclbins{{ xclbin_file.data(), xclbin_file.size() }};
  OCL_CHECK(err, context = cl::Context({device}, NULL, NULL, NULL, &err));
  OCL_CHECK(err, cmd_queue = cl::CommandQueue(context, {device},
                                              CL_QUEUE_PROFILING_ENABLE,
                                              &err));
  OCL_CHECK(err, cl::Program program(context, {device}, xclbins, NULL, &err));

  OCL_CHECK(err, kernel_vadd = cl::Kernel(program, "vta", &err));

  int * insns = (int*)malloc(4096);
  int * uops = (int*)malloc(4096);
  int * inputs = (int*)malloc(4096);
  int * weights = (int*)malloc(4096);
  int * biases = (int*)malloc(4096);
  int * outputs = (int*)malloc(4096);

  OCL_CHECK(err, cl::Buffer insns_buffer(context,
                                        CL_MEM_USE_HOST_PTR,
                                        4096,
                                        insns,
                                        &err));
  OCL_CHECK(err, cl::Buffer uop_buffer(context,
                                        CL_MEM_USE_HOST_PTR,
                                        4096,
                                        uops,
                                        &err));
  OCL_CHECK(err, cl::Buffer inputs_buffer(context,
                                        CL_MEM_USE_HOST_PTR,
                                        4096,
                                        inputs,
                                        &err));
  OCL_CHECK(err, cl::Buffer weights_buffer(context,
                                        CL_MEM_USE_HOST_PTR,
                                        4096,
                                        weights,
                                        &err));
  OCL_CHECK(err, cl::Buffer biases_buffer(context,
                                        CL_MEM_USE_HOST_PTR,
                                        4096,
                                        biases,
                                        &err));
  OCL_CHECK(err, cl::Buffer outputs_buffer(context,
                                        CL_MEM_USE_HOST_PTR,
                                        4096,
                                        outputs,
                                        &err));
  OCL_CHECK(err, err = kernel_vadd.setArg(0, 5));
  OCL_CHECK(err, err = kernel_vadd.setArg(1, insns_buffer));
  OCL_CHECK(err, err = kernel_vadd.setArg(2, uop_buffer));
  OCL_CHECK(err, err = kernel_vadd.setArg(3, inputs_buffer));
  OCL_CHECK(err, err = kernel_vadd.setArg(4, weights_buffer));
  OCL_CHECK(err, err = kernel_vadd.setArg(5, biases_buffer));
  OCL_CHECK(err, err = kernel_vadd.setArg(6, outputs_buffer));
  OCL_CHECK(err, err = cmd_queue.enqueueMigrateMemObjects({insns_buffer,
                                                            uop_buffer,
                                                            inputs_buffer,
                                                            weights_buffer,
                                                            biases_buffer}, 0));
  OCL_CHECK(err, err = cmd_queue.enqueueTask(kernel_vadd));
  OCL_CHECK(err, err = cmd_queue.enqueueMigrateMemObjects({outputs_buffer}, CL_MIGRATE_MEM_OBJECT_HOST));
  cmd_queue.finish();
  cout << "Finished running fake kernel" << endl;
  //vector<int, aligned_allocator<int>> source_in1(DATA_SIZE);
  //vector<int, aligned_allocator<int>> source_in2(DATA_SIZE);
  //vector<int, aligned_allocator<int>> source_hw_res(DATA_SIZE);
  //vector<int, aligned_allocator<int>> source_sw_res(DATA_SIZE);

  //generate(source_in1.begin(), source_in1.end(), rand);
  //generate(source_in2.begin(), source_in2.end(), rand);
  //for(int i = 0; i < DATA_SIZE; i++) {
  //  source_sw_res[i] = source_in1[i] + source_in2[i];
  //  source_hw_res[i] = 0;
  //}

  //OCL_CHECK(err, cl::Buffer in1_buffer(context,
  //                                     CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
  //                                     vector_size_bytes,
  //                                     source_in1.data(),
  //                                     &err));
  //OCL_CHECK(err, cl::Buffer in2_buffer(context,
  //                                     CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
  //                                     vector_size_bytes,
  //                                     source_in2.data(),
  //                                     &err));
  //OCL_CHECK(err, cl::Buffer out_buffer(context,
  //                                     CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
  //                                     vector_size_bytes,
  //                                     source_hw_res.data(),
  //                                     &err));
  //OCL_CHECK(err, err = kernel_vadd.setArg(0, in1_buffer));
  //OCL_CHECK(err, err = kernel_vadd.setArg(1, in2_buffer));
  //OCL_CHECK(err, err = kernel_vadd.setArg(2, out_buffer));
  //int data_size = DATA_SIZE;
  //OCL_CHECK(err, err = kernel_vadd.setArg(3, data_size));

  //OCL_CHECK(err, err = cmd_queue.enqueueMigrateMemObjects({in1_buffer,
  //                                                         in2_buffer}, 0));
  //OCL_CHECK(err, err = cmd_queue.enqueueTask(kernel_vadd));
  //// start a timer...
  //OCL_CHECK(err, err = cmd_queue.enqueueMigrateMemObjects({out_buffer},
  //                                                CL_MIGRATE_MEM_OBJECT_HOST));
  //// end a timer
  //cmd_queue.finish();

  /*
  bool match = true;
  for(int i = 0; i < DATA_SIZE; i++) {
    if(source_hw_res[i] != source_sw_res[i]) {
      cout << "Mismatch!" << endl;
      cout << i << "th, CPU val=" << source_sw_res[i] \
        << ", FPGA val=" << source_hw_res[i] << endl;
      match = false;
      break;
    }
  }
  cout << "Test result : " << (match ? "PASSED" : "FAILED") << endl;
  return (match ? EXIT_SUCCESS : EXIT_FAILURE);*/
  return 0;
}

