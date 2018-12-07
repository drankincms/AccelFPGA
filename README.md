# FPGA Acceleration in CMSSW on AWS

Adapted from SONIC for CMS (https://github.com/hls-fpga-machine-learning/SonicCMS)

## Instructions

(Tested in CMSSW_10_1_11)

Go through the normal F1/CMSSW setup:
```
cd $AWS_FPGA_REPO_DIR                                         
source sdaccel_setup.sh
cd CMSSW_10_1_11/src
sudo sh
source /opt/Xilinx/SDx/2017.4.rte.dyn/setup.sh
source /cvmfs/cms.cern.ch/cmsset_default.sh
cmsenv
```
Checkout package:
```
git clone https://github.com/drankincms/AccelFPGA.git
cd AccelFPGA
```

To setup libraries:
```
chmod +x setup.sh
./setup.sh
scram b
```

To run, place the awsxclbin file (after uploading) in AccelFPGA/HLS4ML/python (an hls4ml example with a conv1d exists as the default).
Then, simply edit the kernel name to match your own and provide the number of inputs and outputs in AccelFPGA/HLS4ML/python/hls4ml_test.py:
```
process.hls4mlProducer = cms.EDProducer('NNAccelProducer',
    JetTag = cms.InputTag('slimmedJetsAK8'),
    kernelName = cms.string("aws_hls4ml"),
    inputSize = cms.uint32(40),
    outputSize = cms.uint32(5),
)
```

Then:
```
cd HLS4ML/python
cmsRun hls4ml_test.py
```

Kernel development should follow the standard instructions here: https://github.com/aws/aws-fpga/tree/master/SDAccel

The particular kernel contained here can be built using: https://github.com/drankincms/SDAccel_Examples

The libhost.so file should be created by adding
```
        $(CXX) $(CXXFLAGS) $($(1)_CXXFLAGS) -fPIC -shared -o lib$$@.so $($(1)_SRCS) $($(1)_LDFLAGS) $(LDFLAGS)
```
to `mk_exe` in /home/centos/src/project_data/aws-fpga/SDAccel/examples/xilinx/utility/rules.mk
