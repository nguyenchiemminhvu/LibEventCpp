/**
 * libevent.h
 *
 * This library provides a framework for event-driven programming in C++.
 * It includes classes for handling messages, queues, and signals/slots.
 * The library is designed to facilitate the development of applications
 * that require asynchronous message handling and event-driven architecture.
 *
 * Copyright © [nguyenchiemminhvu] [2025]. All Rights Reserved.
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
 * 1.0 - [20250117]
 */

#ifndef LIB_FOR_EVENT_DRIVEN_PROGRAMMING
#define LIB_FOR_EVENT_DRIVEN_PROGRAMMING

#if defined(unix) || defined(__unix__) || defined(__unix) || defined(__linux__)
#include <unistd.h>
#include <signal.h>
#endif

#include <type_traits>
#include <cstdint>
#include <utility>
#include <algorithm>
#include <vector>
#include <string>
#include <cstring>
#include <set>
#include <list>
#include <queue>
#include <sstream>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <tuple>
#include <chrono>
#include <thread>
#include <sched.h>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <ctime>
#include <stdexcept>
#include <errno.h>
#include <sys/poll.h>

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

namespace event_handler
{
    using steady_timestamp_t = std::chrono::steady_clock::time_point;

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

    class i_stoppable
    {
    public:
        virtual void stop() = 0;
    };

    class i_message
    {
    public:
        i_message() = default;
        virtual ~i_message() {}
        virtual void execute() = 0;

        steady_timestamp_t get_timestamp() const
        {
            return m_timestamp;
        }

        bool operator<(const i_message& other) const
        {
            return m_timestamp > other.m_timestamp;
        }

    protected:
        steady_timestamp_t m_timestamp;
    };

    template <class Handler, typename... Args>
    class event_message : public i_message
    {
    public:
        event_message(std::shared_ptr<Handler> handler, void(Handler::*act)(Args...), Args... args)
            : m_handler(handler), m_act(act), m_args(std::make_tuple(args...))
        {
            i_message::m_timestamp = std::chrono::steady_clock::now();
        }

        event_message(uint64_t delay_ms, std::shared_ptr<Handler> handler, void(Handler::*act)(Args...), Args... args)
            : m_handler(handler), m_act(act), m_args(std::make_tuple(args...))
        {
            i_message::m_timestamp = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
        }

        virtual ~event_message() {}

        static std::shared_ptr<i_message> create(std::shared_ptr<Handler> handler, void(Handler::*act)(Args...), Args... args)
        {
            std::shared_ptr<i_message> mess = std::make_shared<event_message<Handler, Args...>>(handler, act, args...);
            return mess;
        }

        static std::shared_ptr<i_message> create(uint64_t delay_ms, std::shared_ptr<Handler> handler, void(Handler::*act)(Args...), Args... args)
        {
            std::shared_ptr<i_message> mess = std::make_shared<event_message<Handler, Args...>>(delay_ms, handler, act, args...);
            return mess;
        }

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
        std::shared_ptr<Handler> m_handler;
        void(Handler::*m_act)(Args...);
        std::tuple<Args...> m_args;

        static_assert(std::is_class<Handler>::value, "Handler must be a class type");
        static_assert(std::is_member_function_pointer<decltype(m_act)>::value, "m_act must be a member function pointer");
    };

    class message_queue : public i_stoppable
    {
    public:
        message_queue()
            : m_running(true)
        {

        }

        virtual ~message_queue()
        {
            this->stop();
        }

        void enqueue(std::shared_ptr<i_message> mess)
        {
            bool enqueued = false;
            {
                std::lock_guard<std::mutex> lock(m_mut);
                if (m_running)
                {
                    m_queue.push(std::move(mess));
                    enqueued = true;
                }
            }

            if (enqueued)
            {
                m_cond.notify_all();
            }
        }

