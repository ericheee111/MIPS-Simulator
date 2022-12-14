#ifndef THREAD_HPP
#define THREAD_HPP

#include <thread>
#include <iostream>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "parser.hpp"
#include <functional>

enum class Message{
    step, run, _break_, quit
};

// for cout synchronization
static std::mutex print_mutex;

template<typename MessageType>
class message_queue
{
 public:
  
  void push(MessageType const& message)
  {
    std::unique_lock<std::mutex> lock(the_mutex);
    the_queue.push(message);
    lock.unlock();
    the_condition_variable.notify_one();
  }

  bool empty() const
  {
    std::lock_guard<std::mutex> lock(the_mutex);
    return the_queue.empty();
  }

  bool try_pop(MessageType& popped_value)
  {
    std::lock_guard<std::mutex> lock(the_mutex);
    if(the_queue.empty())
      {
	      return false;
      }
        
    popped_value=the_queue.front();
    the_queue.pop();
    return true;
  }

  void wait_and_pop(MessageType& popped_value)
  {
    std::unique_lock<std::mutex> lock(the_mutex);
    while(the_queue.empty())
      {
	the_condition_variable.wait(lock);
      }
        
    popped_value=the_queue.front();
    the_queue.pop();
  }

 private:
  std::queue<MessageType> the_queue;
  mutable std::mutex the_mutex;
  std::condition_variable the_condition_variable;

};

class Worker
{
public:
    Worker() = default;
    Worker(message_queue<Message> *mqptr)
    {
    m_mq = mqptr;
    };
    void writeVM(VirtualMachine v);
    void operator()();
    VirtualMachine getVM();
    void con(message_queue<Message>* mqptr);

private:
  
    message_queue<Message> * m_mq;
    VirtualMachine VM;
};

























#endif