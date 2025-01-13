#include <benchmark/benchmark.h>

#include "libevent.h"
#include <iostream>

class TestHandler : public event_handler::event_handler
{
public:
    TestHandler()
        : event_handler::event_handler()
    {
    }

    virtual ~TestHandler()
    {
    }

    void do_something()
    {
    }
};

static void benchmark_event_handler(benchmark::State& state)
{
    std::shared_ptr<TestHandler> handler = std::make_shared<TestHandler>();

    for (auto _ : state)
    {
        handler->post_event(&TestHandler::do_something);
        benchmark::ClobberMemory();
    }

    handler->stop();
}

BENCHMARK(benchmark_event_handler);

BENCHMARK_MAIN();