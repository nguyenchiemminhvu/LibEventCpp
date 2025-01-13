#include "libevent.h"
#include <iostream>

/**
 * @brief Simple receiver class with multiple slot methods
 */
class Receiver : public sigslot::base_slot
{
public:
    Receiver(const std::string& name) : m_name(name) {}

    void onValueChanged(int value)
    {
        std::cout << m_name << "::onValueChanged(" << value << ")" << std::endl;
    }

    void onDataUpdated(int value)
    {
        std::cout << m_name << "::onDataUpdated(" << value << ")" << std::endl;
    }

    void onStatusChanged(int value)
    {
        std::cout << m_name << "::onStatusChanged(" << value << ")" << std::endl;
    }

private:
    std::string m_name;
};

/**
 * @brief Sender class that emits signals
 */
class Sender
{
public:
    sigslot::signal<int> valueChanged;
};

void demonstrate_specific_disconnect()
{
    std::cout << "\n=== Demonstrating Specific Member Function Disconnection ===" << std::endl;

    Sender sender;
    Receiver receiver1("Receiver1");
    Receiver receiver2("Receiver2");

    // Connect multiple member functions from receiver1 to the same signal
    sender.valueChanged.connect(&receiver1, &Receiver::onValueChanged);
    sender.valueChanged.connect(&receiver1, &Receiver::onDataUpdated);
    sender.valueChanged.connect(&receiver1, &Receiver::onStatusChanged);

    // Connect one member function from receiver2
    sender.valueChanged.connect(&receiver2, &Receiver::onValueChanged);

    std::cout << "\n[Test 1] Emit signal with all connections active:" << std::endl;
    sender.valueChanged.emit(100);

    // Disconnect only one specific member function from receiver1
    std::cout << "\n[Test 2] Disconnect Receiver1::onDataUpdated (other connections remain):" << std::endl;
    sender.valueChanged.disconnect(&receiver1, &Receiver::onDataUpdated);
    sender.valueChanged.emit(200);

    // Disconnect another specific member function from receiver1
    std::cout << "\n[Test 3] Disconnect Receiver1::onStatusChanged:" << std::endl;
    sender.valueChanged.disconnect(&receiver1, &Receiver::onStatusChanged);
    sender.valueChanged.emit(300);

    // Disconnect the last member function from receiver1
    // This should automatically disconnect receiver1 from the signal
    std::cout << "\n[Test 4] Disconnect Receiver1::onValueChanged (last connection - auto disconnect):" << std::endl;
    sender.valueChanged.disconnect(&receiver1, &Receiver::onValueChanged);
    sender.valueChanged.emit(400);

    // Only receiver2 should still be connected
    std::cout << "\n[Test 5] Only Receiver2 remains connected:" << std::endl;
    sender.valueChanged.emit(500);

    std::cout << "\n=== Test Completed ===" << std::endl;
}

void demonstrate_mixed_disconnect()
{
    std::cout << "\n=== Demonstrating Mixed Disconnection Methods ===" << std::endl;

    Sender sender;
    Receiver receiver1("Receiver1");
    Receiver receiver2("Receiver2");

    // Connect multiple member functions
    sender.valueChanged.connect(&receiver1, &Receiver::onValueChanged);
    sender.valueChanged.connect(&receiver1, &Receiver::onDataUpdated);
    sender.valueChanged.connect(&receiver2, &Receiver::onValueChanged);
    sender.valueChanged.connect(&receiver2, &Receiver::onDataUpdated);

    std::cout << "\n[Test 1] All connections active:" << std::endl;
    sender.valueChanged.emit(100);

    // Disconnect all connections of receiver1 at once (old method)
    std::cout << "\n[Test 2] Disconnect all Receiver1 connections at once:" << std::endl;
    sender.valueChanged.disconnect(&receiver1);
    sender.valueChanged.emit(200);

    // Only receiver2 connections remain
    std::cout << "\n[Test 3] Only Receiver2 connections remain:" << std::endl;
    sender.valueChanged.emit(300);

    // Disconnect specific member function from receiver2
    std::cout << "\n[Test 4] Disconnect Receiver2::onDataUpdated specifically:" << std::endl;
    sender.valueChanged.disconnect(&receiver2, &Receiver::onDataUpdated);
    sender.valueChanged.emit(400);

    std::cout << "\n=== Test Completed ===" << std::endl;
}

