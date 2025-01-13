/**
 * libevent.h
 *
 * LibEventCpp is a lightweight and portable C++14 library for event-driven
 * programming. Implemented in a single header file for easy integration.
 *
 * Features:
 * - Message event handler: Asynchronous message queue with event looper
 * - Signals and slots: Type-safe callback connections (Qt-style)
 * - Time events: POSIX timers with callback support
 * - Once events: Execute callbacks conditionally (once per life, N times,
 *   value change, interval)
 * - Toggle events: One-shot triggers with reset capability
 * - File descriptor events: Monitor file descriptors using poll()
 * - Signal events: POSIX signal handling wrapper
 * - File system events: Monitor file system changes using inotify (Linux)
 *
 * Copyright © [nguyenchiemminhvu] [2026]. All Rights Reserved.
 *
 * Licensed under the MIT License. You may obtain a copy of the License at:
 * https://opensource.org/licenses/MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Author:
 * [nguyenchiemminhvu@gmail.com]
 *
 * Version:
 * 1.0 - [2026-02-02]
 */

#ifndef LIB_FOR_EVENT_DRIVEN_PROGRAMMING
#define LIB_FOR_EVENT_DRIVEN_PROGRAMMING

// C++ Standard Library - Type Traits and Utilities
#include <type_traits>      // Template metaprogramming (std::decay, std::is_same, etc.)
#include <utility>          // std::forward, std::move for perfect forwarding
#include <functional>       // std::function for callable wrappers
#include <memory>           // Smart pointers (std::shared_ptr, std::unique_ptr, std::enable_shared_from_this)
// C++ Standard Library - Containers
#include <string>           // std::string for text handling
#include <vector>           // Dynamic arrays for event storage
#include <list>             // Doubly-linked lists for connection management
#include <queue>            // Priority queue for event scheduling
#include <map>              // Associative containers for descriptor mappings
#include <unordered_map>    // Unordered maps for signal-slot tracking
#include <unordered_set>    // Unordered sets for signal-slot tracking
#include <tuple>            // std::tuple for storing function arguments
// C++ Standard Library - Algorithms and I/O
#include <algorithm>        // std::remove_if, std::find for container operations
#include <sstream>          // String stream operations
// C++ Standard Library - Threading and Synchronization
#include <thread>           // std::thread for background processing
#include <mutex>            // std::mutex, std::lock_guard for thread safety
#include <atomic>           // Atomic operations for lock-free counters
#include <condition_variable> // std::condition_variable for thread signaling
// C++ Standard Library - Time
#include <chrono>           // Time utilities (steady_clock, duration, time_point)
#include <stdexcept>        // Exception classes (std::runtime_error)
// C Standard Library
#include <cstdint>          // Fixed-width integer types (uint64_t, etc.)
#include <cstring>          // C string operations (memset, strcmp)
#include <ctime>            // Time structures for POSIX timers
// POSIX System Calls
#include <unistd.h>         // POSIX API (read, write, close, etc.)
#include <errno.h>          // Error number definitions
#include <dirent.h>         // Directory operations
// POSIX Signals and Event Monitoring
#include <signal.h>         // Signal handling and POSIX timers (timer_create, sigevent)
#include <sys/poll.h>       // poll() for file descriptor monitoring (POLLIN, POLLOUT, etc.)
#include <sys/inotify.h>    // Inotify API for filesystem event monitoring
#include <sys/stat.h>       // File statistics and type checking

#if (defined(__cplusplus) && __cplusplus >= 202002L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
    #define LIB_EVENT_CPP_20
    #define LIB_EVENT_CPP_17
    #define LIB_EVENT_CPP_14
#elif (defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_HAS_CXX17) && _HAS_CXX17 == 1)
    #define LIB_EVENT_CPP_17
    #define LIB_EVENT_CPP_14
#elif (defined(__cplusplus) && __cplusplus >= 201402L) || (defined(_HAS_CXX14) && _HAS_CXX14 == 1)
    #define LIB_EVENT_CPP_14
#endif

#ifndef LIB_EVENT_CPP_14
namespace std
{
    // source: https://stackoverflow.com/a/32223343
    template<std::size_t... Ints>
    struct index_sequence
    {
        using type = index_sequence;
        using value_type = std::size_t;
        static constexpr std::size_t size() noexcept
        {
            return sizeof...(Ints);
        }
    };

    template<class Sequence1, class Sequence2>
    struct merge_and_renumber;

    template<std::size_t... I1, std::size_t... I2>
    struct merge_and_renumber<index_sequence<I1...>, index_sequence<I2...>>
            : index_sequence < I1..., (sizeof...(I1) + I2)... > {};

    template<std::size_t N>
    struct make_index_sequence
        : merge_and_renumber < typename make_index_sequence < N / 2 >::type,
        typename make_index_sequence < N - N / 2 >::type > {};

    template<> struct make_index_sequence<0> : index_sequence<> {};
    template<> struct make_index_sequence<1> : index_sequence<0> {};

    template<typename... Ts>
    using index_sequence_for = make_index_sequence<sizeof...(Ts)>;
}
#endif // LIB_EVENT_CPP_14

/**
 * @brief Converts argument types, transforming C-string types to std::string
 * @tparam T The type to convert
 */
template <typename T>
struct convert_arg
{
    using decay_type = typename std::decay<T>::type;
    using type = typename std::conditional<
        std::is_same<decay_type, char*>::value || std::is_same<decay_type, const char*>::value || std::is_same<decay_type, const char[]>::value,
        std::string,
        T
    >::type;
};

/**
 * @brief Type aliases for callable objects used throughout the library
 */
namespace callables
{
    /**
     * @brief Function object for free functions
     * @tparam Args Argument types for the function
     */
    template <typename... Args>
    using task = std::function<void(typename convert_arg<Args>::type...)>;

    /**
     * @brief Function pointer type for free functions
     * @tparam Args Argument types for the function
     */
    template <typename... Args>
    using task_fptr = void(*)(typename convert_arg<Args>::type...);

    /**
     * @brief Member function pointer type
     * @tparam T Class type
     * @tparam Args Argument types for the member function
     */
    template <typename T, typename... Args>
    using task_member = void(T::*)(typename convert_arg<Args>::type...);
}

/**
 * @brief Interface for stoppable components
 *
 * Components implementing this interface can be stopped and cleaned up.
 */
class i_stoppable
{
public:
    /**
     * @brief Stop the component and clean up resources
     */
    virtual void stop() = 0;
};

/**
 * @brief Event handling system for asynchronous task execution
 *
 * Provides event queues, event loopers, and event handlers for managing
 * asynchronous execution of tasks with scheduling support.
 */
namespace event_handler
{
    /** @brief Timestamp type for event scheduling (steady clock for monotonic timing) */
    using steady_timestamp_t = std::chrono::steady_clock::time_point;

    /**
     * @brief Base interface for all events
     *
     * Events encapsulate a task to be executed at a specific time.
     */
    class i_event
    {
    public:
        /** @brief Default constructor */
        i_event() = default;

        /** @brief Virtual destructor */
        virtual ~i_event() {}

        /**
         * @brief Execute the event task
         */
        virtual void execute() = 0;

        /**
         * @brief Get the scheduled execution timestamp
         * @return Timestamp when the event should execute
         */
        steady_timestamp_t get_timestamp() const
        {
            return m_timestamp;
        }

        /**
         * @brief Compare events for priority queue ordering (earlier time = higher priority)
         * @param other Event to compare with
         * @return true if this event should execute after other
         */
        bool operator<(const i_event& other) const
        {
            return m_timestamp > other.m_timestamp;
        }

    protected:
        steady_timestamp_t m_timestamp; ///< Scheduled execution time
    };

    /**
     * @brief Concrete event implementation for free functions
     * @tparam Args Argument types for the task
     */
    template <typename... Args>
    class event : public i_event
    {
    public:
        /**
         * @brief Construct an event to execute immediately
         * @param act Task to execute
         * @param args Arguments to pass to the task
         */
        event(callables::task<Args...> act, Args... args)
            : m_act(std::move(act)), m_args(std::make_tuple(args...))
        {
            i_event::m_timestamp = std::chrono::steady_clock::now();
        }

        /**
         * @brief Construct a delayed event
         * @param delay_ms Delay in milliseconds before execution
         * @param act Task to execute
         * @param args Arguments to pass to the task
         */
        event(uint64_t delay_ms, callables::task<Args...> act, Args... args)
            : m_act(std::move(act)), m_args(std::make_tuple(args...))
        {
            i_event::m_timestamp = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
        }

        /** @brief Virtual destructor */
        virtual ~event() {}

        /**
         * @brief Create a shared event to execute immediately
         * @param act Task to execute
         * @param args Arguments to pass to the task
         * @return Shared pointer to the created event
         */
        static std::shared_ptr<i_event> create(callables::task<Args...> act, Args... args)
        {
            std::shared_ptr<i_event> ev = std::make_shared<event<Args...>>(std::move(act), args...);
            return ev;
        }

        /**
         * @brief Create a delayed shared event
         * @param delay_ms Delay in milliseconds before execution
         * @param act Task to execute
         * @param args Arguments to pass to the task
         * @return Shared pointer to the created event
         */
        static std::shared_ptr<i_event> create(uint64_t delay_ms, callables::task<Args...> act, Args... args)
        {
            std::shared_ptr<i_event> ev = std::make_shared<event<Args...>>(delay_ms, std::move(act), args...);
            return ev;
        }

        /**
         * @brief Execute the encapsulated task with stored arguments
         */
        virtual void execute()
        {
            this->invoke(std::make_index_sequence<sizeof...(Args)>());
        }

    private:
        template <std::size_t... Indices>
        void invoke(std::index_sequence<Indices...>)
        {
            if (m_act)
            {
                m_act(std::get<Indices>(m_args)...);
            }
        }

    private:
        callables::task<Args...> m_act; ///< Task to execute
        std::tuple<Args...> m_args; ///< Arguments tuple
    };

    /**
     * @brief Event implementation for member functions
     * @tparam Handler Class type containing the member function
     * @tparam Args Argument types for the member function
     */
    template <class Handler, typename... Args>
    class event<Handler, Args...> : public i_event
    {
    public:
        /**
         * @brief Construct a member function event to execute immediately
         * @param handler Shared pointer to the handler object
         * @param act Member function pointer to execute
         * @param args Arguments to pass to the member function
         */
        event(std::shared_ptr<Handler> handler, callables::task_member<Handler, Args...> act, Args... args)
            : m_handler(handler), m_act(std::move(act)), m_args(std::make_tuple(args...))
        {
            i_event::m_timestamp = std::chrono::steady_clock::now();
        }

        /**
         * @brief Construct a delayed member function event
         * @param delay_ms Delay in milliseconds before execution
         * @param handler Shared pointer to the handler object
         * @param act Member function pointer to execute
         * @param args Arguments to pass to the member function
         */
        event(uint64_t delay_ms, std::shared_ptr<Handler> handler,  callables::task_member<Handler, Args...> act, Args... args)
            : m_handler(handler), m_act(std::move(act)), m_args(std::make_tuple(args...))
        {
            i_event::m_timestamp = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
        }

        /** @brief Virtual destructor */
        virtual ~event() {}

        /**
         * @brief Create a shared member event to execute immediately
         * @param handler Shared pointer to the handler object
         * @param act Member function pointer to execute
         * @param args Arguments to pass to the member function
         * @return Shared pointer to the created event
         */
        static std::shared_ptr<i_event> create(std::shared_ptr<Handler> handler, callables::task_member<Handler, Args...> act, Args... args)
        {
            std::shared_ptr<i_event> ev = std::make_shared<event<Handler, Args...>>(handler, std::move(act), args...);
            return ev;
        }

        /**
         * @brief Create a delayed shared member event
         * @param delay_ms Delay in milliseconds before execution
         * @param handler Shared pointer to the handler object
         * @param act Member function pointer to execute
         * @param args Arguments to pass to the member function
         * @return Shared pointer to the created event
         */
        static std::shared_ptr<i_event> create(uint64_t delay_ms, std::shared_ptr<Handler> handler, callables::task_member<Handler, Args...> act, Args... args)
        {
            std::shared_ptr<i_event> ev = std::make_shared<event<Handler, Args...>>(delay_ms, handler, std::move(act), args...);
            return ev;
        }

        /**
         * @brief Execute the member function on the handler object
         */
        virtual void execute()
        {
            this->invoke(std::make_index_sequence<sizeof...(Args)>());
        }

