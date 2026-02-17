// Simple syntax test - compile this to check for basic errors
#include <iostream>
#include <vector>
#include <complex>
#include <cstdint>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <cstring>

// Test includes work
int main() {
    std::cout << "Syntax test passed - all headers included successfully\n";
    
    // Test basic types
    std::vector<int16_t> testVector = {1, 2, 3};
    std::complex<float> testComplex(1.0f, 2.0f);
    
    std::cout << "Vector size: " << testVector.size() << std::endl;
    std::cout << "Complex number: " << testComplex.real() << " + " << testComplex.imag() << "i" << std::endl;
    
    return 0;
}
