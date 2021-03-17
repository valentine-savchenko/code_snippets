#include <iostream>
#include <cstdio>
#include <random>
#include <string>
#include <thread>
#include <chrono>

#include <zmq.hpp>

int main()
{
    zmq::context_t context{};

    // Establish a socket to send messages on 
    zmq::socket_t sender{ context, ZMQ_PUSH };
    sender.bind("tcp://*:5557");

    // Establish a socket to send a start of batch message
    zmq::socket_t sink{ context, ZMQ_PUSH };
    sink.connect("tcp://localhost:5558");

    // Wait for a manual acknowledgment that all workers are ready to receive tasks for processing
    std::cout << "Press Enter when the workers are ready: ";
    std::getchar();
    std::cout << "Sending tasks to workers..." << std::endl;

    // Signal a start of a batch
    auto start_message{ zmq::str_buffer("0") };
    zmq::send_flags start_message_flags{};
    sink.send(start_message, start_message_flags);

    auto total_msec{ 0 };

    // Generate and send 100 tasks for a random workload of 1 to 100 msec.
    constexpr auto kTasksNumber{ 100 };
    std::default_random_engine generator{};
    std::uniform_int_distribution distribution{ 1, 100 };
    for (auto t{ 0 }; t < kTasksNumber; ++t)
    {
        const auto workload_msec{ distribution(generator) };
        total_msec += workload_msec;

        zmq::message_t workload_message{ std::to_string(workload_msec) };
        zmq::send_flags workload_message_flags{};
        sender.send(workload_message, workload_message_flags);
    }

    std::cout << "Total expected cost: " << total_msec << " ms.";
    std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}