        std::shared_ptr<i_message> poll()
        {
            std::unique_lock<std::mutex> lock(m_mut);
            m_cond.wait(lock, [this]() { return !this->m_queue.empty() || !this->m_running; });

            if (!this->m_running)
            {
                return nullptr;
            }

            std::shared_ptr<i_message> mess = m_queue.top();
            m_queue.pop();

            steady_timestamp_t current_timestamp = std::chrono::steady_clock::now();
            std::chrono::duration<double, std::milli> duration_ms = mess->get_timestamp() - current_timestamp;
            if (duration_ms.count() > 0)
            {
                auto wait_duration = std::chrono::milliseconds(static_cast<int>(duration_ms.count()));
                std::cv_status wait_rc = m_cond.wait_for(lock, wait_duration);
                if (wait_rc == std::cv_status::no_timeout)
                {
                    // The wait was notified before the timeout duration expired.
                    lock.unlock(); // unlock so that enqueue can push new message to the queue without dead lock
                    this->enqueue(mess);
                    mess = nullptr;
                }
            }

            return mess;
        }

        virtual void stop()
        {
            bool state_changed = false;
            {
                std::lock_guard<std::mutex> lock(m_mut);
                if (m_running)
                {
                    m_running = false;
                    state_changed = true;
                    m_queue = std::priority_queue<std::shared_ptr<i_message>, std::vector<std::shared_ptr<i_message>>, message_comparator>();
                }
            }

            if (state_changed)
            {
                m_cond.notify_all();
            }
        }

    private:
        struct message_comparator
        {
            bool operator()(const std::shared_ptr<i_message>& lhs, const std::shared_ptr<i_message>& rhs)
            {
                return *lhs < *rhs;
            }
        };

    private:
        std::atomic<bool> m_running;
        std::priority_queue<std::shared_ptr<i_message>, std::vector<std::shared_ptr<i_message>>, message_comparator> m_queue;
        std::mutex m_mut;
        std::condition_variable m_cond;
    };

    class message_looper : public i_stoppable
    {
    public:
        message_looper()
            : m_running(true)
        {
            m_message_queue = std::make_shared<message_queue>();
            m_looper_thread = std::thread(&message_looper::looper, this);
        }

        virtual ~message_looper()
        {
            this->stop();
        }

        std::shared_ptr<message_queue> get_message_queue()
        {
            return m_message_queue;
        }

        virtual void stop()
        {
            bool state_changed = false;
            {
                std::lock_guard<std::mutex> lock(m_mut);
                if (m_running)
                {
                    m_running = false;
                    state_changed = true;
                    m_message_queue->stop();
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

                std::shared_ptr<i_message> mess = nullptr;
                if (m_message_queue != nullptr)
                {
                    mess = m_message_queue->poll();
                }

                if (mess != nullptr)
                {
                    mess->execute();
                }
            }
        }

    private:
        std::atomic<bool> m_running;
        std::thread m_looper_thread;
        std::shared_ptr<message_queue> m_message_queue;
        std::mutex m_mut;
    };

    class message_handler : public i_stoppable, public std::enable_shared_from_this<message_handler>
    {
    public:
        message_handler()
        {
            m_looper = std::make_shared<message_looper>();
            m_message_queue = m_looper->get_message_queue();
        }

        message_handler(std::shared_ptr<message_looper> looper)
            : m_looper(looper)
        {
            if (looper)
            {
                m_message_queue = looper->get_message_queue();
            }
        }

        virtual ~message_handler()
        {
            this->stop();
        }

        /**
         * Highly recommend to bind looper on initializattion phase only.
         */
        void bind_looper(std::shared_ptr<message_looper> looper)
        {
            std::lock_guard<std::mutex> lock(m_mut);
            if (looper)
            {
                m_looper = looper;
                m_message_queue = looper->get_message_queue();
            }
        }

        template<typename T, typename... Args>
        void post_message(void (T::*func)(typename convert_arg<Args>::type...), Args... args)
        {
            static_assert(std::is_base_of<message_handler, T>::value, "T must be derived from message_handler");

            auto shared_this = this->get_shared_ptr();
            if (shared_this)
            {
                std::shared_ptr<i_message> mess = event_message<T, typename convert_arg<Args>::type...>::create(std::dynamic_pointer_cast<T>(shared_this), func, std::forward<typename convert_arg<Args>::type>(args)...);
                std::lock_guard<std::mutex> lock(m_mut);
                if (m_message_queue != nullptr)
                {
                    m_message_queue->enqueue(mess);
                }
            }
        }

