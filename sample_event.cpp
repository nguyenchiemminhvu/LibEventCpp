#include "libevent.h"

#include <unistd.h>
#include <iostream>
#include <string>

class TestHandler
    : public event_handler::message_handler
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

    void init()
    {
        std::cout << "TestHandler init" << std::endl;
        repeat_itself();
    }

    void handle_a()
    {
        std::cout << "Handle A" << std::endl;
    }

    void handle_b()
    {
        std::cout << "Handle B" << std::endl;
    }
    
    void repeat_itself()
    {
        std::cout << "Repeat itself" << std::endl;
        this->post_delayed_message(1000U, &TestHandler::repeat_itself);
    }
};

int main()
{
    std::shared_ptr<TestHandler> handler = std::make_shared<TestHandler>();
    handler->init();

    handler->post_message(&TestHandler::handle_a);
    handler->post_message(&TestHandler::handle_b);

    std::this_thread::sleep_for(std::chrono::seconds(3));
    handler->stop();

    return 0;
}