// Comparable against the original public Parse/VirtualMachine interface.
#include "lexer.hpp"
#include "parser.hpp"
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::invalid_argument("usage: mips_benchmark INSTRUCTIONS (1..100000)");
        const std::string argument(argv[1]);
        if (argument.empty() || argument.find_first_not_of("0123456789") != std::string::npos)
            throw std::invalid_argument("invalid instruction count");
        const unsigned long count = std::stoul(argument);
        if (count == 0 || count > 100000) throw std::invalid_argument("count must be 1..100000");
        std::string input = ".data\n.text\nmain:\n";
        for (unsigned long i=0;i<count-1;++i) input += "li $t0, 1\n";
        input += "j main\n";
        const auto start=std::chrono::steady_clock::now();
        std::istringstream source(input);
        Parse parser;
        if (!parser.parse(tokenize(source))) throw std::runtime_error("parse failed");
        auto machine=parser.getVM();
        const auto parsed=std::chrono::steady_clock::now();
        const unsigned iterations=1000000;
        for (unsigned i=0;i<iterations;++i) machine.simulation();
        const auto finished=std::chrono::steady_clock::now();
        if (machine.getStatus()!=VM_Status::Simulating || machine.readPC()!=iterations%count || machine.readReg(8)!=(count>1 ? 1U : 0U))
            throw std::runtime_error("execution result mismatch");
        std::cout << std::fixed << std::setprecision(6)
                  << "{\"instructions\":" << count << ",\"steps\":" << iterations
                  << ",\"parse_seconds\":" << std::chrono::duration<double>(parsed-start).count()
                  << ",\"execution_seconds\":" << std::chrono::duration<double>(finished-parsed).count() << "}\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
}
