#include "thread.hpp"



void Worker::operator()()
{
    //bool running = false;
    while (true)
    {
        Message m;
        m_mq->wait_and_pop(m);
        std::lock_guard<std::mutex> guard(print_mutex);
        //int n = VM.getInstrVector().size();

        if (m == Message::run) {
            //std::cout << "running" << std::endl;
            VM.simulation();
            //running = true;
            m_mq->push(Message::run);
        }
        else if (m == Message::step) {
            //std::cout << "pc: " << VM.readPC() << std::endl;
            VM.simulation();
            //std::cout << "stepped: " << VM.readPC() << std::endl;

        }
        else if (m == Message::_break_) {
            //std::cout << "breaked" << std::endl;
            //running = false;
            if (m_mq->try_pop(m)) {
                m = Message::_break_;
            }
            
        }
        else if (m == Message::quit) break;

        /*while (running == true && n > 0) {
            VM.simulation();
            n--;
        }*/
        
    }
};

void Worker::writeVM(VirtualMachine v) {
    VM = v;
}

VirtualMachine Worker::getVM() {
    return VM;
}

void Worker::con(message_queue<Message>* mqptr) {
    m_mq = mqptr;
}