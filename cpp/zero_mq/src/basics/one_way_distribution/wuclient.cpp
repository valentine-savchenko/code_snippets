#include <sstream>
#include <iostream>

#include <zmq.hpp>

int main(int argc, char* argv[]) {
    zmq::context_t context{};
    zmq::socket_t subscriber{ context, ZMQ_SUB };
    subscriber.connect("tcp://localhost:5556");

    const auto zipcode_filter{ argc > 1 ? argv[1] : "50000" };
    subscriber.set(zmq::sockopt::subscribe, zipcode_filter);

    constexpr auto kMaxUpdatesToProcess{ 100U };
    double total_temperature{ 0.0 };
    for (auto u{ 0U }; u < kMaxUpdatesToProcess; ++u) {
        zmq::message_t measurements{};
        const auto status{ subscriber.recv(measurements) };
        std::cout << "A measurement: " << measurements.to_string() << "\n";

        std::istringstream stream{ measurements.to_string() };
        double zipcode{};
        double temperature{};
        double relative_humility{};
        stream >> zipcode >> temperature >> relative_humility;

        total_temperature += temperature;
    }

    const auto average_temperature{ total_temperature / kMaxUpdatesToProcess };
    std::cout << "Average temperature at " << zipcode_filter << " is " << average_temperature << "\n";

    return 0;
}