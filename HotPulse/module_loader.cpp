#include <cuda.h>

#include <iostream>
#include <string>
#include <vector>

#include "debug.h"
#include "input_reader.h"
#include "ptx_lexer.h"
#include "ptx_parser.h"

void handle_error(CUresult error) {
  if (error) {
    char error_string[1024] = {0};
    char const* error_string_ptr = nullptr;
    cuGetErrorName(error, &error_string_ptr);

    if (error_string_ptr) {
      DBG::print_char_array(error_string_ptr, 1024, "CUDA error");
    }
  }
}

int main() {
  handle_error(cuInit(0));

  CUdevice device;
  handle_error(cuDeviceGet(&device, 0));

  CUcontext context;
  handle_error(cuCtxCreate(&context, nullptr, 0, device));

  CUmodule module;
  handle_error(cuModuleLoad(&module, "main.ptx"));

  // retrieve all function names from the ptx
  std::vector<std::string> function_names;
  read_ptx(function_names);

  std::vector<CUfunction> kernels;
  for (std::string const& function_name : function_names) {
    CUfunction kernel;
    handle_error(cuModuleGetFunction(&kernel, module, function_name.c_str()));
    kernels.push_back(kernel);
  }

  DBG::print_iterable(function_names, "names");
  std::string const path = "D:\\Coding\\GameDev\\HotPulse\\HotPulse\\main.ptx";
  LEXER::print_tokens_from_file(path);
  PARSER::ParseResult result = PARSER::parse_file(path);
  PARSER::print_parse_summary(result);
}