    private:
        template <std::size_t... Indices>
        void invoke(std::index_sequence<Indices...>)
        {
            if (m_handler != nullptr)
            {
                (m_handler.get()->*m_act)(std::get<Indices>(m_args)...);
            }
        }

    private:
        std::shared_ptr<Handler> m_handler; ///< Handler object owning the member function
        callables::task_member<Handler, Args...> m_act; ///< Member function to execute
        std::tuple<Args...> m_args; ///< Arguments tuple

        static_assert(std::is_class<Handler>::value, "Handler must be a class type");
        static_assert(std::is_member_function_pointer<decltype(m_act)>::value, "m_act must be a member function pointer");
    };

    /**
     * @brief Thread-safe priority queue for managing scheduled events
     *
     * Events are ordered by timestamp, with earlier events having higher priority.
     * The queue is thread-safe and can be used from multiple threads.
     */
    class event_queue : public i_stoppable
    {
    public:
        /**
         * @brief Construct an event queue in running state
         */
        event_queue()
            : m_running(true)
        {

        }

        /**
         * @brief Destructor stops the queue and releases all pending events
         */
        virtual ~event_queue()
        {
            this->stop();
        }

        /**
         * @brief Add an event to the queue
         * @param ev Event to enqueue (must not be null)
         *
         * If the queue has been stopped, the event is discarded.
         * Thread-safe: can be called from multiple threads.
         */
        void enqueue(std::shared_ptr<i_event> ev)
        {
            bool enqueued = false;
            {
                std::lock_guard<std::mutex> lock(m_mut);
                if (m_running)
                {
                    m_queue.push(std::move(ev));
                    enqueued = true;
                }
            }

            if (enqueued)
            {
                m_cond.notify_all();
            }
        }

        /**
         * @brief Retrieve and remove the next event from the queue (blocking)
         * @return Shared pointer to the next event, or nullptr if queue is stopped
         *
         * Blocks until an event is available or the queue is stopped.
         * If the event's scheduled time is in the future, waits until that time.
         * Thread-safe: can be called from multiple threads, but typically called
         * by a single event looper thread.
         */
        std::shared_ptr<i_event> poll()
        {
            std::unique_lock<std::mutex> lock(m_mut);
            m_cond.wait(lock, [this]() { return !this->m_queue.empty() || !this->m_running; });

            if (!this->m_running)
            {
                return nullptr;
            }

            std::shared_ptr<i_event> ev = m_queue.top();
            m_queue.pop();

            steady_timestamp_t current_timestamp = std::chrono::steady_clock::now();
            std::chrono::duration<double, std::milli> duration_ms = ev->get_timestamp() - current_timestamp;
            if (duration_ms.count() > 0)
            {
                auto wait_duration = std::chrono::milliseconds(static_cast<int>(duration_ms.count()));
                std::cv_status wait_rc = m_cond.wait_for(lock, wait_duration);
                if (wait_rc == std::cv_status::no_timeout)
                {
                    // The wait was notified before the timeout duration expired.
                    lock.unlock(); // unlock so that enqueue can push new event to the queue without dead lock
                    this->enqueue(ev);
                    ev = nullptr;
                }
            }

            return ev;
        }

        /**
         * @brief Stop the queue and discard all pending events
         *
         * After calling stop, enqueue() calls are ignored and poll() returns nullptr.
         * Thread-safe: can be called from any thread.
         */
        virtual void stop()
        {
            bool state_changed = false;
            {
                std::lock_guard<std::mutex> lock(m_mut);
                if (m_running)
                {
                    m_running = false;
                    state_changed = true;
                    m_queue = std::priority_queue<std::shared_ptr<i_event>, std::vector<std::shared_ptr<i_event>>, event_comparator>();
                }
            }

            if (state_changed)
            {
                m_cond.notify_all();
            }
        }

    private:
        struct event_comparator
        {
            bool operator()(const std::shared_ptr<i_event>& lhs, const std::shared_ptr<i_event>& rhs)
            {
                return *lhs < *rhs;
            }
        };

    private:
        std::atomic<bool> m_running;
        std::priority_queue<std::shared_ptr<i_event>, std::vector<std::shared_ptr<i_event>>, event_comparator> m_queue;
        std::mutex m_mut;
        std::condition_variable m_cond; ///< Condition variable for signaling
    };

    /**
     * @brief Dedicated thread that continuously processes events from an event_queue
     *
     * The event_looper runs a background thread that polls the queue and executes events.
     */
    class event_looper : public i_stoppable
    {
    public:
        /**
         * @brief Construct and start the event looper thread
         *
         * Creates an internal event_queue and starts the processing loop.
         */
        event_looper()
            : m_running(true)
        {
            m_event_queue = std::make_shared<event_queue>();
            m_looper_thread = std::thread(&event_looper::looper, this);
        }

        /**
         * @brief Destructor stops the looper and joins the thread
         */
        virtual ~event_looper()
        {
            this->stop();
        }

        /**
         * @brief Get the event queue managed by this looper
         * @return Shared pointer to the event queue
         */
        std::shared_ptr<event_queue> get_event_queue()
        {
            return m_event_queue;
        }

        /**
         * @brief Stop the looper and wait for the thread to terminate
         *
         * Stops the internal event_queue and joins the looper thread.
         * Thread-safe: can be called from any thread.
         */
        virtual void stop()
        {
            bool state_changed = false;
            {
                std::lock_guard<std::mutex> lock(m_mut);
                if (m_running)
                {
                    m_running = false;
                    state_changed = true;
                    m_event_queue->stop();
                }
            }

            if (state_changed)
            {
                m_looper_thread.join();
            }
        }

    private:
        void looper()
        {
            while (true)
            {
                {
                    std::lock_guard<std::mutex> lock(m_mut);
                    if (!m_running)
                    {
                        break;
                    }
                }

                std::shared_ptr<i_event> ev = nullptr;
                if (m_event_queue != nullptr)
                {
                    ev = m_event_queue->poll();
                }

                if (ev != nullptr)
                {
                    ev->execute();
                }
            }
        }

    private:
        std::atomic<bool> m_running;
        std::thread m_looper_thread;
        std::shared_ptr<event_queue> m_event_queue;
        std::mutex m_mut; ///< Mutex for thread safety
    };

    /**
     * @brief High-level interface for posting events to be executed asynchronously
     *
     * Manages an event_looper and provides convenient methods for posting free functions
     * and member functions. Classes can inherit from event_handler to post member functions.
     * Thread-safe: all public methods can be called from multiple threads.
     */
    class event_handler : public i_stoppable, public std::enable_shared_from_this<event_handler>
    {
    public:
        /**
         * @brief Construct an event handler with a new dedicated looper
         */
        event_handler()
        {
            m_looper = std::make_shared<event_looper>();
            m_event_queue = m_looper->get_event_queue();
        }

        /**
         * @brief Construct an event handler using a shared looper
         * @param looper Shared event looper to use
         *
         * Multiple event handlers can share the same looper to use a common thread pool.
         */
        event_handler(std::shared_ptr<event_looper> looper)
            : m_looper(looper)
        {
            if (looper)
            {
                m_event_queue = looper->get_event_queue();
            }
        }

        /**
         * @brief Destructor stops the handler
         */
        virtual ~event_handler()
        {
            this->stop();
        }

        /**
         * @brief Bind a different looper to this handler
         * @param looper Shared event looper to bind
         *
         * @note Should be called during initialization only, not after posting events.
         */
        void bind_looper(std::shared_ptr<event_looper> looper)
        {
            std::lock_guard<std::mutex> lock(m_mut);
            if (looper)
            {
                m_looper = looper;
                m_event_queue = looper->get_event_queue();
            }
        }

        /**
         * @brief Post a free function event to execute immediately
         * @tparam Args Argument types
         * @param func Function to execute
         * @param args Arguments to pass to the function
         */
        template <typename... Args>
        void post_event(callables::task<Args...> func, Args... args)
        {
            std::shared_ptr<i_event> ev = event<Args...>::create(std::move(func), std::forward<typename convert_arg<Args>::type>(args)...);
            std::lock_guard<std::mutex> lock(m_mut);
            if (m_event_queue != nullptr)
            {
                m_event_queue->enqueue(ev);
            }
        }

        /**
         * @brief Post a delayed free function event
         * @tparam Args Argument types
         * @param delay_ms Delay in milliseconds before execution
         * @param func Function to execute
         * @param args Arguments to pass to the function
         */
        template <typename... Args>
        void post_delayed_event(uint64_t delay_ms, callables::task<Args...> func, Args... args)
        {
            std::shared_ptr<i_event> ev = event<Args...>::create(delay_ms, std::move(func), std::forward<typename convert_arg<Args>::type>(args)...);
            std::lock_guard<std::mutex> lock(m_mut);
            if (m_event_queue != nullptr)
            {
                m_event_queue->enqueue(ev);
            }
        }

        /**
         * @brief Post a free function event multiple times with fixed intervals
         * @tparam Args Argument types
         * @param times Number of times to execute
         * @param duration_ms Interval in milliseconds between executions
         * @param func Function to execute
         * @param args Arguments to pass to the function
         */
        template <typename... Args>
        void post_repeated_event(std::size_t times, uint32_t duration_ms, callables::task<Args...> func, Args... args)
        {
            for (std::size_t i = 0U; i < times; ++i)
            {
                uint64_t delay_ms = (duration_ms * i);
                this->post_delayed_event(delay_ms, std::move(func), std::forward<typename convert_arg<Args>::type>(args)...);
            }
        }

        /**
         * @brief Post a member function event to execute immediately
         * @tparam T Class type (must derive from event_handler)
         * @tparam Args Argument types
         * @param func Member function pointer to execute
         * @param args Arguments to pass to the member function
         */
        template<typename T, typename... Args>
        void post_event(callables::task_member<T, Args...> func, Args... args)
        {
            static_assert(std::is_base_of<event_handler, T>::value, "T must be derived from event_handler");

            auto shared_this = this->get_shared_ptr();
            if (shared_this)
            {
                std::shared_ptr<i_event> ev = event<T, typename convert_arg<Args>::type...>::create(std::dynamic_pointer_cast<T>(shared_this), std::move(func), std::forward<typename convert_arg<Args>::type>(args)...);
                std::lock_guard<std::mutex> lock(m_mut);
                if (m_event_queue != nullptr)
                {
                    m_event_queue->enqueue(ev);
                }
            }
        }

        /**
         * @brief Post a delayed member function event
         * @tparam T Class type (must derive from event_handler)
         * @tparam Args Argument types
         * @param delay_ms Delay in milliseconds before execution
         * @param func Member function pointer to execute
         * @param args Arguments to pass to the member function
         */
        template<typename T, typename... Args>
        void post_delayed_event(uint64_t delay_ms, callables::task_member<T, Args...> func, Args... args)
        {
            static_assert(std::is_base_of<event_handler, T>::value, "T must be derived from event_handler");

            auto shared_this = this->get_shared_ptr();
            if (shared_this)
            {
                std::shared_ptr<i_event> ev = event<T, typename convert_arg<Args>::type...>::create(delay_ms, std::dynamic_pointer_cast<T>(shared_this), std::move(func), std::forward<typename convert_arg<Args>::type>(args)...);
                std::lock_guard<std::mutex> lock(m_mut);
                if (m_event_queue != nullptr)
                {
                    m_event_queue->enqueue(ev);
                }
            }
        }

        /**
         * @brief Post a member function event multiple times with fixed intervals
         * @tparam T Class type (must derive from event_handler)
         * @tparam Args Argument types
         * @param times Number of times to execute
         * @param duration_ms Interval in milliseconds between executions
         * @param func Member function pointer to execute
         * @param args Arguments to pass to the member function
         */
        template<typename T, typename... Args>
        void post_repeated_event(std::size_t times, uint32_t duration_ms, callables::task_member<T, Args...> func, Args... args)
        {
            static_assert(std::is_base_of<event_handler, T>::value, "T must be derived from event_handler");

            for (std::size_t i = 0U; i < times; ++i)
            {
                uint64_t delay_ms = (duration_ms * i);
                this->post_delayed_event(delay_ms, std::move(func), std::forward<typename convert_arg<Args>::type>(args)...);
            }
        }

        /**
         * @brief Stop the event handler and its looper
         */
        virtual void stop()
        {
            if (m_looper)
            {
                m_looper->stop();
            }
        }

