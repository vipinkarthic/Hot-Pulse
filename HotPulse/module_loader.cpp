#include<cuda.h>
#include<iostream>
#include<vector>
#include<string>

#include"debug.h"
#include"input_reader.h"

void handle_error(CUresult error) {
	if (error) {
		char error_string[1024] = { 0 };
		char const* error_string_ptr = nullptr;
		char const** error_string_ptr_ptr = &error_string_ptr;

		cuGetErrorName(error, error_string_ptr_ptr);
		
		if (error_string_ptr) {
			DBG::print_char_array(error_string_ptr, 1024, "CUDA error");
		}
	}
}

int main() {
	handle_error(cuInit(0));

	CUdevice device;
	CUdevice* device_ptr = &device;
	handle_error(cuDeviceGet(device_ptr, 0));

	CUcontext context;
	CUcontext* context_ptr = &context;
	handle_error(cuCtxCreate(context_ptr, NULL, 0, device));

	CUmodule module;
	CUmodule* module_ptr = &module;
	handle_error(cuModuleLoad(module_ptr, "main.ptx"));

	// retreive all function names from the ptx
	std::vector<std::string> function_names;
	read_ptx(function_names);
	
	std::vector<CUfunction> kernels;
	for (std::string& s : function_names) {
		CUfunction kernel;
		CUfunction* kernel_ptr = &kernel;
		const char* name = s.c_str();
		handle_error(cuModuleGetFunction(kernel_ptr, module, name));
		kernels.push_back(kernel);
	}
	
	DBG::print_iterable(function_names, "names");
}
