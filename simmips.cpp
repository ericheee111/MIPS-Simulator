#include "mips/cli.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#ifdef MIPS_WITH_GUI
#include "virtual_machine_gui.hpp"
#include <QApplication>
#endif
int main(int argc, char* argv[]) {
    try {
        const auto options = mips::parseOptions(std::vector<std::string>(argv + 1, argv + argc));
        if (options.help) {
            std::cout << "Usage: simmips [--gui] FILE.asm\n"
                         "Commands: step, run, break, status, print $REGISTER, print &ADDRESS, quit\n";
            return EXIT_SUCCESS;
        }
        if (options.gui) {
#ifdef MIPS_WITH_GUI
            QApplication application(argc, argv);
            VirtualMachineGUI window;
            window.load(QString::fromStdString(options.filename));
            window.resize(1200, 700);
            window.show();
            return application.exec();
#else
            throw std::runtime_error("GUI not built; configure with -DMIPS_BUILD_GUI=ON");
#endif
        }
        return mips::runCli(mips::loadAssembly(options.filename), std::cin, std::cout, std::cerr);
    } catch (const std::exception& error) {
        const std::string message(error.what());
        std::cerr << (message.compare(0, 6, "Error:") == 0 ? "" : "Error:1: ") << message << '\n';
        return EXIT_FAILURE;
    }
}
