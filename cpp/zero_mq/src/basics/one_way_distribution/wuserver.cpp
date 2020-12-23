#include <random>
#include <algorithm>
#include <string>
#include <iostream>

#include <zmq.hpp>

int main() {
    zmq::context_t context{};
    zmq::socket_t publisher{ context, ZMQ_PUB };
    publisher.bind("tcp://*:5556");
    publisher.bind("ipc://weather.ipc");

    std::default_random_engine generator{};
    std::normal_distribution<double> zipcode_distribution{ 50000, 16500 };
    std::normal_distribution<double> temperature_distribution{ 5.8, 10 };
    std::normal_distribution<double> relative_humidity_distribution{ 77, 7 };

    while (true) {
        const int zipcode{ static_cast<int>(std::max(0.0, zipcode_distribution(generator))) };
        const auto temperature{ temperature_distribution(generator) };
        const auto relative_humidity{ std::max(0.0, relative_humidity_distribution(generator)) };

        const std::string measurements{ std::to_string(zipcode) + " " +
            std::to_string(temperature) + " " +
            std::to_string(relative_humidity) };
        std::cout << "A measurement: " << measurements << "\n";

        zmq::message_t message{ measurements.cbegin(), measurements.cend() };
        const zmq::send_flags flags{};
        publisher.send(message, flags);
    }

    return 0;
}