#include "libevent.h"
#include <iostream>

class DemoClass
{
public:
    void initialize()
    {
        std::cout << "DemoClass initialized." << std::endl;
        m_once_init.call_once(&DemoClass::init_once, this, 42);
    }

    void init_once(int value)
    {
        std::cout << "DemoClass initialized with value: " << value << std::endl;
    }

    void do_something()
    {
        std::cout << "Doing something..." << std::endl;
    }
private:
    once_event::once_per_life m_once_init;
};

void demonstrate_once_per_life()
{
    DemoClass demo_instance;
    demo_instance.initialize();
    demo_instance.initialize(); // This call should not re-initialize

    once_event::once_per_life global_once;
    global_once.call_once([]() {
        std::cout << "Global initialization done." << std::endl;
    });
    global_once.call_once([]() {
        std::cout << "Global initialization done." << std::endl;
    });
}

void demonstrate_once_per_n_times()
{
    once_event::once_per_n_times once_every_3(3);

    DemoClass demo;
    once_event::once_per_n_times once_every_5(5);

    for (int i = 1; i <= 10; ++i)
    {
        once_every_3.call_if_due(&DemoClass::do_something, &demo);
        once_every_5.call_if_due([](int iteration) {
            std::cout << "Action executed at iteration: " << iteration << std::endl;
        }, i);
    }
}

void demonstrate_once_per_value()
{
    once_event::once_per_value<int> once_per_value_checker;

    for (int val : {1, 2, 2, 3, 1, 4, 4, 5})
    {
        bool ret = once_per_value_checker.on_value_change(val, [](int v) {
            std::cout << "New value encountered: " << v << std::endl;
        }, val);

        if (!ret)
        {
            std::cout << "Value " << val << " has already been processed." << std::endl;
        }
    }
}

void demonstrate_once_per_interval()
{
    once_event::once_per_interval once_per_1s(1000U); // 1 second interval
    for (int i = 0; i < 5; ++i)
    {
        bool ret = once_per_1s.call([]() {
            std::cout << "Action executed at interval." << std::endl;
        });

        if (!ret)
        {
            std::cout << "Action skipped due to interval constraint." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Sleep for 500ms
    }
}

void demonstrate_once_at_least()
{
    once_event::once_at_least<std::string> data_stream(
        1000U, // 1 second interval
        [](const std::string& data) {
            std::cout << "Processing data: " << data << std::endl;
        }
    );

    for (int i = 0; i < 5; ++i)
    {
        data_stream.update("Data packet " + std::to_string(i));
        
        // sleep 1s with jiter randomly +/- 20ms
        auto sleep_duration = 1000 + (rand() % 41 - 20);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration));
        std::cout << "Slept for " << sleep_duration << " ms" << std::endl;
    }

    data_stream.stop();
}

int main()
{
    demonstrate_once_per_life();
    demonstrate_once_per_n_times();
    demonstrate_once_per_value();
    demonstrate_once_per_interval();
    demonstrate_once_at_least();

    return 0;
}