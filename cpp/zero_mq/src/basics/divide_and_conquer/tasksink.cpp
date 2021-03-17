#include <chrono>
#include <iostream>

#include <zmq.hpp>

int main()
{
    zmq::context_t context{};

    // Create a socket to receive confirmations from workers
    zmq::socket_t receiver{ context, ZMQ_PULL };
    receiver.bind("tcp://*:5558");

    // Wait for a notification that a batch is coming
    zmq::message_t batch_start_message{};
    const auto batch_start_status{ receiver.recv(batch_start_message) };

    // Start tracking time taken to process all confirmations
    const auto start_point{ std::chrono::system_clock::now() };

    constexpr auto kMaxConfirmations{ 100 };
    for (auto c{ 0 }; c < kMaxConfirmations; ++c)
    {
        // Receive a yet another confirmation from a worker
        zmq::message_t confirmation_message{};
        const auto confirmation_status{ receiver.recv(confirmation_message) };

        // Entertain a user with some ASCII art
        if (0 == c % 10)
        {
            std::cout << ":";
        }
        else
        {
            std::cout << ".";
        }
    }

    // Wrap up tracking of the total elapsed time
    const auto stop_point{ std::chrono::system_clock::now() };
    const auto elapsed{ std::chrono::duration_cast<std::chrono::milliseconds>(stop_point - start_point) };
    std::cout << "\nTotal elapsed time: " << elapsed << "\n";


    return 0;
}