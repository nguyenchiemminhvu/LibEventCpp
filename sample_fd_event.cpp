#include "libevent.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

// Example 1: Basic single FD monitoring
void example_single_fd()
{
    std::cout << "\n=== Example 1: Single FD Monitoring ===" << std::endl;
    
    fd_event::fd_event_manager manager;
    
    // Set error handler
    manager.set_error_handler([](const std::string& error) {
        std::cerr << "fd_event_manager Error: " << error << std::endl;
    });
    
    // Create a pipe for testing
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        std::cerr << "Failed to create pipe" << std::endl;
        return;
    }
    
    // Add read end of pipe to manager
    manager.add_fd(pipefd[0], static_cast<short>(fd_event::event_type::READ),
        [](int fd, short revents, void* user_data) {
            (void)user_data;
            std::cout << "Callback invoked for fd " << fd 
                      << ", events: " << fd_event::event_to_string(revents) << std::endl;
            
            if (revents & POLLIN)
            {
                char buffer[256];
                ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
                if (n > 0)
                {
                    buffer[n] = '\0';
                    std::cout << "Read data: " << buffer << std::endl;
                }
            }
        },
        nullptr,
        "test_pipe_read"
    );
    
    // Write to pipe
    const char* msg = "Hello from pipe!";
    write(pipefd[1], msg, strlen(msg));
    
    // Wait and process
    std::cout << "Waiting for events (timeout: 1000ms)..." << std::endl;
    int ret = manager.wait_and_process(1000);
    std::cout << "Events processed: " << ret << std::endl;
    
    // Cleanup
    close(pipefd[0]);
    close(pipefd[1]);
}

// Example 2: Multiple FD monitoring
void example_multiple_fds()
{
    std::cout << "\n=== Example 2: Multiple FDs Monitoring ===" << std::endl;
    
    fd_event::fd_event_manager manager;
    
    // Create multiple pipes
    int pipe1[2], pipe2[2], pipe3[2];
    pipe(pipe1);
    pipe(pipe2);
    pipe(pipe3);
    
    // Add multiple FDs with different callbacks
    manager.add_fd(pipe1[0], POLLIN,
        [](int fd, short revents, void* user_data) {
            (void)user_data;
            std::cout << "[Pipe 1] Event on fd " << fd << std::endl;
            if (revents & POLLIN)
            {
                char buf[64];
                ssize_t n = read(fd, buf, sizeof(buf) - 1);
                if (n > 0)
                {
                    buf[n] = '\0';
                    std::cout << "[Pipe 1] Data: " << buf << std::endl;
                }
            }
        },
        nullptr,
        "pipe1"
    );
    
    manager.add_fd(pipe2[0], POLLIN,
        [](int fd, short revents, void* user_data) {
            (void)user_data;
            std::cout << "[Pipe 2] Event on fd " << fd << std::endl;
            if (revents & POLLIN)
            {
                char buf[64];
                ssize_t n = read(fd, buf, sizeof(buf) - 1);
                if (n > 0)
                {
                    buf[n] = '\0';
                    std::cout << "[Pipe 2] Data: " << buf << std::endl;
                }
            }
        },
        nullptr,
        "pipe2"
    );
    
    manager.add_fd(pipe3[0], POLLIN,
        [](int fd, short revents, void* user_data) {
            (void)user_data;
            std::cout << "[Pipe 3] Event on fd " << fd << std::endl;
            if (revents & POLLIN)
            {
                char buf[64];
                ssize_t n = read(fd, buf, sizeof(buf) - 1);
                if (n > 0)
                {
                    buf[n] = '\0';
                    std::cout << "[Pipe 3] Data: " << buf << std::endl;
                }
            }
        },
        nullptr,
        "pipe3"
    );
    
    std::cout << "Total FDs registered: " << manager.get_fd_count() << std::endl;
    
    // Write to pipes
    write(pipe1[1], "Message 1", 9);
    write(pipe2[1], "Message 2", 9);
    write(pipe3[1], "Message 3", 9);
    
    // Process all events
    std::cout << "Processing events..." << std::endl;
    manager.wait_and_process(1000);
    
    // Cleanup
    close(pipe1[0]); close(pipe1[1]);
    close(pipe2[0]); close(pipe2[1]);
    close(pipe3[0]); close(pipe3[1]);
}

