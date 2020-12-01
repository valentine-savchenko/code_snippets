#include <iostream>

#include <zmq.hpp>

int main() {
    //  Prepare our context and socket
    constexpr auto io_threads_num{ 1 };
    zmq::context_t context{ io_threads_num };
    zmq::socket_t socket{ context, ZMQ_REQ };
    socket.connect("tcp://localhost:5555");

    constexpr auto requests_num{ 10 };
    for (auto i{ 0 }; i < requests_num; ++i) {
        //  Send request to server
        const std::string r{ "Hello" };
        zmq::message_t request{ r.cbegin(), r.cend() };
        zmq::send_flags flags{};
        socket.send(request, flags);

        //  Wait for next request from client
        zmq::message_t reply{};
        const auto reply_status{ socket.recv(reply) };
        std::cout << "Received " << reply.to_string() << std::endl;
    }

    return 0;
}