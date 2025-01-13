#include "libevent.h"

#include <unistd.h>
#include <iostream>
#include <string>

class TestHandler : public event_handler::message_handler
{
public:
    TestHandler()
        : event_handler::message_handler()
    {

    }

    virtual ~TestHandler()
    {
        std::cout << "TestHandler destructor" << std::endl;
    }

    void print_message(std::string s)
    {
        std::cout << s << std::endl;
    }

    void do_heavy_work(int ith)
    {
        for (int i = 0; i < 1000; i++)
        {
            // do something
            int val = i & 1;
            (void)(val);
        }

        std::cout << "Done a heavy work ith = " << ith << std::endl;
    }

    void repated_work()
    {
        static std::size_t repeated_times = 0U;
        repeated_times++;
        std::cout << "This is a job for repeated message, " << repeated_times << " times" << std::endl;
    }

    void tick()
    {
        std::cout << "Tick every 1 second..." << std::endl;
        this->post_delayed_message(1000U, &TestHandler::tick);
    }
};

class NotHandler
{
public:
    NotHandler()
    {

    }

    void nothing_to_do()
    {
        std::cout << "Nothing to do" << std::endl;
    }
};

int main()
{
    std::shared_ptr<TestHandler> handler = std::make_shared<TestHandler>();

    // error: static assertion failed: T must be derived from MessageHandler
    // handler->post_message(&NotHandler::nothing_to_do);

    handler->post_message(&TestHandler::print_message, std::string("Hello event handler"));
    handler->post_delayed_message(2000U, &TestHandler::print_message, std::string("The delayed message is printed after 2 seconds"));
    handler->post_repeated_message(5, 1000U, &TestHandler::repated_work);
    handler->post_message(&TestHandler::tick);

    for (int i = 0; i < 10; i++)
    {
        handler->post_message(&TestHandler::do_heavy_work, i);
    }

    while (true)
    {
        pause();
    }

    return 0;
}