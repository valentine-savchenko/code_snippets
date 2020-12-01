#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <string>

#include <zmq.hpp>

int main() {
    //  Prepare our context and socket
    constexpr auto io_threads_num{ 1 };
    zmq::context_t context{ io_threads_num };
    zmq::socket_t socket{ context, ZMQ_REP };
    socket.bind("tcp://*:5555");

    while (true) {
        //  Wait for next request from client
        zmq::message_t request{};
        const auto request_status{ socket.recv(request) };
        std::cout << "Received " << request.to_string() << std::endl;

        //  Do some 'work'
        std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });

        //  Send reply back to client
        const std::string r{ "World" };
        zmq::message_t reply{ r.cbegin(), r.cend() };
        zmq::send_flags flags{};
        socket.send(reply, flags);
    }

    return 0;
}