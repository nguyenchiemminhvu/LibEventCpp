#include "libevent.h"

#include <unistd.h>
#include <iostream>
#include <string>

class TestHandler
    : public event_handler::event_handler
{
public:
    TestHandler()
        : event_handler::event_handler()
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
        this->post_delayed_event(1000U, &TestHandler::repeat_itself);
    }
};

class TestHandler1
    : public event_handler::event_handler
{
public:
    TestHandler1()
        : event_handler::event_handler()
    {
    }

    virtual ~TestHandler1()
    {
        std::cout << "TestHandler1 destructor" << std::endl;
    }

    void some_method()
    {
        std::cout << "Some method in Handler 1" << std::endl;
    }
};

class TestHandler2
    : public event_handler::event_handler
{
public:
    TestHandler2()
        : event_handler::event_handler()
    {
    }

    virtual ~TestHandler2()
    {
        std::cout << "TestHandler2 destructor" << std::endl;
    }

    void some_method()
    {
        std::cout << "Some method in Handler 2" << std::endl;
    }
};

class TestMultiHandler
{
public:
    TestMultiHandler()
    {
        m_low_prio_looper = std::make_shared<event_handler::event_looper>();
        m_high_prio_looper = std::make_shared<event_handler::event_looper>();

        m_handler1 = std::make_shared<TestHandler1>();
        m_handler1->bind_looper(m_low_prio_looper);

        m_handler2 = std::make_shared<TestHandler2>();
        m_handler2->bind_looper(m_high_prio_looper);
    }

    virtual ~TestMultiHandler()
    {
        std::cout << "TestMultiHandler destructor" << std::endl;
    }

    void send_to_handler_1()
    {
        m_handler1->post_event(&TestHandler1::some_method);
    }

    void send_to_handler_2()
    {
        m_handler2->post_event(&TestHandler2::some_method);
    }

private:
    std::shared_ptr<TestHandler1> m_handler1;
    std::shared_ptr<TestHandler2> m_handler2;
    std::shared_ptr<event_handler::event_looper> m_low_prio_looper;
    std::shared_ptr<event_handler::event_looper> m_high_prio_looper;
};

int main()
{
    std::shared_ptr<TestHandler> handler = std::make_shared<TestHandler>();
    handler->init();

    handler->post_event(&TestHandler::handle_a);
    handler->post_event(&TestHandler::handle_b);

    std::this_thread::sleep_for(std::chrono::seconds(3));
    handler->stop();

    std::shared_ptr<TestMultiHandler> multi_handler = std::make_shared<TestMultiHandler>();
    multi_handler->send_to_handler_1();
    multi_handler->send_to_handler_2();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    multi_handler->send_to_handler_1();
    multi_handler->send_to_handler_2();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    multi_handler->send_to_handler_1();
    multi_handler->send_to_handler_2();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}