    protected:
        /**
         * @brief Get a shared pointer to this handler
         * @return Shared pointer to this, or nullptr if not managed by shared_ptr
         */
        std::shared_ptr<event_handler> get_shared_ptr()
        {
            try
            {
                return shared_from_this();
            }
            catch (const std::bad_weak_ptr& e)
            {
                return nullptr;
            }
        }

    private:
        std::shared_ptr<event_looper> m_looper;
        std::shared_ptr<event_queue> m_event_queue;
        std::mutex m_mut; ///< Mutex for thread safety
    };
} // namespace event_handler

/**
 * @brief Signal-slot pattern implementation for type-safe callback management
 *
 * Provides a thread-safe signal-slot mechanism similar to Qt's signals and slots.
 * Signals can be connected to multiple slots (member functions or callable objects).
 */
namespace sigslot
{
    /** @brief Forward declaration of base_slot */
    class base_slot;

    /**
     * @brief Base interface for all signal types
     */
    class base_signal
    {
    public:
        /**
         * @brief Disconnect a specific slot from this signal
         * @param p_slot Pointer to the slot to disconnect
         */
        virtual void disconnect(base_slot* p_slot) = 0;

        /**
         * @brief Disconnect all slots from this signal
         */
        virtual void disconnect_all() = 0;
    };

    /**
     * @brief Base connection interface for signal-slot connections
     * @tparam Args Argument types for the signal
     */
    template <typename... Args>
    class base_connection
    {
    public:
        /** @brief Default constructor */
        base_connection() = default;

        /** @brief Virtual destructor */
        virtual ~base_connection() = default;

        /**
         * @brief Get the slot object associated with this connection
         * @return Pointer to the slot, or nullptr for callable connections
         */
        virtual base_slot* get_slot_obj() = 0;

        /**
         * @brief Emit the signal by invoking the connected callable
         * @param args Arguments to pass to the callable
         */
        virtual void emit(Args... args) = 0;

        /**
         * @brief Check if this connection matches the given slot and member function
         * @param p_slot Pointer to the slot object
         * @param func_ptr Pointer to the member function (as void*)
         * @return true if this connection matches, false otherwise
         */
        virtual bool matches(base_slot* p_slot, const void* func_ptr) const = 0;
    };

    /**
     * @brief Base class for slot objects
     *
     * Manages connections to signals and ensures proper cleanup.
     * Tracks the number of connections to each signal for proper disconnection.
     */
    class base_slot
    {
    public:
        /** @brief Default constructor */
        base_slot() = default;

        /**
         * @brief Destructor disconnects from all signals
         */
        virtual ~base_slot()
        {
            this->disconnect_all();
        }

        /**
         * @brief Register this slot with a signal (increment connection count)
         * @param p_signal_obj Pointer to the signal
         */
        void connect(base_signal* p_signal_obj)
        {
            std::lock_guard<std::mutex> lock(m_mut);
            m_signal_connection_counts[p_signal_obj]++;
        }

        /**
         * @brief Unregister a connection from a signal (decrement connection count)
         * @param p_signal_obj Pointer to the signal
         * @return true if this was the last connection to the signal, false otherwise
         */
        bool unregister_connection(base_signal* p_signal_obj)
        {
            std::lock_guard<std::mutex> lock(m_mut);
            auto it = m_signal_connection_counts.find(p_signal_obj);
            if (it != m_signal_connection_counts.end())
            {
                it->second--;
                if (it->second == 0U)
                {
                    m_signal_connection_counts.erase(it);
                    return true; // Last connection removed
                }
            }
            return false; // Still has connections
        }

        /**
         * @brief Unregister multiple connections from a signal (decrement connection count)
         * @param p_signal_obj Pointer to the signal
         * @param count Number of connections to unregister
         * @return true if all connections were removed, false otherwise
         */
        bool unregister_connection(base_signal* p_signal_obj, size_t count)
        {
            std::lock_guard<std::mutex> lock(m_mut);
            auto it = m_signal_connection_counts.find(p_signal_obj);
            if (it != m_signal_connection_counts.end())
            {
                if (it->second > count)
                {
                    it->second -= count;
                    return false; // Still has connections
                }
                else
                {
                    m_signal_connection_counts.erase(it);
                    return true; // Last connection removed
                }
            }
            return false; // No connections found
        }

        /**
         * @brief Disconnect this slot from a specific signal
         * @param p_signal_obj Pointer to the signal
         */
        void disconnect(base_signal* p_signal_obj)
        {
            std::unique_lock<std::mutex> lock(m_mut);
            if (m_signal_connection_counts.erase(p_signal_obj) > 0U)
            {
                lock.unlock();
                p_signal_obj->disconnect(this);
            }
        }

        /**
         * @brief Disconnect this slot from all connected signals
         */
        void disconnect_all()
        {
            std::unique_lock<std::mutex> lock(m_mut);

            std::unordered_set<base_signal*> signals_to_disconnect;
            for (const auto& pair : m_signal_connection_counts)
            {
                if (pair.first != nullptr)
                {
                    signals_to_disconnect.insert(pair.first);
                }
            }

            m_signal_connection_counts.clear();
            lock.unlock();

            for (auto p_signal_obj : signals_to_disconnect)
            {
                p_signal_obj->disconnect(this);
            }
        }

    private:
        std::mutex m_mut;
        std::unordered_map<base_signal*, uint32_t> m_signal_connection_counts; ///< Map of signals to connection counts
    };

    /**
     * @brief Connection implementation for member function slots
     * @tparam T Class type owning the member function
     * @tparam Args Argument types for the slot
     */
    template <class T, typename... Args>
    class sigslot_connection : public base_connection<Args...>
    {
    public:
        /** @brief Default constructor */
        sigslot_connection()
        {
            m_target = nullptr;
            m_member_func = nullptr;
        }

        /**
         * @brief Construct a connection to a member function
         * @param target Pointer to the target object
         * @param member_func Member function pointer
         */
        sigslot_connection(T* target, callables::task_member<T, Args...> member_func)
        {
            m_target = target;
            m_member_func = member_func;
        }

        /** @brief Virtual destructor */
        virtual ~sigslot_connection() = default;

        /**
         * @brief Get the slot object (target)
         * @return Pointer to the target object
         */
        virtual base_slot* get_slot_obj() override
        {
            return m_target;
        }

        /**
         * @brief Invoke the member function on the target object
         * @param args Arguments to pass to the member function
         */
        virtual void emit(Args... args) override
        {
            (m_target->*m_member_func)(std::forward<Args>(args)...);
        }

        /**
         * @brief Check if this connection matches the given slot and member function
         * @param p_slot Pointer to the slot object
         * @param func_ptr Pointer to the member function (as void*)
         * @return true if both slot and member function match, false otherwise
         */
        virtual bool matches(base_slot* p_slot, const void* func_ptr) const override
        {
            if (m_target != p_slot)
            {
                return false;
            }

            if (func_ptr == nullptr)
            {
                return true; // Match any member function for this slot
            }

            // Compare member function pointers
            // Convert both to void* for comparison (portable method)
            void(T::*other_func)(Args...) = *static_cast<void(T::**)(Args...)>(const_cast<void*>(func_ptr));
            return m_member_func == other_func;
        }

        /**
         * @brief Get the member function pointer
         * @return Member function pointer
         */
        void(T::*get_member_func())(Args...) const
        {
            return m_member_func;
        }

    private:
        T* m_target; ///< Target object
        callables::task_member<T, Args...> m_member_func; ///< Member function wrapper
    };

    /**
     * @brief Connection implementation for callable objects (lambdas, function objects)
     * @tparam Args Argument types for the callable
     */
    template <typename... Args>
    class callable_connection : public base_connection<Args...>
    {
    public:
        /** @brief Default constructor */
        callable_connection()
        {
            m_func = nullptr;
        }

        /**
         * @brief Construct a connection to a callable object
         * @param func Callable object to invoke
         */
        callable_connection(callables::task<Args...> func)
        {
            m_func = std::move(func);
        }

        /**
         * @brief Get the slot object (always nullptr for callables)
         * @return nullptr
         */
        virtual base_slot* get_slot_obj() override
        {
            return nullptr;
        }

        /**
         * @brief Invoke the callable object
         * @param args Arguments to pass to the callable
         */
        virtual void emit(Args... args) override
        {
            if (m_func)
            {
                m_func(std::forward<Args>(args)...);
            }
        }

        /**
         * @brief Check if this connection matches the given slot and member function
         * @param p_slot Pointer to the slot object
         * @param func_ptr Pointer to the member function (as void*)
         * @return Always false for callable connections
         */
        virtual bool matches(base_slot* p_slot, const void* func_ptr) const override
        {
            (void)p_slot;    // Unused
            (void)func_ptr;  // Unused
            return false;    // Callable connections don't match slot-based queries
        }

    private:
        callables::task<Args...> m_func; ///< Callable object
    };

    /**
     * @brief Thread-safe signal that can be connected to multiple slots
     * @tparam Args Argument types for the signal
     *
     * Signals can be connected to member functions (slots) or callable objects.
     * All connections are managed and automatically cleaned up.
     * Thread-safe: all public methods can be called from multiple threads.
     */
    template <typename... Args>
    class signal : public base_signal
    {
    public:
        /**
         * @brief Default constructor
         */
        signal()
            : base_signal()
        {
        }

        /**
         * @brief Destructor disconnects all slots
         */
        virtual ~signal()
        {
            this->disconnect_all();
        }

        /**
         * @brief Connect a member function slot to this signal
         * @tparam T Class type owning the member function
         * @param p_slot Pointer to the slot object (must derive from base_slot)
         * @param member_func Member function pointer to connect
         */
        template <class T>
        void connect(T* p_slot, callables::task_member<T, Args...> member_func)
        {
            std::lock_guard<std::mutex> lock(m_mut);

            std::shared_ptr<base_connection<Args...>> p_conn = std::make_shared<sigslot_connection<T, Args...>>(p_slot, member_func);
            m_slot_connections[p_slot].push_back(std::move(p_conn));
            p_slot->connect(this);
        }

        /**
         * @brief Connect a callable object to this signal
         * @param func Callable object (lambda, function object) to connect
         */
        void connect(callables::task<Args...> func)
        {
            std::lock_guard<std::mutex> lock(m_mut);

            std::shared_ptr<base_connection<Args...>> p_conn = std::make_shared<callable_connection<Args...>>(std::move(func));
            m_callable_connections.push_back(std::move(p_conn));
        }

        /**
         * @brief Disconnect a specific member function of a slot from this signal
         * @tparam T Class type owning the member function
         * @param p_slot Pointer to the slot object
         * @param member_func Member function pointer to disconnect
         *
         * This method removes only the connection to the specified member function.
         * If this was the last connection for the slot, the slot is automatically
         * disconnected from the signal.
         */
        template <class T>
        void disconnect(T* p_slot, callables::task_member<T, Args...> member_func)
        {
            std::unique_lock<std::mutex> lock(m_mut);

            auto it = m_slot_connections.find(p_slot);
            if (it == m_slot_connections.end())
            {
                return; // No connections for this slot
            }

            // Convert member function pointer to void* for comparison
            const void* func_ptr = reinterpret_cast<const void*>(&member_func);

            // Remove the specific connection
            auto& connections = it->second;
            auto rem_it = std::remove_if(
                connections.begin(),
                connections.end(),
                [&](const std::shared_ptr<base_connection<Args...>>& p_conn)
                {
                    return p_conn->matches(p_slot, func_ptr);
                }
            );

            if (rem_it != connections.end())
            {
                connections.erase(rem_it, connections.end());

                // If no more connections for this slot, remove the slot entry
                bool still_has_connection = !connections.empty();
                if (!still_has_connection)
                {
                    m_slot_connections.erase(it);
                }

                lock.unlock();

                // Notify the slot about the disconnection
                p_slot->unregister_connection(this);
            }
        }

        /**
         * @brief Disconnect all connections of a specific slot from this signal
         * @param p_slot Pointer to the slot to disconnect
         */
        virtual void disconnect(base_slot* p_slot) override
        {
            std::unique_lock<std::mutex> lock(m_mut);

            auto it = m_slot_connections.find(p_slot);
            if (it != m_slot_connections.end())
            {
                // Get the number of connections to be removed
                size_t num_connections = it->second.size();

                // Remove all connections for this slot
                m_slot_connections.erase(it);
                lock.unlock();

                // Notify the slot about each disconnection
                p_slot->unregister_connection(this, num_connections);
            }
        }

