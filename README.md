- [LibEventCpp](#libeventcpp)
  - [Introduction](#introduction)
  - [License](#license)
  - [Supported Compilers](#supported-compilers)
  - [Usage](#usage)
    - [Message Event Handler](#message-event-handler)
    - [Signals And Slots](#signals-and-slots)
  - [Benchmark](#benchmark)
    - [Message Event Handler](#message-event-handler-1)
    - [Signals And Slots](#signals-and-slots-1)
  - [Pros And Cons](#pros-and-cons)
    - [Message Event Handler](#message-event-handler-2)
    - [Signals And Slots](#signals-and-slots-2)
  - [References](#references)

# LibEventCpp

## Introduction

```LibEventCpp``` is a lightweight and portable ```C++14``` library designed for handling events efficiently. It is implemented in a single header file, making it easy to integrate into projects. The library supports an unlimited number of arguments for event handlers, providing flexibility in event management. ```LibEventCpp``` is designed to be simple to use while offering powerful features for event-driven programming.

## License

MIT License

## Supported Compilers

```GCC (GNU Compiler Collection)```: Version 5.0 and later.

```Clang```: Version 3.4 and later.

```MSVC (Microsoft Visual C++)```: Visual Studio 2015 and later.

```Intel C++ Compiler```: Version 15.0 and later.

To ensure compatibility, you should use the appropriate compiler flags to enable C++14. For example:

**GCC/Clang**: ```-std=c++14```

**MSVC**: ```/std:c++14```

## Usage

### Message Event Handler

To use ```LibEventCpp``` to handle event messages, you can create an object of derived class of ```event_handler::message_handler``` that contains the member functions, which are considered the reaction for the event messages. Internally, ```event_handler::message_handler``` uses a ```event_handler::message_looper``` to continously and asynchronously poll and execute messages from a ```event_handler::message_queue```.

Checkout the sample message event handler source code [HERE](https://github.com/nguyenchiemminhvu/LibEventCpp/blob/main/sample_event.cpp).

**Include necessary headers**

```
#include "libevent.h"
```

**Create a derived LibEvent::MessageHandler class**

```
class TestHandler : public event_handler::message_handler
{
public:
    TestHandler()
        : event_handler::message_handler()
    {

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
```

Here, we created a ```TestHandler``` class that inherits from ```event_handler::message_handler```. We defined several methods (```print_message```, ```do_heavy_work```, ```repated_work```, and ```tick```) to handle different types of events.

**Declare the object of derived LibEvent::MessageHandler class**

```
std::shared_ptr<TestHandler> handler = std::make_shared<TestHandler>();
```

**Start posting event messages**

From this moment, you can post messages to the handler. These messages will be processed asynchronously:

```
// Post a delayed message
handler->post_delayed_message(2000U, &TestHandler::print_message, std::string("The delayed message is printed after 2 seconds"));

// Post a repated message (number of repeating times and duration (ms) between each couple)
handler->post_repeated_message(5, 1000U, &TestHandler::repated_work);

// Post a regular message
handler->post_message(&TestHandler::print_message, std::string("Hello event handler"));

// Call a member function of the handler that trigger an internal event
handler->post_message(&TestHandler::tick);

// Post multiple heavy work messages
for (int i = 0; i < 10; i++)
{
    handler->post_message(&TestHandler::do_heavy_work, i);
}
```

### Signals And Slots

To use ```LibEventCpp``` to create ```signal-slot``` connections just as an alternative of [Qt framework](https://doc.qt.io/qt-6/signalsandslots.html), follow these steps:

**Include neccessary headers**

```
#include "libevent.h"
```

**Declare the signal**

```
class Sender
{
public:
    sigslot::signal<std::string> message_notification;

    void boardcast_message(std::string mess)
    {
        message_notification(mess);
    }
};
```

The ```sigslot::signal``` variable can be declared as member or outside of a class.

**Define a class with slot functions**

```
class Listener : public sigslot::base_slot
{
public:
    void on_boardcast_received(std::string mess)
    {
        std::cout << "Received boardcast message: " << mess << std::endl;
    }
};
```

The created class must inherits from ```sigslot::base_slot```.

**Create instances of the classes**

```
Sender sender;
Listener listener;
```

**Connect signal to slot**

```
sender.message_notification.connect(&listener, &Listener::on_boardcast_received);
```

Or using ```sigslot::connect``` function:

```
sigslot::signal<std::string> global_broadcast;
sigslot::connect(global_broadcast, &listener, &Listener::on_boardcast_received);
```

**Emit the signal**

```
sender.boardcast_message("Hello from Sender");
```

**Close the connection when needed**

```
sender.message_notification.disconnect(&listener);
```

or

```
sender.message_notification.disconnect_all();
```

**The connection can also be closed from Listener side**

```
listener.disconnect(&sender);
```

or

```
listener.disconnect_all();
```

**Advanced signals and slots usage:**

Sometimes, it is helpful to connect a signal to a lambda function or a callable object

```
sigslot::signal<> test_sig_lambda;
test_sig_lambda.connect([]() {
    std::cout << "The signal connected to this lambda is activated" << std::endl;
});
test_sig_lambda();
test_sig_lambda.disconnect_all_callable();
```

Checkout the sample signal and slot source code [HERE](https://github.com/nguyenchiemminhvu/LibEventCpp/blob/main/sample_sigslot.cpp).

## Benchmark

### Message Event Handler

Checkout the benchmark testing source code [HERE](https://github.com/nguyenchiemminhvu/LibEventCpp/blob/main/test_benchmark/benchmark_event_handler.cpp).

```
LibEventCpp/test_benchmark/build$ ./benchmark_event_handler

2025-01-16T18:26:00+09:00
Running ./benchmark_event_handler
Run on (64 X 3300.04 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x32)
  L1 Instruction 32 KiB (x32)
  L2 Unified 1280 KiB (x32)
  L3 Unified 24576 KiB (x2)
Load Average: 9.65, 11.09, 12.99
------------------------------------------------------------------
Benchmark                        Time             CPU   Iterations
------------------------------------------------------------------
benchmark_event_handler        690 ns          635 ns      1130390
```

### Signals And Slots

Checkout the benchmark testing source code [HERE](https://github.com/nguyenchiemminhvu/LibEventCpp/blob/main/test_benchmark/benchmark_sigslot.cpp).

```
LibEventCpp/test_benchmark/build$ ./benchmark_sigslot

2025-01-16T18:30:02+09:00
Running ./benchmark_sigslot
Run on (64 X 3300 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x32)
  L1 Instruction 32 KiB (x32)
  L2 Unified 1280 KiB (x32)
  L3 Unified 24576 KiB (x2)
Load Average: 8.60, 9.72, 11.99
------------------------------------------------------------
Benchmark                  Time             CPU   Iterations
------------------------------------------------------------
benchmark_sigslot        165 ns          165 ns      4229918
```

## Pros And Cons

Each method of event handling/processing has its own advantages and disadvantages. It is important to carefully evaluate the specific requirements and constraints of your application before choosing the appropriate technique (such as performance, complexity, flexibility, scalability...).

### Message Event Handler

**Pros:**

- Messages are processed asynchronously, which helps in managing tasks without blocking the main thread, useful for heavy task or periodic events.
- The ```message_handler``` acts as a centralized handler for all event messages, easy to manage and organize source codes.
- Flexible with different types of events (instant, delayed, or repeated messages).
- A handler function can encapsulate a specific logic and be reused across different parts of the application.

**Cons:**

- Messages must be enqueued, dequeued, and then processed, which can introduce a small latency compared to direct execution.
- Debugging asynchronous messages can be challenging, especially race condition happens.
- A message can be handled by only one handler function.

### Signals And Slots

**Pros:**

- Provide direct and immediate communication between objects.
- Connecting signals to slots is straightforward, easy to use and understand.
- A signal can be connected to multiple slots, useful for broadcasting of events.
- Connection can be easily closed by sender or receiver, provide flexibility in connection management.

**Cons:**

- Synchronous execution can blocked main thread if the slots are time-consuming.
- Does not support different types of triggering events like event handler (instant, delayed, or repeated).
- When the connection graph becomes complex, debugging can be challenging.

## References

[https://github.com/endurodave/StdWorkerThread](https://github.com/endurodave/StdWorkerThread)

[https://stackoverflow.com/questions/9711414/what-is-the-proper-way-of-doing-event-handling-in-c](https://stackoverflow.com/questions/9711414/what-is-the-proper-way-of-doing-event-handling-in-c)

[https://gameprogrammingpatterns.com/event-queue.html](https://gameprogrammingpatterns.com/event-queue.html)

[https://github.com/wqking/eventpp](https://github.com/wqking/eventpp)

[https://sigslot.sourceforge.net/](https://sigslot.sourceforge.net/)
