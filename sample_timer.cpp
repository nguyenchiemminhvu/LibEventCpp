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
    timer.stop();


    // Demo: stop/resume lifecycle for a timer
    std::cout << "\n--- Stop/Resume demo for timer ---" << std::endl;
    time_event::timer timer_sr;
    timer_sr.set_duration(500); // fires every 500 ms
    timer_sr.add_callback([]() {
        std::cout << "[stop/resume timer] callback fired!" << std::endl;
    });
    timer_sr.start(-1); // run indefinitely

    std::this_thread::sleep_for(std::chrono::milliseconds(1600));
    std::cout << "Stopping timer_sr..." << std::endl;
    timer_sr.stop();
    std::cout << "Timer stopped. No more callbacks for 1.5 seconds." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    std::cout << "Resuming timer_sr..." << std::endl;
    timer_sr.resume();
    std::this_thread::sleep_for(std::chrono::milliseconds(1600));
    timer_sr.stop();
    std::cout << "Timer_sr final stop." << std::endl;

    return 0;
}