        template<typename T, typename... Args>
        void post_delayed_message(uint64_t delay_ms, void (T::*func)(typename convert_arg<Args>::type...), Args... args)
        {
            static_assert(std::is_base_of<message_handler, T>::value, "T must be derived from message_handler");

            auto shared_this = this->get_shared_ptr();
            if (shared_this)
            {
                std::shared_ptr<i_message> mess = event_message<T, typename convert_arg<Args>::type...>::create(delay_ms, std::dynamic_pointer_cast<T>(shared_this), func, std::forward<typename convert_arg<Args>::type>(args)...);
                std::lock_guard<std::mutex> lock(m_mut);
                if (m_message_queue != nullptr)
                {
                    m_message_queue->enqueue(mess);
                }
            }
        }

        template<typename T, typename... Args>
        void post_repeated_message(std::size_t times, uint32_t duration_ms, void (T::*func)(typename convert_arg<Args>::type...), Args... args)
        {
            static_assert(std::is_base_of<message_handler, T>::value, "T must be derived from message_handler");

            for (std::size_t i = 0U; i < times; ++i)
            {
                uint64_t delay_ms = (duration_ms * i);
                this->post_delayed_message(delay_ms, func, std::forward<typename convert_arg<Args>::type>(args)...);
            }
        }

        virtual void stop()
        {
            if (m_looper)
            {
                m_looper->stop();
            }
        }

    protected:
        std::shared_ptr<message_handler> get_shared_ptr()
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
        std::shared_ptr<message_looper> m_looper;
        std::shared_ptr<message_queue> m_message_queue;
        std::mutex m_mut;
    };
} // namespace event_handler

namespace sigslot
{
    class base_slot;

    class base_signal
    {
    public:
        virtual void disconnect(base_slot* p_slot) = 0;
        virtual void disconnect_all() = 0;
    };

    template <typename... Args>
    class base_connection
    {
    public:
        base_connection() = default;
        virtual ~base_connection() = default;

        virtual base_slot* get_slot_obj() = 0;
        virtual void emit(Args... args) = 0;
    };

    class base_slot
    {
    public:
        base_slot() = default;
        virtual ~base_slot()
        {
            this->disconnect_all();
        }

        void connect(base_signal* p_signal_obj)
        {
            std::lock_guard<std::mutex> lock(m_mut);
            m_signal_objs.insert(p_signal_obj);
        }

        void disconnect(base_signal* p_signal_obj)
        {
            std::unique_lock<std::mutex> lock(m_mut);
            if (m_signal_objs.erase(p_signal_obj) > 0)
            {
                lock.unlock();
                p_signal_obj->disconnect(this);
            }
        }

        void disconnect_all()
        {
            std::unique_lock<std::mutex> lock(m_mut);

            std::set<base_signal*> signals_to_disconnect;
            for (auto p_signal_obj : m_signal_objs)
            {
                if (p_signal_obj != nullptr)
                {
                    signals_to_disconnect.insert(p_signal_obj);
                }
            }

            m_signal_objs.clear();
            lock.unlock();

            for (auto p_signal_obj : signals_to_disconnect)
            {
                p_signal_obj->disconnect(this);
            }
        }

    private:
        std::mutex m_mut;
        std::set<base_signal*> m_signal_objs;
    };

    template <class T, typename... Args>
    class sigslot_connection : public base_connection<Args...>
    {
    public:
        sigslot_connection()
        {
            m_target = nullptr;
            m_func = nullptr;
        }

        sigslot_connection(T* target, void(T::*member_func)(Args...))
        {
            m_target = target;
            m_func = member_func;
        }

        virtual ~sigslot_connection() = default;

        virtual base_slot* get_slot_obj() override
        {
            return m_target;
        }

