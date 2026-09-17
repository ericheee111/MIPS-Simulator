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
        else if (!positional && argument == "--version") options.version = true;
        else if (!positional && argument == "--smoke-test") options.smokeTest = true;
        else if (!positional && argument == "--gui") {
            if (options.gui) throw std::invalid_argument("duplicate --gui option");
            options.gui = true;
        } else if (!positional && !argument.empty() && argument[0] == '-')
            throw std::invalid_argument("unknown option: " + argument);
        else if (argument.empty() || !options.filename.empty())
            throw std::invalid_argument("expected exactly one assembly filename");
        else options.filename = argument;
    }
    if (!options.help && !options.version && options.filename.empty()) throw std::invalid_argument("missing assembly filename");
    if (options.smokeTest && !options.gui) throw std::invalid_argument("--smoke-test requires --gui");
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
            uint32_t address = 0;
            const bool memory = operand[0] == '&' && parseAddress(operand.substr(1), address);
            auto reply = controller.request(CommandKind::Observe, memory ? MemoryWindow(address,1) : MemoryWindow()).get();
            if (reply.running) { errors << "Error: simulation running. Type break to halt.\n"; continue; }
            if (!reply.accepted) { errors << "Error: memory address out of bounds.\n"; continue; }
            const auto& state = reply.state;
            unsigned index = 0;
            if (parseRegister(operand, index)) output << hexValue(state.registers[index]) << '\n';
            else if (operand == "$pc") output << hexValue(state.pc) << '\n';
            else if (operand == "$hi") output << hexValue(state.hi) << '\n';
            else if (operand == "$lo") output << hexValue(state.lo) << '\n';
            else if (memory) output << hexValue(state.memory.at(0),2) << '\n';
            else errors << "Error: invalid register or address.\n";
        } else if (command == "until") {
            uint32_t target = 0, budget = 1000000;
            std::string limit;
            if (!(words >> operand)) { errors << "Error: until requires a label or instruction index.\n"; continue; }
            if ((words >> limit) && (!parseAddress(limit,budget) || (words >> extra))) {
                errors << "Error: invalid until instruction budget.\n"; continue;
            }
            auto state = controller.request(CommandKind::Observe).get();
            const auto program = state.state.program;
            if (program && program->textLabels.count(operand)) target = static_cast<uint32_t>(program->textLabels.at(operand));
            else if (!parseAddress(operand,target)) { errors << "Error: unknown target label.\n"; continue; }
            state = controller.request(CommandKind::RunUntil, MemoryWindow(),target,budget).get();
            if (!state.accepted) { errors << state.message << '\n'; continue; }
            // Command acknowledgements, not sleeps, order observation. The bounded
            // execution budget and query fairness guarantee an unreachable target stops.
            while (state.running) {
                std::this_thread::yield();
                state = controller.request(CommandKind::Observe).get();
            }
            if (state.state.status == Status::Error) errors << state.state.error << '\n';
            else output << (state.reason == StopReason::StepLimit ? "Budget reached at " : "Target reached at ")
                        << hexValue(state.state.pc) << '\n';
        } else if (words >> extra) errors << "Error: unknown command.\n";
        else if (command == "quit") return EXIT_SUCCESS;
        else if (command == "step") {
            auto state = controller.request(CommandKind::Step).get();
            if (state.accepted) output << hexValue(state.state.pc) << '\n';
            else errors << state.message << '\n';
        } else if (command == "run" || command == "reset") {
            auto state = controller.request(command == "run" ? CommandKind::Run : CommandKind::Reset).get();
            if (!state.accepted) errors << state.message << '\n';
        } else if (command == "break") controller.request(CommandKind::Pause).get();
        else if (command == "status") {
            auto state = controller.request(CommandKind::Observe).get();
            if (state.state.status == Status::Error) errors << state.state.error << '\n';
            else if (state.reason == StopReason::StepLimit) output << "Instruction budget reached.\n";
        } else errors << "Error: unknown command.\n";
    }
    return input.bad() ? EXIT_FAILURE : EXIT_SUCCESS;
}
}
