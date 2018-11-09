#ifndef XILCLIENTLOCAL_H
#define XILCLIENTLOCAL_H

#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "XilClientBase.h"
#include "/home/centos/src/project_data/aws-fpga/SDAccel/examples/xilinx/libs/xcl2/xcl2.hpp"

#include <string>
#include <vector>
#include <memory>

class XilClientLocal : public XilClientBase {
	public:
		//constructors (timeout in seconds)
		XilClientLocal() : XilClientBase() {}
		XilClientLocal(unsigned numStreams, const std::string& kernel_name, const unsigned int inputSize, const unsigned int outputSize);
		~XilClientLocal();
		
		//input is std::vector<data32_t>
		void predict(unsigned dataID, const std::vector<unsigned int>* input, std::vector<unsigned int>* result, edm::WaitingTaskWithArenaHolder holder) override;
		
	private:
		void loadInit(const std::string& kernel_name, const unsigned int inputSize, const unsigned int outputSize);
		std::vector<unsigned int> runNN(const std::vector<unsigned int>);
		
		//members

                unsigned int inputSize_ = 0;
                unsigned int outputSize_ = 0;

                std::vector<unsigned int,aligned_allocator<unsigned int>> source_in;
                std::vector<unsigned int,aligned_allocator<unsigned int>> source_hw_results;

                std::vector<cl::Memory> inBufVec;
                std::vector<cl::Memory> outBufVec;

                cl::Program program;

                cl::Kernel krnl_xil;
        
                cl::CommandQueue q;

};

#endif
