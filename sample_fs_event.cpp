#include "libevent.h"
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <sys/stat.h>

// Global notifier for signal handling
std::shared_ptr<fs_event::notifier> g_notifier;

/**
 * Signal handler for graceful shutdown
 */
void signal_handler(int signum)
{
    std::cout << "\nReceived signal " << signum << ". Stopping monitoring..." << std::endl;
    if (g_notifier)
    {
        g_notifier->stop();
    }
}

/**
 * Custom event handler class (similar to pyinotify.ProcessEvent)
 */
class my_event_handler : public fs_event::process_event
{
public:
    /**
     * Called when a file or directory is created
     */
    virtual void process_IN_CREATE(const fs_event::fs_event_info& event) override
    {
        std::cout << "[CREATE] " << event.get_full_path()
                  << " (is_dir: " << std::boolalpha << event.is_dir << ")"
                  << std::endl;
    }

    /**
     * Called when a file is modified
     */
    virtual void process_IN_MODIFY(const fs_event::fs_event_info& event) override
    {
        std::cout << "[MODIFY] " << event.get_full_path() << std::endl;
    }

    /**
     * Called when a file or directory is deleted
     */
    virtual void process_IN_DELETE(const fs_event::fs_event_info& event) override
    {
        std::cout << "[DELETE] " << event.get_full_path()
                  << " (is_dir: " << std::boolalpha << event.is_dir << ")"
                  << std::endl;
    }

    /**
     * Called when a file is opened
     */
    virtual void process_IN_OPEN(const fs_event::fs_event_info& event) override
    {
        std::cout << "[OPEN]   " << event.get_full_path() << std::endl;
    }

    /**
     * Called when a writable file is closed
     */
    virtual void process_IN_CLOSE_WRITE(const fs_event::fs_event_info& event) override
    {
        std::cout << "[CLOSE_WRITE] " << event.get_full_path() << std::endl;
    }

    /**
     * Called when a file/directory is moved from watched location
     */
    virtual void process_IN_MOVED_FROM(const fs_event::fs_event_info& event) override
    {
        std::cout << "[MOVED_FROM] " << event.get_full_path()
                  << " (cookie: " << event.cookie << ")"
                  << std::endl;
    }

    /**
     * Called when a file/directory is moved to watched location
     */
    virtual void process_IN_MOVED_TO(const fs_event::fs_event_info& event) override
    {
        std::cout << "[MOVED_TO] " << event.get_full_path()
                  << " (cookie: " << event.cookie << ")"
                  << std::endl;
    }

    /**
     * Called for any other events (catch-all)
     */
    virtual void process_default(const fs_event::fs_event_info& event) override
    {
        (void)event;
        // Uncomment to see all events
        // std::cout << "[EVENT] " << event.get_full_path()
        //           << " - " << event.event_to_string()
        //           << std::endl;
    }
};

/**
 * Ensure directory exists, create if needed
 */
bool ensure_directory_exists(const std::string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) == 0)
    {
        if (S_ISDIR(st.st_mode))
        {
            return true; // Directory already exists
        }
        else
        {
            std::cerr << "Error: Path exists but is not a directory: " << path << std::endl;
            return false;
        }
    }

    // Create directory
    if (mkdir(path.c_str(), 0755) == 0)
    {
        std::cout << "Created directory: " << path << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Error: Failed to create directory: " << path
                  << " (" << strerror(errno) << ")" << std::endl;
        return false;
    }
}

/**
 * Monitor a directory for file system events
 */
