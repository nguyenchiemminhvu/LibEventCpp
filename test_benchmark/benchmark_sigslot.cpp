#include <benchmark/benchmark.h>

#include "libevent.h"
#include <iostream>

class Sender
{
public:
    sigslot::signal<std::string> message_notification;

    void boardcast_message(std::string mess)
    {
        message_notification(mess);
    }
};

class Listener : public sigslot::base_slot
{
public:
    void on_boardcast_received(std::string mess)
    {
    }
};

static void benchmark_sigslot(benchmark::State& state)
{
    Sender sender;
    Listener listener;

    for (auto _ : state)
    {
        sender.message_notification.connect(&listener, &Listener::on_boardcast_received);
        sender.message_notification(std::string("sample string"));
        sender.message_notification.disconnect_all();
        benchmark::ClobberMemory();
    }
}

BENCHMARK(benchmark_sigslot);

BENCHMARK_MAIN();