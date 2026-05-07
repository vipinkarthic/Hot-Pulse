#include<iostream>

namespace DBG {
	template<typename T>
	void print_array(T* iterable, int size, const std::string& statement) {
		std::cout << statement << ": ["	;
		for (int i = 0; i < size; i++) {
			std::cout << iterable[i] << " ";
		}
		std::cout << "]" << std::endl;
	}
}
