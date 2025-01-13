#include "libevent.h"

#include <iostream>
#include <string>

void sample_callback()
{
    std::cout << "Sample one-shot timer callback invoked!" << std::endl;
}

class TimerHandler
{
public:
    void on_timer_event()
    {
        std::cout << "Timer event triggered!" << std::endl;
    }
};

int main()
{
    time_event::timer timer;
    timer.set_duration(1000); // 1 second
    timer.add_callback([]() {
        std::cout << "Timer callback invoked!" << std::endl;
    });
    timer.start(5);

    time_event::timer timer2;
    timer2.set_duration(1000); // 1 seconds
    timer2.add_callback([]() {
        std::cout << "Timer2 callback invoked!" << std::endl;
    });
    timer2.start();

    time_event::timer timer3;
    timer3.set_duration(1000); // 1 seconds
    timer3.add_callback(sample_callback);
    timer3.start(0);

    time_event::timer timer4;
    timer4.set_duration(1000); // 1 seconds
    //set callback to a member function
    TimerHandler handler;
    timer4.add_callback(std::bind(&TimerHandler::on_timer_event, &handler));
    timer4.start(0);

    std::this_thread::sleep_for(std::chrono::seconds(10)); // Sleep for 10 seconds to allow timers to trigger

    return 0;
}