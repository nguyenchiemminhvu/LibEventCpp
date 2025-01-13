#include "libevent.h"

#include <unistd.h>
#include <iostream>
#include <string>
#include <thread>

int64_t get_current_time_ms()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

int main()
{
    toggle_event::toggle_event toggle;

    std::thread toggle_thread([&toggle]() {
        while (true)
        {
            bool wait_res = toggle.wait_for(1000U);

            if (wait_res)
            {
                toggle.reset();
            }
            else
            {
                int64_t cur_time = get_current_time_ms();
                std::cout << "Timeout waiting for toggle event, auto trigger at " << cur_time << " ms" << std::endl;
            }
        }
    });

    while (true)
    {
        // random 980-1020 ms sleep
        int sleep_duration = 980 + rand() % 40;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration));
        std::cout << "Sleeped for " << sleep_duration << " ms" << std::endl;
        toggle.trigger_if_not_set([]() {
            int64_t cur_time = get_current_time_ms();
            std::cout << "Toggle event triggered at " << cur_time << " ms" << std::endl;
        });
    }

    if (toggle_thread.joinable())
    {
        toggle_thread.join();
    }

    return 0;
}