// -*- C++ -*-
//
// Package:    AccelFPGA/HLS4ML
// Class:      NNAccelProducer
// 
/**\class NNAccelProducer NNAccelProducer.cc AccelFPGA/HLS4ML/plugins/NNAccelProducer.cc

 Description: [one line class summary]

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  Cloud User
//         Created:  Wed, 07 Nov 2018 20:41:08 GMT
//
//


// system include files
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <string>
#include <cmath>
#include <functional>
#include <vector>
#include <chrono>
#include <map>
#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/StreamID.h"

#include "AccelFPGA/HLS4ML/interface/XilClientBase.h"
#include "AccelFPGA/HLS4ML/interface/XilClientLocal.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/global/EDProducer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Framework/src/PreallocationConfiguration.h"

#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/JetReco/interface/Jet.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"

//
// class declaration
//

class NNCache {
	public:
		const std::vector<unsigned int>& input() const { return input_; }
		std::vector<unsigned int>& input() { return input_; }

		const std::vector<unsigned int>& output() const { return output_; }
		std::vector<unsigned int>& output() { return output_; }

	private:
		std::vector<unsigned int> input_;
		std::vector<unsigned int> output_;
};

class NNAccelProducer : public edm::global::EDProducer<edm::ExternalWork,edm::StreamCache<NNCache>> {
   public:
      explicit NNAccelProducer(edm::ParameterSet const& cfg);
      void preallocate(edm::PreallocationConfiguration const& iPrealloc) override;
      std::unique_ptr<NNCache> beginStream(edm::StreamID) const override;
      void acquire(edm::StreamID iStream, edm::Event const& iEvent, edm::EventSetup const& iSetup, edm::WaitingTaskWithArenaHolder holder) const override;
      void produce(edm::StreamID iStream, edm::Event& iEvent, edm::EventSetup const& iSetup) const;
      ~NNAccelProducer() override;

      //static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

   private:
      unsigned int f_to_ui(float, unsigned int, unsigned int) const;//would not be necessary once ap_fixed is available
      float ui_to_f(unsigned int, unsigned int, unsigned int) const;//would not be necessary once ap_fixed is available
      std::vector<unsigned int> createInput(const edm::View<pat::Jet>& jets) const; //make input data from whatever objects

      //virtual void beginRun(edm::Run const&, edm::EventSetup const&) override;
      //virtual void endRun(edm::Run const&, edm::EventSetup const&) override;
      //virtual void beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) override;
      //virtual void endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&) override;

      // ----------member data ---------------------------
      std::unique_ptr<XilClientBase> client_;
      edm::InputTag JetTag_;
      edm::EDGetTokenT<edm::View<pat::Jet>> JetTok_;
      std::string kernelName_;
      unsigned int inputSize_;
      unsigned int outputSize_;

};

//
// constants, enums and typedefs
//


//
// static data member definitions
//

//
// constructors and destructor
//
NNAccelProducer::NNAccelProducer(edm::ParameterSet const &cfg) :
        JetTag_(cfg.getParameter<edm::InputTag>("JetTag")),
        JetTok_(consumes<edm::View<pat::Jet>>(JetTag_)),
        kernelName_(cfg.getParameter<std::string>("kernelName")),
        inputSize_(cfg.getParameter<unsigned>("inputSize")),
        outputSize_(cfg.getParameter<unsigned>("outputSize"))
{
}


void NNAccelProducer::preallocate(edm::PreallocationConfiguration const& iPrealloc) {
	client_ = std::make_unique<XilClientLocal>(
		iPrealloc.numberOfStreams(),
                kernelName_,
                inputSize_,
                outputSize_
	);
}

unsigned int NNAccelProducer::f_to_ui(float f, unsigned int B, unsigned int I) const {
    float tmpI = fabs(f) - float(int(fabs(f)));
    float tmpF = fabs(f) - tmpI;
    while (tmpI > (float((1 << I)-1)/2.)) {
        tmpI -= float((1 << I)-1);
    }
    unsigned int val = (unsigned int)(tmpF * float(1 << (B-I-1)));
    val += ((unsigned int)(tmpI) << (B-I));
    return val;
}

float NNAccelProducer::ui_to_f(unsigned int ui, unsigned int B, unsigned int I) const { //not quite the right conversion, should be close though
    float tmpI = float((unsigned int)((ui & (((1 << I)-1) << (B-I))) >> (B-I)));
    float tmpF = float((unsigned int)(ui & ((1 << (B-I))-1)))/float(1 << (B-I-1));
    if (tmpI > (float((1 << I)-1)/2.)) {
        tmpI -= float((1 << I)-1);
    }
    float val = tmpI+tmpF;
    return val;
}

std::vector<unsigned int> NNAccelProducer::createInput(const edm::View<pat::Jet>& jets) const {

        std::vector<unsigned int> outvec;

	//int jet_ctr = 0;
	for(const auto& i_jet : jets){
                //jet calcs
                outvec.push_back(f_to_ui(float(i_jet.pt()),32,8));
                if (outvec.size() == inputSize_) break;
                outvec.push_back(f_to_ui(float(i_jet.eta()),32,8));
                if (outvec.size() == inputSize_) break;
                outvec.push_back(f_to_ui(float(i_jet.phi()),32,8));
                if (outvec.size() == inputSize_) break;
                outvec.push_back(f_to_ui(float(i_jet.energy()),32,8));
                if (outvec.size() == inputSize_) break;
 
        }

        unsigned int ctr = 0;
        while (outvec.size() != inputSize_) {
                outvec.push_back(ctr);
                ctr++;
        }

        return outvec;

}

std::unique_ptr<NNCache> NNAccelProducer::beginStream(edm::StreamID) const {
	return std::make_unique<NNCache>();
}

void NNAccelProducer::acquire(edm::StreamID iStream, edm::Event const& iEvent, edm::EventSetup const& iSetup, edm::WaitingTaskWithArenaHolder holder) const {

        edm::Handle<edm::View<pat::Jet>> h_jets;
        iEvent.getByToken(JetTok_, h_jets);

	//reset cache of input and output
	NNCache* streamCacheData = streamCache(iStream);
        std::vector<unsigned int> tmpout;
	streamCacheData->output() = tmpout;
	
	auto t0 = std::chrono::high_resolution_clock::now();

        //dummy data input
        /*std::vector<unsigned int> tmpin;
        for (unsigned int i = 0; i < inputSize_; i++) { // dummy data
            tmpin.push_back(unsigned int(i));
        }
	streamCacheData->input() = tmpin;*/
        streamCacheData->input() = createInput(*h_jets.product());

	auto t1 = std::chrono::high_resolution_clock::now();
	edm::LogInfo("NNAccelProducer") << "Input time: " << std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

	// run the inference on remote or local
	client_->predict(iStream.value(),&streamCacheData->input(),&streamCacheData->output(),holder);

}

