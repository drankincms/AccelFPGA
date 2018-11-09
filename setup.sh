#!/bin/bash

# setup in scram
cat << 'EOF_TOOLFILE' > opencl.xml
  <tool name="OpenCL" version="1.2">
    <lib name="OpenCL"/>
    <client>
      <environment name="OPENCL_BASE" default="/opt/Xilinx/SDx/2017.1.rte.4ddr/runtime"/>
      <environment name="LIBDIR" default="$OPENCL_BASE/lib/x86_64/"/>
    </client>
  </tool>
EOF_TOOLFILE

mv opencl.xml ../../config/toolbox/${SCRAM_ARCH}/tools/selected/
scram setup opencl

# setup in scram
# requires libhost.so to be placed in /home/centos/hostlib
# build libhost.so by modifying mk_exe in /home/centos/src/project_data/aws-fpga/SDAccel/examples/xilinx/utility/rules.mk with: 
#        $(CXX) $(CXXFLAGS) $($(1)_CXXFLAGS) -fPIC -shared -o lib$$@.so $($(1)_SRCS) $($(1)_LDFLAGS) $(LDFLAGS)
cat << 'EOF_TOOLFILE' > host.xml
  <tool name="host" version="1.0">
    <lib name="host"/>
    <client>
      <environment name="LIBDIR" default="/home/centos/hostlib"/>
    </client>
  </tool>
EOF_TOOLFILE

mv host.xml ../../config/toolbox/${SCRAM_ARCH}/tools/selected/
scram setup host
