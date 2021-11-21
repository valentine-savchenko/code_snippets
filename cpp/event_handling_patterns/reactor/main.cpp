#include <iostream>
#include <exception>

#include <boost/asio/io_context.hpp>

int main()
{
    try
    {
        boost::asio::io_context io_context{};
        io_context.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fail to perform the task due to " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Fail to perform the task due to an unknown exception" << std::endl;
        return 2;
    }

    return 0;
}