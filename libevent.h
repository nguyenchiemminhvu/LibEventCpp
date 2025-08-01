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
#include <set>
#include <list>
#include <queue>
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
                m_cond.notify_one();
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

        template<typename T, typename... Args>
        void post_message(void (T::*func)(typename convert_arg<Args>::type...), Args... args)
        {
            static_assert(std::is_base_of<message_handler, T>::value, "T must be derived from message_handler");

            auto shared_this = this->get_shared_ptr();
            if (shared_this)
            {
                std::shared_ptr<i_message> mess = event_message<T, typename convert_arg<Args>::type...>::create(std::dynamic_pointer_cast<T>(shared_this), func, std::forward<typename convert_arg<Args>::type>(args)...);
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

#endif // LIB_FOR_EVENT_DRIVEN_PROGRAMMING