#include "mips/cli.hpp"
#include "mips/controller.hpp"
#include "mips/numbers.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>
namespace mips {
Options parseOptions(const std::vector<std::string>& arguments) {
    Options options;
    bool positional = false;
    for (const auto& argument : arguments) {
        if (!positional && argument == "--") { positional = true; continue; }
        if (!positional && (argument == "--help" || argument == "-h")) options.help = true;
        else if (!positional && argument == "--gui") {
            if (options.gui) throw std::invalid_argument("duplicate --gui option");
            options.gui = true;
        } else if (!positional && !argument.empty() && argument[0] == '-')
            throw std::invalid_argument("unknown option: " + argument);
        else if (argument.empty() || !options.filename.empty())
            throw std::invalid_argument("expected exactly one assembly filename");
        else options.filename = argument;
    }
    if (!options.help && options.filename.empty()) throw std::invalid_argument("missing assembly filename");
    return options;
}
Machine loadAssembly(const std::string& filename) {
    std::ifstream source(filename, std::ios::binary);
    if (!source) throw std::runtime_error("Error:1: cannot open assembly file: " + filename);
    Parse parser;
    if (!parser.parse(tokenize(source))) throw std::runtime_error(parser.error());
    auto machine = parser.getVM();
    if (machine.getStatus() == Status::Error) throw std::runtime_error(machine.error());
    return machine;
}
int runCli(Machine machine, std::istream& input, std::ostream& output, std::ostream& errors) {
    ExecutionController controller(std::move(machine));
    std::string line;
    while (output << "simmips> " << std::flush, std::getline(input, line)) {
        std::istringstream words(line);
        std::string command, operand, extra;
        words >> command;
        if (command == "print") {
            if (!(words >> operand) || (words >> extra)) { errors << "Error: invalid print command.\n"; continue; }
            auto state = controller.snapshot().get();
            if (state.running) { errors << "Error: simulation running. Type break to halt.\n"; continue; }
            unsigned index = 0;
            uint32_t value = 0;
            if (parseRegister(operand, index)) output << hexValue(state.machine.readReg(index)) << '\n';
            else if (operand == "$pc") output << hexValue(state.machine.readPC()) << '\n';
            else if (operand == "$hi") output << hexValue(state.machine.readHI()) << '\n';
            else if (operand == "$lo") output << hexValue(state.machine.readLO()) << '\n';
            else if (operand[0] == '&' && parseAddress(operand.substr(1), value)) {
                try { output << hexValue(state.machine.readMEM(value, 1), 2) << '\n'; }
                catch (const std::out_of_range&) { errors << "Error: memory address out of bounds.\n"; }
            } else errors << "Error: invalid register or address.\n";
        } else if (words >> extra) errors << "Error: unknown command.\n";
        else if (command == "quit") return EXIT_SUCCESS;
        else if (command == "step") {
            auto state = controller.step().get();
            if (state.accepted) output << hexValue(state.machine.readPC()) << '\n';
            else errors << state.message << '\n';
        } else if (command == "run") {
            auto state = controller.run().get();
            if (!state.accepted) errors << state.message << '\n';
        } else if (command == "break") controller.pause().get();
        else if (command == "status") {
            auto state = controller.snapshot().get();
            if (state.machine.getStatus() == Status::Error) errors << state.machine.error() << '\n';
        } else errors << "Error: unknown command.\n";
    }
    // EOF and exceptions use the same stop-and-join path as an explicit quit.
    return input.bad() ? EXIT_FAILURE : EXIT_SUCCESS;
}
}
