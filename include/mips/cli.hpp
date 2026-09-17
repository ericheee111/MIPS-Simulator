#ifndef MIPS_CLI_HPP
#define MIPS_CLI_HPP
#include "machine.hpp"
#include <iosfwd>
#include <vector>
namespace mips {
struct Options { bool gui = false, help = false, version = false, smokeTest = false; std::string filename; };
Options parseOptions(const std::vector<std::string>& arguments);
Machine loadAssembly(const std::string& filename);
int runCli(Machine machine, std::istream& input, std::ostream& output, std::ostream& errors);
}
#endif