        /**
         * @brief Disconnect all slots and callables from this signal
         */
        virtual void disconnect_all() override
        {
            std::unique_lock<std::mutex> lock(m_mut);

            // Collect all slots and their connection counts
            std::unordered_map<base_slot*, size_t> slot_disconnect_counts;
            for (const auto& pair : m_slot_connections)
            {
                slot_disconnect_counts[pair.first] = pair.second.size();
            }

            // Clear all connections
            m_slot_connections.clear();
            m_callable_connections.clear();
            lock.unlock();

            // Notify all slots about disconnections
            for (const auto& pair : slot_disconnect_counts)
            {
                pair.first->unregister_connection(this, pair.second);
            }
        }

        /**
         * @brief Disconnect only callable connections (lambdas, function objects)
         *
         * Member function slots are not affected.
         */
        void disconnect_all_callable()
        {
            std::lock_guard<std::mutex> lock(m_mut);
            m_callable_connections.clear();
        }

        /**
         * @brief Emit the signal, invoking all connected slots and callables
         * @param args Arguments to pass to all connections
         *
         * Thread-safe: can be called from multiple threads.
         */
        void emit(Args... args)
        {
            std::lock_guard<std::mutex> lock(m_mut);

            // Emit to all slot connections
            for (auto& pair : m_slot_connections)
            {
                for (auto& p_conn : pair.second)
                {
                    p_conn->emit(std::forward<Args>(args)...);
                }
            }

            // Emit to all callable connections
            for (auto& p_conn : m_callable_connections)
            {
                p_conn->emit(std::forward<Args>(args)...);
            }
        }

        /**
         * @brief Function call operator to emit the signal
         * @param args Arguments to pass to all connections
         */
        void operator()(Args... args)
        {
            this->emit(std::forward<Args>(args)...);
        }

    private:
        std::mutex m_mut;
        std::unordered_map<base_slot*, std::vector<std::shared_ptr<base_connection<Args...>>>> m_slot_connections; ///< Map of slots to their connections
        std::vector<std::shared_ptr<base_connection<Args...>>> m_callable_connections; ///< Callable connections (lambdas, function objects)
    };

    /**
     * @brief Helper function to connect a signal to a slot
     * @tparam Signal Signal type
     * @tparam Slot Slot type
     * @tparam Args Argument types
     * @param signal Signal to connect
     * @param slot Pointer to the slot object
     * @param member_func Member function pointer
     */
    template <typename Signal, typename Slot, typename... Args>
    void connect(Signal& signal, Slot* slot, callables::task_member<Slot, Args...> member_func)
    {
        signal.connect(slot, member_func);
    }
}; // namespace sigslot

/**
 * @brief Timer and time-based event handling
 */
namespace time_event
{
    /**
     * @brief POSIX timer wrapper with callback support
     *
     * Provides a high-level interface to POSIX timers with support for
     * one-shot, periodic, and limited-repeat modes.
     */
    class timer : public i_stoppable
    {
    public:
        /**
         * @brief Construct a timer in stopped state
         */
        timer()
            : m_timer_id(0)
            , m_duration_ms(0)
            , m_repeat_count(-1) // -1 means infinite
            , m_repeat_counter(0)
            , m_running(false)
        {
        }

        /**
         * @brief Destructor stops the timer and clears callbacks
         */
        virtual ~timer()
        {
            this->stop();
            clear_callbacks();
        }

        /**
         * @brief Start the timer
         * @param repeat_count Number of times to repeat: -1 = infinite, 0 = one-shot, >0 = limited repeats
         *
         * @throws std::runtime_error if timer creation or setup fails
         * @note Must call set_duration() before starting the timer
         */
        void start(int64_t repeat_count = -1)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (m_running)
            {
                return;
            }

            m_repeat_count = repeat_count;
            m_repeat_counter = 0;

            struct sigevent sev{};
            sev.sigev_notify = SIGEV_THREAD;
            sev.sigev_notify_function = timer_timeout_handler;
            sev.sigev_value.sival_ptr = this;
            sev.sigev_notify_attributes = nullptr;
            if (timer_create(CLOCK_MONOTONIC, &sev, &m_timer_id) == -1)
            {
                throw std::runtime_error("Failed to create time_event::timer instance");
            }

            struct itimerspec its{};
            /**
             * it_value is when the timer first expires after being started.
             * If it_value is:
             * Non-zero → the timer starts ticking immediately and will fire after this duration.
             * Zero → the timer is disarmed (i.e., it won't start).
             *
             * it_interval defines the interval between subsequent expirations.
             * If it_interval is:
             * Zero → the timer is one-shot (fires once).
             * Non-zero → the timer is periodic (fires repeatedly every interval after the initial one).
             */

            if (m_repeat_count != 0)
            {
                its.it_value.tv_sec = m_duration_ms / 1000;
                its.it_value.tv_nsec = (m_duration_ms % 1000) * 1000000;
                its.it_interval.tv_sec = m_duration_ms / 1000;
                its.it_interval.tv_nsec = (m_duration_ms % 1000) * 1000000;
            }
            else // one shot timer
            {
                its.it_value.tv_sec = m_duration_ms / 1000;
                its.it_value.tv_nsec = (m_duration_ms % 1000) * 1000000;
                its.it_interval.tv_sec = 0;
                its.it_interval.tv_nsec = 0;
            }

            if (timer_settime(m_timer_id, 0, &its, nullptr) == -1)
            {
                if (m_timer_id != 0)
                {
                    timer_delete(m_timer_id);
                    m_timer_id = 0;
                }
                throw std::runtime_error("Failed to set time_event::timer instance");
            }

            m_running = true;
        }

        /**
         * @brief Stop the timer and release resources
         */
        virtual void stop()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_running)
            {
                m_running = false;
                m_repeat_counter = 0;
            }

            if (m_timer_id != 0)
            {
                timer_delete(m_timer_id);
                m_timer_id = 0;
            }
        }

        /**
         * @brief Set the timer duration
         * @param milliseconds Duration in milliseconds
         *
         * If the timer is running, it will be stopped and restarted with the new duration.
         */
        void set_duration(int64_t milliseconds)
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_duration_ms = milliseconds;
            }

            // If the timer is running, we need to stop and restart it with the new duration
            if (m_running)
            {
                this->stop();
                this->start(m_repeat_count);
            }
        }

        /**
         * @brief Add a callback to be invoked when the timer expires
         * @param cb Callback function with no arguments
         *
         * Thread-safe: can be called from any thread.
         */
        void add_callback(const callables::task<>& cb)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_callbacks.push_back(cb);
        }

        /**
         * @brief Clear all registered callbacks
         */
        void clear_callbacks()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_callbacks.clear();
        }

        /**
         * @brief Check if the timer is currently running
         * @return true if running, false otherwise
         */
        bool is_running() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_running;
        }

    private:
        /** @brief Static callback handler invoked by POSIX timer */
        static void timer_timeout_handler(union sigval sv)
        {
            timer* p_timer = static_cast<timer*>(sv.sival_ptr);
            if (p_timer)
            {
                p_timer->invoke_callbacks();

                bool should_stop = false;
                {
                    std::lock_guard<std::mutex> lock(p_timer->m_mutex);
                    if (p_timer->m_repeat_count == 0)
                    {
                        should_stop = true;
                    }
                    else if (p_timer->m_repeat_count > 0)
                    {
                        p_timer->m_repeat_counter++;
                        if (p_timer->m_repeat_counter >= p_timer->m_repeat_count)
                        {
                            should_stop = true;
                        }
                    }
                    else // repeat forever
                    {
                        // do nothing, just keep the timer running
                    }
                }

                if (should_stop)
                {
                    p_timer->stop();
                }
            }
        }

        void invoke_callbacks()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (const auto& func : m_callbacks)
            {
                func();
            }
        }

    private:
        timer_t m_timer_id;
        mutable std::mutex m_mutex;
        int64_t m_duration_ms;
        int64_t m_repeat_count;
        std::atomic<int64_t> m_repeat_counter;
        std::atomic<bool> m_running;
        std::vector<callables::task<>> m_callbacks; ///< List of callbacks
    };
}

/**
 * @brief Event handlers that execute callbacks conditionally or once
 */
namespace once_event
{
    /**
     * @brief Execute a callback exactly once during the lifetime of the object
     *
     * Thread-safe: uses std::call_once internally.
     * @note Requires linking with -pthread
     */
    class once_per_life
    {
    public:
        /** @brief Default constructor */
        once_per_life() = default;

        /** @brief Destructor */
        ~once_per_life() = default;

        /**
         * @brief Call a free function exactly once
         * @tparam Args Argument types
         * @param func Function to call
         * @param args Arguments to pass to the function
         */
        template <typename... Args>
        void call_once(callables::task<> func, Args&&... args)
        {
            std::call_once(m_flag, std::move(func), std::forward<Args>(args)...);
        }

        /**
         * @brief Call a member function exactly once
         * @tparam Cls Class type
         * @tparam Args Argument types
         * @param member_func Member function pointer
         * @param obj Pointer to the object
         * @param args Arguments to pass to the member function
         */
        template <typename Cls, typename... Args>
        void call_once(callables::task_member<Cls, Args...> member_func, Cls* obj, Args&&... args)
        {
            std::call_once(m_flag, [=]() mutable{
                (obj->*member_func)(std::forward<Args>(args)...);
            });
        }

    private:
        std::once_flag m_flag; ///< Flag for std::call_once
    };

    /**
     * @brief Execute a callback once every N invocations
     *
     * Thread-safe: uses atomic counter.
     */
    class once_per_n_times
    {
    public:
        /**
         * @brief Construct with the interval count
         * @param n Execute callback every n invocations
         */
        explicit once_per_n_times(std::uint64_t n)
            : counter_(0), n_(n)
        {
        }
        /** @brief Destructor */
        ~once_per_n_times() = default;

        /**
         * @brief Call a free function if the invocation count is due
         * @tparam Args Argument types
         * @param func Function to call
         * @param args Arguments to pass
         * @return true if the function was called, false otherwise
         */
        template <typename... Args>
        bool call_if_due(callables::task<Args...> func, Args&&... args)
        {
            uint32_t prev_count = counter_.fetch_add(1, std::memory_order_relaxed);
            if ((prev_count + 1) % n_ == 0)
            {
                std::forward<callables::task<Args...>>(std::move(func))(std::forward<Args>(args)...);
                return true;
            }
            return false;
        }

        /**
         * @brief Call a member function if the invocation count is due
         * @tparam Cls Class type
         * @tparam Args Argument types
         * @param member_func Member function pointer
         * @param obj Pointer to the object
         * @param args Arguments to pass
         * @return true if the function was called, false otherwise
         */
        template <typename Cls, typename... Args>
        bool call_if_due(callables::task_member<Cls, Args...> member_func, Cls* obj, Args&&... args)
        {
            uint32_t prev_count = counter_.fetch_add(1, std::memory_order_relaxed);
            if ((prev_count + 1) % n_ == 0)
            {
                (obj->*member_func)(std::forward<Args>(args)...);
                return true;
            }
            return false;
        }

        /**
         * @brief Reset the counter to zero
         */
        void reset()
        {
            counter_.store(0, std::memory_order_relaxed);
        }

    private:
        std::atomic<uint32_t> counter_; ///< Invocation counter
        const std::uint64_t n_; ///< Interval (every n invocations)
    };

    /**
     * @brief Execute a callback only when a value changes
     * @tparam T Value type (must support operator!=)
     *
     * Thread-safe: uses mutex for value comparison and storage.
     */
    template <typename T>
    class once_per_value
    {
    public:
        /** @brief Default constructor (no initial value) */
        once_per_value() = default;

        /**
         * @brief Construct with an initial value
         * @param initial_value Initial value to compare against
         */
        explicit once_per_value(const T& initial_value)
            : p_last_value_(new T(initial_value))
        {
        }
        /** @brief Destructor */
        ~once_per_value() = default;