void monitor_directory(const std::string& path)
{
    try
    {
        // Ensure the directory exists
        if (!ensure_directory_exists(path))
        {
            return;
        }

        // Create watch manager
        auto wm = std::make_shared<fs_event::watch_manager>();

        // Create event handler
        auto handler = std::make_shared<my_event_handler>();

        // Define the events we want to monitor (bitwise OR)
        fs_event::fs_event_type mask = 
            fs_event::fs_event_type::CREATE |
            fs_event::fs_event_type::MODIFY |
            fs_event::fs_event_type::DELETE |
            fs_event::fs_event_type::MOVED_FROM |
            fs_event::fs_event_type::MOVED_TO |
            fs_event::fs_event_type::OPEN |
            fs_event::fs_event_type::CLOSE_WRITE;

        // Add the watch to the manager
        // Set recursive=true to watch subdirectories
        int wd = wm->add_watch(path, mask, false);
        if (wd < 0)
        {
            std::cerr << "Error: Failed to add watch for path: " << path << std::endl;
            std::cerr << "Error message: " << wm->get_last_error() << std::endl;
            return;
        }

        std::cout << "Successfully added watch for: " << path << " (wd=" << wd << ")" << std::endl;

        // Create notifier
        g_notifier = std::make_shared<fs_event::notifier>(wm, handler);

        std::cout << "Monitoring directory: " << path << std::endl;
        std::cout << "Press Ctrl+C to stop." << std::endl;
        std::cout << "-----------------------------------" << std::endl;

        // Set up signal handlers for graceful shutdown
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);

        // Start the event loop (blocking)
        g_notifier->loop();

        std::cout << "Monitoring stopped." << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

/**
 * Example demonstrating non-blocking event processing
 */
void monitor_directory_non_blocking(const std::string& path)
{
    try
    {
        // Ensure the directory exists
        if (!ensure_directory_exists(path))
        {
            return;
        }

        // Create watch manager and event handler
        auto wm = std::make_shared<fs_event::watch_manager>();
        auto handler = std::make_shared<my_event_handler>();

        // Define events to monitor
        fs_event::fs_event_type mask = 
            fs_event::fs_event_type::CREATE |
            fs_event::fs_event_type::MODIFY |
            fs_event::fs_event_type::DELETE;

        // Add watch
        int wd = wm->add_watch(path, mask, false);
        if (wd < 0)
        {
            std::cerr << "Error: Failed to add watch" << std::endl;
            return;
        }

        // Create notifier
        auto notifier = std::make_shared<fs_event::notifier>(wm, handler);

        std::cout << "Monitoring (non-blocking mode): " << path << std::endl;
        std::cout << "Press Ctrl+C to stop." << std::endl;

        // Set up signal handler
        bool running = true;
        signal(SIGINT, [](int) { static bool& r = *(bool*)nullptr; r = false; });

        // Process events in non-blocking mode
        while (running)
        {
            // Process available events with 1 second timeout
            int count = notifier->process_events(1000);

            if (count > 0)
            {
                std::cout << "Processed " << count << " events" << std::endl;
            }
            else if (count < 0)
            {
                std::cerr << "Error processing events: " << notifier->get_last_error() << std::endl;
                break;
            }

            // Do other work here while monitoring...
            // std::cout << "." << std::flush;
        }

        std::cout << "\nMonitoring stopped." << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

/**
 * Example demonstrating recursive directory monitoring
 */
void monitor_directory_recursive(const std::string& path)
{
    try
    {
        if (!ensure_directory_exists(path))
        {
            return;
        }

        auto wm = std::make_shared<fs_event::watch_manager>();
        auto handler = std::make_shared<my_event_handler>();

        fs_event::fs_event_type mask = 
            fs_event::fs_event_type::CREATE |
            fs_event::fs_event_type::MODIFY |
            fs_event::fs_event_type::DELETE;

        // Add watch with recursive=true to monitor subdirectories
        int wd = wm->add_watch(path, mask, true);
        if (wd < 0)
        {
            std::cerr << "Error: Failed to add recursive watch" << std::endl;
            return;
        }

        std::cout << "Monitoring directory (recursive): " << path << std::endl;
        std::cout << "Total watches: " << wm->get_watch_count() << std::endl;
        std::cout << "Press Ctrl+C to stop." << std::endl;

        g_notifier = std::make_shared<fs_event::notifier>(wm, handler);
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);

        g_notifier->loop();

        std::cout << "Monitoring stopped." << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[])
{
    // Default path to watch
    std::string path_to_watch = "/tmp/watch_dir";

    // Allow user to specify path via command line
    if (argc > 1)
    {
        path_to_watch = argv[1];
    }

    std::cout << "=== File System Event Monitor Sample ===" << std::endl;
    std::cout << "Similar to Python's pyinotify library" << std::endl;
    std::cout << std::endl;

    // Choose monitoring mode
    std::cout << "Select mode:" << std::endl;
    std::cout << "1. Blocking mode (default)" << std::endl;
    std::cout << "2. Non-blocking mode" << std::endl;
    std::cout << "3. Recursive monitoring" << std::endl;
    std::cout << "Enter choice (1-3): ";

    int choice = 1;
    std::cin >> choice;
    std::cin.ignore(); // Clear newline from input buffer

    std::cout << std::endl;

    switch (choice)
    {
        case 2:
            monitor_directory_non_blocking(path_to_watch);
            break;
        case 3:
            monitor_directory_recursive(path_to_watch);
            break;
        case 1:
        default:
            monitor_directory(path_to_watch);
            break;
    }

    return 0;
}
