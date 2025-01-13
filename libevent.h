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
 * 1.0 - [20250116]
 */

#ifndef LIB_FOR_EVENT_DRIVEN_PROGRAMMING
#define LIB_FOR_EVENT_DRIVEN_PROGRAMMING

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

namespace event_handler
{
    using steady_timestamp_t = std::chrono::steady_clock::time_point;

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

        event_message(uint32_t delay_ms, std::shared_ptr<Handler> handler, void(Handler::*act)(Args...), Args... args)
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

        static std::shared_ptr<i_message> create(uint32_t delay_ms, std::shared_ptr<Handler> handler, void(Handler::*act)(Args...), Args... args)
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
            {
                std::unique_lock<std::mutex> lock(m_mut);
                m_queue.push(std::move(mess));
            }

            m_cond.notify_one();
        }

        std::shared_ptr<i_message> poll()
        {
            std::unique_lock<std::mutex> lock(m_mut);
            m_cond.wait(lock, [this]() { return !this->m_queue.empty() || !this->m_running; });

            if (!this->m_running)
            {
                return nullptr;
            }

            std::shared_ptr<i_message> mess = std::move(m_queue.top());
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
                    lock.unlock();
                    this->enqueue(mess);
                    lock.lock();
                    mess = nullptr;
                }
            }

            return mess;
        }

        virtual void stop()
        {
            m_running = false;
            m_cond.notify_all();
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

            sched_param sch{};
            sch.sched_priority = 20;
            (void)pthread_setschedparam(m_looper_thread.native_handle(), SCHED_FIFO, &sch);

            m_looper_thread.detach();
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
            m_running = false;
        }

    private:
        void looper()
        {
            while (m_running)
            {
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
    };

    class message_handler : public i_stoppable, public std::enable_shared_from_this<message_handler>
    {
    public:
        message_handler()
        {
            m_looper = std::make_shared<message_looper>();
            m_message_queue = m_looper->get_message_queue();
        }

        virtual ~message_handler()
        {
            this->stop();
        }

        template<typename T, typename... Args>
        void post_message(void (T::*func)(Args...), Args... args)
        {
            static_assert(std::is_base_of<message_handler, T>::value, "T must be derived from message_handler");

            std::shared_ptr<i_message> mess = event_message<T, Args...>::create(std::dynamic_pointer_cast<T>(shared_from_this()), func, args...);
            if (m_message_queue != nullptr)
            {
                m_message_queue->enqueue(mess);
            }
        }

        template<typename T, typename... Args>
        void post_delayed_message(uint32_t delay_ms, void (T::*func)(Args...), Args... args)
        {
            static_assert(std::is_base_of<message_handler, T>::value, "T must be derived from message_handler");

            std::shared_ptr<i_message> mess = event_message<T, Args...>::create(delay_ms, std::dynamic_pointer_cast<T>(shared_from_this()), func, args...);
            if (m_message_queue != nullptr)
            {
                m_message_queue->enqueue(mess);
            }
        }

        template<typename T, typename... Args>
        void post_repeated_message(std::size_t times, uint32_t duration_ms, void (T::*func)(Args...), Args... args)
        {
            static_assert(std::is_base_of<message_handler, T>::value, "T must be derived from message_handler");

            for (std::size_t i = 0U; i < times; ++i)
            {
                uint32_t delay_ms = (duration_ms * i);
                this->post_delayed_message(delay_ms, func, args...);
            }
        }

        virtual void stop()
        {
            m_message_queue->stop();
            m_looper->stop();
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

#endif // LIB_FOR_EVENT_DRIVEN_PROGRAMMING