        /**
         * @brief Call a free function if the value has changed
         * @tparam Args Argument types
         * @param new_value New value to compare against the last value
         * @param func Function to call if value changed
         * @param args Arguments to pass to the function
         * @return true if the value changed and function was called, false otherwise
         */
        template <typename... Args>
        bool on_value_change(const T& new_value, callables::task<Args...> func, Args&&... args)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!p_last_value_ || *p_last_value_ != new_value)
            {
                p_last_value_ = std::make_unique<T>(new_value);
                std::forward<callables::task<Args...>>(std::move(func))(std::forward<Args>(args)...);
                return true;
            }
            return false;
        }

        /**
         * @brief Call a member function if the value has changed
         * @tparam Cls Class type
         * @tparam Args Argument types
         * @param new_value New value to compare against the last value
         * @param member_func Member function pointer
         * @param obj Pointer to the object
         * @param args Arguments to pass
         * @return true if the value changed and function was called, false otherwise
         */
        template <typename Cls, typename... Args>
        bool on_value_change(const T& new_value, callables::task_member<Cls, Args...> member_func, Cls* obj, Args&&... args)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!p_last_value_ || *p_last_value_ != new_value)
            {
                p_last_value_ = std::make_unique<T>(new_value);
                (obj->*member_func)(std::forward<Args>(args)...);
                return true;
            }
            return false;
        }

        /**
         * @brief Reset the stored value (next comparison will trigger callback)
         */
        void reset()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            p_last_value_.reset();
        }

    private:
        std::unique_ptr<T> p_last_value_; ///< Last observed value
        mutable std::mutex mutex_; ///< Mutex for thread safety
    };

    class once_per_interval
    {
    public:
        explicit once_per_interval(uint64_t interval_ms)
            : interval_ms_(interval_ms),
              last_call_time_(std::chrono::steady_clock::now() - std::chrono::milliseconds(interval_ms))
        {
        }
        /** @brief Destructor */
        ~once_per_interval() = default;

        /**
         * @brief Call a free function if enough time has elapsed since last call
         * @tparam Args Argument types
         * @param func Function to call
         * @param args Arguments to pass
         * @return true if the function was called, false if interval has not elapsed
         */
        template <typename... Args>
        bool call(callables::task<Args...> func, Args&&... args)
        {
            auto now = std::chrono::steady_clock::now();

            std::lock_guard<std::mutex> lock(mutex_);
            uint64_t elapsed_ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now - last_call_time_).count());
            if (elapsed_ms >= interval_ms_)
            {
                last_call_time_ = now;
                std::forward<callables::task<Args...>>(std::move(func))(std::forward<Args>(args)...);
                return true;
            }
            return false;
        }

        /**
         * @brief Call a member function if enough time has elapsed
         * @tparam Cls Class type
         * @tparam Args Argument types
         * @param member_func Member function pointer
         * @param obj Pointer to the object
         * @param args Arguments to pass
         * @return true if the function was called, false if interval has not elapsed
         */
        template <typename Cls, typename... Args>
        bool call(callables::task_member<Cls, Args...> member_func, Cls* obj, Args&&... args)
        {
            auto now = std::chrono::steady_clock::now();

            std::lock_guard<std::mutex> lock(mutex_);
            uint64_t elapsed_ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now - last_call_time_).count());
            if (elapsed_ms >= interval_ms_)
            {
                last_call_time_ = now;
                (obj->*member_func)(std::forward<Args>(args)...);
                return true;
            }
            return false;
        }

        /**
         * @brief Reset the timer (next call will execute immediately)
         */
        void reset()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            last_call_time_ = std::chrono::steady_clock::now() - std::chrono::milliseconds(interval_ms_);
        }

    private:
        std::mutex mutex_; ///< Mutex for thread safety
        const uint64_t interval_ms_; ///< Minimum interval in milliseconds
        std::chrono::steady_clock::time_point last_call_time_; ///< Last invocation timestamp
    };

    /**
     * @brief Call a callback at regular intervals with latest value
     * @tparam T Value type
     *
     * Runs a background thread that invokes a callback at fixed intervals.
     * If multiple updates occur within an interval, only the latest value is passed.
     * Thread-safe: update() can be called from any thread.
     */
    template <typename T>
    class once_at_least : public i_stoppable
    {
    public:
        using clock = std::chrono::steady_clock; ///< Clock type
        using time_point = clock::time_point; ///< Time point type
        using duration = std::chrono::milliseconds; ///< Duration type
        using callback_t = callables::task<const T&>; ///< Callback type

        /**
         * @brief Construct and start the background thread
         * @param interval_ms Interval in milliseconds between callback invocations
         * @param callback Callback to invoke with the latest value
         */
        explicit once_at_least(uint64_t interval_ms, callback_t callback)
            : interval_(duration(interval_ms))
            , callback_(std::move(callback))
            , has_value_(false)
            , new_data_(false)
            , stop_flag_(false)
        {
            worker_thread_ = std::thread(&once_at_least::worker_loop, this);
        }

        /** @brief Copy constructor (deleted) */
        once_at_least(const once_at_least&) = delete;

        /** @brief Copy assignment (deleted) */
        once_at_least& operator=(const once_at_least&) = delete;

        /**
         * @brief Destructor stops the worker thread
         */
        ~once_at_least()
        {
            stop();
        }

        /**
         * @brief Stop the worker thread and wait for it to terminate
         */
        virtual void stop()
        {
            bool expected = false;
            if (stop_flag_.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
            {
                cond_var_.notify_all();
                if (worker_thread_.joinable())
                {
                    worker_thread_.join();
                }
            }
        }

        /**
         * @brief Update with a new value (copy)
         * @param new_value New value to be passed to callback on next interval
         */
        void update(const T& new_value)
        {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                last_value_ = std::make_unique<T>(new_value);
                has_value_ = true;
                new_data_ = true;
            }

            cond_var_.notify_all();
        }

        /**
         * @brief Update with a new value (move)
         * @param new_value New value to be moved and passed to callback on next interval
         */
        void update(T&& new_value)
        {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                last_value_ = std::make_unique<T>(std::move(new_value));
                has_value_ = true;
                new_data_ = true;
            }

            cond_var_.notify_all();
        }

    private:
        /**
         * @brief Worker thread loop
         *
         * Invokes the callback at regular intervals with the latest value
         * from update(). If no new value is provided within an interval,
         * the last value is replayed.
         */
        void worker_loop()
        {
            time_point next_deadline = clock::now() + interval_;

            while (!stop_flag_.load(std::memory_order_acquire))
            {
                std::unique_ptr<T> value_to_process;
                bool should_process = false;

                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    cond_var_.wait_until(lock, next_deadline, [this]() {
                        return stop_flag_.load(std::memory_order_acquire) || new_data_;
                    });

                    if (stop_flag_.load(std::memory_order_acquire))
                    {
                        break;
                    }

                    if (new_data_)
                    {
                        value_to_process = std::make_unique<T>(*last_value_);
                        new_data_ = false;
                        should_process = true;
                        next_deadline = clock::now() + interval_;
                    }
                    else // timeout occurred, no new data coming in
                    {
                        // replay last value if any
                        if (has_value_)
                        {
                            value_to_process = std::make_unique<T>(*last_value_);
                            should_process = true;
                            next_deadline += interval_;
                        }
                    }
                }

                if (should_process)
                {
                    callback_(*value_to_process);
                }
            }
        }

    private:
        const duration interval_;
        callback_t callback_;

        std::mutex mutex_;
        std::condition_variable cond_var_;

        std::unique_ptr<T> last_value_;
        bool has_value_;
        bool new_data_;

        std::atomic<bool> stop_flag_;
        std::thread worker_thread_;
    };
} // namespace once_event

/**
 * @brief Toggle events that trigger once and can be reset
 */
namespace toggle_event
{
    /**
     * @brief Toggle event that triggers callback only once when condition becomes true
     *
     * Provides one-shot trigger semantics with reset capability.
     * Thread-safe: all methods can be called from multiple threads.
     *
     * Usage pattern:
     * @code
     * if (precondition) {
     *     toggle.trigger_if_not_set(callback, args...);
     * } else {
     *     toggle.reset(); // Reset for next trigger
     * }
     * @endcode
     */
    class toggle_event
    {
    public:
        /**
         * @brief Construct a toggle in not-triggered state
         */
        toggle_event()
            : m_triggered(false)
        {
        }

        /** @brief Destructor */
        ~toggle_event() = default;

        /**
         * @brief Check if toggle has been triggered
         * @return true if triggered, false otherwise
         */
        bool is_triggered() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_triggered;
        }

        /**
         * @brief Reset the toggle state to allow future triggers
         */
        void reset()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_triggered = false;
            m_cond_var.notify_all();
        }

        /**
         * @brief Force set the toggle state without calling callback
         */
        void set()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_triggered = true;
            m_cond_var.notify_all();
        }

        /**
         * @brief Trigger callback if not already triggered
         * @tparam Args Argument types
         * @param func Function to call
         * @param args Arguments to pass
         * @return true if function was called (was not triggered), false otherwise
         */
        template <typename... Args>
        bool trigger_if_not_set(callables::task<Args...> func, Args&&... args)
        {
            bool ret = false;
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_triggered)
            {
                func(std::forward<Args>(args)...);
                m_triggered = true;
                ret = true;
            }
            m_cond_var.notify_all();
            return ret;
        }

        /**
         * @brief Trigger member function if not already triggered
         * @tparam Cls Class type
         * @tparam Args Argument types
         * @param member_func Member function pointer
         * @param obj Pointer to the object
         * @param args Arguments to pass
         * @return true if function was called (was not triggered), false otherwise
         */
        template <typename Cls, typename... Args>
        bool trigger_if_not_set(callables::task_member<Cls, Args...> member_func, Cls* obj, Args&&... args)
        {
            bool ret = false;
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_triggered)
            {
                (obj->*member_func)(std::forward<Args>(args)...);
                m_triggered = true;
                ret = true;
            }
            m_cond_var.notify_all();
            return ret;
        }

        /**
         * @brief Wait until toggle is triggered (blocking)
         */
        void wait()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond_var.wait(lock, [this]() { return m_triggered; });
        }

        /**
         * @brief Wait with timeout until toggle is triggered
         * @return true if triggered, false if timeout
         */
        bool wait_for(uint32_t timeout_ms)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            return m_cond_var.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() { return m_triggered; });
        }

        /**
         * @brief Wait until triggered, then execute callback
         * @tparam Args Argument types
         * @param func Function to call after trigger
         * @param args Arguments to pass
         */
        template <typename... Args>
        void wait_then(callables::task<Args...> func, Args&&... args)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond_var.wait(lock, [this]() { return m_triggered; });
            func(std::forward<Args>(args)...);
        }

        /**
         * @brief Wait until triggered, then execute callback
         * Overload for member functions
         */
        template <typename Cls, typename... Args>
        void wait_then(callables::task_member<Cls, Args...> member_func, Cls* obj, Args&&... args)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond_var.wait(lock, [this]() { return m_triggered; });
            (obj->*member_func)(std::forward<Args>(args)...);
        }

        /**
         * @brief Wait with timeout, then execute callback if triggered
         * @tparam Args Argument types
         * @param timeout_ms Timeout in milliseconds
         * @param func Function to call if triggered
         * @param args Arguments to pass
         * @return true if triggered and function was called, false on timeout
         */
        template <typename... Args>
        bool wait_for_then(uint32_t timeout_ms, callables::task<Args...> func, Args&&... args)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            bool triggered = m_cond_var.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() { return m_triggered; });
            if (triggered)
            {
                func(std::forward<Args>(args)...);
            }
            return triggered;
        }

        /**
         * @brief Wait with timeout, then execute member function if triggered
         * @tparam Cls Class type
         * @tparam Args Argument types
         * @param timeout_ms Timeout in milliseconds
         * @param member_func Member function pointer
         * @param obj Pointer to the object
         * @param args Arguments to pass
         * @return true if triggered and function was called, false on timeout
         */
        template <typename Cls, typename... Args>
        bool wait_for_then(uint32_t timeout_ms, callables::task_member<Cls, Args...> member_func, Cls* obj, Args&&... args)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            bool triggered = m_cond_var.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() { return m_triggered; });
            if (triggered)
            {
                (obj->*member_func)(std::forward<Args>(args)...);
            }
            return triggered;
        }

    private:
        mutable std::mutex m_mutex; ///< Mutex for thread safety
        bool m_triggered; ///< Triggered state
        std::condition_variable m_cond_var; ///< Condition variable for waiting
    };
} // namespace toggle_event

/**
 * @brief File descriptor event monitoring using poll()
 */
