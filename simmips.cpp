// implement entry point for simmips here
#include <fstream>
#include <sstream>
#include "lexer.hpp"
#include "parser.hpp"
#include "virtual_machine_gui.hpp"
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <QApplication>
#include "thread.hpp"


int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Error:1: Read file failed." << std::endl;
        return EXIT_FAILURE;
    }
    bool gui = false;
    std::ifstream source;
    std::string filename;
    if (argc == 3 && argv[1] == "--gui") {
        gui = true;
        std::string filename(argv[2]);
        source.open(filename);
    }
    else if (argc == 3 && argv[2] == "--gui") {
        gui = true;
        std::string filename(argv[1]);
        source.open(filename);
    }
    else {
        filename = (argv[1]);
        source.open(filename);
    }
    

    if (source.is_open() == false) {
        std::cerr << "Error : can't open file!" << std::endl;
        return EXIT_FAILURE;
    }

    TokenList tl = tokenize(source);

    Parse syntax;
    bool parsing = syntax.parse(tl);
    auto line = syntax.getLine();
    auto mline = syntax.getMainLine();
    if (parsing == false) {
        std::cerr << "Error:" << line << ": Syntax error occurred" << std::endl;
        return EXIT_FAILURE;
    }

    VirtualMachine VM = syntax.getVM();

    if (gui == true) {
        QApplication app(argc, argv);
        VirtualMachineGUI VMG;
        //VMG.writeVM(VM);
        VMG.load(QString::fromStdString(filename));
        VMG.resize(1200, 700);
        VMG.show();

        return app.exec();
    }
    else {
        message_queue<Message> mqueue;
        Worker w1(&mqueue);
        w1.writeVM(VM);
        std::thread sim_th(std::ref(w1));

        std::cout << "simmips> ";
        std::string input = "";
        mqueue.push(Message::step);
        while (std::getline(std::cin, input)) {
            if (input == "step") {
                //VM.simulation();
                mqueue.push(Message::step);
                VM = w1.getVM();
                auto ans = VM.readPC();
                //std::cout << "pc = " << ans << std::endl;
                std::string value = uint32ToHex(ans);

                std::cout << value << std::endl;
            }
            else if (input == "run") {
                mqueue.push(Message::run);
            }
            else if (input == "break") {
                mqueue.push(Message::_break_);
                VM = w1.getVM();
            }
            else if (input == "quit") {
                mqueue.push(Message::quit);
                sim_th.join();
                break;
            } 
            else if (input == "status") {
                auto status = VM.getStatus();
                if (status == VM_Status::Simulating) {
                    std::cout << "" << std::endl;
                }
                else if (status == VM_Status::Error) {
                    std::cerr << "Error ! FAIL TO STEP!" << std::endl;
                }
            }
            else if (input.substr(0, 5) == "print") {
                if (input[6] == '$') {
                    auto target = input.substr(6, input.size() - 1);
                    if (isRegister(target)) {
                        auto s = target.substr(1, target.size() - 1);
                        int regNum = 0;
                        if (isNum(s) == true) {
                            regNum = stoi(s);
                        }
                        else if (isAlias(s) == true) {
                            regNum = regAlias[s];
                        }
                        auto ans = VM.readReg(regNum);
                        std::string value = uint32ToHex(ans);

                        std::cout << value << std::endl;
                    }
                    else if (target == "$pc" || target == "$hi" || target == "$lo") {
                        uint32_t ans = 0;
                        if (target == "$pc") {
                            ans = VM.readPC();
                        }
                        else if (target == "$hi") {
                            ans = VM.readHI();
                        }
                        else
                            ans = VM.readLO();

                        std::string value = uint32ToHex(ans);

                        std::cout << value << std::endl;
                    }
                    else
                        std::cerr << "Error ! Unknown Register. " << std::endl;
                }
                else if (input.substr(6, 3) == "&0x") {
                    auto addr = input.substr(9, input.size() - 1);
                    if (addr.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos) {
                        unsigned int address = std::stoul(addr, nullptr, 16);
                        uint8_t ans = VM.readMEM(address, 1);
                        auto sol = uint8ToHex(&ans);

                        std::cout << sol << std::endl;
                    }
                    else
                        std::cerr << "Error : Invalid Address." << std::endl;
                }
                else
                    std::cerr << "Error : Unknown Command!" << std::endl;
            }
            else
                std::cerr << "Error : Unknown Command!" << std::endl;

            std::cout << "simmips> ";
        }
    }
    
    

    if (VM.getStatus() == VM_Status::Simulating) {
        return EXIT_SUCCESS;
    }
    else if (VM.getStatus() == VM_Status::Error) {
        std::cerr << "Error ! FAIL TO STEP!" << std::endl;
        return EXIT_FAILURE;
    }
}