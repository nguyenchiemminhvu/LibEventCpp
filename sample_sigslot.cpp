#include "libevent.h"
#include <unistd.h>
#include <iostream>
#include <string>

class Sender
{
public:
    sigslot::signal<std::string> message_notification;

    void boardcast_message(std::string mess)
    {
        message_notification(mess);
    }
};

class Listener : public sigslot::base_slot
{
public:
    void on_boardcast_received(std::string mess)
    {
        std::cout << "Received boardcast message: " << mess << std::endl;
    }
};

int main()
{
    Sender sender;
    Listener listener;
    sender.message_notification.connect(&listener, &Listener::on_boardcast_received);

    sigslot::signal<std::string> global_broadcast;
    sigslot::connect(global_broadcast, &listener, &Listener::on_boardcast_received);
    global_broadcast("This message is broadcasted by global_broadcast signal");

    sigslot::signal<> test_sig_lambda;
    test_sig_lambda.connect([]() {
        std::cout << "The signal connected to this lambda is activated" << std::endl;
    });
    test_sig_lambda();
    test_sig_lambda.disconnect_all_callable();

    while (true)
    {
        std::string mess;
        std::getline(std::cin, mess);
        sender.boardcast_message(mess);

        if (mess == "disconnect_all")
        {
            std::cout << "disconnect_all(), from now on, listener receives nothing" << std::endl;
            sender.message_notification.disconnect_all();
        }
        else if (mess == "disconnect")
        {
            std::cout << "disconnect(), from now on, listener receives nothing" << std::endl;
            sender.message_notification.disconnect(&listener);
        }
        else if (mess == "quit")
        {
            break;
        }
    }

    sender.message_notification.disconnect_all();
    listener.disconnect_all();

    return 0;
}