namespace fd_event
{
    /**
     * @brief Event types for file descriptor monitoring
     */
    enum class event_type
    {
        READ = POLLIN,              ///< Data available to read
        WRITE = POLLOUT,            ///< Ready for writing
        ERROR = POLLERR,            ///< Error condition
        HANGUP = POLLHUP,           ///< Hang up
        INVALID = POLLNVAL,         ///< Invalid request
        PRIORITY = POLLPRI,         ///< Priority data available
        READ_WRITE = POLLIN | POLLOUT  ///< Both read and write
    };

    /**
     * @brief Event callback function signature
     *
     * Callback receives file descriptor, returned events, and user data.
     * @note Parameters: fd (int), revents (short), user_data (void*)
     */
    using event_callback = callables::task<int, short, void*>;

    /**
     * @brief Structure to hold file descriptor information
     */
    struct fd_info
    {
        int fd;                    ///< File descriptor
        short events;              ///< Events to monitor (POLLIN, POLLOUT, etc.)
        event_callback callback;   ///< Callback function when event occurs
        void* user_data;           ///< User-defined data passed to callback
        std::string name;          ///< Descriptive name for logging
        bool enabled;              ///< Whether this FD is currently enabled

        /** @brief Default constructor */
        fd_info() : fd(-1), events(0), callback(nullptr), user_data(nullptr), name(""), enabled(true) {}

        /**
         * @brief Construct fd_info with parameters
         * @param fd_ File descriptor
         * @param events_ Events to monitor
         * @param cb Callback function
         * @param data User data (optional)
         * @param name_ Descriptive name (optional)
         */
        fd_info(int fd_, short events_, event_callback cb, void* data = nullptr, const std::string& name_ = "")
            : fd(fd_), events(events_), callback(std::move(cb)), user_data(data), name(name_), enabled(true) {}
    };

    /**
     * @brief File Descriptor Event Manager
     *
     * Manages multiple file descriptors and their associated events.
     * Uses poll() to monitor multiple FDs efficiently.
     * Thread-safe: all public methods are protected by mutex.
     */
    class fd_event_manager
    {
    public:
        /** @brief Construct an fd_event_manager */
        fd_event_manager() : modified_(false) {}

        /** @brief Destructor clears all file descriptors */
        ~fd_event_manager() { clear(); }

        /**
         * @brief Add a file descriptor to monitor
         * @param fd File descriptor
         * @param events Events to monitor (event_type or combination)
         * @param callback Function to call when event occurs
         * @param user_data Optional user data passed to callback
         * @param name Optional descriptive name for logging
         * @return true if added successfully, false otherwise
         */
        bool add_fd(int fd, short events, event_callback callback,
                    void* user_data = nullptr, const std::string& name = "")
        {
            if (fd < 0)
            {
                log_error("Invalid file descriptor: " + std::to_string(fd));
                return false;
            }

            if (!callback)
            {
                log_error("Null callback for fd: " + std::to_string(fd));
                return false;
            }

            std::lock_guard<std::mutex> lock(mutex_);

            // Check if fd already exists
            if (fd_map_.find(fd) != fd_map_.end())
            {
                log_error("File descriptor already registered: " + std::to_string(fd));
                return false;
            }

            // Add to map
            fd_info info(fd, events, std::move(callback), user_data, name);
            fd_map_[fd] = info;
            modified_ = true;

            return true;
        }

        /**
         * @brief Remove a file descriptor from monitoring
         * @param fd File descriptor to remove
         * @return true if removed successfully, false if not found
         */
        bool remove_fd(int fd)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = fd_map_.find(fd);
            if (it == fd_map_.end())
            {
                log_error("File descriptor not found: " + std::to_string(fd));
                return false;
            }

            fd_map_.erase(it);
            modified_ = true;

            return true;
        }

        /**
         * @brief Enable monitoring for a specific file descriptor
         * @param fd File descriptor to enable
         * @return true if enabled successfully, false if not found
         */
        bool enable_fd(int fd)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = fd_map_.find(fd);
            if (it == fd_map_.end())
            {
                log_error("File descriptor not found: " + std::to_string(fd));
                return false;
            }

            if (!it->second.enabled)
            {
                it->second.enabled = true;
                modified_ = true;
            }

            return true;
        }

        /**
         * @brief Disable monitoring for a specific file descriptor (without removing it)
         * @param fd File descriptor to disable
         * @return true if disabled successfully, false if not found
         */
        bool disable_fd(int fd)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = fd_map_.find(fd);
            if (it == fd_map_.end())
            {
                log_error("File descriptor not found: " + std::to_string(fd));
                return false;
            }

            if (it->second.enabled)
            {
                it->second.enabled = false;
                modified_ = true;
            }

            return true;
        }

        /**
         * @brief Modify events to monitor for a specific file descriptor
         * @param fd File descriptor
         * @param events New events to monitor
         * @return true if modified successfully, false if not found
         */
        bool modify_events(int fd, short events)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = fd_map_.find(fd);
            if (it == fd_map_.end())
            {
                log_error("File descriptor not found: " + std::to_string(fd));
                return false;
            }

            it->second.events = events;
            modified_ = true;

            return true;
        }

        /**
         * @brief Wait for events on registered file descriptors
         * @param timeout_ms Timeout in milliseconds (-1 for infinite)
         * @return Number of file descriptors with events, 0 on timeout, -1 on error
         */
        int wait(int timeout_ms = -1)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            // Rebuild poll_fds_ if modified
            if (modified_)
            {
                rebuild_poll_fds();
                modified_ = false;
            }

            if (poll_fds_.empty())
            {
                // No file descriptors to monitor
                return 0;
            }

            // Call poll
            int ret = poll(poll_fds_.data(), poll_fds_.size(), timeout_ms);

            if (ret < 0)
            {
                std::ostringstream oss;
                oss << "poll() failed: " << strerror(errno) << " (errno: " << errno << ")";
                log_error(oss.str());
                return -1;
            }

            return ret;
        }

        /**
         * @brief Process events and invoke callbacks
         * Should be called after wait() returns > 0
         */
        void process_events()
        {
            std::lock_guard<std::mutex> lock(mutex_);

            for (size_t i = 0; i < poll_fds_.size(); ++i)
            {
                if (poll_fds_[i].revents != 0)
                {
                    int fd = poll_fd_map_[i];
                    auto it = fd_map_.find(fd);

                    if (it != fd_map_.end() && it->second.callback)
                    {
                        // Invoke callback with fd, revents, and user_data
                        it->second.callback(fd, poll_fds_[i].revents, it->second.user_data);
                    }

                    // Clear revents for next iteration
                    poll_fds_[i].revents = 0;
                }
            }
        }

        /**
         * @brief Combined wait and process
         * @param timeout_ms Timeout in milliseconds
         * @return Number of events processed
         */
        int wait_and_process(int timeout_ms = -1)
        {
            int ret = wait(timeout_ms);

            if (ret > 0)
            {
                process_events();
            }

            return ret;
        }

        /**
         * @brief Get the number of registered file descriptors
         * @return Count of registered FDs
         */
        size_t get_fd_count() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return fd_map_.size();
        }

        /**
         * @brief Get the number of enabled file descriptors
         * @return Count of enabled FDs
         */
        size_t get_enabled_fd_count() const
        {
            std::lock_guard<std::mutex> lock(mutex_);

            size_t count = 0;
            for (const auto& pair : fd_map_)
            {
                if (pair.second.enabled)
                {
                    ++count;
                }
            }

            return count;
        }

        /**
         * @brief Check if a file descriptor is registered
         * @param fd File descriptor to check
         * @return true if registered, false otherwise
         */
        bool has_fd(int fd) const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return fd_map_.find(fd) != fd_map_.end();
        }

        /**
         * @brief Clear all registered file descriptors
         */
        void clear()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            fd_map_.clear();
            poll_fds_.clear();
            poll_fd_map_.clear();
            modified_ = false;
        }

        /**
         * @brief Get last error message
         * @return Error message string
         */
        std::string get_last_error() const
        {
            return last_error_;
        }

        /**
         * @brief Set a global error handler callback
         * @param handler Error handler function
         */
        void set_error_handler(std::function<void(const std::string&)> handler)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            error_handler_ = handler;
        }

    private:
        void rebuild_poll_fds()
        {
            poll_fds_.clear();
            poll_fd_map_.clear();

            for (const auto& pair : fd_map_)
            {
                if (pair.second.enabled)
                {
                    pollfd pfd;
                    pfd.fd = pair.second.fd;
                    pfd.events = pair.second.events;
                    pfd.revents = 0;

                    poll_fds_.push_back(pfd);
                    poll_fd_map_.push_back(pair.first);
                }
            }
        }

        void log_error(const std::string& error)
        {
            last_error_ = error;

            if (error_handler_)
            {
                error_handler_(error);
            }
        }

        std::map<int, fd_info> fd_map_;              // Map of fd -> fd_info
        std::vector<pollfd> poll_fds_;               // Array for poll()
        std::vector<int> poll_fd_map_;               // Map poll_fds_ index to fd
        mutable std::mutex mutex_;                   // Thread safety
        bool modified_;                              // Track if fd_map_ changed
        std::string last_error_;                     // Last error message
        std::function<void(const std::string&)> error_handler_;  // Error callback
    };

    /**
     * @brief Convert revents to human-readable string
     */
    inline std::string event_to_string(short revents)
    {
        std::ostringstream oss;
        bool first = true;

        auto append = [&](const char* name) {
            if (!first) oss << "|";
            oss << name;
            first = false;
        };

        if (revents & POLLIN)    append("POLLIN");
        if (revents & POLLOUT)   append("POLLOUT");
        if (revents & POLLPRI)   append("POLLPRI");
        if (revents & POLLERR)   append("POLLERR");
        if (revents & POLLHUP)   append("POLLHUP");
        if (revents & POLLNVAL)  append("POLLNVAL");

        return oss.str();
    }
} // namespace fd_event

/**
 * @brief POSIX signal handling utilities
 *
 * Provides wrappers for signal management, including setting handlers,
 * blocking/unblocking signals, and waiting for signals.
 */
namespace signal_event
{
    /**
     * @brief Set up a signal handler for the specified signal number
     * @param signum Signal number (e.g., SIGINT, SIGTERM)
     * @param handler Function to handle the signal
     * @return true on success, false on failure
     */
    inline bool set_signal_handler(int signum, callables::task_fptr<int> handler)
    {
        if (handler == nullptr)
        {
            return false;
        }

        struct sigaction sa;
        sa.sa_handler = handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        if (sigaction(signum, &sa, nullptr) == -1)
        {
            return false;
        }
        return true;
    }

    /**
     * @brief Set up a signal handler with extended information (siginfo_t)
     * @param signum Signal number
     * @param handler Function to handle the signal with extended info
     * @param flags Additional flags (SA_RESTART, SA_NODEFER, etc.)
     * @return true on success, false on failure
     */
    inline bool set_signal_handler_ex(int signum, callables::task_fptr<int, siginfo_t*, void*> handler, int flags = SA_SIGINFO)
    {
        if (handler == nullptr)
        {
            return false;
        }

        struct sigaction sa;
        sa.sa_sigaction = handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = flags;

        if (sigaction(signum, &sa, nullptr) == -1)
        {
            return false;
        }
        return true;
    }