void NNAccelProducer::produce(edm::StreamID iStream, edm::Event& iEvent, edm::EventSetup const &iSetup) const {
	NNCache* streamCacheData = streamCache(iStream);
	//check the results
	std::stringstream msg;
        auto result = streamCacheData->output();
        for (unsigned int i = 0; i < outputSize_; i++) {
            msg << std::to_string(ui_to_f((unsigned int)(result[i]),32,8)) << "  ";
        }
        edm::LogInfo("NNAccelProducer") << msg.str();
}

NNAccelProducer::~NNAccelProducer()
{
 
   // do anything here that needs to be done at destruction time
   // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

/*
// ------------ method called once each stream before processing any runs, lumis or events  ------------
void
NNAccelProducer::beginStream(edm::StreamID)
{
}

// ------------ method called once each stream after processing all runs, lumis and events  ------------
void
NNAccelProducer::endStream() {
}
*/
// ------------ method called when starting to processes a run  ------------
/*
void
NNAccelProducer::beginRun(edm::Run const&, edm::EventSetup const&)
{
}
*/
 
// ------------ method called when ending the processing of a run  ------------
/*
void
NNAccelProducer::endRun(edm::Run const&, edm::EventSetup const&)
{
}
*/
 
// ------------ method called when starting to processes a luminosity block  ------------
/*
void
NNAccelProducer::beginLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}
*/
 
// ------------ method called when ending the processing of a luminosity block  ------------
/*
void
NNAccelProducer::endLuminosityBlock(edm::LuminosityBlock const&, edm::EventSetup const&)
{
}
*/
 
// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
/*void
NNAccelProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters
  edm::ParameterSetDescription desc;
  desc.setUnknown();
  descriptions.addDefault(desc);
}*/

//define this as a plug-in
DEFINE_FWK_MODULE(NNAccelProducer);
