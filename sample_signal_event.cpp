#include "libevent.h"
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

// Global flag to control the main loop
std::atomic<bool> running(true);
std::atomic<int> signal_count(0);

// Signal handler for SIGINT (Ctrl+C)
void handle_sigint(int signum)
{
    std::cout << "\n[SIGNAL] Caught " << signal_event::get_signal_name(signum) 
              << " (signal " << signum << ")" << std::endl;
    std::cout << "[SIGNAL] Gracefully shutting down..." << std::endl;
    running = false;
}

// Signal handler for SIGTERM
void handle_sigterm(int signum)
{
    std::cout << "\n[SIGNAL] Caught " << signal_event::get_signal_name(signum)
              << " (signal " << signum << ")" << std::endl;
    std::cout << "[SIGNAL] Terminating process..." << std::endl;
    running = false;
}

// Signal handler for SIGUSR1 (custom user signal)
void handle_sigusr1(int signum)
{
    signal_count++;
    std::cout << "\n[SIGNAL] Caught " << signal_event::get_signal_name(signum)
              << " (signal " << signum << ") - Count: " << signal_count << std::endl;
    std::cout << "[SIGNAL] Custom signal received - printing status..." << std::endl;
    std::cout << "[STATUS] Application is running normally" << std::endl;
}

// Signal handler for SIGUSR2 (another custom user signal)
void handle_sigusr2(int signum)
{
    std::cout << "\n[SIGNAL] Caught " << signal_event::get_signal_name(signum)
              << " (signal " << signum << ")" << std::endl;
    std::cout << "[SIGNAL] Toggling debug mode..." << std::endl;
}

// Extended signal handler with siginfo_t
void handle_siginfo(int signum, siginfo_t* info, void* context)
{
    (void)context;
    std::cout << "\n[SIGNAL-EX] Caught " << signal_event::get_signal_name(signum)
              << " with extended info" << std::endl;
    std::cout << "[INFO] Signal code: " << info->si_code << std::endl;
    std::cout << "[INFO] Sender PID: " << info->si_pid << std::endl;
    std::cout << "[INFO] Sender UID: " << info->si_uid << std::endl;
}