        virtual void emit(Args... args) override
        {
            (m_target->*m_func)(std::forward<Args>(args)...);
        }

    private:
        T* m_target;
        void(T::*m_func)(Args...);
    };

    template <typename... Args>
    class callable_connection : public base_connection<Args...>
    {
    public:
        callable_connection()
        {
            m_func = nullptr;
        }

        callable_connection(std::function<void(Args...)> func)
        {
            m_func = std::move(func);
        }

        virtual base_slot* get_slot_obj() override
        {
            return nullptr;
        }

        virtual void emit(Args... args) override
        {
            if (m_func)
            {
                m_func(std::forward<Args>(args)...);
            }
        }

    private:
        std::function<void(Args...)> m_func;
    };

    template <typename... Args>
    class signal : public base_signal
    {
    public:
        signal()
            : base_signal()
        {
        }

        virtual ~signal()
        {
            this->disconnect_all();
        }

        template <class T>
        void connect(T* p_slot, void(T::*member_func)(Args...))
        {
            std::lock_guard<std::mutex> lock(m_mut);

            std::shared_ptr<base_connection<Args...>> p_conn = std::make_shared<sigslot_connection<T, Args...>>(p_slot, member_func);
            m_connections.push_back(p_conn);
            ((base_slot*)(p_slot))->connect(this);
        }

        void connect(std::function<void(Args...)> func)
        {
            std::lock_guard<std::mutex> lock(m_mut);

            std::shared_ptr<base_connection<Args...>> p_conn = std::make_shared<callable_connection<Args...>>(func);
            m_connections.push_back(p_conn);
        }

        virtual void disconnect(base_slot* p_slot) override
        {
            std::unique_lock<std::mutex> lock(m_mut);
            typename std::list<std::shared_ptr<base_connection<Args...>>>::iterator rem_it = std::remove_if(
                m_connections.begin(),
                m_connections.end(),
                [&](std::shared_ptr<base_connection<Args...>> p_conn)
                {
                    return p_conn->get_slot_obj() == p_slot;
                }
            );
            if (rem_it != m_connections.end())
            {
                m_connections.erase(rem_it, m_connections.end());
                lock.unlock();

                p_slot->disconnect(this);
            }
        }

        virtual void disconnect_all() override
        {
            std::unique_lock<std::mutex> lock(m_mut);

            std::set<base_slot*> slots_to_disconnect;
            for (auto& p_conn : m_connections)
            {
                base_slot* p_slot = p_conn->get_slot_obj();
                if (p_slot != nullptr)
                {
                    slots_to_disconnect.insert(p_slot);
                }
            }

            m_connections.clear();
            lock.unlock();

            for (base_slot* p_slot : slots_to_disconnect)
            {
                p_slot->disconnect(this);
            }
        }

        void disconnect_all_callable()
        {
            std::lock_guard<std::mutex> lock(m_mut);
            m_connections.erase(
                std::remove_if(
                    m_connections.begin(),
                    m_connections.end(),
                    [&](const std::shared_ptr<base_connection<Args...>>& p_conn)
                    {
                        return p_conn->get_slot_obj() == nullptr;
                    }
                ),
                m_connections.end()
            );
        }

        void emit(Args... args)
        {
            std::lock_guard<std::mutex> lock(m_mut);

            for (auto& p_conn : m_connections)
            {
                p_conn->emit(std::forward<Args>(args)...);
            }
        }

        void operator()(Args... args)
        {
            this->emit(std::forward<Args>(args)...);
        }

    private:
        std::mutex m_mut;
        std::list<std::shared_ptr<base_connection<Args...>>> m_connections;
    };

    template <typename Signal, typename Slot, typename... Args>
    void connect(Signal& signal, Slot* slot, void(Slot::*member_func)(Args...))
    {
        signal.connect(slot, member_func);
    }
}; // namespace sigslot

#if defined(unix) || defined(__unix__) || defined(__unix) || defined(__linux__)
namespace time_event
{
    class timer
    {
    public:
        timer()
            : m_timer_id(0)
            , m_duration_ms(0)
            , m_repeat_count(-1) // -1 means infinite
            , m_repeat_counter(0)
            , m_running(false)
        {
        }

