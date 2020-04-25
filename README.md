<!--- Licensed to the Apache Software Foundation (ASF) under one -->
<!--- or more contributor license agreements.  See the NOTICE file -->
<!--- distributed with this work for additional information -->
<!--- regarding copyright ownership.  The ASF licenses this file -->
<!--- to you under the Apache License, Version 2.0 (the -->
<!--- "License"); you may not use this file except in compliance -->
<!--- with the License.  You may obtain a copy of the License at -->

<!---   http://www.apache.org/licenses/LICENSE-2.0 -->

<!--- Unless required by applicable law or agreed to in writing, -->
<!--- software distributed under the License is distributed on an -->
<!--- "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY -->
<!--- KIND, either express or implied.  See the License for the -->
<!--- specific language governing permissions and limitations -->
<!--- under the License. -->

# CS 8803 Project Submission
## Environment
The technical stack we used can be compatible with different platforms such as
Windows, OS X, and Linux. However, we recommend you to use Ubuntu for
reproducing the result.

## Installation
### Xilinx Vitis Installation
For running this repo, you need to install Xilinx Vitis platform. Go to this
link https://bit.ly/3cOZeQn and follow the instructions to install Xilinx Vitis
Platform. Please install the Vitis 2019.2 version to reproduce the result. It
may take about an hour to install Vitis.

After installing Vitis, environment variables need to be set. The below path is
an example. Based on your Vitis installation path, please set the proper
environment variables.

```bash
source /tools/reconfig/xilinx/Vitis/Vitis/2019.2/settings64.sh
export XILINX_XRT=/opt/xilinx/xrt
export XCL_EMULATION_MODE=sw_emu
export PATH=/opt/xilinx/xrt/bin:$PATH
export LD_LIBRARY_PATH=/opt/xilinx/xrt/lib:$LD_LIBRARY_PATH
export PYTHONPATH=/opt/xilinx/xrt/python:$PYTHONPATH
```

### Install TVM and VTA
To install TVM & VTA, follow the instructions below.

```bash
# Clone the repository
git clone --recursive https://github.com/jheo4/incubator-tvm tvm
cd tvm
git submodule init
git submodule update

# Install dependencies and build the shared libs
sudo apt-get update
sudo apt-get install -y python3 python3-dev python3-setuptools gcc \
          libtinfo-dev zlib1g-dev build-essential cmake libedit-dev libxml2-dev

git checkout CS8803
mkdir build
cp cmake/config.cmake build
cd build
cmake ..
make -j8


# After install TVM from the source, set up the python interface
# TVM and VTA are based on the python3, please use Python3
export TVM_HOME=/path/to/tvm
export PYTHONPATH=$TVM_HOME/python:$TVM_HOME/topi/python:${PYTHONPATH}
export PYTHONPATH=$TVM_HOME/vta/python:${PYTHONPATH}
```

### Run Example
This example is to reproduce the demo video I submitted.
```bash
# compile the instruction bitstream
cd $TVM_HOME/vta/hardware/vitis/test
make build
# open the vitis_driver codes and modify the xclbin path to the generated
#    bitstream.
# vitis/vitis_driver.cc:69:  std::string binaryFile="YOUR_TVM_HOME/vta/hardware/vitis/test/build_dir.sw_emu.xilinx_u280_xdma_201920_1/vadd.xclbin";
vim $TVM_HOME/vta/src/vitis/vitis_driver.cc
cd $TVM_HOME/build && make -j8

# setup the example environment
sudo apt install virtualenv
cd $TVM_HOME/cs8803
virtualenv venv --python=python3
. ./venv/bin/activate
pip install -r requirements.txt
python example.py
```
