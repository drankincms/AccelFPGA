from FWCore.ParameterSet.VarParsing import VarParsing
# Forked from SMPJ Analysis Framework
import FWCore.ParameterSet.Config as cms
import os, sys, json

options = VarParsing("analysis")
options.register("threads", 1, VarParsing.multiplicity.singleton, VarParsing.varType.int)
options.register("streams", 0, VarParsing.multiplicity.singleton, VarParsing.varType.int)
options.parseArguments()

process = cms.Process('hls4mlTest')

#--------------------------------------------------------------------------------
# Import of standard configurations
#================================================================================
process.load('FWCore/MessageService/MessageLogger_cfi')
process.load('Configuration/StandardSequences/GeometryDB_cff')
process.load('Configuration/StandardSequences/MagneticField_38T_cff')

#process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
#process.GlobalTag.globaltag = cms.string('100X_upgrade2018_realistic_v10')

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(-1) )
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring('file:test_TTbar_100X.root')
)

################### EDProducer ##############################
process.hls4mlProducer = cms.EDProducer('NNAccelProducer',
    JetTag = cms.InputTag('slimmedJetsAK8'),
    kernelName = cms.string("aws_hls4ml"), #requires corresponding awsxclbin file in this directory
    inputSize = cms.uint32(40),
    outputSize = cms.uint32(5),
)

# Let it run
process.p = cms.Path(
    process.hls4mlProducer
)

process.MessageLogger.cerr.FwkReport.reportEvery = 1
keep_msgs = ['NNAccelProducer','XilClientRemote','XilClientLocal']
for msg in keep_msgs:
    process.MessageLogger.categories.append(msg)
    setattr(process.MessageLogger.cerr,msg,
        cms.untracked.PSet(
            optionalPSet = cms.untracked.bool(True),
            limit = cms.untracked.int32(10000000),
        )
    )

if options.threads>0:
    if not hasattr(process,"options"):
        process.options = cms.untracked.PSet()
    process.options.numberOfThreads = cms.untracked.uint32(options.threads)
    process.options.numberOfStreams = cms.untracked.uint32(options.streams if options.streams>0 else 0)
