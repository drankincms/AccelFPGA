
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "AccelFPGA/HLS4ML/interface/XilClientLocal.h"

#include <sstream>
#include <chrono>

XilClientLocal::XilClientLocal(unsigned numStreams, const std::string& kernel_name, const unsigned int inputSize, const unsigned int outputSize) : 
	XilClientBase()
{
	loadInit(kernel_name, inputSize, outputSize);
}

void XilClientLocal::loadInit(const std::string& kernel_name, const unsigned int inputSize, const unsigned int outputSize) {
    // load the graph 
    std::stringstream msg;
    size_t vector_size_in_bytes = sizeof(unsigned int) * inputSize;
    size_t vector_size_out_bytes = sizeof(unsigned int) * outputSize;
    // Allocate Memory in Host Memory
    // When creating a buffer with user pointer (CL_MEM_USE_HOST_PTR), under the hood user ptr 
    // is used if it is properly aligned. when not aligned, runtime had no choice but to create
    // its own host side buffer. So it is recommended to use this allocator if user wish to
    // create buffer using CL_MEM_USE_HOST_PTR to align user buffer to page boundary. It will 
    // ensure that user buffer is used when user create Buffer/Mem object with CL_MEM_USE_HOST_PTR 
    for (unsigned int is = 0; is < inputSize; is++) {
        source_in.push_back((unsigned int)0);
    }
    for (unsigned int is = 0; is < outputSize; is++) {
        source_hw_results.push_back((unsigned int)0);
    }

    inputSize_ = inputSize;
    outputSize_ = outputSize;

// OPENCL HOST CODE AREA START
    // get_xil_devices() is a utility API which will find the xilinx
    // platforms and will return list of devices connected to Xilinx platform
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    cl::Context context(device);
    cl::CommandQueue q_tmp(context, device, CL_QUEUE_PROFILING_ENABLE);
    q = q_tmp;
    std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
    msg << "Found Device=" << device_name.c_str() << "\n";

    // find_binary_file() is a utility API which will search the xclbin file for
    // targeted mode (sw_emu/hw_emu/hw) and for targeted platforms.
    std::string binaryFile = xcl::find_binary_file(device_name,kernel_name.c_str());

    // import_binary_file() ia a utility API which will load the binaryFile
    // and will return Binaries.
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    cl::Program tmp_program(context, devices, bins);
    program = tmp_program;

    // Allocate Buffer in Global Memory
    // Buffers are allocated using CL_MEM_USE_HOST_PTR for efficient memory and 
    // Device-to-host communication
    cl::Buffer buffer_in   (context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, 
            vector_size_in_bytes, source_in.data());
    cl::Buffer buffer_out(context,CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, 
            vector_size_out_bytes, source_hw_results.data());

    inBufVec.clear();
    outBufVec.clear();
    inBufVec.push_back(buffer_in);
    outBufVec.push_back(buffer_out);

    cl::Kernel krnl_tmp(program,kernel_name.c_str());
    krnl_xil = krnl_tmp;

    //int size = DATA_SIZE;
    int narg = 0;
    krnl_xil.setArg(narg++, buffer_in);
    krnl_xil.setArg(narg++, buffer_out);
    edm::LogInfo("XilClientLocal") << msg.str();

}

XilClientLocal::~XilClientLocal() {
    std::stringstream msg;
    edm::LogInfo("XilClientLocal") << msg.str();
}

std::vector<unsigned int> XilClientLocal::runNN(const std::vector<unsigned int> data_in) {

    std::stringstream msg;
    msg << " ====> Accelerate...";
    if (data_in.size() != inputSize_) {
        msg << "Error: incorrect input data size\n";
        return data_in;
    }

    // Create the test data 
    for(unsigned int i = 0 ; i < inputSize_ ; i++){
        source_in[i] = data_in[i];
    }
    for(unsigned int i = 0 ; i < outputSize_ ; i++){
        source_hw_results[i] = 0;
    }

    q.enqueueMigrateMemObjects(inBufVec,0);
    q.enqueueTask(krnl_xil);
    q.enqueueMigrateMemObjects(outBufVec,CL_MIGRATE_MEM_OBJECT_HOST);
    q.finish();

    std::vector<unsigned int> data_out;

    for (unsigned int i = 0; i < outputSize_; ++i) {
        data_out.push_back(source_hw_results[i]);
    }
    edm::LogInfo("XilClientLocal") << msg.str();

    return data_out;

}

//input is "image" in tensor form
void XilClientLocal::predict(unsigned dataID, const std::vector<unsigned int>* input, std::vector<unsigned int>* result, edm::WaitingTaskWithArenaHolder holder) {
	auto t1 = std::chrono::high_resolution_clock::now();

	// Run the NN
	*result = runNN(*input);

	auto t2 = std::chrono::high_resolution_clock::now();
	edm::LogInfo("XilClientLocal") << "Inference time: " << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
	
	//finish
	std::exception_ptr exceptionPtr;
	holder.doneWaiting(exceptionPtr);
}