// Example 3: Enable/Disable FDs dynamically
void example_enable_disable()
{
    std::cout << "\n=== Example 3: Enable/Disable FDs ===" << std::endl;
    
    fd_event::fd_event_manager manager;
    
    int pipefd[2];
    pipe(pipefd);
    
    manager.add_fd(pipefd[0], POLLIN,
        [](int fd, short revents, void* user_data) {
            (void)fd;
            (void)revents;
            (void)user_data;
            std::cout << "Callback called (this should be disabled)" << std::endl;
        },
        nullptr,
        "disabled_pipe"
    );
    
    std::cout << "Total FDs: " << manager.get_fd_count() << std::endl;
    std::cout << "Enabled FDs: " << manager.get_enabled_fd_count() << std::endl;
    
    // Disable the FD
    manager.disable_fd(pipefd[0]);
    std::cout << "After disabling - Enabled FDs: " << manager.get_enabled_fd_count() << std::endl;
    
    // Write data (should not trigger callback)
    write(pipefd[1], "test", 4);
    int ret = manager.wait_and_process(500);
    std::cout << "Events after disable: " << ret << std::endl;
    
    // Re-enable the FD
    manager.enable_fd(pipefd[0]);
    std::cout << "After enabling - Enabled FDs: " << manager.get_enabled_fd_count() << std::endl;
    
    ret = manager.wait_and_process(500);
    std::cout << "Events after enable: " << ret << std::endl;
    
    close(pipefd[0]);
    close(pipefd[1]);
}

// Example 4: User data passing
struct UserContext
{
    int counter;
    std::string name;
};

void example_user_data()
{
    std::cout << "\n=== Example 4: User Data Passing ===" << std::endl;
    
    fd_event::fd_event_manager manager;
    
    UserContext ctx1 = {0, "Context1"};
    UserContext ctx2 = {0, "Context2"};
    
    int pipe1[2], pipe2[2];
    pipe(pipe1);
    pipe(pipe2);
    
    manager.add_fd(pipe1[0], POLLIN,
        [](int fd, short revents, void* user_data) {
            (void)fd;
            (void)revents;
            UserContext* ctx = static_cast<UserContext*>(user_data);
            if (ctx)
            {
                ctx->counter++;
                std::cout << "[" << ctx->name << "] Counter: " << ctx->counter << std::endl;
                
                char buf[64];
                read(fd, buf, sizeof(buf));
            }
        },
        &ctx1,
        "pipe_with_ctx1"
    );
    
    manager.add_fd(pipe2[0], POLLIN,
        [](int fd, short revents, void* user_data) {
            (void)fd;
            (void)revents;
            UserContext* ctx = static_cast<UserContext*>(user_data);
            if (ctx)
            {
                ctx->counter++;
                std::cout << "[" << ctx->name << "] Counter: " << ctx->counter << std::endl;
                
                char buf[64];
                read(fd, buf, sizeof(buf));
            }
        },
        &ctx2,
        "pipe_with_ctx2"
    );
    
    // Trigger events multiple times
    for (int i = 0; i < 3; ++i)
    {
        write(pipe1[1], "test", 4);
        write(pipe2[1], "test", 4);
        manager.wait_and_process(500);
    }
    
    std::cout << "Final counters - " << ctx1.name << ": " << ctx1.counter 
              << ", " << ctx2.name << ": " << ctx2.counter << std::endl;
    
    close(pipe1[0]); close(pipe1[1]);
    close(pipe2[0]); close(pipe2[1]);
}

// Example 5: Error handling
void example_error_handling()
{
    std::cout << "\n=== Example 5: Error Handling ===" << std::endl;
    
    fd_event::fd_event_manager manager;
    
    int pipefd[2];
    pipe(pipefd);
    
    manager.add_fd(pipefd[0], POLLIN,
        [](int fd, short revents, void* user_data) {
            (void)user_data;
            std::cout << "Events: " << fd_event::event_to_string(revents) << std::endl;
            
            if (revents & POLLERR)
            {
                std::cout << "Error detected on fd " << fd << std::endl;
            }
            if (revents & POLLHUP)
            {
                std::cout << "Hang up detected on fd " << fd << std::endl;
            }
            if (revents & POLLNVAL)
            {
                std::cout << "Invalid fd detected: " << fd << std::endl;
            }
        },
        nullptr,
        "error_test_pipe"
    );
    
    // Close write end to trigger POLLHUP
    close(pipefd[1]);
    
    std::cout << "Waiting for events (should get POLLHUP)..." << std::endl;
    manager.wait_and_process(1000);
    
    close(pipefd[0]);
}

int main()
{
    std::cout << "=== fd_event Library Examples ===" << std::endl;
    
    example_single_fd();
    example_multiple_fds();
    example_enable_disable();
    example_user_data();
    example_error_handling();
    
    std::cout << "\n=== All examples completed ===" << std::endl;
    
    return 0;
}
