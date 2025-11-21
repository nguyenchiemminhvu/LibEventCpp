#include "libevent.h"
#include <iostream>

class DemoClass
{
public:
    void initialize()
    {
        std::cout << "DemoClass initialized." << std::endl;
        m_once_init.call_once(&DemoClass::init_once, this, 42);
    }

    void init_once(int value)
    {
        std::cout << "DemoClass initialized with value: " << value << std::endl;
    }
private:
    once_event::once_event m_once_init;
};

void demonstrate_once_event()
{
    DemoClass demo_instance;
    demo_instance.initialize();
    demo_instance.initialize(); // This call should not re-initialize
}

int main()
{
    demonstrate_once_event();

    return 0;
}