# FPGA Acceleration in CMSSW on AWS

Adapted from SONIC for CMS (https://github.com/hls-fpga-machine-learning/SonicCMS)

## Instructions

To setup libraries:
```
source setup.sh
```

To run, place the awsxclbin file (after uploading) in AccelFPGA/HLS4ML/python .
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
cd AccelFPGA/HLS4ML/python
cmsRun hls4ml_test.py
```