        virtual ~timer()
        {
            this->stop();
            clear_callbacks();
        }

        /**
         * @param repeat_count: number of times to repeat the timer
         * -1 means repeat forever
         * 0 means one shot timer
         * repeat_count > 0 means repeat that number of times
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

        void stop()
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

        void add_callback(const std::function<void()>& cb)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_callbacks.push_back(cb);
        }

        void clear_callbacks()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_callbacks.clear();
        }

        bool is_running() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_running;
        }

    private:
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
        std::vector<std::function<void()>> m_callbacks;
    };
}
#else
namespace timer_event
{
    class timer
    {
    public:
        timer()
        {
            static_assert(false, "timer_event is not supported on this platform.");
        }
    };
}
#endif

namespace once_event
{
    /** Compile source code with -pthread flag to get rid of runtime std::system_error exception */
    class once_event
    {
    public:
        once_event() = default;
        ~once_event() = default;

        // Overload for any callable (lambdas, functors, function objects)
        template <typename Callable, typename... Args,
                  typename = typename std::enable_if<
                      !std::is_member_function_pointer<Callable>::value &&
                      !std::is_same<typename std::decay<Callable>::type, std::function<void(Args...)>>::value
                  >::type>
        void call_once(Callable&& func, Args&&... args)
        {
            std::call_once(m_flag, std::forward<Callable>(func), std::forward<Args>(args)...);
        }

        // Overload for member functions
        template <typename Cls, typename... Args>
        void call_once(void (Cls::*member_func)(Args...), Cls* obj, Args... args)
        {
            std::call_once(m_flag, [=]() mutable{
                (obj->*member_func)(std::forward<Args>(args)...);
            });
        }

    private:
        std::once_flag m_flag;
    };
} // namespace once_event

namespace toggle_event
{
    /**
     * @brief Toggle event that triggers callback only once when condition becomes true,
     * and resets when condition becomes false.
     *
     * Usage pattern:
     * if (precondition) {
     *     toggle.trigger_if_not_set(callback, args...);
     * } else {
     *     toggle.reset(); // Reset for next trigger
     * }
     */
    class toggle_event
    {
    public:
        toggle_event()
            : m_triggered(false)
        {
        }

        ~toggle_event() = default;

        /**
         * @brief Check if toggle has been triggered
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
         * Overload for any callable (lambdas, functors, function objects)
         */
        template <typename Callable, typename... Args,
                  typename = typename std::enable_if<
                      !std::is_member_function_pointer<Callable>::value &&
                      !std::is_same<typename std::decay<Callable>::type, std::function<void(Args...)>>::value
                  >::type>
        bool trigger_if_not_set(Callable&& func, Args&&... args)
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
         * @brief Trigger callback if not already triggered
         * Overload for member functions
         */
        template <typename Cls, typename... Args>
        bool trigger_if_not_set(void (Cls::*member_func)(Args...), Cls* obj, Args... args)
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
         * @brief Trigger callback if not already triggered
         * Overload for std::function
         */
        template <typename... Args>
        bool trigger_if_not_set(std::function<void(Args...)> func, Args... args)
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
         * Overload for any callable (lambdas, functors, function objects)
         */
        template <typename Callable, typename... Args,
                  typename = typename std::enable_if<
                      !std::is_member_function_pointer<Callable>::value &&
                      !std::is_same<typename std::decay<Callable>::type, std::function<void(Args...)>>::value
                  >::type>
        void wait_then(Callable&& func, Args&&... args)
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
        void wait_then(void (Cls::*member_func)(Args...), Cls* obj, Args... args)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond_var.wait(lock, [this]() { return m_triggered; });
            (obj->*member_func)(std::forward<Args>(args)...);
        }

        /**
         * @brief Wait until triggered, then execute callback
         * Overload for std::function
         */
        template <typename... Args>
        void wait_then(std::function<void(Args...)> func, Args... args)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond_var.wait(lock, [this]() { return m_triggered; });
            func(std::forward<Args>(args)...);
        }

