#include<vector>
#include<string>
#include<fstream>
#include<cstring>

char const* FP = "main.ptx";

std::string _process_line(char* n_line) {
	char const* ptx_func_entry = ".visible .entry ";
	char const* line = n_line;

	char const* matched_ptr;
	matched_ptr = std::strstr(line, ptx_func_entry);
	
	if (!matched_ptr) return "";
	
	std::string func_name = "";
	for (int i = 16; line[i] != '(' && line[i] != '\0'; i++) func_name += line[i];
	
	return func_name;
}

void read_ptx(std::vector<std::string>& functions) {
	std::ifstream ptx_input_stream;
	ptx_input_stream.open(FP);

	if (!ptx_input_stream.is_open()) return;

	// line by line reading
	while (!(ptx_input_stream.rdstate() & std::ifstream::eofbit)) {
		// process the per line
		char line[1024];
		ptx_input_stream.getline(line, 1024);
		std::string func_name(_process_line(line));
		
		if (!func_name.empty()) {
			functions.push_back(func_name);
		}
	}
}
