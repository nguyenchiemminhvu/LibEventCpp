#include "libevent.h"

#include <unistd.h>
#include <iostream>
#include <string>

class Manager;
class TestHandler
    : public event_handler::message_handler
{
public:
    TestHandler(Manager* pManager)
        : event_handler::message_handler()
    {
        this->pManager = pManager;
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

private:
    Manager* pManager;
};

class Provider : public event_handler::message_handler
{
public:
    Provider(Manager* pManager, std::shared_ptr<TestHandler> handler)
        : event_handler::message_handler()
    {
        this->pManager = pManager;
        this->handler = handler;
        std::cout << "Provider constructor" << std::endl;
    }

    ~Provider()
    {
        std::cout << "Provider destructor" << std::endl;
    }

    void send_a_message()
    {
        std::cout << "Send A message" << std::endl;
        auto handler_ptr = handler.lock();
        if (handler_ptr)
        {
            handler_ptr->post_message(&TestHandler::handle_a);
        }
        else
        {
            std::cout << "Handler is no longer available" << std::endl;
        }
    }

    void send_b_message()
    {
        std::cout << "Send B message" << std::endl;
        auto handler_ptr = handler.lock();
        if (handler_ptr)
        {
            handler_ptr->post_message(&TestHandler::handle_b);
        }
        else
        {
            std::cout << "Handler is no longer available" << std::endl;
        }
    }
private:
    Manager* pManager;
    std::weak_ptr<TestHandler> handler;
};

std::weak_ptr<Provider> g_provider;

class Manager
{
public:
    Manager()
    {
        std::cout << "Manager constructor" << std::endl;
        handler = std::make_shared<TestHandler>(this);
        handler->init();
        provider = std::make_shared<Provider>(this, handler);

        g_provider = provider;
    }

    ~Manager()
    {
        std::cout << "Manager destructor" << std::endl;
    }

private:
    std::shared_ptr<TestHandler> handler;
    std::shared_ptr<Provider> provider;
};

Manager * g_manager = nullptr;

void thread_func()
{
    while (true)
    {
        auto provider_ptr = g_provider.lock();
        if (provider_ptr)
        {
            provider_ptr->send_a_message();
            provider_ptr->send_b_message();
        }

        usleep(1000000U);
    }
}

int main()
{
    g_manager = new Manager();

    std::thread t(thread_func);
    t.detach();
    std::cout << "Thread started" << std::endl;

    while (true)
    {
        pause();
    }

    return 0;
}