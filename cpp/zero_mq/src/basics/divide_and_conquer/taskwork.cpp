#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <zmq.hpp>

int main()
{
    zmq::context_t context{};

    // Create a socket to receive messages on
    zmq::socket_t receiver{ context, ZMQ_PULL };
    receiver.connect("tcp://localhost:5557");

    // Create a socket to send messages to
    zmq::socket_t sender{ context, ZMQ_PUSH };
    sender.connect("tcp://localhost:5558");

    // Process incoming tasks forever
    while (true)
    {
        // Receive a delay in milliseconds
        zmq::message_t delay_as_string{};
        const auto receive_delay_status{ receiver.recv(delay_as_string) };

        // Indicate progress for a viewer
        std::cout << delay_as_string.to_string() << ".";

        // Unwrap the delay
        int delay_ms{};
        try
        {
            delay_ms = std::stoi(delay_as_string.to_string());
        }
        catch (const std::exception&)
        {
            std::cerr << "Failed to cast " << delay_ms << " to a millisecond interval\n";
        }

        // Hold on for an amount prescribed by the delay
        std::this_thread::sleep_for(std::chrono::milliseconds{ delay_ms });

        // Notify that the job is done
        zmq::send_flags send_flags{};
        sender.send(zmq::str_buffer(""), send_flags);
    }

    return 0;
}