        /**
         * @brief Wait with timeout, then execute callback if triggered
         * Overload for any callable (lambdas, functors, function objects)
         * @return true if triggered and callback executed, false if timeout
         */
        template <typename Callable, typename... Args,
                  typename = typename std::enable_if<
                      !std::is_member_function_pointer<Callable>::value &&
                      !std::is_same<typename std::decay<Callable>::type, std::function<void(Args...)>>::value
                  >::type>
        bool wait_for_then(uint32_t timeout_ms, Callable&& func, Args&&... args)
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
         * @brief Wait with timeout, then execute callback if triggered
         * Overload for member functions
         * @return true if triggered and callback executed, false if timeout
         */
        template <typename Cls, typename... Args>
        bool wait_for_then(uint32_t timeout_ms, void (Cls::*member_func)(Args...), Cls* obj, Args... args)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            bool triggered = m_cond_var.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() { return m_triggered; });
            if (triggered)
            {
                (obj->*member_func)(std::forward<Args>(args)...);
            }
            return triggered;
        }

        /**
         * @brief Wait with timeout, then execute callback if triggered
         * Overload for std::function
         * @return true if triggered and callback executed, false if timeout
         */
        template <typename... Args>
        bool wait_for_then(uint32_t timeout_ms, std::function<void(Args...)> func, Args... args)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            bool triggered = m_cond_var.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() { return m_triggered; });
            if (triggered)
            {
                func(std::forward<Args>(args)...);
            }
            return triggered;
        }

    private:
        mutable std::mutex m_mutex;
        bool m_triggered;
        std::condition_variable m_cond_var;
    };
} // namespace toggle_event

namespace fd_event
{
    /**
     * @brief Event types for file descriptor monitoring
     */
    enum class event_type
    {
        READ = POLLIN,              // Data available to read
        WRITE = POLLOUT,            // Ready for writing
        ERROR = POLLERR,            // Error condition
        HANGUP = POLLHUP,           // Hang up
        INVALID = POLLNVAL,         // Invalid request
        PRIORITY = POLLPRI,         // Priority data available
        READ_WRITE = POLLIN | POLLOUT  // Both read and write
    };

    /**
     * @brief Event callback function signature
     * Parameters: fd (int), revents (short), user_data (void*)
     */
    using event_callback = std::function<void(int, short, void*)>;

    /**
     * @brief Structure to hold file descriptor information
     */
    struct fd_info
    {
        int fd;
        short events;              // Events to monitor (POLLIN, POLLOUT, etc.)
        event_callback callback;   // Callback function when event occurs
        void* user_data;          // User-defined data passed to callback
        std::string name;         // Descriptive name for logging
        bool enabled;             // Whether this FD is currently enabled

        fd_info() : fd(-1), events(0), callback(nullptr), user_data(nullptr), name(""), enabled(true) {}

        fd_info(int fd_, short events_, event_callback cb, void* data = nullptr, const std::string& name_ = "")
            : fd(fd_), events(events_), callback(cb), user_data(data), name(name_), enabled(true) {}
    };

    /**
     * @brief File Descriptor Event Manager
     *
     * This class manages multiple file descriptors and their associated events.
     * It uses poll() to monitor multiple FDs efficiently.
     */
    class fd_event_manager
    {
    public:
        fd_event_manager() : modified_(false) {}
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
            fd_info info(fd, events, callback, user_data, name);
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

namespace signal_event
{
    /**
     * @brief Set up a signal handler for the specified signal number
     * @param signum Signal number (e.g., SIGINT, SIGTERM)
     * @param handler Function to handle the signal
     * @return true on success, false on failure
     */
    inline bool set_signal_handler(int signum, void (*handler)(int))
    {
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
    inline bool set_signal_handler_ex(int signum, void (*handler)(int, siginfo_t*, void*), int flags = SA_SIGINFO)
    {
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

#endif // LIB_FOR_EVENT_DRIVEN_PROGRAMMING