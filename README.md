- [LibEventCpp](#libeventcpp)
  - [Introduction](#introduction)
  - [License](#license)
  - [Supported Compilers](#supported-compilers)
  - [Usage](#usage)
    - [Message Event Handler](#message-event-handler)
    - [Signals And Slots](#signals-and-slots)
    - [Time events](#time-events)
    - [Once events](#once-events)
    - [Toggle events](#toggle-events)
    - [File descriptor events](#file-descriptor-events)
    - [Signal events](#signal-events)
    - [File system events](#file-system-events)
  - [Pros And Cons](#pros-and-cons)
    - [Message Event Handler](#message-event-handler-1)
    - [Signals And Slots](#signals-and-slots-1)
    - [Time Event](#time-event)
    - [Once Event](#once-event)
    - [Toggle Event](#toggle-event)
    - [File descriptor Events](#file-descriptor-events-1)
    - [Signal Events](#signal-events-1)
    - [File system Events](#file-system-events-1)
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

To use ```LibEventCpp``` to handle event messages, you can create an object of derived class of ```event_handler::event_handler``` that contains the member functions, which are considered the reaction for the event messages. Internally, ```event_handler::event_handler``` uses a ```event_handler::event_looper``` to continously and asynchronously poll and execute messages from a ```event_handler::event_queue```.

Checkout the sample message event handler source code [HERE](https://github.com/nguyenchiemminhvu/LibEventCpp/blob/main/sample_event.cpp).

**Include necessary headers**

```
#include "libevent.h"
```

**Create a derived event_handler::event_handler class**

```
class TestHandler : public event_handler::event_handler
{
public:
    TestHandler()
        : event_handler::event_handler()
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
        this->post_delayed_event(1000U, &TestHandler::tick);
    }
};
```

Here, we created a ```TestHandler``` class that inherits from ```event_handler::event_handler```. We defined several methods (```print_message```, ```do_heavy_work```, ```repated_work```, and ```tick```) to handle different types of events.

**Declare the object of derived event_handler::event_handler class**

```
std::shared_ptr<TestHandler> handler = std::make_shared<TestHandler>();
```

**Start posting event messages**

From this moment, you can post messages to the handler. These messages will be processed asynchronously:

```
// Post a delayed message
handler->post_delayed_event(2000U, &TestHandler::print_message, std::string("The delayed message is printed after 2 seconds"));

// Post a repated message (number of repeating times and duration (ms) between each couple)
handler->post_repeated_event(5, 1000U, &TestHandler::repated_work);

// Post a regular message
handler->post_event(&TestHandler::print_message, std::string("Hello event handler"));

// Call a member function of the handler that trigger an internal event
handler->post_event(&TestHandler::tick);

// Post multiple heavy work messages
for (int i = 0; i < 10; i++)
{
    handler->post_event(&TestHandler::do_heavy_work, i);
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

### Once events

The `once_event` namespace provides several classes for controlling function execution based on various conditions. These classes help prevent duplicate operations and control when code should run.

Checkout the sample once event source code [HERE](https://github.com/nguyenchiemminhvu/LibEventCpp/blob/main/sample_once_event.cpp).

**Include necessary headers**

```cpp
#include "libevent.h"
```

**once_per_life**

Ensures a function is called only once during the lifetime of the object, similar to `std::call_once`.

```cpp
once_event::once_per_life init_flag;

// This will be called only once, no matter how many times you try
init_flag.call_once([]() {
    std::cout << "Initialize resources" << std::endl;
});

// Subsequent calls are ignored
init_flag.call_once([]() {
    std::cout << "This will not print" << std::endl;
});
```

**once_per_n_times**

Executes a function once every N calls.

```cpp
once_event::once_per_n_times every_third(3);

for (int i = 1; i <= 10; ++i) {
    // This callback is executed on calls 3, 6, 9
    every_third.call_if_due([]() {
        std::cout << "Executed!" << std::endl;
    });
}

// Reset the counter
every_third.reset();
```

**once_per_value**

Executes a function only when a value changes.

```cpp
once_event::once_per_value<int> value_monitor;

value_monitor.on_value_change(5, [](int val) {
    std::cout << "New value: " << val << std::endl;
}); // Prints: New value: 5

value_monitor.on_value_change(5, [](int val) {
    std::cout << "New value: " << val << std::endl;
}); // Does nothing (value unchanged)

value_monitor.on_value_change(10, [](int val) {
    std::cout << "New value: " << val << std::endl;
}); // Prints: New value: 10

// Reset to monitor from scratch
value_monitor.reset();
```

**once_per_interval**

Executes a function at most once per time interval.

```cpp
once_event::once_per_interval rate_limiter(1000); // 1 second interval

for (int i = 0; i < 10; ++i) {
    bool executed = rate_limiter.call([]() {
        std::cout << "Rate-limited action" << std::endl;
    });

    if (!executed) {
        std::cout << "Skipped due to rate limit" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
}
```

**once_at_least**

Ensures a callback is invoked at least once per interval with the latest data, even if no new data arrives. Useful for guaranteeing periodic updates.

```cpp
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
```

### Toggle events

The `toggle_event` namespace provides a synchronization mechanism that triggers a callback only once when a condition becomes true, and can be reset when the condition becomes false. This is useful for state-based event handling.

Checkout the sample toggle event source code [HERE](https://github.com/nguyenchiemminhvu/LibEventCpp/blob/main/sample_toggle_event.cpp).

**Include necessary headers**

```cpp
#include "libevent.h"
```

**Basic usage**

```cpp
toggle_event::toggle_event toggle;

// Trigger callback only on first call
toggle.trigger_if_not_set([]() {
    std::cout << "Event triggered!" << std::endl;
}); // Prints: Event triggered!

// Subsequent calls are ignored
toggle.trigger_if_not_set([]() {
    std::cout << "This won't print" << std::endl;
}); // Does nothing

// Reset to allow next trigger
toggle.reset();

toggle.trigger_if_not_set([]() {
    std::cout << "Triggered again!" << std::endl;
}); // Prints: Triggered again!
```

**State management**

```cpp
toggle_event::toggle_event connection_state;

// Check state
if (!connection_state.is_triggered()) {
    std::cout << "Not yet triggered" << std::endl;
}

// Set without triggering callback
connection_state.set();

// Check state again
if (connection_state.is_triggered()) {
    std::cout << "Now triggered" << std::endl;
}
```

**Blocking wait**

```cpp
toggle_event::toggle_event event;

// Thread 1: Wait for event
std::thread waiter([&event]() {
    std::cout << "Waiting for event..." << std::endl;
    event.wait(); // Blocks until event is triggered
    std::cout << "Event received!" << std::endl;
});

// Thread 2: Trigger event
std::thread trigger([&event]() {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    event.trigger_if_not_set([]() {
        std::cout << "Triggering event" << std::endl;
    });
});

waiter.join();
trigger.join();
```

**Wait with timeout**

```cpp
toggle_event::toggle_event event;

// Wait for event with 2 second timeout
bool result = event.wait_for(2000);

if (result) {
    std::cout << "Event triggered within timeout" << std::endl;
} else {
    std::cout << "Timeout waiting for event" << std::endl;
}
```

**Wait then execute**

```cpp
toggle_event::toggle_event ready_signal;

// Wait for signal, then execute callback
std::thread worker([&ready_signal]() {
    ready_signal.wait_then([]() {
        std::cout << "Ready! Starting work..." << std::endl;
    });
});

// Trigger signal from another thread
std::this_thread::sleep_for(std::chrono::seconds(1));
ready_signal.trigger_if_not_set([]() {
    std::cout << "Signaling ready" << std::endl;
});

worker.join();
```

**Practical example - Connection state monitoring**

```cpp
class ConnectionMonitor {
public:
    ConnectionMonitor() : connected_(false) {}

    void check_connection() {
        if (is_connected()) {
            disconnected_event_.reset(); // Reset disconnect event
            connected_event_.trigger_if_not_set([this]() {
                std::cout << "Connection established" << std::endl;
                on_connected();
            });
        } else {
            connected_event_.reset(); // Reset connect event
            disconnected_event_.trigger_if_not_set([this]() {
                std::cout << "Connection lost" << std::endl;
                on_disconnected();
            });
        }
    }

private:
    bool is_connected() { return connected_; }
    void on_connected() { /* Handle connection */ }
    void on_disconnected() { /* Handle disconnection */ }

    bool connected_;
    toggle_event::toggle_event connected_event_;
    toggle_event::toggle_event disconnected_event_;
};
```

### File descriptor events

**Features**

- **Multiple FD Management**: Monitor multiple file descriptors simultaneously
- **Event-Driven Callbacks**: Register callbacks for specific events (READ, WRITE, ERROR, etc.)
- **Thread-Safe**: All operations are protected with mutex locks
- **Dynamic FD Management**: Add, remove, enable, or disable file descriptors at runtime
- **User Data Support**: Pass custom data to callbacks
- **Error Handling**: Comprehensive error reporting with optional error handlers
- **Clean API**: Simple and intuitive interface

All methods are protected with a mutex, making the `fd_event_manager` thread-safe for:
- Adding/removing FDs from multiple threads
- Enabling/disabling FDs
- Querying FD status

**Note**: Callbacks are invoked from the thread that calls `wait()` or `wait_and_process()`.

**Examples**:

```cpp
#include "libevent.h"

fd_event::fd_event_manager manager;

// Add file descriptor with callback
manager.add_fd(
    socket_fd,
    static_cast<short>(fd_event::event_type::READ),
    [](int fd, short revents, void* user_data) {
        if (revents & POLLIN) {
            char buffer[1024];
            ssize_t n = read(fd, buffer, sizeof(buffer));
            // Process data...
        }
    },
    nullptr,
    "my_socket"
);

// Main loop
while (running) {
    int ret = manager.wait_and_process(1000); // 1 second timeout
    if (ret < 0) {
        std::cerr << "Error: " << manager.get_last_error() << std::endl;
    }
}
```

```cpp
struct Context {
    int counter;
    std::string name;
};

Context my_context = {0, "MyContext"};

manager.add_fd(
    fd,
    POLLIN,
    [](int fd, short revents, void* user_data) {
        Context* ctx = static_cast<Context*>(user_data);
        ctx->counter++;
        std::cout << ctx->name << " event #" << ctx->counter << std::endl;
    },
    &my_context,
    "context_fd"
);
```

```cpp
manager.set_error_handler([](const std::string& error) {
    std::cerr << "fd_event_manager Error: " << error << std::endl;
});

manager.add_fd(
    fd,
    POLLIN,
    [](int fd, short revents, void* user_data) {
        if (revents & POLLERR) {
            std::cerr << "Error on fd " << fd << std::endl;
        }
        if (revents & POLLHUP) {
            std::cerr << "Hangup on fd " << fd << std::endl;
        }
        if (revents & POLLIN) {
            // Handle data...
        }
    }
);
```

```cpp
fd_event::fd_event_manager fd_manager;

// Context for callback
struct GnssContext {
    GnssCommander* commander;
    CParserBuffer* parser;
};
GnssContext ctx = {pCommander, &parser};

// Add GNSS receiver FD
fd_manager.add_fd(
    pCommander->get_receiver()->get_fd(),
    static_cast<short>(fd_event::event_type::READ),
    [](int fd, short revents, void* user_data) {
        GnssContext* ctx = static_cast<GnssContext*>(user_data);

        if (revents & POLLHUP) {
            LOG_ERROR("GNSS receiver hang up");
            return;
        }
        if (revents & POLLERR) {
            LOG_ERROR("GNSS receiver error");
            return;
        }
        if (revents & POLLIN) {
            ctx->commander->lock_operation();
            // Read and process data...
            ctx->commander->unlock_operation();
        }
    },
    &ctx,
    "gnss_receiver"
);

// Main loop
while (pCommander->is_running()) {
    int ret = fd_manager.wait_and_process(timeout);
    if (ret < 0) {
        LOG_ERROR("Event manager error: %s", fd_manager.get_last_error().c_str());
    }
}
```

### Signal events

The `signal_event` namespace provides a C++ wrapper around POSIX signal handling APIs. It offers a clean and safe interface for managing Unix/Linux signals in your application.

Checkout the sample signal event source code [HERE](https://github.com/nguyenchiemminhvu/LibEventCpp/blob/main/sample_signal_event.cpp).

**Include necessary headers**

```cpp
#include "libevent.h"
```

**Setting up signal handlers**

```cpp
void handle_interrupt(int signum) {
    std::cout << "Caught signal " << signum << std::endl;
    std::cout << "Gracefully shutting down..." << std::endl;
    // Cleanup and exit
}

// Set handler for SIGINT (Ctrl+C)
if (signal_event::set_signal_handler(SIGINT, handle_interrupt)) {
    std::cout << "Signal handler set successfully" << std::endl;
}

// Reset to default behavior
signal_event::reset_signal_handler(SIGINT);

// Ignore a signal
signal_event::ignore_signal(SIGUSR1);
```

**Extended signal handler with info**

Get additional information about the signal (sender PID, UID, etc.):

```cpp
void extended_handler(int signum, siginfo_t* info, void* context) {
    std::cout << "Signal: " << signal_event::get_signal_name(signum) << std::endl;
    std::cout << "Signal code: " << info->si_code << std::endl;
    std::cout << "Sender PID: " << info->si_pid << std::endl;
    std::cout << "Sender UID: " << info->si_uid << std::endl;
}

signal_event::set_signal_handler_ex(SIGUSR1, extended_handler);
```

**Blocking and unblocking signals**

```cpp
// Block a signal (it will be pending)
signal_event::block_signal(SIGUSR1);

// Send signal - it will be pending, not delivered yet
signal_event::raise_signal(SIGUSR1);

// Check if signal is pending
if (signal_event::is_signal_pending(SIGUSR1)) {
    std::cout << "SIGUSR1 is pending" << std::endl;
}

// Unblock signal - now it will be delivered
signal_event::unblock_signal(SIGUSR1);

// Block multiple signals at once
std::vector<int> signals = {SIGUSR1, SIGUSR2, SIGTERM};
signal_event::block_signals(signals);

// Check if a signal is blocked
if (signal_event::is_signal_blocked(SIGUSR1)) {
    std::cout << "SIGUSR1 is blocked" << std::endl;
}

// Unblock multiple signals
signal_event::unblock_signals(signals);
```

**Sending signals**

```cpp
// Send signal to current process
signal_event::raise_signal(SIGUSR1);

// Send signal to another process
pid_t target_pid = 1234;
signal_event::send_signal(target_pid, SIGUSR1);
```

**Waiting for signals**

```cpp
// Block signals first (required for sigwait)
signal_event::block_signals({SIGUSR1, SIGUSR2});

// Wait for any of these signals (infinite timeout)
int received = signal_event::wait_for_signal({SIGUSR1, SIGUSR2});
if (received > 0) {
    std::cout << "Received signal: " << signal_event::get_signal_name(received) << std::endl;
}

// Wait with timeout (milliseconds)
received = signal_event::wait_for_signal({SIGUSR1, SIGUSR2}, 5000);
if (received > 0) {
    std::cout << "Received: " << signal_event::get_signal_name(received) << std::endl;
} else {
    std::cout << "Timeout or error" << std::endl;
}
```

**Getting signal names**

```cpp
std::cout << signal_event::get_signal_name(SIGINT) << std::endl;   // Output: SIGINT
std::cout << signal_event::get_signal_name(SIGTERM) << std::endl;  // Output: SIGTERM
std::cout << signal_event::get_signal_name(SIGUSR1) << std::endl;  // Output: SIGUSR1
```

**Practical example - Graceful shutdown**

```cpp
#include "libevent.h"

std::atomic<bool> running(true);

void handle_shutdown(int signum) {
    std::cout << "\nReceived " << signal_event::get_signal_name(signum) << std::endl;
    std::cout << "Shutting down gracefully..." << std::endl;
    running = false;
}

int main() {
    // Set handlers for shutdown signals
    signal_event::set_signal_handler(SIGINT, handle_shutdown);
    signal_event::set_signal_handler(SIGTERM, handle_shutdown);

    std::cout << "Process ID: " << getpid() << std::endl;
    std::cout << "Running... Press Ctrl+C to stop" << std::endl;

    // Main loop
    while (running) {
        // Do work...
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Cleanup completed" << std::endl;
    return 0;
}
```

**Practical example - Inter-process communication**

```cpp
void handle_custom_signal(int signum) {
    static int count = 0;
    count++;
    std::cout << "Received custom signal " << count << " times" << std::endl;
}

int main() {
    signal_event::set_signal_handler(SIGUSR1, handle_custom_signal);

    std::cout << "Process ID: " << getpid() << std::endl;
    std::cout << "Send SIGUSR1 to this process using: kill -USR1 " << getpid() << std::endl;

    // Wait indefinitely
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
```

**Available signals**

Common signals you can handle:
- `SIGHUP`: Hangup detected on controlling terminal
- `SIGINT`: Interrupt from keyboard (Ctrl+C)
- `SIGQUIT`: Quit from keyboard
- `SIGTERM`: Termination signal
- `SIGUSR1`, `SIGUSR2`: User-defined signals
- `SIGALRM`: Timer signal from alarm()
- `SIGCHLD`: Child process stopped or terminated
- `SIGPIPE`: Broken pipe

**Important notes:**

- Some signals cannot be caught or ignored: `SIGKILL` and `SIGSTOP`
- Signal handlers should be simple and avoid calling non-async-signal-safe functions
- Be careful with signals in multithreaded programs
- Always block signals before using `wait_for_signal()`

### File system events

The `fs_event` namespace provides a C++ implementation for monitoring file system events, similar to Python's `pyinotify` library. It uses Linux's `inotify` API to efficiently track changes to files and directories.

The `fs_event` namespace consists of several key components that work together to provide efficient file system monitoring:

- **Event-driven**: Monitor file system changes asynchronously
- **Multiple event types**: Create, modify, delete, move, open, close, and more
- **Recursive monitoring**: Watch entire directory trees
- **Flexible API**: Both blocking and non-blocking modes
- **Python-like interface**: Familiar API for Python developers (pyinotify-style)
- **Thread-safe**: Safe for multi-threaded applications

**`fs_event_type` (Enum)**

Defines the types of file system events to monitor:

```cpp
enum class fs_event_type : uint32_t
{
    ACCESS,          // File was accessed (read)
    MODIFY,          // File was modified
    CREATE,          // File/directory created
    DELETE,          // File/directory deleted
    MOVED_FROM,      // File moved out
    MOVED_TO,        // File moved in
    OPEN,            // File was opened
    CLOSE_WRITE,     // Writable file closed
    // ... and more
};
```

Events can be combined using bitwise OR:
```cpp
auto mask = fs_event_type::CREATE | fs_event_type::MODIFY | fs_event_type::DELETE;
```

**`fs_event_info` (Structure)**

Contains information about a file system event:

```cpp
struct fs_event_info
{
    int wd;                    // Watch descriptor
    uint32_t mask;             // Event type mask
    uint32_t cookie;           // Cookie for event synchronization
    std::string pathname;      // Parent directory path
    std::string name;          // File/directory name
    bool is_dir;              // True if event is for a directory

    std::string get_full_path() const;  // Get pathname + name
    bool is_event(fs_event_type type) const;
    std::string event_to_string() const;
};
```

**`process_event` (Base Class)**

Abstract base class for event handlers (similar to `pyinotify.ProcessEvent`):

```cpp
class process_event
{
public:
    // Override these methods to handle specific events
    virtual void process_IN_CREATE(const fs_event_info& event);
    virtual void process_IN_MODIFY(const fs_event_info& event);
    virtual void process_IN_DELETE(const fs_event_info& event);
    virtual void process_IN_OPEN(const fs_event_info& event);
    virtual void process_IN_CLOSE_WRITE(const fs_event_info& event);
    virtual void process_IN_MOVED_FROM(const fs_event_info& event);
    virtual void process_IN_MOVED_TO(const fs_event_info& event);
    // ... and more

    virtual void process_default(const fs_event_info& event);
};
```

**`watch_manager` (Class)**

Manages file system watches using inotify (similar to `pyinotify.WatchManager`):

```cpp
class watch_manager
{
public:
    // Add a watch for a path
    int add_watch(const std::string& path, fs_event_type mask, bool recursive = false);

    // Remove a watch
    bool remove_watch(int wd);
    bool remove_watch(const std::string& path);

    // Query watches
    std::string get_path(int wd) const;
    int get_wd(const std::string& path) const;
    size_t get_watch_count() const;
};
```

**`notifier` (Class)**

Reads events and dispatches them to event handlers (similar to `pyinotify.Notifier`):

```cpp
class notifier
{
public:
    notifier(std::shared_ptr<watch_manager> wm,
             std::shared_ptr<process_event> handler);

    // Blocking: continuously process events
    void loop();

    // Non-blocking: process available events once
    int process_events(int timeout_ms = 0);

    // Stop the event loop
    void stop();
};
```

**Usage Examples**

**Basic Usage (Blocking Mode)**

```cpp
#include "libevent.h"
#include <iostream>

// Custom event handler
class my_event_handler : public fs_event::process_event
{
public:
    void process_IN_CREATE(const fs_event::fs_event_info& event) override
    {
        std::cout << "Created: " << event.get_full_path() << std::endl;
    }

    void process_IN_MODIFY(const fs_event::fs_event_info& event) override
    {
        std::cout << "Modified: " << event.get_full_path() << std::endl;
    }

    void process_IN_DELETE(const fs_event::fs_event_info& event) override
    {
        std::cout << "Deleted: " << event.get_full_path() << std::endl;
    }
};

int main()
{
    // Create watch manager
    auto wm = std::make_shared<fs_event::watch_manager>();

    // Create event handler
    auto handler = std::make_shared<my_event_handler>();

    // Define events to monitor
    auto mask = fs_event::fs_event_type::CREATE |
                fs_event::fs_event_type::MODIFY |
                fs_event::fs_event_type::DELETE;

    // Add watch
    wm->add_watch("/tmp/watch_dir", mask);

    // Create notifier and start event loop
    auto notifier = std::make_shared<fs_event::notifier>(wm, handler);
    notifier->loop();  // Blocking

    return 0;
}
```

**Non-Blocking Mode**

```cpp
auto notifier = std::make_shared<fs_event::notifier>(wm, handler);

bool running = true;
while (running)
{
    // Process events with 1 second timeout
    int count = notifier->process_events(1000);

    if (count > 0)
    {
        std::cout << "Processed " << count << " events" << std::endl;
    }

    // Do other work here...
}
```

**Recursive Monitoring**

```cpp
// Watch directory and all subdirectories
wm->add_watch("/path/to/dir", mask, true);  // recursive=true

std::cout << "Total watches: " << wm->get_watch_count() << std::endl;
```

**Specific Event Monitoring**

```cpp
// Monitor only specific events
auto mask = fs_event::fs_event_type::MODIFY |
            fs_event::fs_event_type::CLOSE_WRITE;

wm->add_watch("/path/to/file", mask);
```

## Pros And Cons

Each method of event handling/processing has its own advantages and disadvantages. It is important to carefully evaluate the specific requirements and constraints of your application before choosing the appropriate technique (such as performance, complexity, flexibility, scalability...).

### Message Event Handler

**Pros:**

- Messages are processed asynchronously, which helps in managing tasks without blocking the main thread, useful for heavy task or periodic events.
- The ```event_handler``` acts as a centralized handler for all event messages, easy to manage and organize source codes.
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

### Once Event

**Pros:**

- **Precise control**: Different classes for different execution patterns (once ever, once per N times, once per value, once per interval)
- **Thread-safe**: All classes use proper synchronization mechanisms
- **Memory efficient**: Minimal overhead for tracking state
- **Easy to use**: Simple and intuitive API for common patterns
- **Flexible**: Supports lambdas, function pointers, and member functions
- **No external dependencies**: Uses only standard C++ features

**Cons:**

- **No built-in persistence**: State is lost when object is destroyed
- **Limited to single condition**: Cannot combine multiple conditions easily
- **once_at_least complexity**: Runs background thread, adds overhead
- **No callback chaining**: Cannot easily sequence multiple once events
- **Memory overhead for once_per_value**: Stores last value in memory

**Use cases:**
- Initialization that should happen only once
- Rate limiting API calls or log messages
- Detecting value changes in monitoring systems
- Ensuring periodic processing of streaming data
- Preventing duplicate operations in event-driven systems

### Toggle Event

**Pros:**

- **Simple state management**: Clear on/off state for events
- **Thread-safe**: All operations protected with mutex and condition variable
- **Flexible waiting**: Supports blocking, timeout, and non-blocking checks
- **One-time trigger guarantee**: Callback executed exactly once per cycle
- **Bidirectional control**: Can be reset from either producer or consumer side
- **Combines check and action**: `trigger_if_not_set` is atomic

**Cons:**

- **Binary state only**: Cannot handle multi-state or counted events
- **No automatic reset**: Must manually call `reset()` to allow next trigger
- **Blocking can deadlock**: Improper use of `wait()` can cause deadlocks
- **Single callback per trigger**: Cannot register multiple callbacks
- **Memory overhead**: Uses mutex and condition variable per instance

**Use cases:**
- Connection state changes (connected/disconnected)
- One-time error notifications
- State transitions in state machines
- Synchronization between threads
- Detecting threshold crossings (above/below)

### File descriptor Events

**Pros:**

- Efficient monitoring of multiple file descriptors using `poll()`
- Thread-safe with mutex protection for concurrent access
- Dynamic FD management (add/remove/enable/disable at runtime)
- Flexible callback system with user data support
- Cross-platform on Unix-like systems (Linux, BSD, macOS)
- Clean and intuitive API with named FDs for debugging
- Lazy rebuild pattern for efficient event array updates

**Cons:**

- Not scalable for thousands of FDs (O(n) complexity, use `epoll`/`kqueue` for high-performance)
- Unix-only, no Windows support
- Level-triggered only (no edge-triggered mode)
- Callbacks executed while holding mutex (potential contention)
- No per-FD timeout or priority handling
- Requires manual event loop (no built-in dispatcher thread)
- FD lifecycle managed externally (library doesn't own/close FDs)

### Signal Events

**Pros:**

- **Clean API**: Simplifies complex POSIX signal handling APIs
- **Extended information**: Support for `siginfo_t` to get sender details
- **Signal blocking**: Easy block/unblock of single or multiple signals
- **Signal waiting**: Synchronous signal waiting with timeout support
- **Signal names**: Human-readable signal name conversion
- **Type-safe**: C++ wrapper around C APIs
- **Inter-process communication**: Use signals to communicate between processes

**Cons:**

- **Unix/Linux only**: Not portable to Windows
- **Limited by POSIX**: Subject to POSIX signal limitations (async-signal-safe functions)
- **Cannot catch all signals**: `SIGKILL` and `SIGSTOP` cannot be caught
- **Thread interaction complexity**: Signals and multithreading can be tricky
- **Handler restrictions**: Signal handlers must be very simple and avoid most stdlib functions
- **Race conditions**: Possible race conditions between signal delivery and handler execution
- **Global state**: Signal handlers are process-wide, not per-object

**Use cases:**
- Graceful application shutdown (SIGINT, SIGTERM)
- Inter-process communication (SIGUSR1, SIGUSR2)
- Handling child process termination (SIGCHLD)
- Alarm timers (SIGALRM)
- Custom protocol signaling between processes
- Debugging and profiling (SIGUSR1/SIGUSR2 for runtime stats)

### File system Events

**Pros:**

- **Efficient monitoring**: Uses Linux inotify API for kernel-level event notification
- **Python-like API**: Familiar interface for developers who know pyinotify
- **Flexible event handling**: Override only the events you care about
- **Multiple modes**: Both blocking (loop) and non-blocking (process_events) modes
- **Recursive watching**: Can monitor entire directory trees
- **Rich event types**: CREATE, MODIFY, DELETE, MOVE, OPEN, CLOSE, and more
- **Thread-safe**: All operations protected with mutexes
- **Event batching**: Processes multiple events efficiently
- **No polling overhead**: Kernel notifies only when changes occur

**Cons:**

- **Linux only**: Uses inotify, which is Linux-specific (no Windows/macOS support)
- **Watch limits**: System limits on number of watches (`/proc/sys/fs/inotify/max_user_watches`)
- **Recursive complexity**: Manual watch management for new subdirectories in recursive mode
- **Event overflow**: High-frequency changes can overflow inotify queue
- **No automatic cleanup**: Watches for deleted directories must be manually removed
- **Blocking on errors**: Some errors can interrupt the event loop
- **Memory overhead**: Each watch consumes kernel memory
- **Move events**: Tracking file moves requires cookie matching between MOVED_FROM/MOVED_TO

## References

[https://github.com/endurodave/StdWorkerThread](https://github.com/endurodave/StdWorkerThread)

[https://stackoverflow.com/questions/9711414/what-is-the-proper-way-of-doing-event-handling-in-c](https://stackoverflow.com/questions/9711414/what-is-the-proper-way-of-doing-event-handling-in-c)

[https://gameprogrammingpatterns.com/event-queue.html](https://gameprogrammingpatterns.com/event-queue.html)

[https://github.com/wqking/eventpp](https://github.com/wqking/eventpp)

[https://sigslot.sourceforge.net/](https://sigslot.sourceforge.net/)
