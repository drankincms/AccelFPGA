#ifndef XILCLIENTBASE_H
#define XILCLIENTBASE_H

#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Concurrency/interface/WaitingTaskWithArenaHolder.h"

#include <vector>

//base class for xilinx client
class XilClientBase {
	public:
		//constructor
		XilClientBase() {}
		//destructor
		virtual ~XilClientBase() {}
		
		//input is std::vector<float>
		virtual void predict(unsigned dataID, const std::vector<unsigned int>* input, std::vector<unsigned int>* result, edm::WaitingTaskWithArenaHolder holder) {}
};

#endif
