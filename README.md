- [LibEventCpp](#libeventcpp)
  - [Introduction](#introduction)
  - [License](#license)
  - [Supported Compilers](#supported-compilers)
  - [Usage](#usage)
    - [Message Event Handler](#message-event-handler)
    - [Signals And Slots](#signals-and-slots)
    - [Time events](#time-events)
  - [Pros And Cons](#pros-and-cons)
    - [Message Event Handler](#message-event-handler-1)
    - [Signals And Slots](#signals-and-slots-1)
    - [Time Event](#time-event)
  - [Benchmark](#benchmark)
    - [Message Event Handler](#message-event-handler-2)
    - [Signals And Slots](#signals-and-slots-2)
  - [Memory Leak check](#memory-leak-check)
    - [Message Event Handler](#message-event-handler-3)
    - [Signals And Slots](#signals-and-slots-3)
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

**Create a derived event_handler::message_handler class**

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

**Declare the object of derived event_handler::message_handler class**

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

### Time events

The ```time_event::timer``` class provides a thread-safe mechanism for creating and managing timers in a multithreaded environment. It supports ont-shot and periodic timers, with the ability to invoke user-defined callback functions upon expiration.

Checkout the sample message event handler source code [HERE](https://github.com/nguyenchiemminhvu/LibEventCpp/blob/main/sample_timer.cpp).

Note that the ```time_event::timer``` uses ```POSIX``` APIs (```timer_create```, ```timer_settime```, etc...) so it is only supported on platforms that provide these APIs.

**Include necessary headers**

```
#include "libevent.h"
```

**Create timer instance**

```
time_event::timer my_timer;
```

**Set duration**

```
my_timer.set_duration(1000); // 1 second
```

**Adding callbacks**

```
my_timer.add_callback([]() {
    std::cout << "Timer expired!" << std::endl;
});
```

**Start the timer**

```
try
{
  my_timer.start(5); // repeat 5 times
  my_timer.start(0); // one-shot
  my_timer.start(); // repeat forever
}
catch (const std::runtime_error& e)
{
  // It is possible that timer creation is failed.
  // In that case, exception runtime_error is thrown
}
```

**Actively stop the timer**

```
my_timer.stop();
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

### Time Event

**Pros:**

- Flexible timer options: one-time timer, periodic timer, infinite timer.
- Thread-safe
- Support multiple callbacks
- Dynamic duration adjustment

**Cons:**

- Support Unix/Linux only
- Not yet support pause/resume mechanism

## Benchmark

### Message Event Handler

Checkout the benchmark testing source code [HERE](https://github.com/nguyenchiemminhvu/LibEventCpp/blob/main/test_benchmark/benchmark_event_handler.cpp).

```
LibEventCpp/test_benchmark/build$ ./benchmark_event_handler

2025-01-16T23:25:47+07:00
Running ./benchmark_event_handler
Run on (10 X 24 MHz CPU s)
CPU Caches:
  L1 Data 64 KiB
  L1 Instruction 128 KiB
  L2 Unified 4096 KiB (x10)
Load Average: 2.02, 1.85, 1.97
------------------------------------------------------------------
Benchmark                        Time             CPU   Iterations
------------------------------------------------------------------
benchmark_event_handler        321 ns          208 ns      3276463
```

### Signals And Slots

Checkout the benchmark testing source code [HERE](https://github.com/nguyenchiemminhvu/LibEventCpp/blob/main/test_benchmark/benchmark_sigslot.cpp).

```
LibEventCpp/test_benchmark/build$ ./benchmark_sigslot

2025-01-16T23:26:56+07:00
Running ./benchmark_sigslot
Run on (10 X 24 MHz CPU s)
CPU Caches:
  L1 Data 64 KiB
  L1 Instruction 128 KiB
  L2 Unified 4096 KiB (x10)
Load Average: 1.46, 1.71, 1.91
------------------------------------------------------------
Benchmark                  Time             CPU   Iterations
------------------------------------------------------------
benchmark_sigslot        111 ns          111 ns      6251451
```

## Memory Leak check

### Message Event Handler

```
vu.nguyenchiemminh@localhost LibEventCpp % leaks --atExit -- ./sample_event
sample_event(9994) MallocStackLogging: could not tag MSL-related memory as no_footprint, so those pages will be included in process footprint - (null)
sample_event(9994) MallocStackLogging: recording malloc (and VM allocation) stacks using lite mode
Hello event handler
This is a job for repeated message, 1 times
Done a heavy work ith = 0
Done a heavy work ith = 1
Done a heavy work ith = 2
Done a heavy work ith = 3
Done a heavy work ith = 4
Done a heavy work ith = 5
Done a heavy work ith = 6
Done a heavy work ith = 7
Done a heavy work ith = 8
Done a heavy work ith = 9
This is a job for repeated message, 2 times
The delayed message is printed after 2 seconds
This is a job for repeated message, 3 times
This is a job for repeated message, 4 times
This is a job for repeated message, 5 times
TestHandler destructor
Process 9994 is not debuggable. Due to security restrictions, leaks can only show or save contents of readonly memory of restricted processes.

Process:         sample_event [9994]
Path:            /Users/USER/*/sample_event
Load Address:    0x1003a0000
Identifier:      sample_event
Version:         0
Code Type:       ARM64
Platform:        macOS
Parent Process:  leaks [9993]

Date/Time:       2025-01-16 23:34:59.714 +0700
Launch Time:     2025-01-16 23:34:54.206 +0700
OS Version:      macOS 15.2 (24C101)
Report Version:  7
Analysis Tool:   /usr/bin/leaks

Physical footprint:         2641K
Physical footprint (peak):  2673K
Idle exit:                  untracked
----

leaks Report Version: 4.0, multi-line stacks
Process 9994: 186 nodes malloced for 14 KB
Process 9994: 0 leaks for 0 total leaked bytes.
```

=> No memory leak issue detected by Leaks tool.

### Signals And Slots

```
vu.nguyenchiemminh@localhost LibEventCpp % leaks --atExit -- ./sample_sigslot
sample_sigslot(10028) MallocStackLogging: could not tag MSL-related memory as no_footprint, so those pages will be included in process footprint - (null)
sample_sigslot(10028) MallocStackLogging: recording malloc (and VM allocation) stacks using lite mode
Received boardcast message: This message is broadcasted by global_broadcast signal
The signal connected to this lambda is activated
test
Received boardcast message: test
event
Received boardcast message: event
cpp
Received boardcast message: cpp
disconnecting
Received boardcast message: disconnecting
quit
Received boardcast message: quit
Process 10028 is not debuggable. Due to security restrictions, leaks can only show or save contents of readonly memory of restricted processes.

Process:         sample_sigslot [10028]
Path:            /Users/USER/*/sample_sigslot
Load Address:    0x104478000
Identifier:      sample_sigslot
Version:         0
Code Type:       ARM64
Platform:        macOS
Parent Process:  leaks [10027]

Date/Time:       2025-01-16 23:36:07.603 +0700
Launch Time:     2025-01-16 23:35:50.767 +0700
OS Version:      macOS 15.2 (24C101)
Report Version:  7
Analysis Tool:   /usr/bin/leaks

Physical footprint:         2577K
Physical footprint (peak):  2577K
Idle exit:                  untracked
----

leaks Report Version: 4.0, multi-line stacks
Process 10028: 187 nodes malloced for 19 KB
Process 10028: 0 leaks for 0 total leaked bytes.
```

=> No memory leak issue detected by Leaks tool.

## References

[https://github.com/endurodave/StdWorkerThread](https://github.com/endurodave/StdWorkerThread)

[https://stackoverflow.com/questions/9711414/what-is-the-proper-way-of-doing-event-handling-in-c](https://stackoverflow.com/questions/9711414/what-is-the-proper-way-of-doing-event-handling-in-c)

[https://gameprogrammingpatterns.com/event-queue.html](https://gameprogrammingpatterns.com/event-queue.html)

[https://github.com/wqking/eventpp](https://github.com/wqking/eventpp)

[https://sigslot.sourceforge.net/](https://sigslot.sourceforge.net/)
