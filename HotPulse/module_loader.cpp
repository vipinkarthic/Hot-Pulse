#include<cuda.h>
#include"debug.h"
#include<iostream>

void handle_error(CUresult error) {
	if (error) {
		const char error_string[1024] = { 0 };
		const char* error_string_ptr = error_string;
		const char** error_string_ptr_ptr = &error_string_ptr;

		cuGetErrorName(error, error_string_ptr_ptr);
		
		DBG::print_array(error_string_ptr, 1024, "CUDA error");
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


}