    /**
     * @brief Reset the signal handler for the specified signal number to default
     * @param signum Signal number
     * @return true on success, false on failure
     */
    inline bool reset_signal_handler(int signum)
    {
        struct sigaction sa;
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(signum, &sa, nullptr) == -1)
        {
            return false;
        }
        return true;
    }

    /**
     * @brief Ignore a specific signal
     * @param signum Signal number
     * @return true on success, false on failure
     */
    inline bool ignore_signal(int signum)
    {
        struct sigaction sa;
        sa.sa_handler = SIG_IGN;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(signum, &sa, nullptr) == -1)
        {
            return false;
        }
        return true;
    }

    /**
     * @brief Block a specific signal
     * @param signum Signal number to block
     * @return true on success, false on failure
     */
    inline bool block_signal(int signum)
    {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, signum);
        if (sigprocmask(SIG_BLOCK, &set, nullptr) == -1)
        {
            return false;
        }
        return true;
    }

    /**
     * @brief Unblock a specific signal
     * @param signum Signal number to unblock
     * @return true on success, false on failure
     */
    inline bool unblock_signal(int signum)
    {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, signum);
        if (sigprocmask(SIG_UNBLOCK, &set, nullptr) == -1)
        {
            return false;
        }
        return true;
    }

    /**
     * @brief Block multiple signals
     * @param signals Vector of signal numbers to block
     * @return true on success, false on failure
     */
    inline bool block_signals(const std::vector<int>& signals)
    {
        sigset_t set;
        sigemptyset(&set);
        for (int sig : signals)
        {
            sigaddset(&set, sig);
        }
        if (sigprocmask(SIG_BLOCK, &set, nullptr) == -1)
        {
            return false;
        }
        return true;
    }

    /**
     * @brief Unblock multiple signals
     * @param signals Vector of signal numbers to unblock
     * @return true on success, false on failure
     */
    inline bool unblock_signals(const std::vector<int>& signals)
    {
        sigset_t set;
        sigemptyset(&set);
        for (int sig : signals)
        {
            sigaddset(&set, sig);
        }
        if (sigprocmask(SIG_UNBLOCK, &set, nullptr) == -1)
        {
            return false;
        }
        return true;
    }

    /**
     * @brief Check if a signal is currently blocked
     * @param signum Signal number to check
     * @return true if blocked, false otherwise
     */
    inline bool is_signal_blocked(int signum)
    {
        sigset_t set;
        if (sigprocmask(SIG_BLOCK, nullptr, &set) == -1)
        {
            return false;
        }
        return sigismember(&set, signum) == 1;
    }

    /**
     * @brief Check if a signal is pending
     * @param signum Signal number to check
     * @return true if pending, false otherwise
     */
    inline bool is_signal_pending(int signum)
    {
        sigset_t set;
        if (sigpending(&set) == -1)
        {
            return false;
        }
        return sigismember(&set, signum) == 1;
    }

    /**
     * @brief Send a signal to the current process
     * @param signum Signal number to send
     * @return true on success, false on failure
     */
    inline bool raise_signal(int signum)
    {
        return raise(signum) == 0;
    }

    /**
     * @brief Send a signal to a specific process
     * @param pid Process ID
     * @param signum Signal number to send
     * @return true on success, false on failure
     */
    inline bool send_signal(pid_t pid, int signum)
    {
        return kill(pid, signum) == 0;
    }

    /**
     * @brief Wait for any signal in the set
     * @param signals Set of signals to wait for
     * @param timeout_ms Timeout in milliseconds (-1 for infinite)
     * @return Signal number received, or -1 on error/timeout
     */
    inline int wait_for_signal(const std::vector<int>& signals, int timeout_ms = -1)
    {
        sigset_t set;
        sigemptyset(&set);
        for (int sig : signals)
        {
            sigaddset(&set, sig);
        }

        if (timeout_ms < 0)
        {
            int sig;
            if (sigwait(&set, &sig) == 0)
            {
                return sig;
            }
            return -1;
        }
        else
        {
            struct timespec timeout;
            timeout.tv_sec = timeout_ms / 1000;
            timeout.tv_nsec = (timeout_ms % 1000) * 1000000;

            siginfo_t info;
            int result = sigtimedwait(&set, &info, &timeout);
            if (result > 0)
            {
                return result;
            }
            return -1;
        }
    }

    /**
     * @brief Get signal name as string
     * @param signum Signal number
     * @return Signal name string
     */
    inline std::string get_signal_name(int signum)
    {
        switch (signum)
        {
            case SIGHUP:    return "SIGHUP";
            case SIGINT:    return "SIGINT";
            case SIGQUIT:   return "SIGQUIT";
            case SIGILL:    return "SIGILL";
            case SIGTRAP:   return "SIGTRAP";
            case SIGABRT:   return "SIGABRT";
            case SIGBUS:    return "SIGBUS";
            case SIGFPE:    return "SIGFPE";
            case SIGKILL:   return "SIGKILL";
            case SIGUSR1:   return "SIGUSR1";
            case SIGSEGV:   return "SIGSEGV";
            case SIGUSR2:   return "SIGUSR2";
            case SIGPIPE:   return "SIGPIPE";
            case SIGALRM:   return "SIGALRM";
            case SIGTERM:   return "SIGTERM";
            case SIGCHLD:   return "SIGCHLD";
            case SIGCONT:   return "SIGCONT";
            case SIGSTOP:   return "SIGSTOP";
            case SIGTSTP:   return "SIGTSTP";
            case SIGTTIN:   return "SIGTTIN";
            case SIGTTOU:   return "SIGTTOU";
            case SIGURG:    return "SIGURG";
            case SIGXCPU:   return "SIGXCPU";
            case SIGXFSZ:   return "SIGXFSZ";
            case SIGVTALRM: return "SIGVTALRM";
            case SIGPROF:   return "SIGPROF";
            case SIGWINCH:  return "SIGWINCH";
            case SIGIO:     return "SIGIO";
            case SIGPWR:    return "SIGPWR";
            case SIGSYS:    return "SIGSYS";
            default:
                return "UNKNOWN_" + std::to_string(signum);
        }
    }
} // namespace signal_event

/**
 * @brief File system event monitoring using inotify (Linux)
 *
 * Provides a high-level API similar to pyinotify for monitoring file system changes.
 * Uses Linux inotify to watch files and directories for modifications, creation, deletion, etc.
 */
namespace fs_event
{
    /**
     * @brief File system event types (similar to pyinotify)
     */
    enum class fs_event_type : uint32_t
    {
        NONE = 0,                                  ///< No event
        ACCESS = IN_ACCESS,                        ///< File was accessed (read)
        MODIFY = IN_MODIFY,                        ///< File was modified
        ATTRIB = IN_ATTRIB,                        ///< Metadata changed
        CLOSE_WRITE = IN_CLOSE_WRITE,             ///< Writable file was closed
        CLOSE_NOWRITE = IN_CLOSE_NOWRITE,         ///< Unwritable file closed
        OPEN = IN_OPEN,                           ///< File was opened
        MOVED_FROM = IN_MOVED_FROM,               ///< File moved out of watched dir
        MOVED_TO = IN_MOVED_TO,                   ///< File moved into watched dir
        CREATE = IN_CREATE,                       ///< File/directory created in watched dir
        DELETE = IN_DELETE,                       ///< File/directory deleted from watched dir
        DELETE_SELF = IN_DELETE_SELF,             ///< Watched file/directory was deleted
        MOVE_SELF = IN_MOVE_SELF,                 ///< Watched file/directory was moved

        // Convenience combinations
        ALL_EVENTS = IN_ALL_EVENTS,               ///< All events
        MOVE = IN_MOVE,                           ///< Moved (FROM or TO)
        CLOSE = IN_CLOSE,                         ///< Closed (WRITE or NOWRITE)

        // Special flags
        DONT_FOLLOW = IN_DONT_FOLLOW,             ///< Don't follow symlinks
        EXCL_UNLINK = IN_EXCL_UNLINK,             ///< Exclude events on unlinked objects
        MASK_ADD = IN_MASK_ADD,                   ///< Add events to watch mask
        ONESHOT = IN_ONESHOT,                     ///< Only report one event, then remove watch
        ONLYDIR = IN_ONLYDIR                      ///< Only watch if object is directory
    };

    /**
     * @brief Bitwise OR operator for fs_event_type
     */
    inline fs_event_type operator|(fs_event_type a, fs_event_type b)
    {
        return static_cast<fs_event_type>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    /**
     * @brief Bitwise OR assignment operator for fs_event_type
     */
    inline fs_event_type& operator|=(fs_event_type& a, fs_event_type b)
    {
        a = a | b;
        return a;
    }

    /**
     * @brief Bitwise AND operator for fs_event_type
     */
    inline fs_event_type operator&(fs_event_type a, fs_event_type b)
    {
        return static_cast<fs_event_type>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    /**
     * @brief Structure containing file system event information
     */
    struct fs_event_info
    {
        int wd;                      ///< Watch descriptor
        uint32_t mask;               ///< Event type mask
        uint32_t cookie;             ///< Cookie to synchronize events
        std::string pathname;        ///< Full path to the parent directory
        std::string name;            ///< File/directory name (relative to pathname)
        bool is_dir;                 ///< True if the event is for a directory

        /** @brief Default constructor */
        fs_event_info()
            : wd(-1), mask(0), cookie(0), pathname(""), name(""), is_dir(false)
        {
        }

        /**
         * @brief Get the full path (pathname + name)
         * @return Combined full path
         */
        std::string get_full_path() const
        {
            if (name.empty())
            {
                return pathname;
            }

            if (pathname.empty() || pathname.back() == '/')
            {
                return pathname + name;
            }

            return pathname + "/" + name;
        }

        /**
         * @brief Check if event matches a specific type
         * @param type Event type to check
         * @return true if event mask contains the specified type
         */
        bool is_event(fs_event_type type) const
        {
            return (mask & static_cast<uint32_t>(type)) != 0;
        }

        /**
         * @brief Convert event mask to human-readable string
         * @return String representation of event types
         */
        std::string event_to_string() const
        {
            std::ostringstream oss;
            bool first = true;

            auto append = [&](const char* name) {
                if (!first) oss << "|";
                oss << name;
                first = false;
            };

            if (mask & IN_ACCESS)        append("ACCESS");
            if (mask & IN_MODIFY)        append("MODIFY");
            if (mask & IN_ATTRIB)        append("ATTRIB");
            if (mask & IN_CLOSE_WRITE)   append("CLOSE_WRITE");
            if (mask & IN_CLOSE_NOWRITE) append("CLOSE_NOWRITE");
            if (mask & IN_OPEN)          append("OPEN");
            if (mask & IN_MOVED_FROM)    append("MOVED_FROM");
            if (mask & IN_MOVED_TO)      append("MOVED_TO");
            if (mask & IN_CREATE)        append("CREATE");
            if (mask & IN_DELETE)        append("DELETE");
            if (mask & IN_DELETE_SELF)   append("DELETE_SELF");
            if (mask & IN_MOVE_SELF)     append("MOVE_SELF");
            if (mask & IN_ISDIR)         append("ISDIR");

            return oss.str();
        }
    };

    /**
     * @brief Base class for event handlers (similar to pyinotify.ProcessEvent)
     *
     * Inherit from this class and override process_* methods to handle specific events.
     * The dispatch_event() method routes events to appropriate handlers.
     */
    class process_event
    {
    public:
        /** @brief Virtual destructor */
        virtual ~process_event() = default;

        /**
         * @brief Called when any event occurs (default handler)
         * @param event Event information
         *
         * Override specific process_* methods for targeted handling.
         * This catch-all handler is called by all specific handlers by default.
         */
        virtual void process_default(const fs_event_info& event)
        {
            // Default implementation does nothing
            // User can override this for catch-all handling
            (void)event; // Suppress unused parameter warning
        }

        // Specific event handlers (similar to pyinotify)
        /** @brief Handle file access event */
        virtual void process_IN_ACCESS(const fs_event_info& event) { process_default(event); }
        /** @brief Handle file modification event */
        virtual void process_IN_MODIFY(const fs_event_info& event) { process_default(event); }
        /** @brief Handle metadata change event */
        virtual void process_IN_ATTRIB(const fs_event_info& event) { process_default(event); }
        /** @brief Handle close-write event */
        virtual void process_IN_CLOSE_WRITE(const fs_event_info& event) { process_default(event); }
        /** @brief Handle close-nowrite event */
        virtual void process_IN_CLOSE_NOWRITE(const fs_event_info& event) { process_default(event); }
        /** @brief Handle file open event */
        virtual void process_IN_OPEN(const fs_event_info& event) { process_default(event); }
        /** @brief Handle moved-from event */
        virtual void process_IN_MOVED_FROM(const fs_event_info& event) { process_default(event); }
        /** @brief Handle moved-to event */
        virtual void process_IN_MOVED_TO(const fs_event_info& event) { process_default(event); }
        /** @brief Handle create event */
        virtual void process_IN_CREATE(const fs_event_info& event) { process_default(event); }
        /** @brief Handle delete event */
        virtual void process_IN_DELETE(const fs_event_info& event) { process_default(event); }
        /** @brief Handle delete-self event */
        virtual void process_IN_DELETE_SELF(const fs_event_info& event) { process_default(event); }
        /** @brief Handle move-self event */
        virtual void process_IN_MOVE_SELF(const fs_event_info& event) { process_default(event); }

        /**
         * @brief Dispatch event to appropriate handler method
         * @param event Event information
         */
        void dispatch_event(const fs_event_info& event)
        {
            if (event.mask & IN_ACCESS)        process_IN_ACCESS(event);
            if (event.mask & IN_MODIFY)        process_IN_MODIFY(event);
            if (event.mask & IN_ATTRIB)        process_IN_ATTRIB(event);
            if (event.mask & IN_CLOSE_WRITE)   process_IN_CLOSE_WRITE(event);
            if (event.mask & IN_CLOSE_NOWRITE) process_IN_CLOSE_NOWRITE(event);
            if (event.mask & IN_OPEN)          process_IN_OPEN(event);
            if (event.mask & IN_MOVED_FROM)    process_IN_MOVED_FROM(event);
            if (event.mask & IN_MOVED_TO)      process_IN_MOVED_TO(event);
            if (event.mask & IN_CREATE)        process_IN_CREATE(event);
            if (event.mask & IN_DELETE)        process_IN_DELETE(event);
            if (event.mask & IN_DELETE_SELF)   process_IN_DELETE_SELF(event);
            if (event.mask & IN_MOVE_SELF)     process_IN_MOVE_SELF(event);
        }
    };

    /**
     * @brief Watch Manager (similar to pyinotify.WatchManager)
     *
     * Manages file system watches using inotify.
     * Watches can be added for individual files or directories, with recursive support.
     * Thread-safe: all public methods are protected by mutex.
     */
    class watch_manager
    {
    public:
        /**
         * @brief Construct a watch manager and initialize inotify
         * @throws std::runtime_error if inotify initialization fails
         */
        watch_manager()
            : inotify_fd_(-1)
        {
            inotify_fd_ = inotify_init1(IN_NONBLOCK);
            if (inotify_fd_ < 0)
            {
                throw std::runtime_error("Failed to initialize inotify: " + std::string(strerror(errno)));
            }
        }

        /**
         * @brief Destructor removes all watches and closes inotify fd
         */
        ~watch_manager()
        {
            if (inotify_fd_ >= 0)
            {
                // Remove all watches
                for (const auto& pair : wd_to_path_)
                {
                    inotify_rm_watch(inotify_fd_, pair.first);
                }
                close(inotify_fd_);
            }
        }

        /**
         * @brief Add a watch for a path
         * @param path Path to watch (file or directory)
         * @param mask Event mask (fs_event_type)
         * @param recursive If true and path is directory, watch subdirectories too
         * @return Watch descriptor on success, -1 on failure
         */
        int add_watch(const std::string& path, fs_event_type mask, bool recursive = false)
        {
            return add_watch(path, static_cast<uint32_t>(mask), recursive);
        }

        /**
         * @brief Add a watch for a path
         * @param path Path to watch (file or directory)
         * @param mask Event mask (uint32_t)
         * @param recursive If true and path is directory, watch subdirectories too
         * @return Watch descriptor on success, -1 on failure
         */
        int add_watch(const std::string& path, uint32_t mask, bool recursive = false)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            // Check if path already watched
            auto it = path_to_wd_.find(path);
            if (it != path_to_wd_.end())
            {
                // Path already watched, update the mask
                int wd = it->second;
                if (inotify_add_watch(inotify_fd_, path.c_str(), mask | IN_MASK_ADD) < 0)
                {
                    last_error_ = std::string("Failed to update watch for ") + path + ": " + strerror(errno);
                    return -1;
                }
                return wd;
            }

            // Add new watch
            int wd = inotify_add_watch(inotify_fd_, path.c_str(), mask);
            if (wd < 0)
            {
                last_error_ = std::string("Failed to add watch for ") + path + ": " + strerror(errno);
                return -1;
            }

            // Store mappings
            wd_to_path_[wd] = path;
            path_to_wd_[path] = wd;

            // If recursive and path is a directory, add watches for subdirectories
            if (recursive)
            {
                struct stat st;
                if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
                {
                    add_recursive_watches(path, mask);
                }
            }

            return wd;
        }

        /**
         * @brief Remove a watch by watch descriptor
         * @param wd Watch descriptor
         * @return true on success, false on failure
         */
        bool remove_watch(int wd)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = wd_to_path_.find(wd);
            if (it == wd_to_path_.end())
            {
                last_error_ = "Watch descriptor not found: " + std::to_string(wd);
                return false;
            }

            if (inotify_rm_watch(inotify_fd_, wd) < 0)
            {
                last_error_ = std::string("Failed to remove watch: ") + strerror(errno);
                return false;
            }

            std::string path = it->second;
            wd_to_path_.erase(it);
            path_to_wd_.erase(path);

            return true;
        }

        /**
         * @brief Remove a watch by path
         * @param path Path to stop watching
         * @return true on success, false on failure
         */
        bool remove_watch(const std::string& path)
        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = path_to_wd_.find(path);
            if (it == path_to_wd_.end())
            {
                last_error_ = "Path not watched: " + path;
                return false;
            }

            int wd = it->second;
            if (inotify_rm_watch(inotify_fd_, wd) < 0)
            {
                last_error_ = std::string("Failed to remove watch: ") + strerror(errno);
                return false;
            }

            path_to_wd_.erase(it);
            wd_to_path_.erase(wd);

            return true;
        }