void demonstrate_signal_blocking()
{
    std::cout << "\n=== Demo: Signal Blocking ===" << std::endl;
    
    // Block SIGUSR1
    std::cout << "Blocking SIGUSR1..." << std::endl;
    if (signal_event::block_signal(SIGUSR1))
    {
        std::cout << "✓ SIGUSR1 blocked successfully" << std::endl;
    }
    
    // Check if blocked
    if (signal_event::is_signal_blocked(SIGUSR1))
    {
        std::cout << "✓ Confirmed: SIGUSR1 is blocked" << std::endl;
    }
    
    // Send signal to self (it will be pending)
    std::cout << "Sending SIGUSR1 to self (should be pending)..." << std::endl;
    signal_event::raise_signal(SIGUSR1);
    
    // Check if pending
    if (signal_event::is_signal_pending(SIGUSR1))
    {
        std::cout << "✓ SIGUSR1 is pending" << std::endl;
    }
    
    // Unblock the signal (handler will be called now)
    std::cout << "Unblocking SIGUSR1..." << std::endl;
    signal_event::unblock_signal(SIGUSR1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void demonstrate_multiple_signal_blocking()
{
    std::cout << "\n=== Demo: Multiple Signal Blocking ===" << std::endl;
    
    std::vector<int> signals = {SIGUSR1, SIGUSR2};
    
    // Block multiple signals
    std::cout << "Blocking SIGUSR1 and SIGUSR2..." << std::endl;
    if (signal_event::block_signals(signals))
    {
        std::cout << "✓ Signals blocked successfully" << std::endl;
    }
    
    // Send both signals
    std::cout << "Sending SIGUSR1 and SIGUSR2..." << std::endl;
    signal_event::raise_signal(SIGUSR1);
    signal_event::raise_signal(SIGUSR2);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "Signals are pending (no handlers called yet)" << std::endl;
    
    // Unblock signals
    std::cout << "Unblocking signals..." << std::endl;
    signal_event::unblock_signals(signals);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void demonstrate_signal_ignore()
{
    std::cout << "\n=== Demo: Signal Ignoring ===" << std::endl;
    
    std::cout << "Ignoring SIGUSR2..." << std::endl;
    if (signal_event::ignore_signal(SIGUSR2))
    {
        std::cout << "✓ SIGUSR2 will be ignored" << std::endl;
    }
    
    std::cout << "Sending SIGUSR2 (should be ignored)..." << std::endl;
    signal_event::raise_signal(SIGUSR2);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "SIGUSR2 was ignored (no handler called)" << std::endl;
    
    // Restore handler
    std::cout << "Restoring SIGUSR2 handler..." << std::endl;
    signal_event::set_signal_handler(SIGUSR2, handle_sigusr2);
}

void demonstrate_extended_handler()
{
    std::cout << "\n=== Demo: Extended Signal Handler ===" << std::endl;
    
    std::cout << "Setting up extended handler for SIGALRM..." << std::endl;
    if (signal_event::set_signal_handler_ex(SIGALRM, handle_siginfo))
    {
        std::cout << "✓ Extended handler registered for SIGALRM" << std::endl;
    }
    
    std::cout << "Raising SIGALRM..." << std::endl;
    signal_event::raise_signal(SIGALRM);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void demonstrate_wait_for_signal()
{
    std::cout << "\n=== Demo: Waiting for Signal ===" << std::endl;
    
    // Create a thread that sends a signal after a delay
    std::thread sender([pid = getpid()]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "[SENDER] Sending SIGUSR1 from thread..." << std::endl;
        signal_event::send_signal(pid, SIGUSR1);
    });
    
    // Block SIGUSR1 first (required for sigwait)
    signal_event::block_signal(SIGUSR1);
    
    std::cout << "Waiting for SIGUSR1 (timeout: 5 seconds)..." << std::endl;
    int received = signal_event::wait_for_signal({SIGUSR1}, 5000);
    
    if (received > 0)
    {
        std::cout << "✓ Received signal: " << signal_event::get_signal_name(received) << std::endl;
    }
    else
    {
        std::cout << "✗ Timeout or error occurred" << std::endl;
    }
    
    signal_event::unblock_signal(SIGUSR1);
    sender.join();
}

void demonstrate_signal_names()
{
    std::cout << "\n=== Demo: Signal Names ===" << std::endl;
    
    std::vector<int> common_signals = {
        SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGABRT,
        SIGBUS, SIGFPE, SIGKILL, SIGUSR1, SIGSEGV, SIGUSR2,
        SIGPIPE, SIGALRM, SIGTERM, SIGCHLD
    };
    
    std::cout << "Common signal names:" << std::endl;
    for (int sig : common_signals)
    {
        std::cout << "  Signal " << sig << ": " 
                  << signal_event::get_signal_name(sig) << std::endl;
    }
}


int main()
{
    std::cout << "=== Signal Event Handler Demo ===" << std::endl;
    std::cout << "Process ID: " << getpid() << std::endl;
    std::cout << std::endl;

    // Set up signal handlers
    std::cout << "=== Setting up signal handlers ===" << std::endl;
    
    if (signal_event::set_signal_handler(SIGINT, handle_sigint))
    {
        std::cout << "✓ " << signal_event::get_signal_name(SIGINT) << " handler registered (Ctrl+C)" << std::endl;
    }
    else
    {
        std::cerr << "✗ Failed to register SIGINT handler" << std::endl;
        return 1;
    }

    if (signal_event::set_signal_handler(SIGTERM, handle_sigterm))
    {
        std::cout << "✓ " << signal_event::get_signal_name(SIGTERM) << " handler registered" << std::endl;
    }
    else
    {
        std::cerr << "✗ Failed to register SIGTERM handler" << std::endl;
        return 1;
    }

    if (signal_event::set_signal_handler(SIGUSR1, handle_sigusr1))
    {
        std::cout << "✓ " << signal_event::get_signal_name(SIGUSR1) << " handler registered" << std::endl;
    }
    else
    {
        std::cerr << "✗ Failed to register SIGUSR1 handler" << std::endl;
        return 1;
    }

    if (signal_event::set_signal_handler(SIGUSR2, handle_sigusr2))
    {
        std::cout << "✓ " << signal_event::get_signal_name(SIGUSR2) << " handler registered" << std::endl;
    }
    else
    {
        std::cerr << "✗ Failed to register SIGUSR2 handler" << std::endl;
        return 1;
    }

    std::cout << std::endl;
    
    // Run demonstrations
    demonstrate_signal_names();
    demonstrate_signal_blocking();
    demonstrate_multiple_signal_blocking();
    demonstrate_signal_ignore();
    demonstrate_extended_handler();
    demonstrate_wait_for_signal();
    
    std::cout << "\n=== Interactive Mode ===" << std::endl;
    std::cout << "Signal handlers are now active!" << std::endl;
    std::cout << std::endl;
    std::cout << "Instructions:" << std::endl;
    std::cout << "  - Press Ctrl+C to send SIGINT and exit" << std::endl;
    std::cout << "  - Run 'kill -TERM " << getpid() << "' to send SIGTERM" << std::endl;
    std::cout << "  - Run 'kill -USR1 " << getpid() << "' to send SIGUSR1" << std::endl;
    std::cout << "  - Run 'kill -USR2 " << getpid() << "' to send SIGUSR2" << std::endl;
    std::cout << std::endl;

    // Main application loop
    int counter = 0;
    while (running)
    {
        std::cout << "Running... (iteration " << ++counter << ", signals received: " 
                  << signal_count << ")" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // Demonstrate reset after 5 iterations
        if (counter == 5)
        {
            std::cout << "\n[ACTION] Resetting SIGUSR2 handler to default..." << std::endl;
            signal_event::reset_signal_handler(SIGUSR2);
            std::cout << "SIGUSR2 handler reset. Sending SIGUSR2 will now use default behavior." << std::endl;
            std::cout << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << "Application terminated cleanly after " << counter << " iterations." << std::endl;
    std::cout << "Total signals received: " << signal_count << std::endl;
    std::cout << "Goodbye!" << std::endl;

    return 0;
}