void demonstrate_multiple_signals()
{
    std::cout << "\n=== Demonstrating Connection Tracking Across Multiple Signals ===" << std::endl;

    // Create multiple senders
    Sender sender1;
    Sender sender2;
    Receiver receiver("Receiver");

    // Connect the same receiver to multiple signals
    sender1.valueChanged.connect(&receiver, &Receiver::onValueChanged);
    sender1.valueChanged.connect(&receiver, &Receiver::onDataUpdated);
    sender2.valueChanged.connect(&receiver, &Receiver::onValueChanged);
    sender2.valueChanged.connect(&receiver, &Receiver::onStatusChanged);

    std::cout << "\n[Test 1] Emit from sender1:" << std::endl;
    sender1.valueChanged.emit(100);

    std::cout << "\n[Test 2] Emit from sender2:" << std::endl;
    sender2.valueChanged.emit(200);

    // Disconnect specific member function from sender1
    std::cout << "\n[Test 3] Disconnect onDataUpdated from sender1:" << std::endl;
    sender1.valueChanged.disconnect(&receiver, &Receiver::onDataUpdated);
    sender1.valueChanged.emit(300);

    std::cout << "\n[Test 4] sender2 still has its connections:" << std::endl;
    sender2.valueChanged.emit(400);

    // Disconnect all from sender2
    std::cout << "\n[Test 5] Disconnect all receiver connections from sender2:" << std::endl;
    sender2.valueChanged.disconnect(&receiver);
    sender2.valueChanged.emit(500);

    std::cout << "\n[Test 6] sender1 still has onValueChanged:" << std::endl;
    sender1.valueChanged.emit(600);

    std::cout << "\n=== Test Completed ===" << std::endl;
}

void demonstrate_callable_connections()
{
    std::cout << "\n=== Demonstrating Mixed Slot and Callable Connections ===" << std::endl;

    Sender sender;
    Receiver receiver("Receiver");

    // Connect member functions
    sender.valueChanged.connect(&receiver, &Receiver::onValueChanged);
    sender.valueChanged.connect(&receiver, &Receiver::onDataUpdated);

    // Connect callable objects (lambdas)
    sender.valueChanged.connect([](int value) {
        std::cout << "Lambda1: value = " << value << std::endl;
    });

    sender.valueChanged.connect([](int value) {
        std::cout << "Lambda2: value = " << value * 2 << std::endl;
    });

    std::cout << "\n[Test 1] All connections (slots + callables) active:" << std::endl;
    sender.valueChanged.emit(100);

    // Disconnect specific member function
    std::cout << "\n[Test 2] Disconnect Receiver::onDataUpdated:" << std::endl;
    sender.valueChanged.disconnect(&receiver, &Receiver::onDataUpdated);
    sender.valueChanged.emit(200);

    // Disconnect all callable connections
    std::cout << "\n[Test 3] Disconnect all callable connections (lambdas):" << std::endl;
    sender.valueChanged.disconnect_all_callable();
    sender.valueChanged.emit(300);

    std::cout << "\n=== Test Completed ===" << std::endl;
}

int main()
{
    std::cout << "LibEventCpp - Specific Member Function Disconnection Demo\n";
    std::cout << "========================================================\n";

    // Run demonstrations
    demonstrate_specific_disconnect();
    demonstrate_mixed_disconnect();
    demonstrate_multiple_signals();
    demonstrate_callable_connections();

    std::cout << "\n========================================================" << std::endl;
    std::cout << "All demonstrations completed successfully!" << std::endl;

    return 0;
}