        /**
         * @brief Get the inotify file descriptor
         */
        int get_fd() const
        {
            return inotify_fd_;
        }

        /**
         * @brief Get the path for a watch descriptor
         */
        std::string get_path(int wd) const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = wd_to_path_.find(wd);
            return (it != wd_to_path_.end()) ? it->second : "";
        }

        /**
         * @brief Get the watch descriptor for a path
         */
        int get_wd(const std::string& path) const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = path_to_wd_.find(path);
            return (it != path_to_wd_.end()) ? it->second : -1;
        }

        /**
         * @brief Get the number of active watches
         */
        size_t get_watch_count() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return wd_to_path_.size();
        }

        /**
         * @brief Get last error message
         */
        std::string get_last_error() const
        {
            return last_error_;
        }

    private:
        void add_recursive_watches(const std::string& dir_path, uint32_t mask)
        {
            DIR* dir = opendir(dir_path.c_str());
            if (!dir)
            {
                return;
            }

            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr)
            {
                // Skip . and ..
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                {
                    continue;
                }

                std::string full_path = dir_path;
                if (full_path.back() != '/')
                {
                    full_path += "/";
                }
                full_path += entry->d_name;

                struct stat st;
                if (stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
                {
                    // Add watch for subdirectory
                    int wd = inotify_add_watch(inotify_fd_, full_path.c_str(), mask);
                    if (wd >= 0)
                    {
                        wd_to_path_[wd] = full_path;
                        path_to_wd_[full_path] = wd;

                        // Recursively add watches for nested directories
                        add_recursive_watches(full_path, mask);
                    }
                }
            }

            closedir(dir);
        }

    private:
        int inotify_fd_;
        std::map<int, std::string> wd_to_path_;  // Watch descriptor to path mapping
        std::map<std::string, int> path_to_wd_;  // Path to watch descriptor mapping
        mutable std::mutex mutex_;
        std::string last_error_;
    };

    /**
     * @brief Notifier class (similar to pyinotify.Notifier)
     *
     * Reads events from watch manager and dispatches to event handler.
     * Provides both blocking loop() and non-blocking process_events() modes.
     */
    class notifier : public i_stoppable
    {
    public:
        /**
         * @brief Construct a notifier
         * @param wm Shared pointer to watch manager
         * @param handler Shared pointer to process_event handler
         * @throws std::invalid_argument if wm or handler is null
         */
        notifier(std::shared_ptr<watch_manager> wm, std::shared_ptr<process_event> handler)
            : wm_(wm)
            , handler_(handler)
            , running_(false)
        {
            if (!wm_)
            {
                throw std::invalid_argument("watch_manager cannot be null");
            }
            if (!handler_)
            {
                throw std::invalid_argument("process_event handler cannot be null");
            }
        }

        /**
         * @brief Destructor stops the notifier
         */
        ~notifier()
        {
            stop();
        }

        /**
         * @brief Start the event loop (blocking)
         *
         * Continuously reads and processes events until stop() is called.
         * Polls with 100ms timeout to check for stop condition.
         */
        void loop()
        {
            running_ = true;

            const size_t EVENT_SIZE = sizeof(struct inotify_event);
            const size_t BUF_LEN = 1024 * (EVENT_SIZE + 16);
            char buffer[BUF_LEN];

            int fd = wm_->get_fd();

            while (running_)
            {
                struct pollfd pfd;
                pfd.fd = fd;
                pfd.events = POLLIN;
                pfd.revents = 0;

                int ret = poll(&pfd, 1, 100); // 100ms timeout

                if (ret < 0)
                {
                    if (errno == EINTR)
                    {
                        continue; // Interrupted by signal, retry
                    }
                    last_error_ = std::string("poll() failed: ") + strerror(errno);
                    break;
                }

                if (ret == 0)
                {
                    // Timeout, continue loop
                    continue;
                }

                // Read events
                ssize_t length = read(fd, buffer, BUF_LEN);
                if (length < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        continue; // No data available
                    }
                    last_error_ = std::string("read() failed: ") + strerror(errno);
                    break;
                }

                // Process events
                size_t i = 0;
                while (i < static_cast<size_t>(length))
                {
                    struct inotify_event* event = reinterpret_cast<struct inotify_event*>(&buffer[i]);

                    // Create fs_event_info
                    fs_event_info event_info;
                    event_info.wd = event->wd;
                    event_info.mask = event->mask;
                    event_info.cookie = event->cookie;
                    event_info.pathname = wm_->get_path(event->wd);
                    event_info.is_dir = (event->mask & IN_ISDIR) != 0;

                    if (event->len > 0)
                    {
                        event_info.name = std::string(event->name);
                    }

                    // Dispatch to handler
                    if (handler_)
                    {
                        handler_->dispatch_event(event_info);
                    }

                    i += EVENT_SIZE + event->len;
                }
            }
        }

        /**
         * @brief Process events once (non-blocking)
         * Reads available events and processes them, then returns
         * @param timeout_ms Timeout in milliseconds (-1 for non-blocking check)
         * @return Number of events processed, -1 on error
         */
        int process_events(int timeout_ms = 0)
        {
            const size_t EVENT_SIZE = sizeof(struct inotify_event);
            const size_t BUF_LEN = 1024 * (EVENT_SIZE + 16);
            char buffer[BUF_LEN];

            int fd = wm_->get_fd();

            struct pollfd pfd;
            pfd.fd = fd;
            pfd.events = POLLIN;
            pfd.revents = 0;

            int ret = poll(&pfd, 1, timeout_ms);

            if (ret < 0)
            {
                last_error_ = std::string("poll() failed: ") + strerror(errno);
                return -1;
            }

            if (ret == 0)
            {
                // Timeout, no events
                return 0;
            }

            // Read events
            ssize_t length = read(fd, buffer, BUF_LEN);
            if (length < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    return 0; // No data available
                }
                last_error_ = std::string("read() failed: ") + strerror(errno);
                return -1;
            }

            // Process events
            int event_count = 0;
            size_t i = 0;
            while (i < static_cast<size_t>(length))
            {
                struct inotify_event* event = reinterpret_cast<struct inotify_event*>(&buffer[i]);

                // Create fs_event_info
                fs_event_info event_info;
                event_info.wd = event->wd;
                event_info.mask = event->mask;
                event_info.cookie = event->cookie;
                event_info.pathname = wm_->get_path(event->wd);
                event_info.is_dir = (event->mask & IN_ISDIR) != 0;

                if (event->len > 0)
                {
                    event_info.name = std::string(event->name);
                }

                // Dispatch to handler
                if (handler_)
                {
                    handler_->dispatch_event(event_info);
                }

                event_count++;
                i += EVENT_SIZE + event->len;
            }

            return event_count;
        }

        /**
         * @brief Stop the event loop
         */
        virtual void stop()
        {
            running_ = false;
        }

        /**
         * @brief Check if notifier is running
         * @return true if running, false otherwise
         */
        bool is_running() const
        {
            return running_;
        }

        /**
         * @brief Get last error message
         * @return Error message string
         */
        std::string get_last_error() const
        {
            return last_error_;
        }

    private:
        std::shared_ptr<watch_manager> wm_; ///< Watch manager
        std::shared_ptr<process_event> handler_; ///< Event handler
        std::atomic<bool> running_; ///< Running state
        std::string last_error_; ///< Last error message
    };
} // namespace fs_event

#endif // LIB_FOR_EVENT_DRIVEN_PROGRAMMING
